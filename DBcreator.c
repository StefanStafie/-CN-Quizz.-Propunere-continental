/**
 * 
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

sqlite3 *db; //database identifier
char *err_msg = 0; //error identifier
/**
 * function for displaying the database after the changes
*/
int callback(void *NotUsed, int argc, char **argv, char **azColName);

/* programul */
int main ()
{
  int rc = sqlite3_open("concurs.db", &db);
  if (rc != SQLITE_OK) {
      fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
  }
  char *sql = "DROP TABLE IF EXISTS quiz;"
              "CREATE TABLE quiz ( id int(3), intrebare varchar(1000), raspuns char(1) );"
              "INSERT INTO quiz VALUES(1, '1. In ce an s-a nascut Mihai Eminescu?    \nA) 1850    B) 255    C) 18    D) 2020\n', 'A');" 
              "INSERT INTO quiz VALUES(2, '2. Cate planete sunt in sistemul solar?    \nA) 8    B) 100    C) 120    D) 3\n', 'B');" 
              "INSERT INTO quiz VALUES(3, '3. Cate tipuri de temperament sunt?    \nA) 90    B) 6    C) 2    D) 1\n', 'C');" 
              "INSERT INTO quiz VALUES(4, '4. Cate tari strabate Dunarea?    \nA) 90    B) 6    C) 2    D) 1\n', 'D');" 
              "INSERT INTO quiz VALUES(5, '5. Care este numele capitalei Italiei?    \nA) 90    B) 6    C) 2    D) 1\n', 'B');"
              "INSERT INTO quiz VALUES(6, '6. La finalul actiunii din 102 Dalmatieni, Cruella este atrasa de catei intr-o fabrica si prinsa cu totul intr-un mare...    \nA) Camion    B) Dulap    C) Caine    D) Conflict emotional\n', 'B');" 
              "INSERT INTO quiz VALUES(7, '7. In ce an a fost lansat jocul cu numele 102 Dalmatieni?    \nA) 120   B) 6    C) 1999    D) 1199\n', 'B');" 
              "INSERT INTO quiz VALUES(8, '8. Ce culoare au zgarzile catelusilor (masculilor) in animatia 101 Dalmatieni?    \nA) verde    B) lila    C) turcuaz    D) bleu\n', 'B');" 
              "INSERT INTO quiz VALUES(9, '9. 102 Dalmatieni a avut incasari mai mici decat prima parte, de doar...   \nA) 90$    B) 6$    C) 2$    D) 1milion $$$\n', 'B');" 
              "INSERT INTO quiz VALUES(10, '10. Ce nume poarta catelusul despre care se credea ca nu a supravietuit? \nA) Jim    B) Jong    C) Kun    D) Lao\n', 'B');" 
              "INSERT INTO quiz VALUES(11, '11. Cum se numeste actrita care o interpreteaza pe Anita?   \nA) Julia Roberts    B) Reese Remorthson    C) Maria Deluge    D) Linux Mychild\n', 'B');" 
              "INSERT INTO quiz VALUES(12, '12. In ce an a aparut filmul artistic 102 Dalmatieni?    \nA) 1    B) 19    C) 199    D) 1999\n', 'B');" 
              "INSERT INTO quiz VALUES(13, '13. Ce animal este Capitanul din animatia 101 Dalmatieni?    \nA) Om    B) Pisica    C) Melc    D) Dalmatian\n', 'B');" ;
  
  rc = sqlite3_exec(db, sql,0,0,&err_msg);

  if(rc!= SQLITE_OK){
    fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
      sqlite3_close(db);
      return -1;
  }

  rc = sqlite3_exec(db, "SELECT * FROM quiz",callback,0,&err_msg);
  if(rc!= SQLITE_OK){
    sqlite3_close(db);
    return -1; 
  }

  sqlite3_close(db);
  return 0;
}


int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    for (int i = 0; i < argc; i++) {
       printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}