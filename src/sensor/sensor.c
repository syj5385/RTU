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
#include "../protocol/rtu_serial.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>

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

int MPU;

uint16_t readReg(int devID, int reg);
int controlDevice(Frame* frame);
void readData(Frame* frame,uint16_t* size,uint16_t* result);
void setRelay(Frame* frame);
void statusRelayAndSensor(Frame* frame);
void initializeI2CDev();
#define debug printf("error : %d line, %s function in %s\n",__LINE__,__func__,__FILE__)

/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		writeLog(WARNING,HARDWARE_TAG,"SIGINT signal Received");
		closeLogFile();
		exit(1);
	}
	if(signo == SIGTERM){
		writeLog(WARNING,HARDWARE_TAG,"SIGTERM signal Received");
		closeLogFile();
		exit(1);
	}
	if(signo == SIGTSTP){
		writeLog(WARNING,HARDWARE_TAG,"SIGTSTP signal Received");
		closeLogFile();
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
	int i=0, j=0; 

	while(1){
	/* 1.  read IPC data from network */
		bzero(ipc_read_buf,sizeof(ipc_read_buf));
		bzero(ipc_send_buf,sizeof(ipc_send_buf));

		n = readIPC_Data(*sockfd, ipc_read_buf, IPC_BUF_SIZE);
		if(n==0)
			continue;
		if(n == -1)
			goto close;

		/*
		for(i=0; i<n ; i++)
			printf("%d ", (int)ipc_read_buf[i]);
		printf("\n");
		*/

		/* 2.  get I2C or Serial data */
		/////////////////////////////

		for(i=0; i<6; i++){
			printf("%d : %02x\n",i, ipc_read_buf[i]);
		}

		Frame* frame = (Frame*)malloc(sizeof(Frame));
		memcpy(frame,ipc_read_buf,IPC_BUF_SIZE);
		


		/*
		printf("op_code : %d\n",frame->opcode);
		printf("device_id : %d\n",frame->device_id);
		printf("size : %d\n",frame -> size);
		*/

		printf("\nbefore\n");
		printf("%04x ",frame->device_id);
		printf("%02x ",frame->opcode);
		printf("%04x ",frame->size);
		for(i=0; i<frame->size ; i++)
			printf("%02x ",*(frame->data+i));
		printf("\n");
		if(controlDevice(frame)){
			debug;
		}
		
		printf("\nafter\n");
		printf("%04x ",frame->device_id);
		printf("%02x ",frame->opcode);
		printf("%04x ",frame->size);
		for(i=0; i<frame->size ; i++)
			printf("%02x ",*(frame->data+i));
		printf("\n");
		
		//usleep(500000);


		/* 3. send result to network process */

		write(*sockfd,frame,sizeof(Frame));
		//n = write(*sockfd,ipc_read_buf,n);
		//printf("Write : %d\n",n);
	}
close :
	close(*sockfd);
	free(sockfd);
}

int main(int argc, char** argv){

	if(openLogFile() == 0)
		exit(1);

	writeLog(NORMAL,HARDWARE_TAG, "Start Hardware Process");

	initializeI2CDev();

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

uint16_t readReg(int devID, int reg){
	uint16_t result;
	result = wiringPiI2CReadReg8(devID,reg);
	result = result<<8 | wiringPiI2CReadReg8(devID,reg+1);

	//if(result>=0x8000) result -= 0x10000;

	return result;

}

int controlDevice(Frame* frame){
	uint8_t op = frame->opcode;
	uint16_t* result= (uint16_t*)malloc(sizeof(uint16_t)*0x10);
	uint16_t size =0;
	switch(op){
		case REQUEST_DATA:
			readData(frame,&size,result);
			frame->size = size;
			//printf("frame size : %d\n",frame->size);
			memcpy(frame->data,result,size);
			break;
		case REQUEST_SET_RELAY : 
			setRelay(frame);
			break;
			//edit this line
		case REQUEST_STATUS:
			statusRelayAndSensor(frame);
			break;
		default : 
			return -1;
	}
	return 0;
}

void readData(Frame* frame,uint16_t* size,uint16_t* result){
	switch(frame->device_id){
		case 0x01://acc
			result[0] = readReg(MPU,0x3b);
			result[1] = readReg(MPU,0x3d);
			result[2] = readReg(MPU,0x3f);
			*size = 6;
			break;
		case 0x02://gyro
			result[0] = readReg(MPU,0x43);
			result[1] = readReg(MPU,0x45);
			result[2] = readReg(MPU,0x47);
			*size = 6;
			break;
		case 0x03://temper
			result[0] = readReg(MPU,0x41);
			*size =2;
		//default:
	}
}

void setRelay(Frame* frame){//normal : 0, unnormal : 1
	uint8_t setting = 0;
	uint8_t result = 0;
	int i;
	for(i=0;i<8;i++){
		setting += result<<i;
	}
	memcpy(frame->data,&setting,sizeof(uint8_t));
}

void statusRelayAndSensor(Frame* frame){
	frame->size = 6;
	frame->data[0] = 0xff;
	frame->data[1] = 0xff;
	frame->data[2] = 0xff;
	frame->data[3] = 0xff;
	frame->data[4] = 0xff;
	frame->data[5] = 0xff;
}

void initializeI2CDev(){//need to edit parameter
	MPU = wiringPiI2CSetup(0x68);
	wiringPiI2CWriteReg8(MPU,0x6b,0);
}


