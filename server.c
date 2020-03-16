/*
*some kind of Kahoot
*server multithreading
*syncronises users
*has access to Database and sends info to users
*know the score of each user
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h> 
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "string.h"
#include <sqlite3.h>
#include <stdio.h>    

    

#define PORT 5555 //Used port 
#define TIME_PER_QUESTION 10 //Time for each question

typedef struct tabel{
  int descriptor;
  int scor;
  char nume[100];
}tabel;
typedef struct thData{
  int idThread;
  int cl; //file descriptor of the client
}thData;

tabel marcaj[100];
int nrJucatori = 0, intrebare_curenta = 1, nrIntrebari = 0;
time_t current_t, login_t, intrebare_t; // for sync
extern int errno;   // for error 
sqlite3 *db; // database identifier
char *err_msg = 0; // error identifier


char *conv_addr (struct sockaddr_in address); // convert IP address   sockaddr_in in char*
void initMarcaj(); // sets score = -1 for all players
void afisMarcaj(); // prints score for all players
static void *clientHandler(void *fd);  // client thread function
int sayHello(int fd); // receive name from client
int incepeJoc(int fd); // sends "begin game" msg
int intrebariRaspunsuri(int fd); // sends questions and receives answers for as long as there are questions left
void getIntrebare(int nr, char*); // overwrites char* with the question with id = nr
char getRaspuns(int nr); // returns answer of question with id = nr
int getNrIntrebari(); // returns nr of questions
int anuntCastigator(int fd);
int fdIsValid(int fd); 
int decript(char* ); //char* to int
void aiFostEliminat(int fd); //send elimination message to eliminated clients
int callback(void *NotUsed, int argc, char **argv, char **azColName); //used in query


int main ()
{

	initMarcaj();
  
	int rc = sqlite3_open("concurs.db", &db);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return 1;
	}
	nrIntrebari = getNrIntrebari();

	current_t = time(NULL); // starts timer for sync
	login_t = time(NULL) + 10;

	pthread_t th[100];    //Threads for users
	int nrJucatori = 0;
	_Bool joining = 1;

	struct sockaddr_in server;  //server and client struct
	struct sockaddr_in from;
	int sd, client;   //socket descriptors
	int optval=1;       /* for setsockopt()*/ 
	unsigned int len;      /*  sockaddr_in length*/

	/* create socket */
	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
		perror ("[server] Eroare la socket().\n");
		return errno;
	}
  
	/*set SO_REUSEADDR for socket (more conections on a single socket)*/ 
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));


	/*prepare server struct*/
	bzero (&server, sizeof (server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl (INADDR_ANY);
	server.sin_port = htons (PORT);

	/* bind socket to server */
	if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1) {
	perror ("[server] Eroare la bind().\n");
	return errno;
	}

	/* make server listen for connections */
	if (listen (sd, 5) == -1) {
	perror ("[server] Eroare la listen().\n");
	return errno;
	}
  
	printf ("[server] Asteptam la portul %d...\n", PORT); fflush (stdout);

	int flags = fcntl(sd, F_GETFL);
	fcntl(sd, F_SETFL,flags| O_NONBLOCK); // sets socket to nonblocking

	/* server will handle new connections to the server until the game starts */
	while (joining) {
		len = sizeof (from);
		bzero (&from, sizeof (from));
		client = accept(sd, (struct sockaddr *) &from, &len);//client is accepted in connection
    
		if (client < 1  ) {
			/*error check*/
			if (errno == EWOULDBLOCK) {
				current_t = time(NULL);
				if(current_t>login_t){
					joining = 0;
					printf("S-a terminat perioada de join\n");
				}
			} else {
				perror("error when accepting connection");
				continue;
			}
		} else {
			/*no errors -> add new connection into a thread*/
			thData* td;
			td=(struct thData*)malloc(sizeof(struct thData)); 
			td->idThread = nrJucatori++;
			td->cl = client;
			pthread_create(&th[nrJucatori], NULL, &clientHandler, td); 
		}
	}

	intrebare_t = time(NULL);
	current_t = time(NULL);

	/*iterate questions (sync)*/
	for(intrebare_curenta = 1; intrebare_curenta <= nrIntrebari; intrebare_curenta++) { 
		printf("Se trimite intrebarea %d\n", intrebare_curenta), fflush(stdout);
		while(intrebare_t > current_t - intrebare_curenta*TIME_PER_QUESTION) {
			current_t = time(NULL);
		}
	}
	
	intrebare_curenta = -1;//stops game
	printf("Gata jocul\n");
	afisMarcaj();//prints score of players

	sleep(5);
	sqlite3_close(db);
	close(sd);
	return 0;
}



char *conv_addr (struct sockaddr_in address) {
	static char str[25];
	strcpy (str, inet_ntoa (address.sin_addr));
	char port[7];
	bzero (port, 7);
	sprintf (port, ":%d", ntohs (address.sin_port));  
	strcat (str, port);
	return (str);
}

void initMarcaj() {
	for(int i=0;i<100; i++)
		marcaj[i].scor = -2;
}

void afisMarcaj() {
	printf("Scorul este:\n\n");
	int ok = 0;
	for(int i=0;i<100; i++)
		if(marcaj[i].scor>=0) {
			printf("%s -> %d cu descriptorul %d \n", marcaj[i].nume, marcaj[i].scor, marcaj[i].descriptor);
			ok = 1;
		}
	if(!ok){
		printf("Nimeni nu a stiut nimic. ");
	}
}

static void *clientHandler(void *arg) {
	nrJucatori = nrJucatori + 1;
	struct thData fd; 
	fd = *((struct thData*)arg);  
  
	if(!sayHello(fd.cl)){
		pthread_exit(NULL);
	}

	while(current_t<login_t){
		sleep(1);
	}
	if(!incepeJoc(fd.cl)){
		pthread_exit(NULL);
	}
	if(!intrebariRaspunsuri(fd.cl)){
		pthread_exit(NULL);
	}
	if(anuntCastigator(fd.cl) != 1){
		pthread_exit(NULL);
	}
	sleep(5);
	close(fd.cl);
	pthread_exit(NULL);
}

int sayHello(int fd) {
  char msg[100];
  char msgrasp[100];

  /*receive message from client*/
  if (read (fd, msg, 100) <= 0) {
    perror ("Eroare la read() de la client.\n");
    return 0;
  }
  printf ("%s s-a alaturat jocului cu descriptorul %d\n", msg, fd);
      

  marcaj[fd].descriptor = fd;
  strcpy(marcaj[fd].nume, msg);
  marcaj[fd].scor = 0;
  bzero(msgrasp,100);
  strcpy(msgrasp,"Hello ");
  strncat(msgrasp,msg, 60);
  strcat(msgrasp, "Jocul va incepe in curand.");
      
  /*if game has already started when you sent name you get disqualified*/
  if(current_t>login_t){
    write (fd, "FFF", 100);
    close(fd);
    marcaj[fd].scor = -1;
    return 0;
  }
  /*sends message that connection is successful*/
  if (write (fd, msgrasp, 100) <= 0) {
    perror ("[server] Eroare la write() catre client.\n");
    return 0;
  }
  return 1;
}

int incepeJoc(int fd){ 
  if (write (fd, "Jocul incepe acum", 100) <= 0) {
    perror ("[server] Eroare la write() catre client.\n");
    return 0;
  }
  return 1;
}

int intrebariRaspunsuri(int fd){
	char msg[1000], dump[2];
	int i;
	for(i=1;; i++){
		while(i > intrebare_curenta && intrebare_curenta != -1){sleep(1);} //sleeps until next question starts
		if(i > nrIntrebari ){
			/*check if client is still connected*/
			if (read (fd, &dump, 1) <= 0) {
				perror ("Eroare la read() de la client.\n");
				marcaj[fd].scor = -1;
				return 0;
			}
			if (write (fd, "Concurs Terminat", 1000) <= 0) {
				perror ("[server] Eroare la write() catre client.\n");
				return 0;
			}
			return 1;
		}
		bzero(msg, sizeof(msg));
		char intrebare[1000] = "";
		getIntrebare(i, intrebare);
		msg[2] = i%10 + '0'; 
		msg[1] = (i/10)%10 + '0';
		msg[0] = (i/100)%10 + '0';
		strcat(msg, intrebare);
		if(fdIsValid(fd)){
			/*check if client is still connected*/
			if (read (fd, dump, 2) <= 0) {
				perror ("Eroare la read() de la client.\n");
				marcaj[fd].scor = -1;
				return 0;
			}
			/*sends question*/
			if (write (fd, msg, 1000) <= 0) {
				perror ("[server] Eroare la write() catre client.\n");
				marcaj[fd].scor = -1;
				return 0;
			}
			bzero(msg, sizeof(msg));
			/*reads answer*/
			if (read (fd, msg, 5) <= 0) {
				perror ("Eroare la read() de la client.\n");
				marcaj[fd].scor = -1;
				return 0;
			}
			/*didn't answer to the right question*/
			if(intrebare_curenta != decript(msg)){
				aiFostEliminat(fd);
				marcaj[fd].scor = -1;
				return 0;
			}
			/*right answer => score++ */
			if(getRaspuns(i) == msg[3])
				marcaj[fd].scor++;
		}else{
			printf("am ajuns pe else %d   ", intrebare_curenta);
		}
		printf ("Clientul %s cu scor curent %d si descriptorul %d a raspuns %c\n", marcaj[fd].nume, marcaj[fd].scor, fd,msg[3]);
		fflush(stdout);
	}
	return 1;
}

int fdIsValid(int fd){
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

int decript(char* cod){
  return (cod[0]-'0')*100 + (cod[1]-'0')*10 + (cod[2]-'0'); 
}

void encript(int x, char *sir){
  sir[0] = x/100 + '0';
  sir[1] = x/10%10 + '0';
  sir[2] = x%10 + '0';
}

void aiFostEliminat(int fd){
	char dump, msg[100];
	printf("Clinetul %s cu descriptorul %d a fost eliminat din concurs\n", marcaj[fd].nume, fd);
	/*checks if client is still connected*/
	if (read (fd, &dump, 1) <= 0) {
		perror ("Eroare la read() de la client.\n");
		marcaj[fd].scor = -1;
		return;
	}
	/*sends message for not answering in time ("EEE")*/
	strcpy(msg, "EEE");
	if (write (fd, msg, 100) <= 0) {
		perror ("[server] Eroare la write() catre client.\n");
		marcaj[fd].scor = -1;
		return;
	}
}
int getNrIntrebari(){
	int n = 0, rc;
	char eval[100] = "";
	rc = sqlite3_exec(db, "Select count(*) from quiz;",callback,eval,&err_msg); 
	if (rc != SQLITE_OK ) {
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);        
		return 0;
	}
	/*converts db result to int*/
	for(int i = 0; i<strlen(eval); i++){
		n = n*10 + eval[i]-'0';
	}
	return n;
}

void getIntrebare(int nr, char* eval){
	char sql[100] = "SELECT intrebare from quiz where id == ";
	char c[2] = "";
	if(nr>99){
		c[0] = nr/100+'0';
		strcat(sql,c);
	}
	if(nr>9){
		c[0] = (nr/10)%10+'0';
		strcat(sql,c);
	}
	c[0] = nr%10+'0';
	strcat(sql,c);
	strcat(sql, ";");
	int rc = sqlite3_exec(db, sql,callback,eval,&err_msg); 
	if (rc != SQLITE_OK ) {
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);        
		return;
	}
}
char getRaspuns(int nr){
	char val[10] = "";
	char sql[100] = "SELECT raspuns FROM quiz WHERE id == ";
	char c[2] = "";
	if(nr>99  ){
		c[0] = nr/100+'0';
		strcat(sql,c);
	}
	if(nr>9){
		c[0] = (nr/10)%10+'0';
		strcat(sql,c);
	}
	c[0] = nr%10+'0';
	strcat(sql,c);
	strcat(sql, ";");


	int rc = sqlite3_exec(db, sql,callback,&val,&err_msg); 
	if (rc != SQLITE_OK ) {
		fprintf(stderr, "SQL error: %s\n", err_msg);
		sqlite3_free(err_msg);        
		sqlite3_close(db);
		return 0;
	}
	return val[0];
}
int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
        strcat(NotUsed, argv[i]);
    }
    return 0;
}
int anuntCastigator(int fd){
	int maxim = 0;
	for(int i = 1; i<= 99 ; i++){
		if(marcaj[i].scor > maxim)
			maxim = marcaj[i].scor;
	}
	if(maxim == 0){
		if(write(fd, "Nimeni nu a castigat. Trist.\n", 1000)<= 0)
			return errno;
	} else {
		char castigatori[1000] = "Concurs castigat de: ";
		for(int i = 1; i<= 99 ; i++){
			if(marcaj[i].scor == maxim){
			strcat(castigatori, " ");
			strcat(castigatori, marcaj[i].nume);
			}
		}
		if(write(fd, castigatori, 1000)<= 0)
			return errno;
	}
	return 1;
}