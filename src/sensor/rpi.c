# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>
# include <errno.h>
#include <string.h>
#include <stdint.h>

#include <termios.h>
#include <wiringPi.h>
#include <pthread.h>

//
#include "../define.h"
#include "../etc/ipc.h"
#include "../etc/log_util.h"
#include "../protocol/rtu_serial.h"
#include "rpi.h"
//

int serial_fd;
#define RPI_IO_G1 1
static pthread_mutex_t request_mutex; 

void setupSerial(){
	serial_fd = open("/dev/ttyUSB0",O_RDWR | O_NOCTTY);

	if(serial_fd == -1){
		LOG_ERROR("Serial Setup Error");
		exit(1);
	}

	struct termios options;

	tcgetattr(serial_fd, &options);
	options.c_cflag = B115200|CS8|CLOCAL|CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;

	
	tcflush(serial_fd,TCIFLUSH);
	tcsetattr(serial_fd,TCSANOW,&options);
	//fcntl(serial_fd, F_SETFL,0);
	if(wiringPiSetup() == -1){
		LOG_ERROR("wiringPi Set up Error");
		exit(1);
	}
	//pinMode(RPI_IO_G1,OUTPUT);
	//digitalWrite(RPI_IO_G1,LOW);
}

void setupLED(){
	if(wiringPiSetup() == -1){
		LOG_ERROR("LED setup Error");
		exit(1);
	}
	pinMode(RLED,OUTPUT);
	pinMode(GLED,OUTPUT);
	digitalWrite(RLED,HIGH);
	digitalWrite(GLED,HIGH);
}


void serialFlush(int fd){
	char* buf[128] = {0,};
	int cnt;
	struct termios options;

	tcgetattr(fd, &options);/*
	options.c_cflag = B115200|CS8|CLOCAL|CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(fd,TCIFLUSH);
	*/
	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;
	tcsetattr(fd,TCSANOW,&options);

	delay(50);
	read(fd,buf,128);

	tcgetattr(fd, &options);

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;
	tcsetattr(fd,TCSANOW,&options);

}

int readData(uint8_t* output){
	int i=0;
	int rx_length=0;
	int cnt,len,j;
	if(serial_fd ==-1){
		LOG_ERROR("No Serial file descriptor");
		exit(-1);
	}
#ifdef READ_ONE_BYTE

	while(1){
		cnt =0;
		cnt = read(serial_fd,output+i,1);
		if(cnt<=0) break;
		i++;
		rx_length++;
		if(i==6){
			len = output[5];
			for(j=0;j<len+1;j++){
				read(serial_fd,output+i+j,1);
				rx_length++;
			}
			break;
		}
	}
	if(rx_length==0){
		LOG_ERROR("Error : There is no data to read");
	}else{
		printf("length : %d\n",rx_length);
		printf("READ DATA : ");
		for(i=0;i<rx_length;i++){
			printf("%02x ",output[i]);
		}
		printf("\n");
	}
	//cnt = 0; 
	//options.c_cc[VMIN] = 0;
	//options.c_cc[VTIME] = 10;
	//tcsetattr(fd,TCSANOW,&options);
//	cnt = read(serial_fd, output, 100);
//	printf("length : %d\n",cnt);
//	rx_length = cnt; 

#endif
#ifndef READ_ONE_BYTE
	rx_length = read(serial_fd,output,100);
	printf("READ DATA : ");
//	for(i=0;i<rx_length;i++){
//		printf("%02x ",output[i]);
	//}
//		printf("\n");
#endif
	//delay(5);
	serialFlush(serial_fd);
	return rx_length;
}

void sendData(uint8_t *frame,int size){
	int i, n=0; 
	uint8_t tx_buffer[128];
	if(serial_fd == -1){
		LOG_ERROR("ERROR : Invalid File descriptor");
		exit(-1);
	}

	for(i=0;i<size;i++){
		n+=write(serial_fd,frame+i,1);
	}

}


int requestAndResponseSerial(uint8_t* input,uint8_t* output,int inputSize){

	usleep(1000);
	pthread_mutex_lock(&request_mutex);
	int output_length;
	//digitalWrite(RPI_IO_G1,HIGH);
	printf("Send data to Sensor Modules\n");
	sendData(input,inputSize);
//	delay(5);
//	digitalWrite(RPI_IO_G1,LOW);
	printf("Before : Ready to receive data\n");
	output_length = readData(output);
	printf("After : Finished to receive data from sensor\n");
	//usleep(1000);
	pthread_mutex_unlock(&request_mutex);
	return output_length;

}

void haltsignal(){
	uint8_t halt[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
	if(serial_fd == -1){
		LOG_ERROR("ERROR : Invalid File descriptor");
		exit(-1);
	}

	int cnt=0;
	int i;
	//digitalWrite(RPI_IO_G1,HIGH);
	for(i=0;i<8;i++){
		cnt+=write(serial_fd,halt+i,1);
	}
	//digitalWrite(RPI_IO_G1,LOW);
	//delay(5);
	printf("HALT SIGNAL COUNT : %d \n",cnt);
	serialFlush(serial_fd);
	delay(10);
	if(cnt<0){
		LOG_ERROR("ERROR : Failed to send halt signal to sensor module");
	}

	sleep(1);
}
