#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include "log.h"
#include "../define.h"


static FILE* log_fp; 
int shmid;
/* check if log file exists
*  if not exists, create config file
*/

void writeLogContent(FILE* file, const char type, const char* tag, const char* content){
	struct tm *tm_ptr; 
	time_t m_time;
	char log_buf[1000];

	time(&m_time);
	tm_ptr = gmtime(&m_time);
	bzero(log_buf,sizeof(log_buf));
	if(strlen(tag) < 6){
		sprintf(log_buf,"%d.%d.%d %d:%d:%d\t%c  %s\t\t\t%s",
			tm_ptr -> tm_mday,
			tm_ptr -> tm_mon+1,
			tm_ptr -> tm_year+1900,
			tm_ptr -> tm_hour+9,
			tm_ptr -> tm_min,
			tm_ptr -> tm_sec,
			type, tag,
			content);
	}
	if(strlen(tag) >= 6){
		sprintf(log_buf,"%d.%d.%d %d:%d:%d\t%c  %s\t\t%s",
			tm_ptr -> tm_mday,
			tm_ptr -> tm_mon+1,
			tm_ptr -> tm_year+1900,
			tm_ptr -> tm_hour+9,
			tm_ptr -> tm_min,
			tm_ptr -> tm_sec,
			type, tag,
			content);
	}

	if(log_fp){
		fprintf(log_fp,"%s\n",log_buf);
		fflush(log_fp);
	}
#ifdef	STAND_PRINT
	printf("%s", log_buf);
#endif



	
}

int openLogFile(){
	 
	char buf[100];
	bzero(buf, sizeof(buf));
	if((log_fp = fopen(LOG_LOCATION, "a+")) == NULL){
		return 0;
	}

	shmid = shmget((key_t)SHM_KEY,sizeof(int), 0666 | IPC_CREAT);
	if(shmid == -1){
		return 0;
	}
	void* shmmem = shmat(shmid, (void*)0, 0);
	if(shmmem == (void*)-1){
		return 0;
	}
	*(int*)shmmem = 0;
	shmdt(shmmem);

	return 1;
}

void closeLogFile(){
	fclose(log_fp);
}

void writeFinishLog(){
	//if(openLogFile(log_fp) == 0)
	//	return;

	char buf[100];
	bzero(buf,sizeof(buf));
	sprintf(buf,"%s Process is finished\n",SERVERNAME);
	writeLogContent(log_fp, NORMAL,"LOG",buf);

	//fclose(log_fp);
}


void writeLog(const char type,const char* tag ,const char* content){
	shmid = shmget((key_t)SHM_KEY,sizeof(int),0);
	if(shmid == -1){
		perror("Failed : shmget");
		return ;
	}
	void* p_shmval = shmat(shmid, (void*)0, 0666 | IPC_CREAT);
	if(p_shmval == (void*)-1){
		puts("Failed shared_memory");
		return; 
	}

	while((void*)(p_shmval = shmat(shmid, (void*)0, 0)) != (void*)-1){
		if((*(int*)p_shmval) == 0){
			break; 
			*(int*)p_shmval = 1;
			puts("enter");
		}
		else
			usleep(1000);

	}
	
	writeLogContent(log_fp,type,tag,content);

	*(int*)p_shmval = 0 ;
	shmdt(p_shmval);
	
	//fclose(log_fp);
}

