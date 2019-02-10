/*
 * config.c
 *
 *  Created on: Jan 21, 2019
 *      Author: jjun
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "log.h"

int openConfig(FILE* file){

	if((file = fopen(CONF_LOCATION,"r")) == NULL){
		writeLog(ERROR,"CONFIG","FAILED TO OPEN CONFIG");
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
		writeLog(ERROR,"CONFIG","FAILED TO OPEN CONFIG");
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



int getConfigPort(){
	FILE* file; 
	char line[1024]; 
	char* pStr; 
	char* dest = (char*)malloc(sizeof(1024));
	char* strTemp; 
	
	if((file = fopen(PORT_CONF_LOCATION,"r")) == NULL){
		writeLog(ERROR,"CONFIG","FAILED TO OPEN CONFIG");
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

