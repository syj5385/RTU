#ifndef _IPC_H_
#define _IPC_H_

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <stdint.h>

#include "../define.h"

#define IPC_BUF_SIZE	100
#define 	NH_SOCKET	"/etc/rtu/nhsocket"

typedef struct{
	int ipc_sockfd;
	struct sockaddr_un ipc_addr;
	socklen_t ipc_addr_len;
}IPC_CLIENT;

int initializeLocalSocket_Server(int* sockfd, struct sockaddr_un* server, int* serverSize, const char* sockpath);
int acceptLocalSocket(int* serversockfd, struct sockaddr_un* client, socklen_t* size);
int initialzieLocalSocket_Client(int* sockfd, struct sockaddr_un* client, socklen_t* size, const char* sockpath, int TIMEOUT);
int writeIPC_Data(int* sockfd, void* buf, size_t* size);
int readIPC_Data(int sockfd, uint8_t* buf, size_t size);

#endif /*_IPC_H_*/
