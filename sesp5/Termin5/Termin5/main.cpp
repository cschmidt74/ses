#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <CCommQueue.h>

using namespace std;

#define SHM_NAME        "/estSHM"
#define QUEUE_SIZE      100
#define NUM_MESSAGES    10000

int main()
{
    cout << "Creating a child process ..." << endl;
    pid_t pid = fork();

    if (0 == pid)
    {
        // Child process - Reads all Messages from the Queue and outputs them with auxiliary data.
    }
    else if (pid > 0)
    {
        // Parent process - Writes all Messages into the Queue

    }
    else
    {
        // Error
        cerr << "Couldn't create a child process. Exiting" << endl;
        abort();
    }

    return 0;
}
