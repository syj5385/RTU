
#include "ipc.h"
#include "log_util.h"


#include <unistd.h>
#include <sys/socket.h>

static const char* IPC_TAG = "IPC";

/* Named Local Socket implementation */
int initializeLocalSocket_Server(int* sockfd, struct sockaddr_un *server, int* serverSize, const char* sockpath){
	int r;
	unlink(sockpath);
	*sockfd = socket(PF_LOCAL,SOCK_STREAM,0);
	bzero(server,sizeof(server));
	server->sun_family = AF_LOCAL;
	strcpy(server->sun_path, sockpath);
	*serverSize = sizeof(*server);
	r = bind(*sockfd, (struct sockaddr*)server, *serverSize);
	if(r  == -1){
		LOG_ERROR("ERROR : Failed to connect to local socket\n");
		return -1; 
	}
	listen(*sockfd,10 );
	return 0;
}

int acceptLocalSocket(int* serversockfd, struct sockaddr_un *client, socklen_t* size){
	int sockfd;
	sockfd = accept(*serversockfd, (struct sockaddr*)client, size);
	if(sockfd == -1){
		LOG_ERROR("ERROR : Failed to accept new Local Socket Client");
		return -1;
	}
	return sockfd;
}

int initialzieLocalSocket_Client(int* sockfd, struct sockaddr_un *server, socklen_t* size, const char* sockpath ,int TIMEOUT){
	int max_count = TIMEOUT/1000;
	int count = 0;
	struct timeval tv; 
	tv.tv_sec = IPC_TIMEOUT_SEC;
	tv.tv_usec = IPC_TIMEOUT_MILLI;
	*sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
	setsockopt(*sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));
	if(fcntl(*sockfd, F_SETFL, O_NONBLOCK) == -1){
		return -1; 
	}
	bzero(server, sizeof(server));
	server->sun_family = AF_LOCAL;
	strcpy(server->sun_path,sockpath);
	*size = sizeof(*server);
	if(connect(*sockfd, (struct sockaddr*)server,*(int*)size) < 0){
		return -1; 
	}
	fcntl(*sockfd, F_SETFL, 0);
	return 0 ;
}

int writeIPC_Data(int* sockfd, void* buf, size_t* size){

	int n;
	n = write(*sockfd, buf, *size);
	if(n < 0)
		return -1;

	return n;
}



int readIPC_Data(int sockfd, uint8_t* buf, size_t size){
	/* This function is implemented for reading more 8-byte from file descriptor */
	int n;
	int read_bytes =0;
/*	while(1){
		n = read(sockfd, buf+read_bytes, 8);
		if(n < 0){
			return -1;
		}
		if(n == 0){
			read_bytes = n;
			break;
		}
		else if(n > 0 && n <8){
			read_bytes += n;
			break;
		}
		else if(n == 8){
			read_bytes += n;
			continue;
		}
	}*/ 
	return read(sockfd, buf, size);
}

