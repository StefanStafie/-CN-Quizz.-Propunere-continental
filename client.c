/* cliTCP.c - Exemplu de client TCP */
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <pthread.h>

extern int errno; // error return
int port;// port used for connecting to server

int trimiteNume(int sd); //sends client name to server, @param sd represents the socket descriptor used to communicate with server  
int semnalIncepereJoc(int sd); //receives signal from server that game has started
int joc(int sd); //question and answer exchange
int afisCastigator(int sd); // display winner of the game
void afisOptiuni();
void clear();

int main (int argc, char *argv[]) {

  int sd; //socket descriptor
  struct sockaddr_in server; // server address 

  /*Checks if run command is executed properly*/
  if (argc != 3)
    {
      printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* set port */
  port = atoi (argv[2]);

  /* create socket */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[client] Eroare la socket().\n");
      return errno;
    }
  
  /* initialize server struct */
  /* socket family */
  server.sin_family = AF_INET;
  /* IP address */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* port */
  server.sin_port = htons (port);
  
  /* connect to server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  if(trimiteNume(sd) != 1) {
    perror("[client] Eroare la trimiteNume()");
    return errno;
  }
  if(semnalIncepereJoc(sd) != 1) {
    perror("[client] Eroare la semanl joc()");
    return errno;
  }
  if(joc(sd) != 1) {
    perror("[client] Eroare la joc()");
    return errno;
  }
  if(afisCastigator(sd) < 1) {
    perror("[client] Eroare la afisCastigator()");
    return errno;
  }
  close (sd);
}

int trimiteNume(int sd) {
  char msg[100];
  printf ("[client]Introduceti un nume (maxim 99 caractere): ");
  fflush (stdout);

  /* read name from keyboard*/
  bzero(msg,sizeof(msg));
  read (0, msg, 100);

  /* send name to server */
  if (write (sd, msg, 100) <= 0) {
    perror ("[client]Eroare la write() spre server.\n");
    return errno;
  }

  /* read server response*/
  bzero(msg,sizeof(msg));
  strcpy(msg,"");
  if (read (sd, msg, 100) <= 0) {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
  }
  if(msg[0] == 'F'){
    printf("Nu ai trimis numele la timp. Jocul a inceput deja\n");
    return 0;
  }
  printf ("%s\n", msg);
  return 1;
}

int semnalIncepereJoc(int sd) {
  /* blocks process until read from server */
  char msg[100];
  bzero(msg,sizeof(msg));
  if (read (sd, msg, 100) <= 0) {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
  }
  printf("%s\n", msg);
  return 1;
}

int joc(int sd) {
  while(1){
   
    /*checks if server is still open*/
    printf("Serverul s-a inchis\n"); fflush(stdout);
    int x = write (sd, "Y", 2);
    printf("\033[A"); printf("\33[2K"); fflush(stdout);
    if (x <= 0) {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }

    /*client receives question from server*/
    char msg[1000];
    bzero(msg,sizeof(msg));
    if (read (sd, msg, 1000) <= 0) {
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    if(msg[0] == 'C') {//se termina jocul
      printf("%s", msg); fflush(stdout);
      return 1;
    }
    if(msg[0] == 'E') {//ai fost eliminat
      printf("%s\n", "Ai fost eliminat pentru ca nu ai raspuns la timp");
      return -1;
    }
    
    char idIntrebare[5]; //record question id
    strncpy(idIntrebare, msg, 3);

    /*read answer from keyboard*/
    printf("%s\n", msg+3);
    bzero(msg,sizeof(msg));
    do{
      printf("\033[A");
      printf("\33[2K");
      read (0, msg, 1000);    
      printf("Input incorect\n");
    }while(msg[0]!='A' && msg[0]!='B' && msg[0]!='C' && msg[0]!='D');
    printf("\033[A");
    printf("\33[2K");
    /*prepare for sending answer*/
    msg[3]=msg[0];
    msg[0]=idIntrebare[0];
    msg[1]=idIntrebare[1];
    msg[2]=idIntrebare[2];
    msg[4]=0;

    printf("A fost inregistrat raspunsul %c\n\n\n", msg[3]); 
    if (write (sd, msg, 5) <= 0) {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }
  }
}

void afisOptiuni(){
  printf("Raspunsurile posibile sunt \"A\", \"B\", \"C\" sau \"D\" \n");
}

int afisCastigator(int sd){
  /*receive winner list from server*/
  char msg[1000];
  bzero(msg, 1000);
  if (read (sd, msg, 1000) <= 0) {//se citeste de la server
      perror ("[client]2Eroare la read() de la server.\n");
      return 0;
  }
  printf("%s",msg);fflush(stdout);
  return 1;
}