
#include "sensor_protocol.h"
#include <stdio.h>
#include "../sensor/rpi.h"
#include "../define.h"

void analysisMessage(uint8_t* buf, Frame *dest){
	int index = 0; 
	int i=0; 

	dest -> device_id = ((buf[index+1]<<8)&0xff00) + buf[index]; 
	index+=2; 
	dest -> opcode = buf[index++];
	dest -> size = ((buf[index+1]<<8)&0xff00) + buf[index]; 
	index+=2; 
	for(i=0; i<dest->size; i++)
		*(dest->data+i) = buf[index++]; 

}

int executeMessage(Frame *dest){
	uint8_t buf[1024];
       	uint8_t msg[128];
	int output_len;	
	int i;
	
	msg[0] = STX;
	msg[1] = (uint8_t)(SENSOR1>>8)&0xff;
	msg[2] = (uint8_t)SENSOR1&0xff;
	msg[3] = (dest->opcode)+0x20;
	msg[4] = (uint8_t)((dest->size)>>8);
	msg[5] = (uint8_t)(dest->size);
	switch(dest->opcode){
		case REQUEST_STATUS: 
		case REQUEST_DATA: 
			msg[6] = ETX;
			
			for(i=0;i<7;i++){
				printf("%02x ",msg[i]);
			}
			printf("\n");
			if((output_len=requestAndResponseSerial(msg,buf,dest->size+7))>0){		
				//*(uint8_t*)(dest+3) = *(buf+4);
				//*(uint8_t*)(dest+4) = *(buf+5);
				dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				//beforeRequest = *(uint8_t*)(dest+4);
				for(i=0;i<*(uint8_t*)(dest+4);i++){
					*(uint8_t*)(dest+5+i) = *(buf+6+i);
				}
				printf("PROTOCOL\n");
				for(i=0; i<dest->size+7; i++)
					printf("%02x ", buf[i]);
				printf("\n");
				break;
			}
			else return -1;


		case REQUEST_SET_RELAY : 
		case REQUEST_CHANNEL_RST : 
			msg[6] = dest->data[0];
			msg[7] = ETX;
			if((output_len=requestAndResponseSerial(msg,buf,dest->size+7))>0){		
				dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				for(i=0;i<*(uint8_t*)(dest+4);i++){
					*(uint8_t*)(dest+5+i) = *(buf+6+i);
				}
				printf("test : ");
				for(i=0; i<dest->size+7; i++)
					printf("%02x ", buf[i]);
				printf("\n");
				break;
			}
			else return -1;


			break;

		default : 
			return -1;
	}		

}
int writeFrameToNetwork(int sockfd, Frame* frame){
	uint8_t out[frame->size+5]; 
	int index = 0; 
	int i=0; 

	out[index++] = frame->device_id & 0xff; 
	out[index++] = (frame->device_id >> 8) &0xff; 
	out[index++] = frame->opcode; 
	out[index++] = frame->size & 0xff; 
	out[index++] = (frame->size >>8) & 0xff; 
	for(i=0; i<frame->size; i++)
		out[index++] = *(frame->data+i);

	printf("Sensor : ");
	for(i=0; i<frame->size+5; i++)
		printf("%02x ", out[i]);
	printf("\n");

	if(write(sockfd, out, sizeof(out)) < 0)
		return -1; 
	else return 0;
	 
	return 0;

}
