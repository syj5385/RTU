 /*
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "../define.h"
#include "../etc/ipc.h"
#include "../etc/config.h"
#include "../protocol/rtu_serial.h"
#include "tcp.h"
#include "../etc/log_util.h"

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
		LOG_WARNING("SIGINT signal Received");
		close(server->sockfd);
		freeServer(server);
		exit(EXIT_KILL_ALL);
	}
	if(signo == SIGTERM){
		LOG_WARNING("SIGTERM signal Received");
		close(server->sockfd);
		freeServer(server);
		exit(EXIT_KILL_ALL);
	}
	if(signo == SIGTSTP){
		LOG_WARNING("SIGTSTP signal Received");
		close(server->sockfd);
		freeServer(server);
		exit(EXIT_KILL_ALL);
	}
}
static void initializeSignalHandler(){
	sigemptyset(&pset);
	sigaddset(&pset,SIGQUIT);
	sigprocmask(SIG_BLOCK, &pset, NULL);

	if(signal(SIGINT,sigHandler) == SIG_ERR){
		exit(EXIT_KILL_ME);
	}
	if(signal(SIGTSTP, sigHandler) == SIG_ERR){
		exit(EXIT_KILL_ME);
	}
	if(signal(SIGTERM,sigHandler) == SIG_ERR){
		exit(EXIT_KILL_ME);
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
	int execute_result; 
	int connect_result; 
	int analysis_result; 
	int i=0; 
	
	//connect_result = (int*)malloc(sizeof(int));
	
	LOG_CALL(connect_result = initialzieLocalSocket_Client(&client->ipc.ipc_sockfd, &(client->ipc.ipc_addr), &(client->ipc.ipc_addr_len),NH_SOCKET,10000));


	if(connect_result == -1){
		/* This code is for situation that local socket is not connected */
		/* If failed, connectiong with PC client will be disconnected 
		   after send error message */ 
		LOG_ERROR("ERROR : Failed to connect Local socket");
		frame -> opcode = 0xff;
		frame -> size = 0;
		memset(frame->data,0, sizeof(frame->data));
		if((tcp_n = send(client->sockfd, tcp_send_buf, frame->size+7 ,0)) < 0){
			LOG_ERROR("Error : Failed to send Message");
			errorcode = 1;
		}
		free(frame);
		goto close; 
	}

	while(1){
		frame = (Frame*)malloc(sizeof(Frame));
		bzero(tcp_read_buf,sizeof(tcp_read_buf));
		bzero(ipc_buf, sizeof(ipc_buf));
		tcp_n = recv(client -> sockfd, tcp_read_buf,BUFSIZ,0);

		if(tcp_n <= 0){
			free(frame);
			goto close;
		}
		LOG_CALL(analysis_result = analyzeReceivedMsg(frame, tcp_read_buf,tcp_n));
		if(analysis_result == RESULT_INVALID){
			LOG_WARNING("Message from Client is not valid");
			frame -> opcode = 0xff;
			frame -> size = 0;
			memset(frame->data,0, sizeof(frame->data));
			goto send; 
		}
		
		LOG_CALL(execute_result = executeProtocol(frame,&client->ipc.ipc_sockfd));
		if(execute_result != 0 || frame -> opcode == 0xff) {
			/* Failed to execute protocol command */ 
			printf("Network : Failed to execute protocol\n");
			frame -> opcode = 0xff;
			frame -> size = 6;
			memset(frame->data,0,sizeof(frame->data));
			for(i=0; i<6; i++)
				frame->data[i] = 0xff; 
			goto send; 
		}

		LOG_TRACE("Success to get result");

send :
		makeMsgUsingframe(frame, tcp_send_buf);

		if((tcp_n = send(client->sockfd, tcp_send_buf, frame->size+7 ,0)) < 0){
			LOG_ERROR("Error : Failed to send Message");
			errorcode = 1;
			free(frame);
			goto close;
		}
		/* Success to process one protocol message */
		/* After that, Wait for new message */ 
		free(frame);
	}

close :
	pthread_mutex_lock(&socket_mutex);
	/* close ipc sockfd */
	close((client -> ipc).ipc_sockfd);
	freeClient(client);
	pthread_mutex_unlock(&socket_mutex);
	decreaseClient();

}

void* tcpServerThread(void* arg){
	/* Loop for Accepting Client
	 * If new Client is accepted, it will be assigned to process thread.
	 * This Thread is blocked until new client is accepted.
	 */
	int listened;
	int accepted; 
	while(1){
		listened = listenClient(server);
		if(listened < 0){
			LOG_ERROR("Error : Failed to listen client");
			exit(EXIT_KILL_ME);
		}

		Client *client = (Client*)malloc(sizeof(Client));
		bzero(client, sizeof(Client));

		LOG_CALL(accepted = acceptClient(server,client));
		if(accepted < 0){
			LOG_ERROR("Error : Failed to accept client");
			exit(EXIT_KILL_ME);
		}
		addClient();
		if(client == NULL){
			decreaseClient();
			continue;
		}

		// if new client is accepted
		pthread_create(&client->thread, NULL, newClientThread, client);
		pthread_detach(client->thread);
	}
}

int main(int argc, char** argv){
	//getConfigContent("SERVERNAME");

	LOGsetInfo(LOG_PATH,basename(argv[0]));
	LOG_TRACE("Start NetworkProcess");


	initializeSignalHandler();

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
	TCPPORT = getConfigPort(); 
	RTU_ID = getRTUID();
	if(TCPPORT <= 0x00 || TCPPORT >= 0xffff){
		LOG_ERROR("Error : Port number is not valied");
		exit(EXIT_NORMAL);
	}
	sprintf(log_buf,"bind port : %d",TCPPORT);
	LOG_TRACE(log_buf);
	LOG_TRACE("Binding Port");
	if((server = initializeTCP_Server(TCPPORT)) == NULL){
		/* Failed to create TCP Server socket.
		 * In this case, this process has to send quit command to main and hardware process
		 */
		exit(EXIT_NORMAL);
	}
	pthread_create(&server->thread, NULL,tcpServerThread, NULL);

	/* 3. Read Fifo command from Hardware process */
	pthread_join(server->thread , NULL);

	exit(EXIT_NORMAL);
}
