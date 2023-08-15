#ifndef ARRAY_H
#define ARRAY_H
#define ARRAY_SIZE 8
#define MAX_NAME_LENGTH 100
#include <semaphore.h>

typedef struct {
    char array[ARRAY_SIZE][MAX_NAME_LENGTH];                  // storage array for integers
    int front;
    int back; 
    sem_t mutex; 
    sem_t space_avail;
    sem_t items_avail;                                // array index indicating where the top is
} array;

int  array_init(array *s);                   // initialize the array
int  array_put (array *s, char *hostname);   // place element into the array, block when full
int  array_get (array *s, char **hostname);  // remove element from the array, block when empty
void array_free(array *s);                   // free the array's resources

#endif