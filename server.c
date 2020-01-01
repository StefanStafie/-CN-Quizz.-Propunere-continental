/* servTCPCSel.c - Exemplu de server TCP concurent 
   
   Asteapta un "nume" de la clienti multipli si intoarce clientilor sirul
   "Hello nume" corespunzator; multiplexarea intrarilor se realizeaza cu select().
   
   Cod sursa preluat din [Retele de Calculatoare,S.Buraga & G.Ciobanu, 2003] si modificat de 
   Lenuta Alboaie  <adria@infoiasi.ro> (c)2009
   
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

    
/* portul folosit */
#define PORT 5555
#define TIMP_INTREBARE 10

typedef struct tabel{
  int descriptor;
  int scor;
  char nume[100];
}tabel;
typedef struct thData{
  int idThread; //id-ul thread-ului tinut in evidenta de acest program
  int cl; //descriptorul intors de accept
}thData;

tabel marcaj[100]; // retine scor si nume participanti
int nrJucatori = 0, intrebare_curenta = 1, nrIntrebari = 0; // date esentiale
time_t current_t, login_t, intrebare_t; // pentru sincronizare 
extern int errno;   // eroarea returnata de unele apeluri
sqlite3 *db;//baza de date folosita
char *err_msg = 0; // eroare returnata de alte apeluri


char *conv_addr (struct sockaddr_in address); // functie de convertire a adresei IP a clientului in sir de caractere 
void initMarcaj(); // initializeaza tabela de marcaj cu scor = -1
void afisMarcaj(); // afiseaza castigatorul cu scor = max
static void *clientHandler(void *fd);  // thread pentru un client
int sayHello(int fd); // primeste numele de la client
int incepeJoc(int fd); // trimite semnal de incepere joc
int intrebariRaspunsuri(int fd); // schimb de intrebari
void getIntrebare(int nr, char*); // returneaza intrebarea cu id = nr
char getRaspuns(int nr); // returneaza raspunsul la intrebarea cu id = nr
int getNrIntrebari(); // returneaza nr de intrebari
int anuntCastigator(int fd);
void afisCastigator();
int fd_is_valid(int fd);
int decript(char* );
void aiFostEliminat(int fd);
int callback(void *NotUsed, int argc, char **argv, char **azColName);

/* programul */
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
  nrIntrebari = 3;
  printf("aici, %d\n", nrIntrebari);
  char *tabel = malloc(10000), rasp;
  for(int g = 1; g<=nrIntrebari; g++){
    bzero(tabel, 10000);
    getIntrebare(g, tabel);
    printf("%s\n",tabel);
    rasp = getRaspuns(g);
    printf("%c\n",rasp);
  }

  current_t = time(NULL); // starts timer for 
  login_t = time(NULL) + 10;

  pthread_t th[100];    //Identificatorii thread-urilor care se vor crea
  int nrJucatori = 0;
  _Bool joining = 1;

  struct sockaddr_in server;  /* structurile pentru server si clienti */
  struct sockaddr_in from;
  int sd, client;   /* descriptori de socket */
  int optval=1;       /* optiune folosita pentru setsockopt()*/ 
  unsigned int len;      /* lungimea structurii sockaddr_in */

  /* creare socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
    perror ("[server] Eroare la socket().\n");
    return errno;
  }
  
  /*setam pentru socket optiunea SO_REUSEADDR */ 
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));

  /* pregatim structurile de date */
  bzero (&server, sizeof (server));

  /* umplem structura folosita de server */
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (PORT);

  /* atasam socketul */
  if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1) {
    perror ("[server] Eroare la bind().\n");
    return errno;
  }

  /* punem serverul sa asculte daca vin clienti sa se conecteze */
  if (listen (sd, 5) == -1) {
    perror ("[server] Eroare la listen().\n");
    return errno;
  }
  
  /* completam multimea de descriptori de citire */
  printf ("[server] Asteptam la portul %d...\n", PORT);
  fflush (stdout);

  int flags = fcntl(sd, F_GETFL);
  fcntl(sd, F_SETFL,flags| O_NONBLOCK);
  /* servim in mod concurent clientii... */
  while (joining) {
    len = sizeof (from);
    bzero (&from, sizeof (from));
    client = accept(sd, (struct sockaddr *) &from, &len);
    thData * td;
    if (client < 1  ) {
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
    } else {//creez un thread pt client;
      //printf("client login %ld\n", time(NULL));
      td=(struct thData*)malloc(sizeof(struct thData)); 
      td->idThread = nrJucatori++;
      td->cl = client;
      pthread_create(&th[nrJucatori], NULL, &clientHandler, td); 
    }
  }

  intrebare_t = time(NULL);
  current_t = time(NULL);
  for(intrebare_curenta = 1; intrebare_curenta <= nrIntrebari; intrebare_curenta++) { // itereaza intrebari dupa o sincronizare standard
    printf("Se trimite intrebarea %d\n", intrebare_curenta), fflush(stdout);
    while(intrebare_t > current_t - intrebare_curenta*TIMP_INTREBARE)
      {current_t = time(NULL);}
    
  }

  intrebare_curenta = -1;
  printf("Gata jocul\n");

  afisMarcaj();

  sleep(5);
  sqlite3_close(db);
  close(sd);
  return 0;
}



char * conv_addr (struct sockaddr_in address) {
  static char str[25];
  char port[7];
  strcpy (str, inet_ntoa (address.sin_addr)); // adresa ip a clientului 
  bzero (port, 7); // port utilizat
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
static void *clientHandler(void *arg){
  nrJucatori = nrJucatori + 1;
  struct thData fd; 
  fd = *((struct thData*)arg);  
  
  if(!sayHello(fd.cl)){
    pthread_exit(NULL);
  }

  while(current_t<login_t){ // asteapta inceperea jocului
    sleep(1);
  }
  if(!incepeJoc(fd.cl)){//trimite mesaj de incepere
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
int sayHello(int fd) {/* realizeaza primirea si retrimiterea unui mesaj unui client */
  char msg[100];    //mesajul primit de la client 
  char msgrasp[100];        //mesaj de raspuns pentru client

  if (read (fd, msg, 100) <= 0) {
    perror ("Eroare la read() de la client.\n");
    return 0;
  }
  printf ("%s s-a alaturat jocului cu descriptorul %d\n", msg, fd);
      
  /*pregatim mesajul de raspuns */
  marcaj[fd].descriptor = fd;
  strcpy(marcaj[fd].nume, msg);
  marcaj[fd].scor = 0;

  bzero(msgrasp,100);
  strcpy(msgrasp,"Hello ");
  strncat(msgrasp,msg, 60);
  strcat(msgrasp, "Jocul va incepe in curand.");
      
  //printf("[server]Trimitem mesajul inapoi...%s  %ld   \n",msgrasp, strlen(msgrasp));
  if(current_t>login_t){
    write (fd, "FFF", 100);
    close(fd);
    marcaj[fd].scor = -1;
    return 0;
  }
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
    while(i > intrebare_curenta && intrebare_curenta != -1){sleep(1);} 
    if(i > nrIntrebari ){
      if (read (fd, &dump, 1) <= 0) {
        perror ("Eroare la read() de la client.\n");
        marcaj[fd].scor = -1;
        return 0;
      }
      if (write (fd, "Concurs Terminat", 1000) <= 0) { // s-au terminat intrebarile, anunt clientul
        perror ("[server] Eroare la write() catre client.\n");
        return 0;
      }
      return 1;
    }
    bzero(msg, sizeof(msg));
    //printf("intrebari si raspunsuri \n");
    char intrebare[1000] = "";
    getIntrebare(i, intrebare);
    msg[2] = i%10 + '0'; 
    msg[1] = (i/10)%10 + '0';
    msg[0] = (i/100)%10 + '0';
    strcat(msg, intrebare);
    if(fd_is_valid(fd)){
      if (read (fd, dump, 2) <= 0) {
        perror ("Eroare la read() de la client.\n");
        marcaj[fd].scor = -1;
        return 0;
      }
      if (write (fd, msg, 1000) <= 0) {
        perror ("[server] Eroare la write() catre client.\n");
        marcaj[fd].scor = -1;
        return 0;
      }
       bzero(msg, sizeof(msg));
      if (read (fd, msg, 5) <= 0) {
        perror ("Eroare la read() de la client.\n");
        marcaj[fd].scor = -1;
        return 0;
      }
      //printf("\n%d\n%d\n", decript(msg), intrebare_curenta);
      if(intrebare_curenta != decript(msg)){// nu a raspuns la intrebarea buna
        aiFostEliminat(fd);
        marcaj[fd].scor = -1;
        return 0;
      }
      if(getRaspuns(i) == msg[3])
        marcaj[fd].scor++;
    } else {printf("am ajuns pe else %d   ", intrebare_curenta);}
    printf ("Clientul %s cu scor curent %d si descriptorul %d a raspuns %c\n", marcaj[fd].nume, marcaj[fd].scor, fd,msg[3]);
    fflush(stdout);
  }
  return 1;
}
void afisCastigator(){
  int max = 0;
  char msg[1000];
  strcpy(msg, "Punctajul maxim a fost obtinut de: ");
  for(int i = 0; i < 99; i++){
    if(marcaj[i].scor > max)
      max = marcaj[i].scor;
  }
  for(int i = 0; i < 99; i++){
    if(marcaj[i].scor == max){
      strcat(msg, ", ");
      strcat(msg, marcaj[i].nume);
    }
  }
  strcat(msg, ".");
}
int fd_is_valid(int fd){
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
  if (read (fd, &dump, 1) <= 0) {
    perror ("Eroare la read() de la client.\n");
    marcaj[fd].scor = -1;
    return;
  }
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
        sqlite3_close(db);
        return 0;
    }
  for(int i = 0; i<strlen(eval); i++){
    n = n*10 + eval[i]-'0';
  }
  return n;
}
void getIntrebare(int nr, char* eval){
  char sql[100] = "SELECT intrebare from quiz where id == ";
  char c[2] = "";
  if(nr>99){//sute
    c[0] = nr/100+'0';
    strcat(sql,c);
  }
  if(nr>9){//zeci
    c[0] = (nr/10)%10+'0';
    strcat(sql,c);
  }
  c[0] = nr%10+'0';//unitati
  strcat(sql,c);
  strcat(sql, ";");
  int rc = sqlite3_exec(db, sql,callback,eval,&err_msg); 
  if (rc != SQLITE_OK ) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        return;
  }
}
char getRaspuns(int nr){
  char val[10] = "";
  char sql[100] = "SELECT raspuns FROM quiz WHERE id == ";
  char c[2] = "";
  if(nr>99  ){//sute
    c[0] = nr/100+'0';
    strcat(sql,c);
  }
  if(nr>9){//zeci
    c[0] = (nr/10)%10+'0';
    strcat(sql,c);
  }
  c[0] = nr%10+'0';//unitati
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