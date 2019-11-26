# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>
# include <errno.h>
#include <string.h>
#include <stdint.h>

#include <termios.h>
#include <wiringPi.h>


#define RPI_IO_G1 1
#define RLED 24
#define GLED 25

#define READ_ONE_BYTE
int serial_fd =0;

void setupSerial();
void setupLED();
void serialFlush(int fd);

int main(){
	char output[0x80] = {0,}; // output - module response
	int i=0;
	int cnt=0;
	int rx_length = 0;
	setupSerial();
	setupLED();

	printf("Only Read Program\n");

	while(1){
#ifdef READ_ONE_BYTE
#define RESPONSE_SIZE 10 // response buffer size
		rx_length =0;

		do{
			 cnt = read(serial_fd,output+i,1);
			 if(cnt<=0) break;
			 rx_length +=cnt;
			 cnt =0;
			 i++;
		}while(1);

		if(rx_length==0) continue;

		printf("rx_length : %d\n ",rx_length);

		printf("RESPONSE : ");

		for(i=0;i<rx_length;i++){
			printf("%02x ",output[i]);
		}
		printf("\n");
#endif

#ifndef READ_ONE_BYTE
		int cnt = read(serial_fd,output,64);
		printf("RESPONSE : ");
		for(i=0;i<cnt;i++){
			printf("%02x ",output[i]);
		}
		printf("\n");
#endif
		
	}
	return 0;
}


void setupSerial(){
	int i;
	serial_fd = open("/dev/ttyAMA0",O_RDWR | O_NOCTTY);

	if(serial_fd == -1){
		fprintf(stderr,"Serial Setup Error\n");
		exit(-1);
	}

	struct termios options;

	tcgetattr(serial_fd, &options);
	options.c_cflag = B115200|CS8|CLOCAL|CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(serial_fd,TCIFLUSH);

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 1;

	tcsetattr(serial_fd,TCSANOW,&options);
	//fcntl(serial_fd, F_SETFL,0);
	if(wiringPiSetup() == -1){
		fprintf(stderr,"wiringPi Set up Error\n");
		exit(1);
	}
	pinMode(RPI_IO_G1,OUTPUT);
	digitalWrite(RPI_IO_G1,LOW);
}

void setupLED(){
	if(wiringPiSetup() == -1){
		fprintf(stderr,"LED setup Error\n");
		exit(1);
	}
	pinMode(RLED,OUTPUT);
	pinMode(GLED,OUTPUT);
	digitalWrite(RLED,HIGH);
	digitalWrite(GLED,HIGH);
}


void serialFlush(int fd){
	tcflush(fd,TCIOFLUSH);
}
