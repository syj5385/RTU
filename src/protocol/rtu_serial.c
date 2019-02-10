/*
 * rtu_serial.c
 *
 *  Created on: Jan 22, 2019
 *      Author: jjun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "rtu_serial.h"
#include "../etc/ipc.h"
#include "../etc/log.h"
#include "../network/netinfo.h"

const char* rtu_TAG = "PROTOCOL";

static uint16_t calculateCRC16(uint8_t* buf, const int length){
	uint16_t initCRC = 0xffff;
	uint16_t crc = 0;
	int i,j;

	if((buf == 0) || (length == 0))
		return 0;

	for(i=0; i<length; i++){
		initCRC ^= buf[i];
		for(j=0; j<8; j++){
			crc = initCRC;
			initCRC >>= 1;
			if((crc & 0x0001) > 0){
				initCRC ^= 0xa001;
			}
		}
	}
	return initCRC;
}

uint16_t read16(uint8_t high, uint8_t low){
	return (high<<8) + low;
}

int analyzeReceivedMsg(Frame* frame, uint8_t* rcv_buf, size_t size){
	int index=0, i=0;
	uint8_t stx, etx;
	uint16_t calculated_crc, crcInMsg;

	/* 1. STX & ETX Check */
	stx = rcv_buf[0];
	etx = rcv_buf[size-1];
	if(stx != STX || etx != ETX)
		return RESULT_INVALID;

	/* 2. calculate checksum */
	crcInMsg = read16(rcv_buf[size-3], rcv_buf[size-2]);
	rcv_buf[size-3] = 0;
	rcv_buf[size-2] = 0;
	calculated_crc = calculateCRC16(rcv_buf+1, size-4);

#ifdef	ENABLE_CHECKSUM
	if(crcInMsg != calculated_crc)
		return RESULT_INVALID_CHECKSUM;
#endif

	/* 3. make frame box */
	index = 1;
	frame -> device_id = read16(rcv_buf[index], rcv_buf[index+1]);
	index+=2;
	frame -> opcode = rcv_buf[index++];
	frame -> size = read16(rcv_buf[index], rcv_buf[index+1]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = rcv_buf[index++];

	return RESULT_OK;
}


int request_current_status(Frame* frame, int* ipc_sockfd){
	/* implementation for request current relay and sensor status */
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;
	bzero(ipc_read_buf,sizeof(ipc_read_buf));

	ipc_tmp[index++] = (uint8_t)((frame -> device_id) & 0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> device_id) >> 8) & 0xff);
	ipc_tmp[index++] = frame -> opcode;
	ipc_tmp[index++] = (uint8_t)((frame->size)&0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	for(i=0; i<frame->size; i++)
		ipc_tmp[index++] = *(frame -> data + i);

	/* send <opcode> <data_size_high> <data_size_low> <data> - - - <data> */
	n = write(*ipc_sockfd, ipc_tmp, frame->size+5);
	if(n<0)
		return -1;

	/* result : <data size> <data> ... <data>
	/* Receive result from sersor */
	while((n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024)) <= 0){
		if(n==-1)
			return n;
	}

	if(n<0){
		writeLog(ERROR,rtu_TAG,"Failed to read IPC data from sensor process");
		return -1;
	}

	bzero(frame -> data, sizeof(frame->data));

	index = 3 ;
	frame -> size = read16(ipc_read_buf[index+1], ipc_read_buf[index]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = ipc_read_buf[index++];

	return 0;
}

int request_sensor_data(Frame* frame, int* ipc_sockfd){
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;
	bzero(ipc_read_buf,sizeof(ipc_read_buf));

	ipc_tmp[index++] = (uint8_t)((frame -> device_id) & 0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> device_id) >> 8) & 0xff);
	ipc_tmp[index++] = frame -> opcode;
	ipc_tmp[index++] = (uint8_t)((frame->size)&0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	for(i=0; i<frame->size; i++)
		ipc_tmp[index++] = *(frame -> data + i);

	/* send <opcode> <data_size_high> <data_size_low> <data> - - - <data> */
	n = write(*ipc_sockfd, ipc_tmp, frame->size+5);
	if(n<0)
		return -1;

	/* result : <data size> <data> ... <data>
	/* Receive result from sersor */
	while((n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024)) <= 0){
		if(n==-1)
			return n;
	}

	if(n<0){
		writeLog(ERROR,rtu_TAG,"Failed to read IPC data from sensor process");
		return -1;
	}

	bzero(frame -> data, sizeof(frame->data));

	/* Ignore rtu_id */
	index = 3;
	frame -> size = read16(ipc_read_buf[index+1], ipc_read_buf[index]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = ipc_read_buf[index++];

	return 0;
}

int request_rtu_info(Frame* frame){
	/* implementation for get Current RTU network information */
	u_long ip_addr; // unsigned long Big_Endian
	u_long netmask; // unsigned long Big_Endian
	u_long gateway; // unsigned long Big_Endian
	char interface_name[100]; 
	int server_port, rtu_id;
	int result;
	int index = 0;
	int i=0;
	bzero(interface_name, sizeof(interface_name));

	

	if((gateway = get_gatewayip(interface_name)) == -1){
		writeLog(ERROR,rtu_TAG,"Error : Failed to get default gateway");

		return -1;
	}

	if((ip_addr = get_IPAddress(interface_name)) == -1){
		writeLog(ERROR,rtu_TAG,"Error : Failed to get IP address");
		return -1;
	}
	if((netmask = get_SubnetMask(interface_name)) == -1){
		writeLog(ERROR,rtu_TAG,"Error : Failed to get netmask");
		return -1;
	}

	server_port = TCP_PORT;
	rtu_id = RTU_ID;

	/* set frame */
	frame -> size = 16;

	for(i=0; i<4; i++){	// ip_address
		*(frame->data + index) = (ip_addr & 0xff);
//		printf("%d\n", *(frame -> data + index));
		ip_addr = ip_addr >> 8;
		index++;
	}

	for(i=0; i<4; i++){	// subnet mask
		*(frame->data + index) = (netmask & 0xff);
//		printf("%d\n", *(frame -> data + index));
		netmask = netmask >> 8;
		index++;
	}


	for(i=0; i<4; i++){	// gateway mask
		*(frame->data + index) = (gateway & 0xff);
//		printf("%d\n", *(frame -> data + index));
		gateway = gateway >> 8;
		index++;
	}

	// server Port number
	*(frame -> data + index++) = ((server_port >> 8) & 0xff);
	*(frame -> data + index++) = ((server_port) & 0xff);

	// server Port number
	*(frame -> data + index++) = ((rtu_id >> 8) & 0xff);
	*(frame -> data + index++) = ((rtu_id) & 0xff);
}

int request_set_rtu(Frame* frame){
	int index = 0;
	u_long ip_addr=0, netmask=0, gateway=0;
	uint16_t server_port=0, rtu_id=0;
	char *ip_addr_string, *netmask_string, *gateway_string;
	int result;

	/* get ip_address */
	ip_addr = frame->data[index] + ((frame->data[index+1] << 8) & 0xff00) + ((frame->data[index+2]<<16) & 0xff0000)
			+ ((frame->data[index+3]<<24) & 0xff000000);
	index+=4;

	/* get netmask */
	netmask = frame->data[index] + ((frame->data[index+1] << 8) & 0xff00) + ((frame->data[index+2]<<16) & 0xff0000)
			+ ((frame->data[index+3]<<24) & 0xff000000);
	index+=4;

	/* get gateway */
	gateway = frame->data[index] + ((frame->data[index+1] << 8) & 0xff00) + ((frame->data[index+2]<<16) & 0xff0000)
			+ ((frame->data[index+3]<<24) & 0xff000000);
	index+=4;

	/* get server port # */
	server_port = ((frame->data[index]<<8)&0xff00) | (frame->data[index+1] & 0xff);
	index+=2;

	/* get RTU number */
	rtu_id = ((frame->data[index]<<8)&0xff00) | (frame->data[index+1] & 0xff);

	ip_addr_string = getAddressFromIntegerArray(ip_addr);
	netmask_string = getAddressFromIntegerArray(netmask);
	gateway_string = getAddressFromIntegerArray(gateway);

	result = setNetworkInformation(ip_addr_string, netmask_string, gateway_string);
}

int request_set_relay(Frame* frame, int* ipc_sockfd){
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;
	bzero(ipc_read_buf,sizeof(ipc_read_buf));

	ipc_tmp[index++] = (uint8_t)((frame -> device_id) & 0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> device_id) >> 8) & 0xff);
	ipc_tmp[index++] = frame -> opcode;
	ipc_tmp[index++] = (uint8_t)((frame->size)&0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	for(i=0; i<frame->size; i++)
		ipc_tmp[index++] = *(frame -> data + i);


	char buf[100];
	sprintf(buf, "size : %02x, %02x",ipc_tmp[3], ipc_tmp[4]);
	writeLog(NORMAL, rtu_TAG, buf);
	/* send <opcode> <data_size_high> <data_size_low> <data> - - - <data> */
	n = write(*ipc_sockfd, ipc_tmp, frame->size+5);
	if(n<0)
		return -1;

	/* result : <data size> <data> ... <data>
	/* Receive result from sersor */
	while((n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024)) <= 0){
		if(n==-1)
			return n;
	}

	if(n<0){
		writeLog(ERROR,rtu_TAG,"Failed to read IPC data from sensor process");
		return -1;
	}

	bzero(frame -> data, sizeof(frame->data));

	index = 3 ;
	frame -> size = read16(ipc_read_buf[index+1], ipc_read_buf[index]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = ipc_read_buf[index++];

	return 0;
}

int request_soft_reset(Frame* frame){	
	pid_t reboot_pid; 

	if((reboot_pid = fork()) == -1){
		frame -> size = 1;
		bzero(frame, 1024);
		*(frame -> data) = 0; 
		return -1;
	}
	else if(reboot_pid == 0){
		writeLog(NORMAL, rtu_TAG, "Reboot will be started in 2 sec");
		sleep(2);
		execl("/sbin/reboot", "/sbin/reboot", NULL);
		
	}

	frame -> size = 1;
	bzero(frame, 1024);
	*(frame -> data) = 1; 
	return 0; 

}

int executeProtocol(Frame* frame, int* ipc_sockfd){
	int opcode = frame -> opcode;
	int result;
	switch(opcode){
		case REQUEST_STATUS :
			/* request current relay and sensor status info */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request current relay/sensor status");
			return (result = request_current_status(frame,ipc_sockfd));
			

		case REQUEST_DATA :
			/* request sensor data */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request current relay/sensor data");
			return (result = request_sensor_data(frame,ipc_sockfd));

		case REQUEST_RTU_INFO :
			/* request current RTU setting information */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request RTU Information");
			return (result = request_rtu_info(frame));

		case REQUEST_SET_RTU :
			/* request to set RTU */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request RTU setting");
			return (result = request_set_rtu(frame));

		case REQUEST_SET_RELAY :
			/* request to control relay */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request to set relay");
			return (result = request_set_relay(frame, ipc_sockfd));

		case REQUEST_SOFT_RST :
			/* request to reset rtu */
			writeLog(NORMAL,rtu_TAG,"REQUEST : request to reset RTU");
			return (result = request_soft_reset(frame));
	}
}

void makeMsgUsingframe(Frame* frame, uint8_t* dest){
	int index = 0;
	int i=0, crc = 0, crc_check_length = 0;

	bzero(dest,sizeof(dest));
	dest[index++] = STX;
	dest[index++] = (uint8_t)(((frame -> device_id)>>8)&0xff);
	dest[index++] = (uint8_t)((frame -> device_id) & 0xff);
	dest[index++] = frame -> opcode;
	dest[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	dest[index++] = (uint8_t)((frame -> size) & 0xff);

	for(i=0; i<frame->size; i++)
		dest[index++] = *(frame -> data + i);

	crc_check_length = index-1;

	crc = calculateCRC16(dest+1, crc_check_length);
	dest[index++] = (uint8_t)(crc & 0xff);
	dest[index++] = (uint8_t)((crc>>8)&0xff);
	dest[index++] = ETX;

}
