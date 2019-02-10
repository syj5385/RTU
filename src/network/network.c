 /*
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../define.h"
#include "../etc/log.h"
#include "../etc/ipc.h"
#include "../etc/config.h"
#include "../protocol/rtu_serial.h"
#include "tcp.h"

#define	PORT	1

const char* TAG = "NETWORK";

const char *network_pid_file = "/var/run/network.pid";

static Server* server;
static pthread_mutex_t client_mutex;
static pthread_mutex_t socket_mutex;
static int clientNum = 0;

static void addClient(){
	char log_buf[100];
	pthread_mutex_lock(&client_mutex);
	clientNum++;
	pthread_mutex_unlock(&client_mutex);
}

static void decreaseClient(){
	char log_buf[100];
	pthread_mutex_lock(&client_mutex);
	clientNum--;
	pthread_mutex_unlock(&client_mutex);
}

/*
 *  Process Signal initialize And Handler Definition
 */
static sigset_t pset;
/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		writeLog(WARNING,TAG,"SIGINT signal Received");
		close(server->sockfd);
		freeServer(server);
		closeLogFile();
		exit(0);
	}
	if(signo == SIGTERM){
		writeLog(WARNING,TAG,"SIGTERM signal Received");
		close(server->sockfd);
		freeServer(server);
		closeLogFile();
		exit(0);
	}
	if(signo == SIGTSTP){
		writeLog(WARNING,TAG,"SIGTSTP signal Received");
		close(server->sockfd);
		freeServer(server);
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
	if(signal(SIGTSTP, sigHandler) == SIG_ERR){
		exit(1);
	}
	if(signal(SIGTERM,sigHandler) == SIG_ERR){
		exit(1);
	}
}

void* newClientThread(void* arg){
	uint8_t tcp_read_buf[BUFSIZ];
	uint8_t tcp_send_buf[BUFSIZ];
	uint8_t ipc_buf[IPC_BUF_SIZE];
	Frame* frame;
	int result;
	size_t size;
	int tcp_n, ipc_n;
	Client* client = (Client*)arg;
	int errorcode = 0;

	if(!initialzieLocalSocket_Client(&client->ipc.ipc_sockfd, &(client->ipc.ipc_addr), &(client->ipc.ipc_addr_len),NH_SOCKET,10000)){
		exit(1);
	}

	while(1){
		frame = (Frame*)malloc(sizeof(Frame));
		bzero(tcp_read_buf,sizeof(tcp_read_buf));
		bzero(ipc_buf, sizeof(ipc_buf));
		tcp_n = recv(client -> sockfd, tcp_read_buf,BUFSIZ,0);

		if(tcp_n <= 0){
			result = FAILED_NETWORK;
			errorcode = 1;
			goto close;
		}
		int analysis_result = analyzeReceivedMsg(frame, tcp_read_buf,tcp_n);
/*		int execute_result = executeProtocol(frame,&client->ipc.ipc_sockfd);

		makeMsgUsingframe(frame, tcp_send_buf);

		if((tcp_n = send(client->sockfd, tcp_send_buf, frame->size+9 ,0)) < 0){
			writeLog(ERROR,TAG,"Error : Failed to send message\n");
			errorcode = 1;
			goto close;
		}*/



		if((tcp_n = send(client->sockfd, tcp_read_buf, frame->size+9 ,0)) < 0){
			writeLog(ERROR,TAG,"Error : Failed to send message\n");
			errorcode = 1;
			goto close;
		}

		free(frame);
	}

check :
/* analysis of result code */
	switch(result){
	case FAILED_NETWORK :
		writeLog(ERROR,TAG,"ERROR:Failed to read tcp data");
		goto close;

	}
close :
	pthread_mutex_lock(&socket_mutex);
	/* close ipc sockfd */
	close((client -> ipc).ipc_sockfd);
	freeClient(client);
	pthread_mutex_unlock(&socket_mutex);
	goto exit;

exit :
	decreaseClient();

}

void* tcpServerThread(void* arg){
	/* Loop for Accepting Client
	 * If new Client is accepted, it will be assigned to process thread.
	 * This Thread is blocked until new client is accepted.
	 */
	while(1){
		if(listenClient(server) < 0){
			writeLog(ERROR, TAG, " Error : failed to listen client");
			exit(1);
		}

		Client *client = (Client*)malloc(sizeof(Client));
		bzero(client, sizeof(client));

		if(acceptClient(server,client) < 0){
			writeLog(ERROR, TAG, " Error : failed to listen client");
			exit(1);
		}
		addClient();
		if(client == NULL){
			decreaseClient();
			continue;
		}

		// if new client is accepted
		pthread_create(&client->thread, NULL, newClientThread, client);
		pthread_detach(client->thread);
		usleep(50000);
	}
}

int main(int argc, char** argv){
	//getConfigContent("SERVERNAME");

	if(openLogFile() == 0)
		exit(1);

	writeLog(NORMAL,TAG,"Start Network process");



	pid_t network_pid = getpid();
	FILE* pid_file = fopen(network_pid_file,"w+");
	if(!pid_file)
		return 1;

	fprintf(pid_file,"%d",(int)network_pid);
	fclose(pid_file);

	/*
	 *  Initialize TCP Server
	 */
	char log_buf[1000];
	int user_port = getConfigPort(); 
	if(user_port <= 0x00 || user_port >= 0xffff){
		writeLog(ERROR,TAG,"Error : port number is not valid");
		perror("Error : Port number is invalid");
		exit(0);
	}
	sprintf(log_buf,"bind port : %d",getConfigPort());
	writeLog(NORMAL,TAG,log_buf);
	if((server = initializeTCP_Server(getConfigPort())) == NULL){
		/* Failed to create TCP Server socket.
		 * In this case, this process has to send quit command to main and hardware process
		 */
		exit(1);
	}
	pthread_create(&server->thread, NULL,tcpServerThread, NULL);

	/* 3. Read Fifo command from Hardware process */
	pthread_join(server->thread , NULL);

	return 0;
}
