/*
 * config.c
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "config.h"
#include "log_util.h"

#define PORT_OLD "/etc/rtu/ports_old.conf"
#define PORT	 "/etc/rtu/ports.conf"
#define RTU_CONF_OLD "/etc/rtu/rtu_old.conf"
#define	RTU_CONF	"/etc/rtu/rtu.conf"

int openConfig(FILE* file){

	if((file = fopen(CONF_LOCATION,"r")) == NULL){
		LOG_ERROR("Failed to open Config file");
		return -1;
	}
	return 0;
}


char* getConfigContent(char* title){
	FILE* file; 
	char line[1024]; 
	char* pStr; 
	char* dest = (char*)malloc(sizeof(1024));
	char* strTemp; 
	
	if((file = fopen(CONF_LOCATION,"r")) == NULL){
		LOG_ERROR("Failed to open Config File");
		return NULL;
	}
	while((pStr = fgets(line, 1024, file)) != NULL){
		if(line[0] == '#')
			continue;
		else{
			char *ptr = strtok(line, "=");
			while(ptr != NULL){
				if(!strcmp(ptr, title) == 0)
					break; 
				else{
					ptr = strtok(NULL,"=");
					strcpy(dest, ptr);
					fclose(file);
					char c; 
					int i;
					for(i=0; i<strlen(dest); i++){
						if(dest[i] == '\n')
							dest[i] = '\0';

					}
					
					return dest;
				}
			}	

		}
	} 
	fclose(file);
	return NULL;
}


uint16_t getSensorID(int index){
	FILE* file; 
	char line[1024]; 
	char* pStr; 
	char* dest = (char*)malloc(sizeof(1024));
	char* strTemp; 
	char buf[1024]; 
	sprintf(buf,"SENSOR%d_ID", index);
	
	if((file = fopen(CONF_LOCATION,"r")) == NULL){
		LOG_ERROR("Failed to open Config File");
		return -1;
	}
	while((pStr = fgets(line, 1024, file)) != NULL){
		if(line[0] == '#')
			continue;
		else{
			char *ptr = strtok(line, "=");
			while(ptr != NULL){
				if(!strcmp(ptr, buf) == 0)
					break; 
				else{
					ptr = strtok(NULL,"=");
					strcpy(dest, ptr);
					fclose(file);
					char c; 
					int i;
					for(i=0; i<strlen(dest); i++){
						if(dest[i] == '\n')
							dest[i] = '\0';

					}

					return atoi(dest);
				}
			}	

		}
	} 
	fclose(file);
	return -1;
}

int getConfigPort(){
	FILE* file; 
	char line[1024]; 
	char* pStr; 
	char* dest = (char*)malloc(sizeof(1024));
	char* strTemp; 
	
	if((file = fopen(PORT_CONF_LOCATION,"r")) == NULL){
		LOG_ERROR("Failed to open Config File");
		return -1;
	}
	while((pStr = fgets(line, 1024, file)) != NULL){
		if(line[0] == '#')
			continue;
		else{
			char *ptr = strtok(line, "=");
			while(ptr != NULL){
				if(!strcmp(ptr, "Listen") == 0)
					break; 
				else{
					ptr = strtok(NULL,"=");
					strcpy(dest, ptr);
					fclose(file);
					char c; 
					int i;
					for(i=0; i<strlen(dest); i++){
						if(dest[i] == '\n')
							dest[i] = '\0';

					}

					return atoi(dest);
				}
			}	

		}
	} 
	fclose(file);
	return -1;
}

int setConfigPort(int newport){
	FILE *old, *new;
	char line[1024];

	rename(PORT,PORT_OLD);
	
	if((old = fopen(PORT_OLD,"r")) == NULL){
		LOG_ERROR("Failed to open old Config File");
		return -1;
	}
	if((new = fopen(PORT,"w")) == NULL){
		LOG_ERROR("Failed to open new Config File");
		return -1;
	}

	while(fgets(line,1024, old) != NULL){
		if((strstr(line,"Listen=")-line) == 0){
			fprintf(new,"Listen=%d\n",newport);

		}
		else{
			fprintf(new,line);
		}

	}
	fclose(old);
	fclose(new);
	remove(PORT_OLD);
}


int setRTUid(int newid){
	FILE *old, *new;
	char line[1024];

	rename(RTU_CONF,RTU_CONF_OLD);
	
	if((old = fopen(RTU_CONF_OLD,"r")) == NULL){
		LOG_ERROR("Failed to open old Config File");
		return -1;
	}
	if((new = fopen(RTU_CONF,"w")) == NULL){
		LOG_ERROR("Failed to open old Config File");
		return -1;
	}

	while(fgets(line,1024, old) != NULL){
		if((strstr(line,"RTU_ID=")-line) == 0){
			fprintf(new,"RTU_ID=%d\n",newid);
		}
		else{
			fprintf(new,line);
		}

	}
	fclose(old);
	fclose(new);
	remove(PORT_OLD);
}

uint16_t getRTUID(){
	FILE* file; 
	char line[1024]; 
	char* pStr; 
	char* dest = (char*)malloc(sizeof(1024));
	char* strTemp; 
	char buf[1024]; 
	
	if((file = fopen(CONF_LOCATION,"r")) == NULL){
		LOG_ERROR("Failed to open config file");
		return -1;
	}
	while((pStr = fgets(line, 1024, file)) != NULL){
		if(line[0] == '#')
			continue;
		else{
			char *ptr = strtok(line, "=");
			while(ptr != NULL){
				if(!strcmp(ptr, "RTU_ID") == 0)
					break; 
				else{
					ptr = strtok(NULL,"=");
					strcpy(dest, ptr);
					fclose(file);
					char c; 
					int i;
					for(i=0; i<strlen(dest); i++){
						if(dest[i] == '\n')
							dest[i] = '\0';

					}

					return atoi(dest);
				}
			}	

		}
	} 
	fclose(file);
	return -1;
}
