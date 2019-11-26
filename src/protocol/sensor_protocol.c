
#include "sensor_protocol.h"
#include <stdio.h>
#include "../sensor/rpi.h"
#include "../define.h"
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../etc/log_util.h"
#define MAXTRIAL 0
#define DELAY 10

static pthread_mutex_t mut;


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



int executeSensorProtocol(Frame *frame){
	int opcode = frame -> opcode;
	int i, index, n; 
	uint8_t input[100];
	uint8_t output1[100];
	uint8_t output2[100]; 
	bzero(input,100);
	bzero(output1,100);
	bzero(output2,100);

	switch(opcode){
		case REQUEST_STATUS : /* 0x60 */
			pthread_mutex_lock(&data_mutex);

			frame -> size = 6; 
			
			frame -> data[0] = cmd_60_data->sensor1_data[0]; 
			frame -> data[1] = cmd_60_data->sensor2_data[0];
			frame -> data[2] = cmd_60_data->sensor1_data[1];
			frame -> data[3] = cmd_60_data->sensor2_data[1]; 
			frame -> data[4] = cmd_60_data->sensor1_data[2];
			frame -> data[5] = cmd_60_data->sensor2_data[2];
			printf("sensor1 %u\tsensor2 %u\n",cmd_60_data->sensor1_data[1], cmd_60_data->sensor2_data[1]);
			pthread_mutex_unlock(&data_mutex);
			return 0;

		case REQUEST_DATA : /* 0x61 */

			printf("0x61 request\n");
			pthread_mutex_lock(&data_mutex);
			frame -> size = 24;
			index = 0;

			printf("Make Frame : ");
			for(i=0; i<12; i++){
				printf("%02x ", cmd_61_data->sensor1_data[i]);
				frame -> data[index++] = cmd_61_data -> sensor1_data[i]; 
			}
			
			for(i=0; i<12; i++){
				printf("%02x ", cmd_61_data->sensor2_data[i]); 
				frame -> data[index++] = cmd_61_data -> sensor2_data[i]; 
			}
			printf("\n");
			pthread_mutex_unlock(&data_mutex);
			return 0; 

		case REQUEST_SET_RELAY : /* 0xy64 */
			input[0] = 0x02; 
			input[1] = (SENSOR1>>8)&0xff;
			input[2] = SENSOR1&0xff;
			input[3] = 0x84; 
			input[4] = (frame->size>>8)&0xff; 
			input[5] = frame->size&0xff;
			for(i=0; i<frame->size; i++)
				input[i+6] = (frame->data[i] >> 4) & 0x0f; 
			input[6+frame->size] = 0x03;
			while(pthread_mutex_trylock(&halt_mutex) != 0);
			pthread_mutex_unlock(&halt_mutex); 
			if(n = requestAndResponseSerial(input,output1,7+frame->size) == -1){
				frame -> size = 1;
				frame -> data[0] = 1; 
				return 1; 
			}


			input[0] = 0x02; 
			input[1] = (SENSOR2>>8)&0xff;
			input[2] = SENSOR2&0xff;
			input[3] = 0x84; 
			input[4] = (frame->size>>8)&0xff; 
			input[5] = frame->size&0xff; 
			for(i=0; i<frame->size; i++)
				input[i+6] = frame->data[i] & 0x0f; 
			input[6+frame->size] = 0x03; 
			if(n = requestAndResponseSerial(input,output2,7+frame->size) == -1){
				frame -> size = 1; 
				frame -> data[0] = 1; 
				return 1; 
			}
			printf("1 : %u\t2 : %u\n",output1[6], output2[6]);
			if(output1[6] != 0x00 || output2[6] != 0x00){
				frame -> size = 1; 
				frame -> data[0] = 1; 
				return 1; 
			}
			frame->size = 1; 
			frame->data[0] = 0; 
			return 0;

		case REQUEST_CHANNEL_RST : /* 0x66 */

			input[0] = 0x02; 
			input[1] = (SENSOR1>>8)&0xff;
			input[2] = SENSOR1&0xff;
			input[3] = 0x86; 
			input[4] = (frame->size>>8)&0xff; 
			input[5] = frame->size&0xff;
			for(i=0; i<frame->size; i++)
				input[i+6] = frame->data[i];
			input[6+frame->size] = 0x03;
			while(pthread_mutex_trylock(&halt_mutex) != 0);
			pthread_mutex_unlock(&halt_mutex); 
			if(n = requestAndResponseSerial(input,output1,7+frame->size) == -1){
				frame -> size = 1;
				frame -> data[0] = 1; 
				return 1; 
			}


			input[0] = 0x02; 
			input[1] = (SENSOR2>>8)&0xff;
			input[2] = SENSOR2&0xff;
			input[3] = 0x86; 
			input[4] = (frame->size>>8)&0xff; 
			input[5] = frame->size&0xff; 
			for(i=0; i<frame->size; i++)
				input[i+6] = frame->data[i];
			input[6+frame->size] = 0x03; 
			if(n = requestAndResponseSerial(input,output2,7+frame->size) == -1){
				frame -> size = 1; 
				frame -> data[0] = 1; 
				return 1; 
			}
			if(output1[6] != 0x00 || output2[6] != 0x00){
				frame -> size = 1; 
				frame -> data[0] = 1; 
				return 1; 
			}
			frame->size = 1; 
			frame->data[0] = 0; 

			return 0;

		default : 
			
			return 1; 

	}
}

int executeMessage(Frame *dest){
	uint8_t buf[1024];
	uint8_t buf1[1024];
	uint8_t msg[128];
	int output_len;	
	int i,beforeRequest;
	int tick=0;

	printf("Sensor Id : %u\t%u\n",SENSOR1, SENSOR2);
	msg[0] = STX;
	msg[1] = (uint8_t)(SENSOR1>>8)&0xff;
	msg[2] = (uint8_t)SENSOR1&0xff;
	msg[3] = (dest->opcode)+0x20;
	msg[4] = (uint8_t)((dest->size)>>8);
	msg[5] = (uint8_t)(dest->size);
	switch(dest->opcode){
		case REQUEST_STATUS: 
			msg[6] = ETX;
again0x80:
			if((output_len=requestAndResponseSerial(msg,buf,dest->size+7))>0){		
				if(!checkCorrect(buf,output_len)){
					delay(DELAY);
					tick++;
					if(tick>MAXTRIAL){
						sendhalt(dest);
						return -1;
					}
					goto again0x80;
				}
				dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				for(i=0;i<dest->size;i++){
					*(uint8_t*)(dest+5+i) = *(buf+6+i);
				}
			}
			else {
				delay(DELAY);
				tick++;
				if(tick>MAXTRIAL){
					sendhalt(dest);
					return -1;
				}
				goto again0x80;
			}
			printf("RESPONSE : ");
			for(i=0;i<dest->size+5;i++){
				printf("%02x ",*((uint8_t*)(dest)+i));
			}
			printf("\n");

			break;

		case REQUEST_DATA: 
			msg[6] = ETX;
			int frameSize = dest->size+7;

again0x81:
			if((output_len=requestAndResponseSerial(msg,buf,frameSize))>0){		
				if(!checkCorrect(buf,output_len)){
					tick++;
					delay(DELAY);
					if(tick>MAXTRIAL){
						sendhalt(dest);
						return -1;
					}
					goto again0x81;
				}
				dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				beforeRequest = *(uint8_t*)(buf+5);
				for(i=0;i<beforeRequest;i++){
					*(uint8_t*)(dest+5+i) = *(buf+6+i);
				}
			}else{
				tick++;
				delay(DELAY);
				if(tick>MAXTRIAL){
					sendhalt(dest);
					return -1;
				}
				goto again0x81;
				
			}

			printf("RESPONSE : ");
			for(i=0;i<dest->size+5;i++){
				printf("%02x ",*((uint8_t*)(dest)+i));
				//printf("%02x ", buf[i]);
			}
			printf("\n");
#ifndef TEST_ONE_BOARD
			msg[1] = (uint8_t)(SENSOR2>>8)&0xff;
			msg[2] = (uint8_t)SENSOR2&0xff;

again0x81_2:
			delay(100);
			if((output_len=requestAndResponseSerial(msg,buf1,frameSize))>0){
				if(!checkCorrect(buf1,output_len)){
					tick++;
					delay(DELAY);
					if(tick>MAXTRIAL){
						sendhalt(dest);
						return -1;
					}
					goto again0x81_2;
				}
				dest->size += ((uint16_t)(buf1[4])<<8)|(buf1[5]&0xff);
				
				for(i=0;i<*(buf1+5);i++){
					*(uint8_t*)(dest+beforeRequest+i) = *(buf1+6+i);
				}

			}else{
				tick++;
				delay(DELAY);
				if(tick>MAXTRIAL){
					sendhalt(dest);
					return -1;
				}
				goto again0x81_2;
			}
#endif
			break;


		case REQUEST_SET_RELAY : 
			msg[6] = dest->data[0];
			msg[7] = ETX;
			uint8_t sensorData1 = msg[6]&0x0f;
			uint8_t sensorData2 = msg[6]>>4;

			if(sensorData1>=0){
				msg[6] = sensorData1;
again0x84:
				if((output_len=requestAndResponseSerial(msg,buf,dest->size+7))>0){
					if(!checkCorrect(buf,output_len)||output_len!=8){
						tick++;
						delay(DELAY);
						if(tick>MAXTRIAL){
							sendhalt(dest);
							return -1;
						}
						goto again0x84;
					}
				//dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				}else{
					tick++;
					delay(DELAY);
					if(tick>MAXTRIAL){
						sendhalt(dest);
						return -1;
					}
					goto again0x84;
				}
			
		}
#ifdef TEST_ONE_BOARD
			
			for(i=0;i<dest->size;i++){
				*((uint8_t*)(dest)+5+i) = *(buf+6+i);
			}
			printf("RESPONSE : ");
			for(i=0;i<dest->size+5;i++){
				printf("%02x ",*((uint8_t*)(dest)+i));
			}
			printf("\n");
#endif
#ifndef TEST_ONE_BOARD
			if(sensorData2>0){
				msg[6] = sensorData2;
				msg[1] = (uint8_t)(SENSOR2>>8)&0xff;
				msg[2] = (uint8_t)SENSOR2&0xff;

again0x84_2:
				if((output_len=requestAndResponseSerial(msg,buf1,dest->size+7))>0){		
					if(!checkCorrect(buf1,output_len)||output_len!=8){
						tick++;
						delay(DELAY);
						if(tick>MAXTRIAL){
							sendhalt(dest);
							return -1;
						}
						goto again0x84_2;
					}
					dest->size = ((uint16_t)(buf1[4])<<8)|(buf1[5]&0xff);
				}else{
					tick++;
					delay(DELAY);
					if(tick>MAXTRIAL){
						sendhalt(dest);
						return -1;
					}
					goto again0x84_2;
				}

			}
			for(i=0;i<dest->size;i++){
				*((uint8_t*)(dest)+5+i) = (*(buf+6+i))|(*(buf1+6+i));
			}
#endif
			break;


		case REQUEST_CHANNEL_RST : 
			msg[6] = dest->data[0];
			msg[7] = ETX;
			if((output_len=requestAndResponseSerial(msg,buf,dest->size+7))>0){		
				dest->size = ((uint16_t)(buf[4])<<8)|(buf[5]&0xff);
				for(i=0;i<*(uint8_t*)(dest+4);i++){
					*(uint8_t*)(dest+5+i) = *(buf+6+i);
				}
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

bool checkCorrect(uint8_t* buf, int size){
	if(buf[0]==0x02&&buf[size-1]==0x03) return true;
	else return false;
}

int sendhalt(Frame* dest){
	int flag =0;
	if(pthread_mutex_trylock(&mut)==0){
		haltsignal();
		pthread_mutex_unlock(&mut);
		flag =1;
	}
	if(flag == 0) delay(1500);
	dest->opcode = 0xff;
	return 1;
}
