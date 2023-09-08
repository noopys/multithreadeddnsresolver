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

// Resolver thread function
void * resolver(void * s){
    int printCount = 0; 
    char  str[100];
    char * ptr = str;
    char ** ptrptr = &ptr;
    // Typecast to appropriate struct
    struct resolverStruc *far = (struct resolverStruc *)s;
    while(1){
        // Get domain from shared array
        array_get(far->arr, ptrptr);
        
        // Check for poison pill to terminate
        if(strcmp(ptr, "POISON") == 0){
            break;
        }
        
        // Remove the last character if needed
        int len = strlen(ptr);
        ptr[len-1] = '\0';
        
        char ip[MAX_IP_LENGTH];
        // Perform DNS lookup
        int err = dnslookup(ptr, ip, MAX_IP_LENGTH);
        
        // Write result to the output file
        if(err != -1){
            writeToResloved(far->file, ptr, ip, &far->writeResolved);
        }
        else{
            writeToResloved(far->file, ptr, "NOT_RESOLVED", &far->writeResolved);
        }
        printCount++;
    }
    // Insert poison pill back for other threads
    array_put(far->arr, "POISON");
    // Log how many domains this thread resolved
    printf("thread %lu resolved %d hostnames \n", pthread_self(), printCount);
    return NULL;
};

// Requester thread function
void * requester(void * s){
    int printCount = 0; 
    struct requesterStruc *far = (struct requesterStruc *)s;
    FILE * fp;
    int size = 100;
    char line[size];

    // Semaphore for controlling access to shared index
    sem_wait(&far->indexS);
    
    // Loop to read each file
    while(far->index < far->size){
        int index = far->index;
        far->index++;
        sem_post(&far->indexS);
        
        // Open file
        fp = fopen(far->names[index], "r");
        if (fp == NULL){
            printf("File Error\n");
            break;
        }
        
        // Read each line and write to shared array and log file
        while(fgets(line, size, fp)){
            writeToServiced(far->file, line);
            array_put(far->arr, line);
        }
        
        fclose(fp);
        printCount++;
        sem_wait(&far->indexS);
    }
    
    sem_post(&far->indexS);
    // Log how many files this thread processed
    printf("thread %lu serviced %d files \n", pthread_self(), printCount);
    pthread_exit(NULL);
}

// Main function
int main(int argc, char * argv[]){
    // Check command-line arguments
    if(argc < 6){
        printf("Incorrect arguments\n");
        return -1;
    }

    // Measure program runtime
    struct timeval startTime;
    struct timeval endTime; 
    gettimeofday(&startTime, NULL);

    // Initialize values from command line arguments
    int numRequest = atoi(argv[1]);
    int numResolve = atoi(argv[2]);
    char * servOutput = argv[3];
    char* resultOutput = argv[4];

    // Error checks for argument values
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

    // Initialize semaphores
    sem_t request, serviceFile, writeResolved, indexS;
    sem_init(&request, 0, 1);
    sem_init(&serviceFile, 0, 1);
    sem_init(&writeResolved, 0, 1);
    sem_init(&indexS, 0, 1);

    // Open output files
    FILE *serviced = fopen(servOutput, "w+");
    FILE *resolved = fopen(resultOutput, "w+");
    if(serviced == NULL || resolved == NULL){
        printf("File not opened correctly\n");
        return -1;
    }

    // Initialize shared array
    array  * sharedArray = (array*)malloc(sizeof(array));
    array_init(sharedArray);

    // Initialize requester thread struct
    struct requesterStruc rs;
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

    // Create requester threads
    pthread_t  ids[numRequest];
    for(int i = 0; i < numRequest; i++){
        pthread_t idTemp; 
        pthread_create(&idTemp, NULL, requester, &rs);
        ids[i] = idTemp; 
    }

    // Create resolver threads
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

    // Wait for all threads to complete
    for(int i = 0; i < numRequest; i++){
        pthread_join(ids[i], NULL);
    }

    // Insert "POISON" to signal resolver threads to terminate
    array_put(sharedArray, "POISON");

    // Wait for resolver threads to complete
    for(int i = 0; i < numResolve; i++){
        pthread_join(resolveids[i], NULL);
    }

    // Close output files and free resources
    fclose(serviced);
    fclose(resolved);

    // Destroy semaphores
    sem_destroy(&indexS);
    sem_destroy(&request);
    sem_destroy(&serviceFile);
    sem_destroy(&writeResolved);

    // Free memory
    free(sharedArray);

    // Print program runtime
    gettimeofday(&endTime, NULL);
    long double start = startTime.tv_sec + (startTime.tv_usec/1000000.0);
    long double end = endTime.tv_sec + (endTime.tv_usec/1000000.0);
    printf("total time is %Lf seconds\n", end-start);

    return 0;
}

// Function to write to the serviced log
void writeToServiced(FILE * fp, char* name){
    fprintf(fp, "%s", name);
}

// Function to write to the resolved log
void writeToResloved(FILE * fp, char* name, char* ip, sem_t* sem){
    sem_wait(sem);
    fprintf(fp, "%s,%s\n", name, ip);
    sem_post(sem);
}
