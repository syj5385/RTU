#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <signal.h>
#include <stdint.h>

#include "../define.h"
#include "../etc/log.h"
#include "../etc/ipc.h"

static sigset_t pset;
static const char* HARDWARE_TAG = "HARDWARE";
const char *sensor_pid_file = "/var/run/sensor.pid";
/*  this variable can be used as msgid in case of IPC_MSGQUEUE
 *  ipc sock file descriptor
*/
static int *nh_fd;
static struct sockaddr_un *server, *client;
static int *serverlen;
static socklen_t *clientlen;


/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		writeLog(WARNING,HARDWARE_TAG,"SIGINT signal Received");
		closeLogFile();
		exit(0);
	}
	if(signo == SIGTERM){
		writeLog(WARNING,HARDWARE_TAG,"SIGTERM signal Received");
		closeLogFile();
		exit(0);
	}
	if(signo == SIGTSTP){
		writeLog(WARNING,HARDWARE_TAG,"SIGTSTP signal Received");
		closeLogFile();
		exit(0);
	}
}
static void initializeSignalHandler(){
	sigemptyset(&pset);
	sigaddset(&pset,SIGQUIT);
	sigprocmask(SIG_BLOCK, &pset, NULL);

	if(signal(SIGINT,sigHandler) == SIG_ERR){
		exit(1);
	}
	if(signal(SIGTERM,sigHandler) == SIG_ERR){
		exit(1);
	}
	if(signal(SIGTSTP,sigHandler) == SIG_ERR){
		exit(1);
	}
}

void* ipc_client_thread(void* arg){
	int n;
	size_t size;
	uint8_t ipc_send_buf[IPC_BUF_SIZE];
	uint8_t ipc_read_buf[IPC_BUF_SIZE];
	int* sockfd = (int*)arg;
	int i=0;

	while(1){
	/* 1.  read IPC data from network */
		bzero(ipc_read_buf,sizeof(ipc_read_buf));
		bzero(ipc_send_buf,sizeof(ipc_send_buf));

		n = readIPC_Data(*sockfd, ipc_read_buf, IPC_BUF_SIZE);
		if(n==0)
			continue;
		if(n == -1)
			goto close;


		for(i=0; i<n ; i++)
			printf("%d ", (int)ipc_read_buf[i]);
		printf("\n");
		/* 2.  get I2C or Serial data */
		/////////////////////////////
		//usleep(500000);
		/* 3. send result to network process */

		// size : dats's real length not buf size
		size = n;
		n = writeIPC_Data(sockfd, ipc_read_buf, &size);
	}
close :
	close(*sockfd);
	free(sockfd);
}

int main(int argc, char** argv){

	if(openLogFile() == 0)
		exit(1);

	writeLog(NORMAL,HARDWARE_TAG, "Start Hardware Process");

	pid_t sensor_pid = getpid();
	FILE* pid_file = fopen(sensor_pid_file,"w+");
	if(!pid_file)
		return 1;

	fprintf(pid_file,"%d",(int)sensor_pid);
	fclose(pid_file);

	/* 0. Signal Initialize */
	initializeSignalHandler();

	/* 1. initialize IPC(Inter Process Communication) with TCP Manager */
	nh_fd = (int*)malloc(sizeof(int));



	server = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	client = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	serverlen = (int*)malloc(sizeof(int));
	clientlen = (socklen_t*)malloc(sizeof(socklen_t));

	if(initializeLocalSocket_Server(nh_fd, server, serverlen, NH_SOCKET) == 0){
		exit(1);
	}

	while(1){
		pthread_t* client_thread = (pthread_t*)malloc(sizeof(pthread_t));
		int *hn_fd = (int*)malloc(sizeof(int));
		*hn_fd = acceptLocalSocket(nh_fd, server, serverlen);
		pthread_create(client_thread, NULL,ipc_client_thread, hn_fd);
//		pthread_detach(client_thread);
	}
	return 0;
}
