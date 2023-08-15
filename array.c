#include "array.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>



int array_init(array * s){
    sem_init(&s->mutex, 0, 1); 
    sem_init(&s->space_avail,0,ARRAY_SIZE);
    sem_init(&s->items_avail, 0, 0);
    s->front = -1;
    s->back = -1; 
    return 0; 
}


int array_put(array * s, char * hostname){
    sem_wait(&s->space_avail);
    sem_wait(&s->mutex);
    if(s->front == -1) {
        s->front = 0;
    }
    s->back++;
    s->back %= ARRAY_SIZE;

    //insert element 
    strcpy(s->array[s->back], hostname);

    
    sem_post(&s->mutex);
    sem_post(&s->items_avail);
    return 0; 
}


int array_get(array *s , char ** hostname){
    sem_wait(&s->items_avail);
    sem_wait(&s->mutex);
    strcpy(*hostname, s->array[s->front]);
    if(s->front == s->back){
        s->front = -1;
        s->back = -1; 
    }
    else{
        s->front++;
        s->front%=ARRAY_SIZE;
    }
    sem_post(&s->mutex);
    sem_post(&s->space_avail);
    return 0;
}







void array_free(array * s){
    //free semaphores
    sem_destroy(&s->mutex);
    sem_destroy(&s->space_avail);
    sem_destroy(&s->items_avail);
    return;
}