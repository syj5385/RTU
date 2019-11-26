# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>
# include <errno.h>
#include <string.h>
#include <stdint.h>

#include <termios.h>
#include <wiringPi.h>

//
#include "../define.h"
#include "../etc/log_util.h"
#include "../etc/ipc.h"
#include "../etc/config.h"
#include "../protocol/rtu_serial.h"
#include "../etc/config.h"
#include "../protocol/sensor_protocol.h"
#include "rpi.h"


int main(){
	char input[7] = {0x02, 0x80, 0x80,0x80,0x00,0x00,0x03};
	//char input[1] = {0xaa};
	char output[0x80] = {0,};
	int i;
	SENSOR1 = getSensorID(1);
	SENSOR2 = getSensorID(1);

	Frame* frame = (Frame*)malloc(sizeof(Frame));
	frame->device_id = 1;
	frame->opcode = 0x60;
	frame->size = 0;
	

	setupSerial();
	setupLED();
	while(1){
		switch(i%3){
			case 0:
				frame->device_id = 1;
				frame->opcode = 0x60;
				frame->size = 0;
				//frame->data[0] = 0x44;
				/*
				   digitalWrite(RPI_IO_G1,HIGHH);
				   for(i=0;i<7;i++){
				   write(serial_fd,&input[i],1);
				   }
				//write(serial_fd,&input,1);
				delay(5);
				digitalWrite(RPI_IO_G1,LOWW);
				for(i=0;i<10;i++){
				int cnt = read(serial_fd,output,1);
				printf("%d %02x\n",cnt,output[0]);
				}*/

				//readData(output);
				//int size = requestAndResponseSerial(input,output,7);
				//delay(5);
				//digitalWrite(RPI_IO_G1,LOWW);
				break;
			case 1:
				frame->device_id = 1;
				frame->opcode = 0x61;
				frame->size = 0;
				break;
			case 2: 		
				frame->device_id = 1;
				frame->opcode = 0x64;
				frame->size = 1;
				frame->data[0] = 0x04;
				break;
		}

		executeMessage(frame);
		i++;

		printf("testmain\n");
		
	/*	
		for(i=0;i<size;i++){
			printf("%02x ",output[i]);
		}
		
		memset(output,0,0x80);
	 */
		delay(2000);
	}
	return 0;
}
