
# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>
# include <errno.h>
#include <string.h>
#include <stdint.h>

#include <termios.h>
#include <wiringPi.h>

#define REQUEST_SIZE 7 //request buffer size

#define RPI_IO_G1 1
#define RLED 24
#define GLED 25

#define READ_ONE_BYTE

int serial_fd =0;

void setupSerial();
void setupLED();

int main(){
	char input[REQUEST_SIZE] = {0x02, 0x80, 0x81,0x80,0x00,0x00,0x03}; // input - pi request

	uint8_t input2[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	//char input[1] = {0xaa};
	char output[0x80] = {0,}; // output - module response
	int i;
	int cnt; 
	setupSerial();

	while(1){

		tcflush(serial_fd, TCOFLUSH);
		for(i=0; i<8 ;i++){
			write(serial_fd, input2+i, 1);
		}
		sleep(1);
		cnt = 0; 
		printf("REQUEST : ");
		for(i=0;i<REQUEST_SIZE;i++){
			printf("%02x ",input[i]);
		}
		printf("\n");


		for(i=0;i<REQUEST_SIZE;i++){ //write 1 byte
			write(serial_fd,&input[i],1);
		}

#ifdef READ_ONE_BYTE
#define RESPONSE_SIZE 10 // response buffer size

		tcflush(serial_fd, TCIFLUSH);
		for(i=0;i<RESPONSE_SIZE;i++){
			cnt += read(serial_fd,output+i,1);
			printf("cnt %d : %02x\n", cnt, *(output+i));
		}
		printf("Read : %d\n",cnt);
#endif

#ifndef READ_ONE_BYTE
		int cnt = read(serial_fd,output,64);
		printf("RESPONSE : ");
		for(i=0;i<cnt;i++){
			printf("%02x ",output[i]);
		}
		printf("\n");
#endif
		
		delay(2000);
	}
	return 0;
}


void setupSerial(){
	int i;
	serial_fd = open("/dev/ttyUSB0",O_RDWR | O_NOCTTY);

	if(serial_fd == -1){
		fprintf(stderr,"Serial Setup Error\n");
		exit(-1);
	}

	struct termios options;

	tcgetattr(serial_fd, &options);
	options.c_cflag = B115200|CS8|CLOCAL|CREAD|CRTSCTS;
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(serial_fd,TCIFLUSH);

	options.c_cc[VMIN] = 0;
	options.c_cc[VTIME] = 5;

	tcsetattr(serial_fd,TCSANOW,&options);
	fcntl(serial_fd, F_SETFL,0);
	if(wiringPiSetup() == -1){
		fprintf(stderr,"wiringPi Set up Error\n");
		exit(1);
	}
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
