
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdlib.h>
#include "log_util.h"

static char log_file_prefix[64];
static char log_folder[1024];
static FILE *fp_log_file; 
static int log_level = LOG_LVL_TRACE;

int LOGsetLevel(int log_lvl){
	int temp = LOGgetLevel();
	log_level = log_lvl;
	return temp;
}

int LOGgetLevel(){
	char *log_env;
	static int is_env_check = 0; 
	if(is_env_check == 0){
		if((log_env = getenv("LOG_LEVEL")) == NULL){
			log_level = LOG_LVL_TRACE;
		}
		else{
			if(strcmp(log_env, "TRACE") == 0){
				log_level = LOG_LVL_TRACE;
			}
			else if(strcmp(log_env, "DEBUG") == 0){
				log_level = LOG_LVL_DEBUG;
			}
			else if(strcmp(log_env, "INFO") == 0){
				log_level = LOG_LVL_INFO;
			}
			else if(strcmp(log_env, "WARNINIG") == 0){
				log_level = LOG_LVL_WARNING;
			}
			else if(strcmp(log_env, "ERROR") == 0){
				log_level = LOG_LVL_ERROR;
			}
			else if(strcmp(log_env, "FATAL") == 0){
				log_level = LOG_LVL_FATAL;
			}
			else{
			 	log_level = LOG_LVL_TRACE;
			}
		}
		is_env_check = 1;
	}
	return log_level; 
}

int LOGsetInfo(const char *dir, const char *prefix){
	if(dir == NULL || dir[0] == 0x00){
		fprintf(stderr,"log folder set error");
		return -1; 
	}
	if(prefix == NULL || prefix[0] == 0x00){
		fprintf(stderr,"log file prefix set error");
		return -1;
	}
	if(strcmp(dir, log_folder) == 0 && strcmp(prefix, log_file_prefix) == 0){
		return 0;
	}
	strncpy(log_file_prefix, prefix, 64);
	strncpy(log_folder, dir, 1024);

	if(fp_log_file != NULL){
		fclose(fp_log_file);
		fp_log_file = NULL;
	}
	return 0;
}

static int LOGcreateFile(struct tm *tm1, const char *src_file){
	
	char filename[1024];
	char *ext;

	if(log_folder[0] == 0x00){
		strcpy(log_folder,".");
	}
	if(log_file_prefix[0] == 0x00){
		strncpy(log_file_prefix,src_file,sizeof(log_file_prefix));
		if((ext = strchr(log_file_prefix, '.')) != NULL){
			*ext = 0x00;
		}
	}

	snprintf(filename, 1024, "%s/%s-%04d%02d%02d.log",
			log_folder,log_file_prefix, 1900+tm1->tm_year, tm1->tm_mon+1, tm1->tm_mday);

	if(fp_log_file != NULL){
		fclose(fp_log_file);
		fp_log_file = NULL;
	}

	if((fp_log_file = fopen(filename, "a")) == NULL){
		return -1;
	}
	setvbuf(fp_log_file, NULL, _IOLBF, 0);
	return 0;
}

int LOGlogging(char log_type, const char *src_file, const char* func, int line_no, const char *fmt, ...){
	va_list ap; 
	int sz = 0; 
	struct timeval tv; 
	struct tm* tm1; 
	static int day = -1;
	static pid_t pid = 01; 
	char src_info[128];

	gettimeofday(&tv, NULL);
	tm1 = localtime(&tv.tv_sec);
	va_start(ap, fmt);

	if(pid == -1){
		pid = getpid();
	}

	if(day != tm1->tm_mday){
		if(LOGcreateFile(tm1,src_file) != 0){
			return -1;
		}
		day = tm1->tm_mday; 
	}

	sz += fprintf(fp_log_file, "(%c", log_type);
	sz += fprintf(fp_log_file, "%04d%02d%02d:%02d%02d%02d%06dld:%05d",
			1900+tm1->tm_year, tm1->tm_mon+1, tm1->tm_mday, 
			tm1->tm_hour, tm1->tm_min, tm1->tm_sec, tv.tv_usec, pid);

	snprintf(src_info, 128, "%s:%s(%d)", src_file, func, line_no);
	sz += fprintf(fp_log_file, ":%-50.50s: ", src_info);
	sz += fprintf(fp_log_file, fmt,ap);
	sz += fprintf(fp_log_file,"\n");
	va_end(ap);
	return sz; 

}

