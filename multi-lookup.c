#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include "array.h"
#include "multi-lookup.h"
#include "util.h"



void * resolver(void * s){
    int printCount = 0; 
    char  str[100];
    char * ptr = str;
    char ** ptrptr = &ptr;
    struct resolverStruc *far = (struct resolverStruc *)s;
    while(1){
        array_get(far->arr, ptrptr);
        if(strcmp(ptr, "POISON") == 0){
            break;
        }
        //Cut last character maybe 
        int len = strlen(ptr);
        ptr[len-1] = '\0';
        char ip[MAX_IP_LENGTH];
        int err = dnslookup(ptr, ip, MAX_IP_LENGTH);
        if(err != -1){
            writeToResloved(far->file, ptr, ip, &far->writeResolved);
        }
        else{
            writeToResloved(far->file, ptr, "NOT_RESOLVED", &far->writeResolved);
        }
        printCount++;
    }
    array_put(far->arr, "POISON");
    printf("thread %lu resolved %d hostnames \n", pthread_self(), printCount);
    return NULL;
};


void * requester(void * s){
    int printCount = 0; 
    //struct fileAndArray *far = s;
    struct requesterStruc *far = (struct requesterStruc *)s;
    //fileName = (char*)fileName;
    //Need to fix arguements being passed in so they work for now gonna continue assuming as if they do 
    
    FILE * fp;
    int size = 100;
    char line[size];

    sem_wait(&far->indexS);
    while(far->index < far->size){
    int index = far->index;
    far->index++;
    sem_post(&far->indexS);
        fp = fopen(far->names[index], "r");
        if (fp == NULL){
            printf("File Error\n");
            break;
        }
        printCount++;
        //sem_wait(&far->indexS);
        //far->index++;
        //sem_post(&far->indexS);
        while(fgets(line, size, fp)){
        //sem_post(&request);
            //printf("%s", line);
            //Write to logfile 
            writeToServiced(far->file, line);
            //Add to shared array
            array_put(far->arr, line);
            //sleep(1);
        }
        fclose(fp);
        //sem_wait(&far->indexS);
            //far->index++;
        //sem_post(&far->indexS);
    sem_wait(&far->indexS);
    }
    sem_post(&far->indexS);
    printf("thread %lu serviced %d files \n", pthread_self(), printCount);
    pthread_exit(NULL);
    //array_put(s, "hello");
    //array_put(s, "test");
    //sleep(5);
    //array_put(s, "testdfa");
}

int main(int argc, char * argv[]){
    if(argc < 6){
        printf("Incorrect arguments\n");
        return -1;
    }
    //Run time recording code 
    struct timeval startTime;
    struct timeval endTime; 
    gettimeofday(&startTime, NULL);


    //Handle command line args
    int numRequest = atoi(argv[1]);
    int numResolve = atoi(argv[2]);
    char * servOutput = argv[3];
    char* resultOutput = argv[4];

    if(argc > MAX_INPUT_FILES + 5){
        fprintf(stderr, "%s", "Too many files\n");
        return -1;
    }

    if(numRequest > MAX_REQUESTER_THREADS){
        fprintf(stderr, "%s", "Too many requester threads\n");
        return -1;
    }
    if(numResolve > MAX_RESOLVER_THREADS){
        fprintf(stderr, "%s", "Too many resolver threads\n");
        return -1;
    }
    //char array[100][100];
    //array[0] =  

    //     add to struct:
    // - array of filenames (malloced array of char* where each element points to the filenames in argv5 -> argc)
    // - length of the above array
    // - an index and a mutex that locks it


    //Create array of filenames 
    //pass into requester thread 
    //also index and size
    //one instance of everything in struct  
    //Init semaphores
    sem_t request;
    sem_t serviceFile; 
    sem_t writeResolved;
    sem_t indexS;
    sem_init(&request, 0, 1);
    sem_init(&serviceFile, 0, 1);
    sem_init(&writeResolved, 0, 1);
    sem_init(&indexS, 0, 1);

    //Create logfile 
    FILE *serviced = fopen(servOutput, "w+");
    FILE *resolved = fopen (resultOutput, "w+");

    if(serviced == NULL || resolved == NULL){
        printf("File not opened correctly\n");
        return -1;
    }
    //Create shared array 
    array  * sharedArray = (array*)malloc(sizeof(array));
    array_init(sharedArray);
    //Create requester thread
    struct requesterStruc rs;
    //Fill array with filenames
    for(int i = 5; i < argc; i++){
        strcpy(rs.names[i-5], argv[i]);
    }
    rs.file = serviced;
    rs.arr = sharedArray;
    rs.index = 0; 
    rs.size = argc-5;
    rs.request = request;
    rs.serviceFile = serviceFile;
    rs.writeResolved = writeResolved;
    rs.indexS = indexS;

    pthread_t  ids[numRequest];
    for(int i = 0; i < numRequest; i++){
        pthread_t idTemp; 
        pthread_create(&idTemp, NULL, requester, &rs);
        ids[i] = idTemp; 
    }

    //Join requester threads first and then put in poison pill in main

    //Create another requester thread
    // struct fileAndArray faa2; 
    // strcpy(faa2.fileName, "./input/names2.txt");
    // faa2.serviced = serviced;
    // faa2.arr = sharedArray;
    // pthread_t id2;
    //pthread_create(&id2, NULL, requester, &faa2);


    //Create resolver thread 
    struct resolverStruc res;
    res.file = resolved;
    res.arr = sharedArray;
    res.request = request;
    res.serviceFile = serviceFile;
    res.writeResolved = writeResolved;
    res.indexS = indexS;
    pthread_t  resolveids[numResolve];
    for(int j = 0; j < numResolve; j++){
        pthread_t idTemp2; 
        pthread_create(&idTemp2, NULL, resolver, &res);
        resolveids[j] = idTemp2; 
    }


    //Join threads
    for(int i = 0; i < numRequest; i++){
        pthread_join(ids[i], NULL);
    }
    //Poison pill here
    array_put(sharedArray, "POISON");

    //Try to join resolver threads
    for(int i = 0; i < numResolve; i++){
        pthread_join(resolveids[i], NULL);
    }
    //Close file
    fclose(serviced);
    fclose(resolved);

    //Destroy semaphores
    sem_destroy(&indexS);
    sem_destroy(&request);
    sem_destroy(&serviceFile);
    sem_destroy(&writeResolved);
    free(sharedArray);
    //Program runtime code 
    gettimeofday(&endTime, NULL);
    long double start = startTime.tv_sec + (startTime.tv_usec/1000000.0);
    long double end = endTime.tv_sec + (endTime.tv_usec/1000000.0);
    printf("total time is %Lf seconds\n", end-start);
    return 0;
}

int writeToServiced(FILE * filename, char * line){
    fwrite(line, 1, strlen(line), filename);
    fflush(stdin);
    return 0; 
}

int writeToResloved(FILE * filename, char * hostname, char * ip, sem_t * writeResolved){
    sem_wait(writeResolved);
    strcat(hostname, ", ");
    strcat(hostname, ip);
    fwrite(hostname, 1, strlen(hostname), filename);
    fwrite("\n", 1, sizeof(char), filename);
    fflush(stdin);
    sem_post(writeResolved);    
    return 0;
}
