/*
 * sensor.c
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

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

const char* TAG = "SENSOR";
int nh_fd;
static struct sockaddr_un *server, *client;
static int *serverlen;
static socklen_t *clientlen;

static sigset_t pset;
/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		writeLog(WARNING,TAG,"SIGINT signal Received");
		exit(1);
	}
	if(signo == SIGTERM){
		writeLog(WARNING,TAG,"SIGTERM signal Received");
		exit(1);
	}
	if(signo == SIGTSTP){
		writeLog(WARNING,TAG,"SIGTSTP signal Received");
		exit(1);
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

	while(1){
		/* 1.  read IPC data from network */
		bzero(ipc_read_buf,sizeof(ipc_read_buf));
		bzero(ipc_send_buf,sizeof(ipc_send_buf));
		n = readIPC_Data(*sockfd, ipc_read_buf, sizeof(ipc_read_buf));
		if(n == -1){
			goto exit;
		}
		/* 2.  get I2C or Serail data */

		/////////////////////////////

		/* 3. send result to network process */
		sprintf(ipc_send_buf,"Hello My name is Yeongjun");
		// size : dats's real length not buf size
		size = strlen(ipc_send_buf);
		n = writeIPC_Data(sockfd, ipc_send_buf, &size);


	}
exit :
	close(*sockfd);
	free(sockfd);
}
int main(int argc, char** argv){
	writeLog(NORMAL,TAG,"Start Sensor process");
	/* 0. Signal Initialize */
	initializeSignalHandler();

	/* 1. initialize IPC(Inter Process Communication) with TCP Manager */
	server = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	client = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	serverlen = (int*)malloc(sizeof(int));
	clientlen = (socklen_t*)malloc(sizeof(socklen_t));

	if(initializeLocalSocket_Server(&nh_fd, server,serverlen,NH_SOCKET) == 0){
		printf("Failed to initialize Local socket server\n");
		exit(1);
	}

	while(1){
		pthread_t* client_thread = (pthread_t*)malloc(sizeof(pthread_t));
		int hn_fd;
		hn_fd = acceptLocalSocket(&nh_fd, client, clientlen);
		printf("accept a new ipc client %d\n",hn_fd);
		pthread_create(client_thread, NULL,ipc_client_thread, &hn_fd);
	//		pthread_detach(client_thread);
	}
	while(1)
		sleep(1);
	return 0;
}
