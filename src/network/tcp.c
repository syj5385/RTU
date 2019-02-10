#include "tcp.h"

#include "../etc/log.h"

#include <stdio.h>
#include <string.h>
#include <linux/tcp.h>
#include <linux/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>


static const char* TCP_TAG = "TCP";

void freeServer(Server* server){
//	writeLog(NORMAL,TCP_TAG,"Close this Server");
	//close(server->sockfd);
	free(server);
}

int listenClient(Server* server){
	int listened ; 
	listened = listen(server->sockfd, MAX_CLIENT);
		
	return listened;
}

int bindTCP_Server(Server* server){
	if((server -> sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
//		writeLog(ERROR,TCP_TAG,"Failed to open tcp socket");
		return 0;
	}

//	bzero(server->addr, sizeof(server->addr));
	(server -> addr).sin_family = AF_INET;
	(server -> addr).sin_addr.s_addr = htonl(INADDR_ANY);
	(server -> addr).sin_port = htons(server -> port);
	

	int option=1;
	if(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) == -1){
		writeLog(WARNING,TCP_TAG,"Failed set socketoption");
	}

	if(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1){
		writeLog(WARNING,TCP_TAG,"Failed set socketoption");
	}
	server -> len = sizeof(server->addr);
	int resultBind = bind(server->sockfd, (struct sockaddr*)&server->addr,server->len);
	if(resultBind == -1){
		writeLog(ERROR,TCP_TAG,"Failed to bind socket");
		return 0;
	}

//	if(listenClient(server) == 0){
//		return 0;
//	}

	writeLog(NORMAL,TCP_TAG,"Success to bind TCP Socket");
	return 1;
}

Server* initializeTCP_Server(int port){
	Server *server = (Server*)malloc(sizeof(Server));
	if(!server)
		return NULL;

	server -> port = port; 

	// Bind Server socket
	if(bindTCP_Server(server) == 0){
		freeServer(server);
		return NULL;
	}

	return server; 
}

void freeClient(Client* client){

	/* close tcp client */
	close(client->sockfd);
	free(client);
	client = NULL;
}

int tcpSetKeepAlive(int sockfd, int nKeepAlive_, int nKeepAliveIdle_, int nKeepAliveCnt_, int nKeepAliveInterval_){
	int result;
	if((result = setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &nKeepAlive_, sizeof(nKeepAlive_))) == -1){
		writeLog(WARNING,TCP_TAG,"[TCP Server]Failed: setsockopt():so_keepalive");
		return 0;
	}
//	if((result = setsockopt(*sockfd, 6, TCP_KEEPIDLE, &nKeepAliveIdle_, sizeof(nKeepAliveIdle_))) == -1){
//		writeLog(WARNING,"[TCP Server]Failed: setsockopt():so_keepalive");
//		return 0;
//	}
//	if((result = setsockopt(*sockfd, 6, TCP_KEEPCNT, &nKeepAliveCnt_, sizeof(nKeepAliveCnt_))) == -1){
//		writeLog(WARNING,"[TCP Server]Failed: setsockopt():so_keepalive");
//		return 0;
//	}
//	if((result = setsockopt(*sockfd, 6, TCP_KEEPINTVL, &nKeepAliveInterval_, sizeof(nKeepAliveInterval_))) == -1){
//		writeLog(WARNING,"[TCP Server]Failed: setsockopt():so_keepalive");
//		return 0;
//	}
	return 1;
}

int acceptClient(Server* server, Client* client){
//	Client* client = (Client*)malloc(sizeof(Client));
	if(!client)
		return 0;

	// Initialize Client structure
	// this structure must free after using


	/* client -> message would be managed in network process \*
	//client -> message = (MB_MSG*)malloc(sizeof(MB_MSG));


	// Accept Client 
	*client -> len = sizeof(client->addr);
	/* network thread will be blocked until new client is accepted */
	client -> len = sizeof(client->addr);
	client -> sockfd = accept(server->sockfd, (struct sockaddr*)&(client->addr), &client->len);
	char log_buf[200];
	bzero(log_buf,sizeof(log_buf));
	sprintf(log_buf, "Client IP Address : %s",inet_ntoa((client->addr).sin_addr));
	writeLog(NORMAL,TCP_TAG,log_buf);
	if(tcpSetKeepAlive(client->sockfd, 1, 30,5,2) == 0){
		writeLog(WARNING,TCP_TAG,"Failed to setsockopt");
		return -1; 
	}

	return 1;
}

int readTcpData(Client* client, uint8_t* buf){
	int n;
	bzero(buf,sizeof(buf));
	n = recv(client->sockfd, buf, sizeof(buf),0);

	return n;
}

int writeTcpMessage(Client* client, uint8_t* buf, size_t* len){
	int n=0;
	n = send(client->sockfd, buf,*len,0);
	return n;
}

