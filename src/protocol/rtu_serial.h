/*
 * rtu_serial.h
 *
 *  Created on: Jan 22, 2019
 *      Author: jjun
 */

#ifndef SRC_PROTOCOL_RTU_SERIAL_H_
#define SRC_PROTOCOL_RTU_SERIAL_H_

#include <stdlib.h>
#include <stdint.h>

/* STX & ETX */
#define		STX			0x02
#define		ETX 			0x03

/* Definition for RTU serial frame */
typedef struct{
	uint16_t device_id;
	uint8_t opcode;
	uint16_t size;
	uint8_t data[1024];
} __attribute__((packed))Frame;

/* Definition for RTU serial op code */
#define 	REQUEST_STATUS		0x60
#define		REQUEST_DATA		0x61
#define		REQUEST_RTU_INFO	0x62
#define		REQUEST_SET_RTU		0x63
#define		REQUEST_SET_RELAY	0x64
#define		REQUEST_SOFT_RST	0x65
#define		REQUEST_CHANNEL_RST	0x66

/* Definition for RTU Serial Result code */
#define 	RESULT_OK			0x00
#define		RESULT_INVALID			0x01
#define		RESULT_INVALID_CHECKSUM		0x02 /* not used */ 
#define		RESULT_FAILED			0x03

/* proto type of function for RTU serial */

uint16_t read16(uint8_t high, uint8_t low);
int analyzeReceivedMsg(Frame* frame, uint8_t* rcv_buf,size_t size);
int executeProtocol(Frame* frame, int* ipc_sockfd);
void makeMsgUsingframe(Frame* frame, uint8_t* dest);

#endif /* SRC_PROTOCOL_RTU_SERIAL_H_ */
