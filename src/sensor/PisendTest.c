
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

#define REQUEST_SIZE 7 //request buffer size

#define RPI_IO_G1 1
#define RLED 24
#define GLED 25

//#define READ_ONE_BYTE

int serial_fd =0;

void setupSerial();
void setupLED();
void* readThread(void* arg);

pthread_mutex_t serial; 

int main(){
	uint8_t input[REQUEST_SIZE] = {0x02, 0x80, 0x80,0x80,0x00,0x00,0x03}; // input - pi request
	uint8_t input2[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	//char input[1] = {0xaa};
	uint8_t output[10];
	int i;
	pthread_t readThreadvar; 
	setupSerial();
	setupLED();

	//pthread_create(&readThreadvar, NULL, readThread, NULL);
	
	while(1){
		//for(i=0;i<REQUEST_SIZE;i++){
		//	printf("%02x ",input[i]);
		//}
		//printf("\n");

		//memset(output, 0, 100);
		//digitalWrite(RPI_IO_G1,HIGH);
		for(i=0; i<8; i++){
			write(serial_fd, input2+i, 1);
		}
		sleep(1);
		for(i=0;i<REQUEST_SIZE;i++){ //write 1 byte
			write(serial_fd,input+i,1);
		}

		printf("REQUEST : ");
		//int cnt = read(serial_fd, (void*)output, 10);

		//delay(5);
		//digitalWrite(RPI_IO_G1,LOW);

		/*
#ifdef READ_ONE_BYTE
#define RESPONSE_SIZE 10 // response buffer size

		for(i=0;i<RESPONSE_SIZE;i++){
			int cnt = read(serial_fd,output+i,1);
			printf("cnt : %d\n",cnt);
		}
		printf("RESPONSE : ");

		for(i=0;i<RESPONSE_SIZE;i++){
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
		*/
		tcflush(serial_fd, TCOFLUSH);
		delay(2000);

	}
	return 0;
}

void* readThread(void* arg){
	
	int cnt = 0 ;
	int i; 
	uint8_t output[100];
	int n=0, read_bytes=0;

	
	while(1){
		bzero(output,sizeof(output));
		//cnt = read(serial_fd, output,8);

	while(1){
		printf("Read!!\n");
		n = read(serial_fd,  output+read_bytes, 1);
		if(n < 0){
			break; 
		}
		if(n == 0){
			printf("Read over\n");
			read_bytes = n;
			break;
		}
		/*else if(n > 0 && n <8){
			read_bytes += n;
			break;
		}*/
		else if(n == 1){
			read_bytes += n;
			continue;
		}
	}
	//	for(i=0; i<10; i++){
	///		cnt += read(serial_fd, output+i, 1);
			//usleep(500);
	//	}

		printf("READ : " ); 
		for(i=0; i<cnt; i++){ 
			printf("%02x ",output[i]); 
		}
	       	printf("\n");

		tcflush(serial_fd, TCIFLUSH);


	}


}


void setupSerial(){
	int i;
	serial_fd = open("/dev/ttyAMA0",O_RDWR | O_NOCTTY);
	if(serial_fd == -1){
		printf("Failed to open serial\n");
		exit(1);
	}

	if(serial_fd == -1){
		fprintf(stderr,"Serial Setup Error\n");
		exit(-1);
	}

	struct termios options;

	tcgetattr(serial_fd, &options);
	options.c_cflag = B115200|CS8|CLOCAL|CREAD;
	//options.c_cflag = B115200|CLOCAL|CREAD;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(serial_fd,TCIFLUSH);

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 0;;

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
