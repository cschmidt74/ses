

#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace std;



void attribut_aendern(char *p){
    *p = 'C';
}
// the function being executed as thread 2 writing x
void* main_thread(void* parameterPtr) {

    // explicit cast of parameterPtr into a pointer to int
    char *xPtr = (char *) parameterPtr;
    printf("Thread: %c \n",*xPtr);
    attribut_aendern(xPtr);
    printf("Nach Aenderung: %c \n",*xPtr);
    return NULL;
}

// main runs thread 1 writing y
// and creates 2nd thread to write x in parallel
int main() {

    // thread ID for second thread
    pthread_t workerthread1, workerthread2;
   /* sched_param s;
    s.__sched_priority = 1;
    pthread_setschedparam(workerthread1,SCHED_BATCH,&s);
    s.__sched_priority = 1;
    pthread_setschedparam(workerthread2,SCHED_BATCH,&s);
    */
    char x,y;
    x = 'A';
    y = 'B';
    // create second thread executing function thread2_main */
    if(pthread_create(&workerthread1, NULL, main_thread, &x)) {
        fprintf(stderr,"Error: thread not created\n");
        return 1;
    }

    if ( pthread_create(&workerthread2, NULL, main_thread,&y) ) {
        fprintf(stderr,"Error: thread not created\n");
        return 2;
    }
    sleep(1);
    return 0;
}
