#include "tcp.h"

#include "../etc/log_util.h"
#include <stdio.h>
#include <string.h>
#include <linux/tcp.h>
#include <linux/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>




static const char* TCP_TAG = "TCP";

void freeServer(Server* server){
	free(server);
}

int listenClient(Server* server){
	int listened ; 
	listened = listen(server->sockfd, MAX_CLIENT);
		
	return listened;
}

int bindTCP_Server(Server* server){
	if((server -> sockfd = socket(AF_INET,SOCK_STREAM,0)) == -1){
		return 0;
	}

	(server -> addr).sin_family = AF_INET;
	(server -> addr).sin_addr.s_addr = htonl(INADDR_ANY);
	(server -> addr).sin_port = htons(server -> port);
	

	int option=1;
	if(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option)) == -1){
		LOG_ERROR("Failed to set socketoption");
		return -1; 
	}

	if(setsockopt(server->sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1){
		LOG_ERROR("Failed to set socketoption");
		return -1; 
	}
	server -> len = sizeof(server->addr);
	int resultBind = bind(server->sockfd, (struct sockaddr*)&server->addr,server->len);
	if(resultBind == -1){
		LOG_ERROR("Failed to bind");
		return -1;
	}

	return 0;
}

Server* initializeTCP_Server(int port){
	Server *server = (Server*)malloc(sizeof(Server));
	if(!server)
		return NULL;

	server -> port = port; 

	// Bind Server socket
	if(bindTCP_Server(server) < 0){
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

		LOG_ERROR("Failed to set KeepAlive");
		return -1;
	}
	return 0;
}

int acceptClient(Server* server, Client* client){
	if(!client)
		return 0;

	/* network thread will be blocked until new client is accepted */
	client -> len = sizeof(client->addr);
	client -> sockfd = accept(server->sockfd, (struct sockaddr*)&(client->addr), &client->len);
	if(client -> sockfd < 0)
		return -1; 
	char log_buf[200];
	bzero(log_buf,sizeof(log_buf));
	sprintf(log_buf, "New Client Accepted : Client IP Address : %s",inet_ntoa((client->addr).sin_addr));
	LOG_TRACE(log_buf);
	if(tcpSetKeepAlive(client->sockfd, 1, 30,5,2) < 0){
		LOG_ERROR("Failed to set KeepAlive");
		return -1; 
	}

	return 0;
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

