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

/* codul de eroare returnat de anumite apeluri */
extern int errno;
/* portul de conectare la server*/
int port;
time_t current_t, intrebare_t;

void clearScreen();
int trimiteNume(int fd); // trimite numele clientului la server
int semnalIncepereJoc(int sd); // clientul primeste semnalul de la server ca a inceput jocul
int joc(int sd); //clientul primeste intrebari si transmite raspunsuri
int afisCastigator(int sd);
void afisOptiuni();
void clear();

int main (int argc, char *argv[]) {

  int sd;     // descriptorul de socket
  struct sockaddr_in server;  // structura folosita pentru conectare 

  /* exista toate argumentele in linia de comanda? */
  if (argc != 3)
    {
      printf ("[client] Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    }

  /* stabilim portul */
  port = atoi (argv[2]);

  /* cream socketul */
  if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("[client] Eroare la socket().\n");
      return errno;
    }
  
  //ioctl(sd, FIONBIO, 0);  
  /* umplem structura folosita pentru realizarea conexiunii cu serverul */
  /* familia socket-ului */
  server.sin_family = AF_INET;
  /* adresa IP a serverului */
  server.sin_addr.s_addr = inet_addr(argv[1]);
  /* portul de conectare */
  server.sin_port = htons (port);
  
  /* ne conectam la server */
  if (connect (sd, (struct sockaddr *) &server,sizeof (struct sockaddr)) == -1)
    {
      perror ("[client]Eroare la connect().\n");
      return errno;
    }

  if(trimiteNume(sd) != 1) {//trimiti numele la server
    perror("[client] Eroare la trimiteNume()");
    return errno;
  }
  if(semnalIncepereJoc(sd) != 1) {//se primeste semnalul de incepere
    perror("[client] Eroare la semanl joc()");
    return errno;
  }
  if(joc(sd) != 1) {//se joaca
    perror("[client] Eroare la joc()");
    return errno;
  }
  if(afisCastigator(sd) < 1) {//se joaca
    perror("[client] Eroare la afisCastigator()");
    return errno;
  }
  close (sd);
}

int trimiteNume(int sd) {
  char msg[100];

  bzero (msg, 100);
  printf ("[client]Introduceti un nume (maxim 99 caractere): ");
  fflush (stdout);

  bzero(msg,sizeof(msg));
  read (0, msg, 100);
  //clearScreen();
  /* trimiterea mesajului la server */
  if (write (sd, msg, 100) <= 0) {
    perror ("[client]Eroare la write() spre server.\n");
    return errno;
  }

  bzero(msg,sizeof(msg)); // se pregateste mesajul
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
  char msg[100];
  bzero(msg,sizeof(msg)); // se pregateste mesajul
  if (read (sd, msg, 100) <= 0) {
      perror ("[client]Eroare la read() de la server.\n");
      return errno;
  }
  printf("%s\n", msg); // primeste "jocul incepe in 5 secunde";
  return 1;
}

int joc(int sd) {
  while(1){
    char msg[1000];
    bzero(msg,sizeof(msg)); // se pregateste mesajul'
    printf("Serverul s-a inchis\n"); fflush(stdout);
    int x = write (sd, "Y", 2);
    printf("\033[A"); printf("\33[2K"); fflush(stdout);
    if (x <= 0) {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }
    if (read (sd, msg, 1000) <= 0) {//se citeste de la server
        perror ("[client]Eroare la read() de la server.\n");
        return errno;
    }
    //printf("<<%c>>", msg[0]);fflush(stdout);
    if(msg[0] == 'C') {//se termina jocul
      printf("%s", msg); fflush(stdout);
      //printf("<<TTTTTTTTTT>>");fflush(stdout);
      return 1;
    }
    if(msg[0] == 'E') {//ai fost eliminat
      printf("%s\n", "Ai fost eliminat pentru ca nu ai raspuns la timp");
      return -1;
    }

    char idIntrebare[5];
    strncpy(idIntrebare, msg, 3);
    printf("%s\n", msg+3); // ar trebui sa afiseze intrebarea si variantele de raspuns;

    bzero(msg,sizeof(msg));
    do{
      printf("\033[A");
      printf("\33[2K");
      read (0, msg, 1000);    
      printf("Input incorect\n");
    }while(msg[0]!='A' && msg[0]!='B' && msg[0]!='C' && msg[0]!='D');
    printf("\033[A");
    printf("\33[2K");
    msg[3]=msg[0];//se pregateste raspuns
    msg[0]=idIntrebare[0];
    msg[1]=idIntrebare[1];
    msg[2]=idIntrebare[2];
    msg[4]=0;
    printf("A fost trimis raspunsul %c\n\n\n", msg[3]); 
    if (write (sd, msg, 5) <= 0) {
      perror ("[client]Eroare la write() spre server.\n");
      return errno;
    }
  }
}

void afisOptiuni(){
  printf("Raspunsurile posibile sunt \"A\", \"B\", \"C\" sau \"D\" \n");
}
void clearScreen() {
  const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
  write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
}
int afisCastigator(int sd){
  char msg[1000];
  bzero(msg, 1000);
  if (read (sd, msg, 1000) <= 0) {//se citeste de la server
      perror ("[client]2Eroare la read() de la server.\n");
      return 0;
  }
  printf("%s",msg);fflush(stdout);
  return 1;
}