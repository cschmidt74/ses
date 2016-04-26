#include <pthread.h>
#include <unistd.h>
#include <iostream>
using namespace std;

// the function being executed as thread 2 writing x
void* main_thread2(void* parameterPtr) {

    // explicit cast of parameterPtr into a pointer to int
    int *xPtr = (int *) parameterPtr;

    // increment x up to 10 and write on screen
    while ( ++(*xPtr) < 10 ) {
        cout << "(x=" << *xPtr << ")" << flush;
        sleep(1); // wait a second
    }

    cout << endl << "reached end of incrementing x" << endl;

    // return NULL as function demands for a return value
    return NULL;
}

// main runs thread 1 writing y
// and creates 2nd thread to write x in parallel
int main() {

    // variables we modify in two threads
    int x = 0, y = 0;

    // show the initial values of x and y
    cout << "start count x=" << x << ", y=" << y << endl;

    // thread ID for second thread
    pthread_t thread2_id;

    // create second thread executing function thread2_main */
    if(pthread_create(&thread2_id, NULL, main_thread2, &x)) {
        cerr << "Error: thread not created" << endl;
        return 1;
    }
    // sleep 2 seconds, than start incrementing y up to 5
    sleep(2);

    // increment y up to 5 and write on screen
    while ( ++y < 5 ) {
        cout << "(y=" << y << ")" << flush;
        sleep(1); // wait a second
    }

    cout << endl << "reached end of incrementing y" << endl;

    // wait for the second thread till it finishes
    if ( pthread_join(thread2_id, NULL) ) {
        cerr << "Error: thread not joined" << endl;
        return 2;
    }
    // show the results - x is now 10, thank you thread 2
    cout << endl << "end count x=" << x << ", y=" << y << endl;
    return 0;
}
