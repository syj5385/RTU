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
#include <libgen.h>
#include <signal.h>
#include <stdint.h>

#include "../define.h"
#include "../etc/log_util.h"
#include "../etc/ipc.h"
#include "../etc/config.h"
#include "../protocol/rtu_serial.h"
#include "../protocol/sensor_protocol.h"
#include <wiringPi.h>
#include <wiringPiI2C.h>

#include "rpi.h"

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
static pthread_t sensorThread; 

void* sensorRequestThread(void* arg);

/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		LOG_ERROR("SIGINT signal Received");
		exit(0);
	}
	if(signo == SIGTERM){
		LOG_ERROR("SIGTERM signal Received");
		exit(0);
	}
	if(signo == SIGTSTP){
		LOG_ERROR("SIGTSTP signal Received");
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
	if(signal(SIGTERM,sigHandler) == SIG_ERR){
		exit(1);
	}
	if(signal(SIGTSTP,sigHandler) == SIG_ERR){
		exit(1);
	}
}

void initializeData(){
	int i=0; 

	cmd_60_data = (CMD_60_DATA*)malloc(sizeof(CMD_60_DATA));
	cmd_61_data = (CMD_61_DATA*)malloc(sizeof(CMD_61_DATA));

	memset(cmd_60_data->sensor1_data,0, sizeof(cmd_60_data->sensor1_data));
	memset(cmd_60_data->sensor2_data,0, sizeof(cmd_60_data->sensor2_data));
	memset(cmd_61_data->sensor1_data,0, sizeof(cmd_61_data->sensor1_data));
	memset(cmd_61_data->sensor2_data,0, sizeof(cmd_61_data->sensor2_data));
	
}

void* ipc_client_thread(void* arg){
	int n;
	size_t size;
	uint8_t ipc_send_buf[IPC_BUF_SIZE];
	uint8_t ipc_read_buf[IPC_BUF_SIZE];
	int* sockfd = (int*)arg;
	int i=0, j=0; 
	Frame *frame; 
	while(1){

	/* 1.  read IPC data from network */
		bzero(ipc_read_buf,sizeof(ipc_read_buf));
		bzero(ipc_send_buf,sizeof(ipc_send_buf));

		n = readIPC_Data(*sockfd, ipc_read_buf, IPC_BUF_SIZE);
		if(n==0){
			goto close; 
		}
		if(n == -1)
			goto close;
		LOG_TRACE("Read IPC data from network client");
		printf("Sensor Received : %d\n",n);

		frame = (Frame*)malloc(sizeof(Frame));
		bzero(frame, sizeof(Frame));
		LOG_CALL(analysisMessage(ipc_read_buf, frame));

#ifndef	SENSOR_NOT_EXECUTE		
		//pthread_mutex_lock(&mutex);
		//LOG_CALL(executeMessage(frame));
		LOG_CALL(executeSensorProtocol(frame));
		//pthread_mutex_unlock(&mutex);
#endif
		/* 3. send result to network process */
		
		LOG_CALL(writeFrameToNetwork(*sockfd, frame));
		
		free(frame);
		//printf("Write : %d\n",n);
	}
close :
	printf("close ipc socket in sensor\n");
	close(*sockfd);
	free(sockfd);
}

int main(int argc, char** argv){

	int local_result; 
	LOGsetInfo(LOG_PATH, basename(argv[0]));
	LOG_TRACE("Start Hardware Process");
	printf("Sensor start++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");


	pid_t sensor_pid = getpid();
	FILE* pid_file = fopen(sensor_pid_file,"w+");
	if(!pid_file)
		return 1;

	initializeData();
	fprintf(pid_file,"%d",(int)sensor_pid);
	fclose(pid_file);

	SENSOR1 = getSensorID(1);
	SENSOR2 = getSensorID(2);
	printf("Sensor Id : %u\t%u\n",SENSOR1, SENSOR2);

#ifndef	SENSOR_NOT_EXECUTE
	LOG_CALL(setupSerial());
	LOG_CALL(setupLED());
#endif
	/* 0. Signal Initialize */
	initializeSignalHandler();

	/* 1. initialize IPC(Inter Process Communication) with TCP Manager */
	nh_fd = (int*)malloc(sizeof(int));

	server = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	client = (struct sockaddr_un*)malloc(sizeof(struct sockaddr_un));
	serverlen = (int*)malloc(sizeof(int));
	clientlen = (socklen_t*)malloc(sizeof(socklen_t));

	LOG_CALL(local_result = initializeLocalSocket_Server(nh_fd, server, serverlen, NH_SOCKET));
	if(local_result < 0){
		exit(1);
	}
	
	pthread_create(&sensorThread, NULL, sensorRequestThread, NULL);

	while(1){
		pthread_t* client_thread = (pthread_t*)malloc(sizeof(pthread_t));
		int *hn_fd = (int*)malloc(sizeof(int));
		LOG_CALL(*hn_fd = acceptLocalSocket(nh_fd, server, serverlen));
		LOG_TRACE("New local socket client accepted");
		pthread_create(client_thread, NULL,ipc_client_thread, hn_fd);
//		pthread_detach(client_thread);
	}
	return 0;
}

int checkSerialPacket(uint8_t* output, int size){
	/* check stx & etx */ 
	uint16_t  data_size; 
	int start = 5; 
	printf("STX : %02x\tETC : %02x\n", output[0], output[size-1]);
	if(output[0] != 0x02 || output[size-1] != 0x03){
		printf("STX & ETX not matched\n");
		return 1;
	}


	return 0; 
}

void* sensorRequestThread(void* arg){
	int n, index,i;  
	uint8_t input[100];
	uint8_t output1[100];
	uint8_t output2[100]; 

	while(1){
REQUEST_0x60:
		memset(input,0,sizeof(input));
		memset(output1,0,sizeof(output1));
		memset(output2,0,sizeof(output2));
		printf("0x60 to sensor1 start\n");
		/* 0x60 Request */
		usleep(250*INTERVAL_OF_SENSOR_REQUEST);
		input[0] = 0x02; 
		input[1] = (SENSOR1 >> 8) & 0xff;
		input[2] = SENSOR1 & 0xff; 
		input[3] = 0x80;
		input[4] = 0x00; 
		input[5] = 0x00; 
		input[6] = 0x03; 
		n = requestAndResponseSerial(input, output1, 7);
		if(n <= 0)
			goto halt; 
		if(checkSerialPacket(output1, n) != 0)
			goto REQUEST_0x61; 

		usleep(250*INTERVAL_OF_SENSOR_REQUEST);
		printf("0x60 to sensor2\n");
		input[0] = 0x02; 
		input[1] = (SENSOR2 >> 8) & 0xff;
		input[2] = SENSOR2 & 0xff; 
		input[3] = 0x80;
		input[4] = 0x00; 
		input[5] = 0x00; 
		input[6] = 0x03; 
		n = requestAndResponseSerial(input, output2, 7);
		if(n <= 0)
			goto halt; 
		if(checkSerialPacket(output2, n) != 0)
			goto REQUEST_0x61; 

		pthread_mutex_lock(&data_mutex);
		cmd_60_data->sensor1_data[0] = output1[6];
		cmd_60_data->sensor1_data[1] = output1[7];
		cmd_60_data->sensor1_data[2] = output1[8]; 
		
		cmd_60_data->sensor2_data[0] = output2[6];
		cmd_60_data->sensor2_data[1] = output2[7];
		cmd_60_data->sensor2_data[2] = output2[8]; 
		pthread_mutex_unlock(&data_mutex);

REQUEST_0x61: 
		memset(input,0,sizeof(input));
		memset(output1,0,sizeof(output1));
		memset(output2,0,sizeof(output2));
		/* 0x61 Request */ 

		usleep(250*INTERVAL_OF_SENSOR_REQUEST);
		input[0] = 0x02; 
		input[1] = (SENSOR1 >> 8) & 0xff;
		input[2] = SENSOR1 & 0xff; 
		input[3] = 0x81;
		input[4] = 0x00; 
		input[5] = 0x00; 
		input[6] = 0x03; 
		n = requestAndResponseSerial(input, output1, 7);

		if(n<=0)
			goto halt; 

		if(checkSerialPacket(output1,n) != 0)
			goto REQUEST_0x60;

		usleep(250*INTERVAL_OF_SENSOR_REQUEST);
		input[0] = 0x02; 
		input[1] = (SENSOR2 >> 8) & 0xff;
		input[2] = SENSOR2 & 0xff; 
		input[3] = 0x81;
		input[4] = 0x00; 
		input[5] = 0x00; 
		input[6] = 0x03; 
		n = requestAndResponseSerial(input, output2, 7);
		if(n<=0)
			goto REQUEST_0x60; 
		if(checkSerialPacket(output1,n) != 0)
			goto REQUEST_0x60;

		pthread_mutex_lock(&data_mutex);
		for(i=0; i<12; i++){
			cmd_61_data->sensor1_data[i] = output1[6+i];
		}
		index = 7; 
		for(i=0; i<12; i++){
			cmd_61_data->sensor2_data[i] = output2[6+i];
		}
		pthread_mutex_unlock(&data_mutex);

		goto REQUEST_0x60;
halt:
		pthread_mutex_lock(&halt_mutex);
		haltsignal(); 
		pthread_mutex_unlock(&halt_mutex); 


	}
	
}


