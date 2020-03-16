Descrierea problemei: 

*QuizzGame [Propunere Continental]
*Implementati un server multithreading care suporta oricati clienti. Serverul va coordona clientii care raspund la un set de *intrebari pe rand, in ordinea in care s-au inregistrat. Fiecarui client i se pune o intrebare si are un numar n de secunde *pentru a raspunde la intrebare. Serverul verifica raspunsul dat de client si daca este corect va retine punctajul pentru acel *client. De asemenea, serverul sincronizeaza toti clienti intre ei si ofera fiecaruia un timp de n secunde pentru a raspunde. *Comunicarea intre server si client se va realiza folosind socket-uri. Toata logica va fi realizata in server, clientul doar *raspunde la intrebari. Intrebarile cu variantele de raspuns vor fi stocate fie in fisiere XML fie intr-o baza de date SQLite. *Serverul va gestiona situatiile in care unul din participanti paraseste jocul astfel incat jocul sa continue fara probleme.
*
*Indicatii:
*Activitati:
*inregistrarea clientilor
*incarcarea de intrebari din formatul mentionat anterior de catre server
*intrebare adresata catre clienti in ordinea inregistrarii lor, clientul va alege din optiunile oferite intr-un numar de *secunde n
*parasirea unui client va face ca el sa fie eliminat din rundele de intrebari
*terminarea jocului cand intrebarile au fost parcurse -> anuntare castigator catre toti clientii.




Descrierea solutiei:

BAZA DE DATE:
*   -contine un singur tabel cu 3 campuri: "id" cheie primara, "intrebare", "raspuns";
*   -retine intrebarile si raspusurile corespunzatoare;
*   -interogarea se face in functie de id.
SERVER:
*   -se conecteaza la baza de date;
*   -permite conectarea clientilor;
*   -creeaza cate un thread pentru fiecare client conectat;
*   -thread-ul principal realizeaza sincronizarea dintre clienti;
*   -permite inregistrarea clientilor prin transmiterea numelor acestora;
*   -transmite intrebarile din baza de date catre client si primeste raspunsurile acestora. In cazul in care raspunsul este   *   corect se mareste scorul acelui client;
*   -dupa ce se parcurg toate intrebarile din baza de date, se trimite numele castigatorului (castigatorilor, in cazul in care *   este egalitate) catre toti clientii.
CLIENT:
*   -se conecteaza la server;
*   -isi transmite numele;
*   -primeste intrebari si transmite raspunsuri;
*   -la sfarsitul intrebarilor primeste numele castigatorului/ilor.


Use cases:
  #clientul se deconecteaza --> serverul elimina clientul din joc
  #serverul se inchide in timpul jocului --> clientii primesc mesaj "deconectat de la server"
  #clientul trimite raspunsul la o intrebare dupa perioada alocata acelei intrebari --> Serverul elimina clientul din joc. Clientul primeste mesaj de eliminare 
  #clientul se conecteaza. Jocul incepe si apoi clientul transmite numele --> Clientul ramane in void ca pedeapsa. Serverul nu ia in considerare acel client

PS: pentru a rula server in linux: ./server  
    pentru a rula client in linux: ./client 127.0.0.1 5555
    pentru compilare: gcc -o server server.c -lpthread -lsqlite3
