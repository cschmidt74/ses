

#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <stdio.h>

using namespace std;

// the function being executed as thread 2 writing x
void* main_thread2(void* parameterPtr) {

    // explicit cast of parameterPtr into a pointer to int
    int *xPtr = (int *) parameterPtr;

    // increment x up to 10 and write on screen
    while ( ++(*xPtr) < 200 ) {
        printf("x= %d",*xPtr);
      //  fflush(stdout);
        //sleep(1); // wait a second
    }

    printf("reached end of incrementing x");
  //  fflush(stdout);
    // return NULL as function demands for a return value
    return NULL;
}

// main runs thread 1 writing y
// and creates 2nd thread to write x in parallel
int main() {

    // variables we modify in two threads
    int x = 0, y = 0;

    // show the initial values of x and y
    printf("start count x= %d, y= %d",x,y);
    //fflush(stdout);

    // thread ID for second thread
    pthread_t thread2_id;

    // create second thread executing function thread2_main */
    if(pthread_create(&thread2_id, NULL, main_thread2, &x)) {
        fprintf(stderr,"Error: thread not created\n");
        return 1;
    }
    // sleep 2 seconds, than start incrementing y up to 5
    //sleep(2);
    sleep(3);
    // increment y up to 5 and write on screen
    while ( ++y < 200) {
        printf("(y=%d)",y);
        //  fflush(stdout);
       // sleep(1); // wait a second
    }
    printf("reached end of incrementing y");
   // fflush(stdout);

    // wait for the second thread till it finishes
    if ( pthread_join(thread2_id, NULL) ) {
        fprintf(stderr,"Error: thread not joined\n");
        return 2;
    }
    // show the results - x is now 10, thank you thread 2
    printf("end of count x= %d, y= %d\n",x,y);
  //  fflush(stdout);
    return 0;
}


/*
 * Zu 3
printf Systemcall= write(1, "(y=3)\n", 6(y=3))

 Zu 4
 Welchen Puffer gibt es?


 Zu 5
 Was heißt zeichen abwechselnd ausgeben hääääääää?!?!

 Zu 6
 Geht nur mit PI BIHATCH
 */
