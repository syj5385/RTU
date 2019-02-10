/*
 * netinfo.h
 *
 *  Created on: Jan 25, 2019
 *      Author: jjun
 */

#ifndef SRC_NETWORK_NETINFO_H_
#define SRC_NETWORK_NETINFO_H_

#include <stdlib.h>
#include <arpa/inet.h>
#include "../define.h"
#define GATEWAY_CMD 		"cat /proc/net/route > /etc/rtu/gateway.txt"
#define	GATEWAY_FILENAME	"/etc/rtu/gateway.txt"
#define GATEWAY_DEFAULT 	"default"

struct route_info{
	struct in_addr dstAddr;
	struct in_addr srcAddr; 
	struct in_addr gateWay; 
	char ifName[1024];
};

char* getAddressFromIntegerArray(u_long before);
u_long get_IPAddress(char* interface);
u_long get_SubnetMask(char* interface);
u_long get_gatewayip(char* ifname);

//u_long get_DefaultGateway(char* interface_name);

int setNetworkInformation(char *ipaddr, char *netmask, char *gateway);
#endif /* SRC_NETWORK_NETINFO_H_ */
