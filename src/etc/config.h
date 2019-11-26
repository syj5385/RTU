/*
 * config.h
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#ifndef SRC_CONFIG_CONFIG_H_
#define SRC_CONFIG_CONFIG_H_

#include <stdint.h>

#define		CONF_LOCATION		"/etc/rtu/rtu.conf"
#define 	PORT_CONF_LOCATION	"/etc/rtu/ports.conf"

char* getConfigContent(char* title);
int getConfigPort();
int setConfigPort(int newport);
uint16_t getSensorID(int index);
int setRTUid(int newid);
uint16_t getRTUID();

#endif /* SRC_CONFIG_CONFIG_H_ */
