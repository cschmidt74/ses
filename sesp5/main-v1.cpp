#include <unistd.h>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <new>

#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>

#include "CBinarySemaphore.h"
#include "CCommQueue.h"

#define SHOW_MESSAGE_INFO true

extern int errno;

// This funtion creates or opens a shared memory and return the file descriptor
int acces_shared_memory() {
	int fd=shm_open("ese_shm",O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	bool success = true;
	if (fd == -1) {
		if (errno != EEXIST) {
			printf("!!! cant open !!!\n");
			success = false;
		} else {
			fd = shm_open("ese_shm", O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
			if (fd == -1) {
				printf("!!! check permission !!!\n");
				success = false;
			} else {
				printf("open existing shared memory.\n");
			}
		}
	} else {
		printf("created shared memory.\n");
	}
	if (true == success) {
		ftruncate(fd, 1000);
	}
	return fd;
}

void child_func() {
	printf("Child ready.\n");

	int fd = acces_shared_memory();
	char* ptr = (char *) mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	
	CBinarySemaphore* ptr_sem = new(ptr) CBinarySemaphore();
	CCommQueue* ptr_queue = new(ptr + sizeof(CBinarySemaphore)) CCommQueue(4, *ptr_sem);	

	sleep(1); // Wait for other process to finish startup

	CMessage msg;
	const Int8* buff;
	bool end = false;
	while(!end) {
		bool received = ptr_queue->getMessage(msg);
		if(received==true) {
			buff = msg.getParam4();
			#if SHOW_MESSAGE_INFO			
			printf("A Message was received! : %s\n", buff);
			#endif
			if(strcmp((char*)buff, "END")==0) end = true;
		};
	}
};	

void parent_func() {
	printf("Parent ready.\n");

	int fd = acces_shared_memory();
	char* ptr = (char *) mmap(NULL, 1000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	CBinarySemaphore* ptr_sem = new(ptr) CBinarySemaphore();
	CCommQueue* ptr_queue = new(ptr + sizeof(CBinarySemaphore)) CCommQueue(4, *ptr_sem);

	sleep(1); // Wait for other process to finish startup
	
	CMessage msg;
	Int8 msg_content[16] = "";

	for(int i=0; i<1000; i++) {
		sprintf((char*)msg_content, "%ld", i);
		msg.setParam4(msg_content, 16);
		ptr_queue->add(msg, true);
		#if SHOW_MESSAGE_INFO		
		printf("Sending! : %d\n", i);
		#endif
	}
	
	// Sending END Message
	sleep(1);
	strcpy((char*)msg_content, "END");
	msg.setParam4(msg_content, 16);
	ptr_queue->add(msg, true);
	printf("Sending END!\n");
};

int main() {
	pid_t pid = fork();
		
	if(pid==-1) return -1;
	if(pid==0) child_func();
	if(pid>0) {
		parent_func(); 
		waitpid(pid, NULL, 0);
		shm_unlink("ese_shm");
	}	
	return 0;
}
