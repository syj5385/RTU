
#include "ipc.h"
#include "log.h"


#include <unistd.h>

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
//		writeLog(ERROR,IPC_TAG,"Failed to bind local socket client");
		return 0;
	}
	listen(*sockfd,10 );
	return 1;
}

int acceptLocalSocket(int* serversockfd, struct sockaddr_un *client, socklen_t* size){
	int sockfd;
	sockfd = accept(*serversockfd, (struct sockaddr*)client, size);
	if(sockfd == -1){
//		writeLog(ERROR,IPC_TAG,"Failed to accept local socket client");
		return -1;
	}
	return sockfd;
}

int initialzieLocalSocket_Client(int* sockfd, struct sockaddr_un *server, socklen_t* size, const char* sockpath ,int TIMEOUT){
	int max_count = TIMEOUT/1000;
	int count = 0;

	*sockfd = socket(PF_LOCAL, SOCK_STREAM, 0);
	bzero(server, sizeof(server));
	server->sun_family = AF_LOCAL;
	strcpy(server->sun_path,sockpath);
	*size = sizeof(*server);
	while(1){
		if(connect(*sockfd, (struct sockaddr*)server,*(int*)size) == -1){
			char log_buf[100];
			bzero(log_buf,sizeof(log_buf));
//			sprintf(log_buf,"try to connect to local server socket again (%d/%d)",count+1, max_count);
//			writeLog(WARNING,IPC_TAG,log_buf);
			if(count++ < max_count){
				sleep(1);
				continue;
			}
			else
				return 0;
		}
		else{
			return 1;
		}
	}

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
	while(1){
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
	}
	return read_bytes;
}

