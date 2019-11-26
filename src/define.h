/*
 * define.h
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#ifndef SRC_DEFINE_H_
#define SRC_DEFINE_H_

#include <stdint.h>
#include <pthread.h>

#define 	STAND_PRINT

/*
 * Definition of RTU
 */
#define		SERVERNAME 			"RTU"

#define 	NETWORK_PROC			"network"
#define		SERSOR_PROC			"sensor"

#define		EXIT_NORMAL			0
#define		EXIT_KILL_ALL			1
#define		EXIT_KILL_ME			2

#define		LOG_LOCATION			"/var/log/rtu.log"

/*
 *  Definition for RTU TCP Server
 */
#define 	MAX_CLIENT			10

int TCPPORT ;
uint16_t SENSOR1; 
uint16_t SENSOR2;
uint16_t RTU_ID ;

typedef struct {
	/* sensor status information */
	uint8_t sensor1_data[3];
	uint8_t sensor2_data[3]; 
}CMD_60_DATA;

typedef struct {
	/* Sensor data */
	uint8_t sensor1_data[12];
	uint8_t sensor2_data[12];
}CMD_61_DATA;


CMD_60_DATA *cmd_60_data; 
CMD_61_DATA *cmd_61_data; 
pthread_mutex_t halt_mutex; 

/* Definition for Inter-Process-Communication(IPC) */
#define		IPC_TIMEOUT_SEC		3
#define		IPC_TIMEOUT_MILLI	0
//#define		SENSOR_NOT_EXECUTE

/* Definition for LOG */

#define	LOG_LVL_TRACE	50
#define	LOG_LVL_DEBUG	40
#define	LOG_LVL_INFO	30
#define	LOG_LVL_WARNING	20
#define	LOG_LVL_ERROR	10
#define	LOG_LVL_FATAL	0

#define LOG_PATH	"/var/log/rtu/"
#define LOG_LEVEL	LOG_LVL_ERROR

#define INTERVAL_OF_SENSOR_REQUEST	1000  /* unit : [ms] */ 
//#define TEST_ONE_BOARD
//#define READ_ONE_BYTE
//#define		DEMO
#endif /* SRC_DEFINE_H_ */
