#ifndef MULTILOOKUP_H
#define MULTILOOKUP_H
#define MAX_IP_LENGTH 46
#define MAX_INPUT_FILES 100
#define MAX_REQUESTER_THREADS 10
#define MAX_RESOLVER_THREADS 10

struct requesterStruc{
    array * arr; 
    FILE * file;
    char names[100][100];
    int index;
    int size;
    sem_t request;
    sem_t serviceFile; 
    sem_t writeResolved;
    sem_t indexS;
};

struct resolverStruc{
    array * arr;
    FILE * file;
    sem_t request;
    sem_t serviceFile; 
    sem_t writeResolved;
    sem_t indexS;
};

void * requester(void * s);
void * resolver (void * s);
int writeToServiced(FILE * filename, char * line);
int writeToResloved(FILE * filename, char * hostname, char * ip, sem_t  * writeResolved);

#endif