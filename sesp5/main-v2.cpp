#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdint.h>
#include "CCommQueue.h"
#include <sstream>

using namespace std;

#define SHM_NAME        "/estSHM"
#define QUEUE_SIZE      100
#define NUM_MESSAGES    10000

bool run = true;

void setPriority(int prio)
{
  if (prio < 1)
    prio = 1;
  else if (prio > 98)
    prio = 98;

  sched_param myprio;
  myprio.sched_priority = prio;
  sched_setscheduler(0, SCHED_FIFO, &myprio);
}


class PingMessageHandler : public IMessageHandler
{
public:
  virtual void handleMessage(const CMessage& msg)
  {
    cout << "PingHandler" << endl;
    cout << "From: " << (char)msg.getSenderID() << endl;
    cout << "To: " << (char)msg.getReceiverID() << endl;
    cout << "Type: " << msg.getMessageType() << endl;
    cout << "Param1: " << msg.getParam1() << endl;
    cout << "Param4: " << msg.getParam4() << endl;
    cout << endl;
  }
};

class PongMessageHandler : public IMessageHandler
{
public:
  virtual void handleMessage(const CMessage& msg)
  {
    cout << "PongHandler" << endl;
    cout << "From: " << (char)msg.getSenderID() << endl;
    cout << "To: " << (char)msg.getReceiverID() << endl;
    cout << "Type: " << msg.getMessageType() << endl;
    cout << "Param1: " << msg.getParam1() << endl;
    cout << "Param4: " << msg.getParam4() << endl;
    cout << endl;
  }
};

timespec totaltime;
timespec runtime;
int total_message_to_send;
int queue_depth;
bool verbose;
bool scheduler;
int prio_parent;
int prio_child;

class TimeMeasureHandler : public IMessageHandler
{
public:
  TimeMeasureHandler()
  {
    runtime = getTime();
  }

  inline void setupMessage(CMessage &msg)
  {
    timespec value = getTime();
    msg.setParam4((Int8*)&value, sizeof(timespec));
  }

  virtual void handleMessage(const CMessage &msg)
  {
    timespec diff = sub(getTime(), (*(timespec*)msg.getParam4()));
    totaltime = add(totaltime, diff);
  }

  static inline timespec sub(timespec a, timespec b)
  {
    timespec result = a;
    result.tv_sec -= b.tv_sec;
    result.tv_nsec -= b.tv_nsec;
    if (result.tv_nsec < 0)
    {
      result.tv_nsec += 1 * 1000 * 1000 * 1000;
      result.tv_sec -= 1;
    }

    return result;
  }

 static inline timespec add(timespec a, timespec b)
  {
    timespec result = a;
    result.tv_sec += b.tv_sec;
    result.tv_nsec += b.tv_nsec;
    if (result.tv_nsec >=  1 * 1000 * 1000 * 1000)
    {
      result.tv_nsec -= 1 * 1000 * 1000 * 1000;
      result.tv_sec += 1;
    }

    return result;
  }

  static inline timespec getTime()
  {
    timespec value;
    clock_gettime(CLOCK_REALTIME, &value);
    return value;
  }
};

class ShutdownHandler : public IMessageHandler
{
public:
  virtual void handleMessage(const CMessage& msg)
  {
    runtime = TimeMeasureHandler::sub(TimeMeasureHandler::getTime(), runtime);
    cout << "Shutdown received" << endl;
    Int64 value = totaltime.tv_nsec;
    value += Int64(totaltime.tv_sec) * Int64(1 * 1000 * 1000 * 1000);
    cout << "Total time : " << totaltime.tv_sec << " . " << totaltime.tv_nsec << endl;
    cout << "Time per Message : " << value / total_message_to_send << endl;
    cout << "Runtime: " << runtime.tv_sec << " . " << runtime.tv_nsec << endl;
    exit(0);
  }
};

PingMessageHandler pinger;
PongMessageHandler ponger;
ShutdownHandler shutdownhandler;
TimeMeasureHandler timehandler;

template<typename T>
void convert(const char* src, T &value)
{
  stringstream ss;
  ss << src;
  ss >> value;
}

#include <time.h>

int main(int argc, const char** argv)
{
  if (argc != 6)
  {
    cout << "usage: arg1 = message till terminate; arg2 = queue depth; arg3 = bool turn on verbose, arg4 = prio parent; arg5 = prio child";
    return 0;
  }

  convert(argv[1], total_message_to_send);
  convert(argv[2], queue_depth);
  convert(argv[3], verbose);
  convert(argv[4], prio_parent);
  convert(argv[5], prio_child);

  totaltime.tv_sec = 0;
  totaltime.tv_nsec = 0;
  timespec mytime;
  

  cout << "Open shared Memory" << endl;
  int memfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

  if (memfd < 0)
  {
    cerr << "error opnening shared memory" << endl;
    abort();
  }

  //////////////////////////////////

  cout << "Setting memory size" << endl;
  int memsize = CCommQueue::getNumOfBytesNeeded(queue_depth) + sizeof(CBinarySemaphore);

  int truncresult = ftruncate(memfd, memsize);

  //////////////////////////////////

  cout << "Mapping memory to address space" << endl;
  void* memptr = mmap(NULL, memsize, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);

  if (memptr == 0)
  {
    cerr << "error maping shared memory" << endl;
    abort();
  }

  ///////////////////////////////////

  cout << "Creating Signaling Semaphore in memory using placement new" << endl;
  uint8_t *curptr = (uint8_t*) memptr;
  CBinarySemaphore *sigsem = new(curptr) CBinarySemaphore();

  ///////////////////////////////////

  cout << "Creating Queue in memory using placement new" << endl;
  curptr += sizeof(CBinarySemaphore);
  CCommQueue *comqueue = new(curptr)CCommQueue(queue_depth, *sigsem);

  ///////////////////////////////////

  cout << "Creating a child process ..." << endl;
  pid_t pid = fork();

  if (0 == pid)
  {
    setPriority(prio_child);
    // Child process - Reads all Messages from the Queue and outputs them with auxiliary data.
    CMessage rcvmsg;
    while (true)
    {
      if(verbose)
        cout << "Child waiting on Message" << endl;
      sigsem->take(); // queue is not empty
      while (comqueue->getMessage(rcvmsg))
      {
        if (verbose)
          cout << "Child got Message" << endl;
        IMessageHandler * hndl = rcvmsg.getMessageHandlerPtr();
        hndl->handleMessage(rcvmsg);
      }
      if (verbose)
        cout << "Queue empty now!" << endl;
    }
  }
  else if (pid > 0)
  {
    setPriority(prio_parent);
    // Parent process - Writes all Messages into the Queue
    CMessage sndmsg;
    //sndmsg.setMessageType(CMessage::Internal_App_Type);
    //sndmsg.setSenderID('P');
    //sndmsg.setReceiverID('C');
    int msgnum = 0;
    while (run)
    {
      int msgnumend = msgnum + (rand() % 10 + 1);
      while (msgnum < msgnumend)
      {
        ++msgnum;
        if (verbose)
        {
          sndmsg.setParam1(msgnum);
          if (msgnum & 0x1)
          {
            sndmsg.setMessageHandlerPtr(&pinger);
          }
          else
          {
            sndmsg.setMessageHandlerPtr(&ponger);
          }
          Int8 buffer[11];
          int offset = msgnum % 10;
          for (int i = 0; i < 10; ++i)
          {
            buffer[i] = 'A' + offset + i;
          }
          buffer[10] = 0;
          sndmsg.setParam4(buffer, 11);


          cout << "Parent adding Message" << endl;
        }
        else
        {
          timehandler.setupMessage(sndmsg);
          sndmsg.setMessageHandlerPtr(&timehandler);
        }
        comqueue->add(sndmsg);

        if (msgnum >= total_message_to_send)
        {
          CMessage shtdwn;
          shtdwn.setMessageHandlerPtr(&shutdownhandler);
          if (verbose)
            cout << "Sending shutdown" << endl;
          comqueue->add(shtdwn);
          // now wait pid child
          waitpid(pid, 0, 0);
          // now close shared memory
          if (verbose)
            cout << "closing shared memory" << endl;
          shm_unlink(SHM_NAME);
          // now exit parent
          if (verbose)
            cout << "Parent going down" << endl;
          exit(0);
        }
      }
      //sleep(1);
    }
  }
  else
  {
    // Error
    cerr << "Couldn't create a child process. Exiting" << endl;
    abort();
  }

  return 0;
}
