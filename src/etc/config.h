/*
 * config.h
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#ifndef SRC_CONFIG_CONFIG_H_
#define SRC_CONFIG_CONFIG_H_

#define		CONF_LOCATION		"/etc/rtu/rtu.conf"
#define 	PORT_CONF_LOCATION	"/etc/rtu/ports.conf"

char* getConfigContent(char* title);
int getConfigPort();

#endif /* SRC_CONFIG_CONFIG_H_ */
