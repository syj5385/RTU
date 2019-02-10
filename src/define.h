/*
 * define.h
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#ifndef SRC_DEFINE_H_
#define SRC_DEFINE_H_

#define 	STAND_PRINT

/*
 * Definition of RTU
 */
#define		SERVERNAME 			"RTU"

#define 	NETWORK_PROC			"network"
#define		SERSOR_PROC				"sensor"
#define		LOG_LOCATION			"/var/log/rtu.log"

/*
 *  Definition for RTU TCP Server
 */
#define 	TCP_PORT			10000
#define		RTU_ID				0x10
#define 	MAX_CLIENT			10

/*
 *  Definition for Protocol
 */
//#define 	ENABLE_CHECKSUM

#endif /* SRC_DEFINE_H_ */
