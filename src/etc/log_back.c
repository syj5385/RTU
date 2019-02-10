
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"
#include "../define.h"


static FILE* log_fp; 

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
		sprintf(log_buf,"echo %d.%d.%d %d:%d:%d\t%c  %s\t\t\t%s\n >> %s",
			tm_ptr -> tm_mday,
			tm_ptr -> tm_mon+1,
			tm_ptr -> tm_year+1900,
			tm_ptr -> tm_hour+9,
			tm_ptr -> tm_min,
			tm_ptr -> tm_sec,
			type, tag,
			content,LOG_LOCATION);
	}
	if(strlen(tag) >= 6){
		sprintf(log_buf,"echo %d.%d.%d %d:%d:%d\t%c  %s\t\t%s\n>>%s",
			tm_ptr -> tm_mday,
			tm_ptr -> tm_mon+1,
			tm_ptr -> tm_year+1900,
			tm_ptr -> tm_hour+9,
			tm_ptr -> tm_min,
			tm_ptr -> tm_sec,
			type, tag,
			content,LOG_LOCATION);
	}

	system(log_buf);
#ifdef	STAND_PRINT
	printf("%s", log_buf);
#endif
	// You have to close file pointer after thie function is finished
}

int openLogFile(){

	char buf[100];
	bzero(buf, sizeof(buf));
	if((log_fp = fopen(LOG_LOCATION, "a+")) == NULL){
		printf("Failed to create Log File\n");
		return 0;
	}
	else return 1;
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
	//if(openLogFile(log_fp) == 0)
	//	return;
	writeLogContent(log_fp,type,tag,content);

	//fclose(log_fp);
}
