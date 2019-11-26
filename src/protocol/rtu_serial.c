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
#include "../etc/config.h"
#include "../etc/log_util.h"
#include "../network/netinfo.h"
#include "../define.h"

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
	

	/* 3. make frame box */
	index = 1;
	frame -> device_id = read16(rcv_buf[index], rcv_buf[index+1]);
	index+=2;
	frame -> opcode = rcv_buf[index++];
	frame -> size = read16(rcv_buf[index], rcv_buf[index+1]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = rcv_buf[index++];
	
	if(frame->device_id != RTU_ID){
		LOG_WARNING("Device id is not matched");
		return RESULT_INVALID;
	}
	return RESULT_OK;
}


int request_current_status(Frame* frame, int* ipc_sockfd){ 
	/* implementation for request current relay and sensor status */
	/* OPCODE = 0x60 */
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;
#ifdef DEMO
	frame -> size = 6; 
	for(i=0; i<frame->size; i++)
		*(frame -> data+i) = (uint8_t)(rand()*255+1)%0xff; 
	return 0;

#endif

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
	/* result : <data size> <data> ... <data> */ 
	/* Receive result from sersor */
	n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024);
	printf("Read : %d\n",n);
	if(n<0){
		LOG_ERROR("ERROR : Failed to get current status : Read IPC Data Failed");
		return -1; 
	}
	LOG_TRACE("Let's write Data to PC client");
	bzero(frame -> data, sizeof(frame->data));

	index = 3;
	if(ipc_read_buf[2] == 0xff)
		return -1;
	frame -> size = read16(ipc_read_buf[index+1], ipc_read_buf[index]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = ipc_read_buf[index++];

	return 0;
}

int request_sensor_data(Frame* frame, int* ipc_sockfd){
	/* OPcode : 0x61 */ 
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;

#ifdef DEMO
	frame -> size = 24; 
	for(i=0; i<frame->size; i++)
		*(frame -> data+i) = (uint8_t)(rand()*255+1)%0xff; 
	return 0;

#endif
	bzero(ipc_read_buf,sizeof(ipc_read_buf));
	//ipc_tmp[index++] = 0x02;
	ipc_tmp[index++] = (uint8_t)((frame -> device_id) & 0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> device_id) >> 8) & 0xff);
	ipc_tmp[index++] = frame -> opcode;
	ipc_tmp[index++] = (uint8_t)((frame->size)&0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	for(i=0; i<frame->size; i++)
		ipc_tmp[index++] = *(frame -> data + i);


	/* send <opcode> <data_size_high> <data_size_low> <data> - - - <data> */
	n = write(*ipc_sockfd, ipc_tmp, frame->size+5);
	n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024);
	if(n<0){
		LOG_ERROR("ERROR : Failed to get sensor data : Read IPC Data Failed");
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
	/* OPCODE : 0x62 */ 
	u_long ip_addr; // unsigned long Big_Endian
	u_long netmask; // unsigned long Big_Endian
	u_long gateway; // unsigned long Big_Endian
	char interface_name[100]; 
	int server_port, rtu_id;
	int result;
	int index = 0;
	int i=0;
	bzero(interface_name, sizeof(interface_name));

	LOG_CALL(gateway = get_gatewayip(interface_name));
	if(gateway == -1){
		LOG_ERROR("Error : Failed to get default gateway");
		goto failed; 
	}

	LOG_CALL(ip_addr = get_IPAddress(interface_name));
	if(ip_addr == -1){
		LOG_ERROR("Error : Failed to get IP address");
		goto failed ;
	}

	LOG_CALL(netmask = get_SubnetMask(interface_name));
	if(netmask == -1){
		LOG_ERROR("Error : Failed to get netmask");
		goto failed;
	}

	server_port = TCPPORT;
	rtu_id = RTU_ID;

	/* set frame */
	frame -> size = 16;

	for(i=0; i<4; i++){	// ip_address
		*(frame->data + index) = (ip_addr & 0xff);
		ip_addr = ip_addr >> 8;
		index++;
	}

	for(i=0; i<4; i++){	// subnet mask
		*(frame->data + index) = (netmask & 0xff);
		netmask = netmask >> 8;
		index++;
	}


	for(i=0; i<4; i++){	// gateway mask
		*(frame->data + index) = (gateway & 0xff);
		gateway = gateway >> 8;
		index++;
	}

	// server Port number
	*(frame -> data + index++) = ((server_port >> 8) & 0xff);
	*(frame -> data + index++) = ((server_port) & 0xff);

	// server Port number
	*(frame -> data + index++) = ((rtu_id >> 8) & 0xff);
	*(frame -> data + index++) = ((rtu_id) & 0xff);

	return 0;

failed : 
	return -1;
}

int request_set_rtu(Frame* frame){

	/* OPCODE : 0x63 */ 
	int index = 0;
	u_long ip_addr=0, netmask=0, gateway=0;
	uint16_t server_port=0, rtu_id=0;
	char ip_addr_string[100];
	char netmask_string[100];
	char gateway_string[100];
	int result;
	char buf[1000]; 
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

	//ip_addr_string=getAddressFromIntegerArray(ip_addr);
	strcpy(ip_addr_string, getAddressFromIntegerArray(ip_addr));
	//netmask_string=getAddressFromIntegerArray(netmask);
	strcpy(netmask_string,getAddressFromIntegerArray(netmask));
	//gateway_string=getAddressFromIntegerArray(gateway,gateway_string);
	strcpy(gateway_string, getAddressFromIntegerArray(gateway));
	sprintf(buf, "ip : %s\t,netmask : %s\tgatesay : %s\tport : %d\trtu_id : %d", ip_addr_string, netmask_string, gateway_string,server_port, rtu_id);
	LOG_TRACE(buf);
	setConfigPort(server_port);
	setRTUid(rtu_id);
	result = setNetworkInformation(ip_addr_string, netmask, gateway_string);
	if(result == -1)
		return -1;

	else return 0; 
}

int request_set_relay(Frame* frame, int* ipc_sockfd){
	/* OPcode : 0x64 */ 
	uint8_t ipc_tmp[frame->size + 5];
	uint8_t ipc_read_buf[1024];
	int i, n;
	int index = 0;
	bzero(ipc_read_buf,sizeof(ipc_read_buf));
	
#ifdef DEMO
	frame -> size = 1;
	*(frame->data) = 0x00;
	return 0;

#endif

	//ipc_tmp[index++] = 0x02; 
	ipc_tmp[index++] = (uint8_t)((frame -> device_id) & 0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> device_id) >> 8) & 0xff);
	ipc_tmp[index++] = frame -> opcode;
	ipc_tmp[index++] = (uint8_t)((frame->size)&0xff);
	ipc_tmp[index++] = (uint8_t)(((frame -> size)>>8)&0xff);
	for(i=0; i<frame->size; i++)
		ipc_tmp[index++] = *(frame -> data + i);
	//ipc_tmp[index++] = 0x03; 


	char buf[100];
	/* send <opcode> <data_size_high> <data_size_low> <data> - - - <data> */
	n = write(*ipc_sockfd, ipc_tmp, frame->size+5);
	if(n<0)
		return -1;

	n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024);
	/* result : <data size> <data> ... <data> */ 
	/* Receive result from sersor */
	if(n < 0){
		LOG_ERROR("Failed to set Relay : Failed to read IPC");
		return -1;
	}
	printf("Network received : ");
	for(i=0; i<n; i++)
		printf("%02x ", ipc_read_buf[i]);
	printf("\n");
	bzero(frame -> data, sizeof(frame->data));

	index = 3 ;
	frame -> size = read16(ipc_read_buf[index+1], ipc_read_buf[index]);
	index+=2;
	for(i=0; i<frame->size; i++)
		*(frame -> data + i) = ipc_read_buf[index++];

	return 0;
}

int request_soft_reset(Frame* frame){	

	/* OPcode : 0x65 */ 
	pid_t reboot_pid; 

	if((reboot_pid = fork()) == -1){
		frame -> size = 1;
		bzero(frame, 1024);
		*(frame -> data) = 0; 
		return -1;
	}
	else if(reboot_pid == 0){
		LOG_INFO("Reboot is request from client");
		sleep(2);
		execl("/sbin/reboot", "/sbin/reboot", NULL);
		
	}

	frame -> size = 1;
	bzero(frame, 1024);
	*(frame -> data) = 1; 
	return 0; 

}


int request_reset_channel(Frame *frame, int *ipc_sockfd){
	/* Opcode : 0x66 */
	uint8_t ipc_read_buf[1024];
	uint8_t ipc_tmp[frame->size + 5];
	int i, n;
	int index = 0;
	bzero(ipc_read_buf,sizeof(ipc_read_buf));

#ifdef DEMO
	frame -> size = 1; 
	*(frame->data) = 0x00;
	return 0;

#endif

	//ipc_tmp[index++] = 0x02; 
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

	n = readIPC_Data(*ipc_sockfd, ipc_read_buf, 1024);
	if(n<0){
		LOG_ERROR("ERROR : Failed to reset Channel : Failed to read IPC");
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

int executeProtocol(Frame* frame, int* ipc_sockfd){
	int opcode = frame -> opcode;
	int result;
	switch(opcode){
		case REQUEST_STATUS :
			/* request current relay and sensor status info */
			LOG_INFO("Request : Request current relay/sensor status");
			LOG_CALL(result = request_current_status(frame,ipc_sockfd));
			return result; 

		case REQUEST_DATA :
			/* request sensor data */
			LOG_INFO("Request : Request current relay/sensor data");
			LOG_CALL(result = request_sensor_data(frame, ipc_sockfd));
			return result;

		case REQUEST_RTU_INFO :
			/* request current RTU setting information */
			LOG_INFO("Request : request RTU information");
			LOG_CALL(result = request_rtu_info(frame));
			return result; 

		case REQUEST_SET_RTU :
			/* request to set RTU */
			LOG_INFO("Request : request to set RTU");
			LOG_CALL(result = request_set_rtu(frame));
			return result; 

		case REQUEST_SET_RELAY :
			/* request to control relay */
			LOG_INFO("request : request to set relay");
			LOG_CALL(result = request_set_relay(frame, ipc_sockfd));
			return result;

		case REQUEST_SOFT_RST :
			/* request to reset rtu */
			LOG_INFO("Request : request to reset RTU");
			LOG_CALL(result = request_soft_reset(frame));
			return result;

		case REQUEST_CHANNEL_RST : 
			LOG_INFO("Request : request to reset channel data");
			LOG_CALL(result = request_reset_channel(frame,ipc_sockfd)); 
			return result; 

		default : 

			return 1; /* Invalid OP code */ 
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

	dest[index++] = ETX;

}
