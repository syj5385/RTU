/*
 i netinfo.c
 *
 *  Created on: Jan 25, 2019
 *      Author: jjun
 */

#include "netinfo.h"
#include <stdio.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <bits/ioctls.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <net/route.h>
#include "../etc/log_util.h"
#include <linux/rtnetlink.h>

#include "../define.h"

#define DHCPCD_OLD "/etc/dhcpcd_old.conf"
#define	DHCPCD	"/etc/dhcpcd.conf"

char* getAddressFromIntegerArray(u_long before){
	
	struct in_addr after_addr;
	after_addr.s_addr = before;
	return inet_ntoa(after_addr);
}

u_long get_IPAddress(char* interface){
	int sockfd;
	struct ifreq ifr;
	struct sockaddr_in *addr;
	u_long ip_address;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(socket < 0){
		return -1;
	}

	strcpy(ifr.ifr_name, interface);
	if(ioctl(sockfd, SIOCGIFADDR, &ifr) < 0){
		close(sockfd);
		return -1;
	}

	addr = (struct sockaddr_in*)&(ifr.ifr_addr);

	ip_address = (u_long)addr->sin_addr.s_addr;
	close(sockfd);
	return ip_address;
}

u_long get_SubnetMask(char* interface){
	int sockfd;
	struct ifreq ifr;
	struct sockaddr_in *addr;
	u_long netmask;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		return -1;
	}
	strcpy(ifr.ifr_name, interface);

	if (ioctl(sockfd, SIOCGIFNETMASK, &ifr)< 0){
		close(sockfd);
		return -1;
	}
	addr = (struct sockaddr_in*)&ifr.ifr_addr;
	netmask = (u_long)addr->sin_addr.s_addr;
	close(sockfd);
	return netmask;
}

int readNlSocket(int sockfd, char* bufPtr, int seqNum, int pid){
        struct nlmsghdr* nlHdr;
        int readLen = 0, msgLen=0;

        do{

                if((readLen = recv(sockfd, bufPtr, BUFSIZ-msgLen,0)) < 0){
                        perror("SOCK READ: ");
                        return -1;
                }
                nlHdr = (struct nlmsghdr*)bufPtr;

                if(NLMSG_OK(nlHdr,readLen) == 0 || (nlHdr -> nlmsg_type == NLMSG_ERROR)){
                        perror("Error in received packet");
                        return -1;
                }

                if(nlHdr -> nlmsg_type == NLMSG_DONE)
                        break;

                else{
                        bufPtr += readLen;
                        msgLen += readLen;
                }

                if((nlHdr -> nlmsg_flags & NLM_F_MULTI)==0){
                        break;
                }

        }while((nlHdr -> nlmsg_seq != seqNum)|| (nlHdr->nlmsg_pid != pid));

        return msgLen;
}

void parseRoutes(struct nlmsghdr* nlHdr, struct route_info *rtInfo){
        struct rtmsg *rtMsg;
        struct rtattr *rtAttr;
        int rtLen;

        rtMsg = (struct rtmsg*)NLMSG_DATA(nlHdr);

        if((rtMsg->rtm_family != AF_INET)||(rtMsg->rtm_table != RT_TABLE_MAIN))
                return;

        rtAttr = (struct rtattr*)RTM_RTA(rtMsg);
        rtLen = RTM_PAYLOAD(nlHdr);

        for(;RTA_OK(rtAttr,rtLen); rtAttr=RTA_NEXT(rtAttr,rtLen)){
                switch(rtAttr -> rta_type){
                        case RTA_OIF:
                                if_indextoname(*(int*)RTA_DATA(rtAttr),rtInfo->ifName);
                                break;

                        case RTA_GATEWAY:
                                memcpy(&rtInfo->gateWay,RTA_DATA(rtAttr),sizeof(rtInfo->gateWay));
                                break;

                        case RTA_PREFSRC:
                                memcpy(&rtInfo->srcAddr, RTA_DATA(rtAttr),sizeof(rtInfo->srcAddr));
                                break;

                        case RTA_DST:
                                memcpy(&rtInfo->dstAddr, RTA_DATA(rtAttr),sizeof(rtInfo->dstAddr));
                                break;
                }
        }
        return;
}

u_long get_gatewayip(char* ifname){
        int found_gatewayip;

        struct nlmsghdr *nlMsg;
        struct rtmsg *rtMsg;
        struct route_info *rtInfo;
        char msgBuf[BUFSIZ];
	u_long gatewayip; 

        int sock, len, msgSeq = 0;

        if((sock = socket(PF_NETLINK,SOCK_DGRAM, NETLINK_ROUTE))<0){
                perror("Socket creation");
                return -1;
        }

        memset(msgBuf,0,BUFSIZ);

        nlMsg = (struct nlmsghdr*)msgBuf;
        rtMsg = (struct rtmsg*)NLMSG_DATA(nlMsg);

        nlMsg -> nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
        nlMsg -> nlmsg_type = RTM_GETROUTE;

        nlMsg -> nlmsg_flags = NLM_F_DUMP|NLM_F_REQUEST;
        nlMsg -> nlmsg_seq = msgSeq++;
        nlMsg -> nlmsg_pid = getpid();
        if(send(sock,nlMsg, nlMsg -> nlmsg_len, 0)<0){
                return -1;
        }

        if((len = readNlSocket(sock, msgBuf,msgSeq, getpid())) < 0){
                return -1;
        }


        rtInfo = (struct route_info*)malloc(sizeof(struct route_info));

        for(; NLMSG_OK(nlMsg,len); nlMsg=NLMSG_NEXT(nlMsg,len)){
                memset(rtInfo, 0 ,sizeof(struct route_info));
                parseRoutes(nlMsg, rtInfo);

                if(strstr((char*)inet_ntoa(rtInfo->dstAddr),"0.0.0.0")){
			gatewayip = (u_long)rtInfo->gateWay.s_addr;
                        strcpy(ifname, rtInfo -> ifName);
                        found_gatewayip = 1;
                        break;
                }
        }
        free(rtInfo);
        close(sock);
        return gatewayip;

}

int setNetworkInformation(char *ipaddr, u_long netmask, char *gateway){

	char information[1000];
	uint8_t netmask_arr[4]; 
	int netmask_count = 0; 
	int i=0, j=0, found=0;
	FILE *dhcpcd_file, *new_file;
	char buf_in[BUFSIZ];
	char buf_out[BUFSIZ];
	u_long offset = 0; 
	rename(DHCPCD, DHCPCD_OLD);
	if((dhcpcd_file = fopen(DHCPCD_OLD,"r")) == NULL){
		perror("Failed to open /etc/dhcpcd.conf file");
		return -1;
	}
	if((new_file = fopen(DHCPCD,"w+")) == NULL){
		perror("Failed to open new File");
		return -1;
	}

	netmask_arr[0] = netmask & 0xff; 
	netmask_arr[1] = (netmask>>8) & 0xff; 
	netmask_arr[2] = (netmask>>16) & 0xff;
	netmask_arr[3] = (netmask>>24) & 0xff;

	for(j=0; j<4; j++){
		for(i=0; i<8; i++){
			if((netmask_arr[j] & 0x80)>0){
				netmask_count++;
			}
			else{
				break; 
			}
			netmask_arr[j] = netmask_arr[j] << 1; 
			printf("continue : %d\n",netmask_arr[j]);
		}	
	}
	

	sprintf(information, "ip : %s\tnetmask : %d\tgateway : %s", ipaddr, netmask_count, gateway);

	LOG_TRACE(information);

	/* search target */

	
	bzero(buf_in,BUFSIZ);
	while(fgets(buf_in,BUFSIZ,dhcpcd_file) != NULL){

		if((strstr(buf_in,"interface eth0")-buf_in) == 0){
			break; 
		}
		else{
			fprintf(new_file, buf_in); 
		}
	}
	fprintf(new_file,"interface eth0\nstatic ip_address=%s/%d\nstatic routers=%s\n",ipaddr, netmask_count, gateway);
	fclose(dhcpcd_file);
	fclose(new_file);
	remove(DHCPCD_OLD);
	execl("/sbin/reboot", "/sbin/reboot", NULL);
}






