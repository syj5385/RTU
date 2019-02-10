
#ifndef _TCP_H_
#define _TCP_H_

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../define.h"
#include "../etc/ipc.h"

/* TCP Network connection */
/* network Structure definition */
typedef struct{
	int sockfd;
	socklen_t len;
	struct sockaddr_in addr;
	pthread_t thread;
	void* message;
	/* ipc information */
	IPC_CLIENT ipc;
}Client;

typedef struct{
	int sockfd;
	socklen_t len;
	struct sockaddr_in addr;
	int port;
	pthread_t thread;
}Server;

/* Error Code */
#define SUCCESS_TO_ANALYSIS	0
#define FAILED_NETWORK		1


Server* initializeTCP_Server(int port);
int listenClient(Server* server);
int acceptClient(Server* server, Client* client);

void freeClient(Client* client);
void freeServer(Server* server);
int readTcpData(Client* client,uint8_t* buf);
int writeTcpMessage(Client* client, uint8_t* buf, size_t* len);


#endif /*_TCP_H_*/
