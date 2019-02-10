#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "define.h"
#include "etc/log.h"
#include "etc/config.h"

#define	PORT	1

int server_port = -1; 
const char* MAIN_TAG = "MAIN";
int initializeRTU();
void initializeChildProcess();

pid_t network_pid, sensor_pid;
int network_status, sensor_status;


/*
 *  Process Signal initialize And Handler Definition
 */
static sigset_t pset;
/* signal Handler */
static void sigHandler(int signo){
	if(signo == SIGINT){
		writeLog(WARNING,MAIN_TAG,"SIGINT signal Received");
		kill(network_pid, SIGTERM);
		kill(sensor_pid, SIGTERM);
		closeLogFile();
		exit(1);
	}
	if(signo == SIGTSTP){
		writeLog(WARNING,MAIN_TAG,"SIGTSTP signal Received");
		kill(0,SIGTERM);
		closeLogFile();
		exit(1);
	}
/*	if(signo == SIGCHLD){
		writeLog(WARNING,MAIN_TAG,"Such a child process was killed.");
		writeLog(ERROR,MAIN_TAG,"Exit Process. Bye!");
		kill(0,SIGTERM);
		exit(1);
	}*/
	if(signo == SIGTERM){
		writeLog(WARNING,MAIN_TAG,"Main Process get a SIGTERM signal");
		writeLog(WARNING,MAIN_TAG,"Exit process after kill child process. Bye!!");
		kill(0, SIGTERM);
		exit(1);
	}
}
static void initializeSignalHandler(){
	sigemptyset(&pset);
	sigaddset(&pset,SIGQUIT);
	sigprocmask(SIG_BLOCK, &pset, NULL);

	if(signal(SIGINT,sigHandler) == SIG_ERR){
		exit(1);
	}
//	if(signal(SIGCHLD,sigHandler) == SIG_ERR){
//		exit(1);
//	}
	if(signal(SIGTSTP,sigHandler) == SIG_ERR){
		exit(1);
	}
	if(signal(SIGTERM,sigHandler) == SIG_ERR){
		exit(1);
	}
}

void startChildProcess(){

	char network_proc_str[100];
	char sensor_proc_str[100];
	memset(sensor_proc_str, 0 ,100);
	memset(network_proc_str,0,100);

	sprintf(sensor_proc_str,"/usr/sbin/sensor_proc");
	if((sensor_pid = fork()) < 0)
		return;

	if(sensor_pid == 0){
		execl(sensor_proc_str, sensor_proc_str,NULL);
	}

	sprintf(network_proc_str,"/usr/sbin/network_proc");
	if((network_pid = fork()) < 0)
		return;

	if(network_pid == 0){
		// This process is network child process
		execl(network_proc_str, network_proc_str, NULL);
	}


	return ;
}


int main(int argc, char **argv) {

	int status;

	initializeSignalHandler();
	if(openLogFile() == 0){
		exit(1);
	}
	while(1){
		writeLog(NORMAL, MAIN_TAG, "Start RTU process");
		startChildProcess();

		waitpid(-1, &status, 0);
		status = (status >> 8)& 0xff; 
		if(status == 0)
			break; 

		sleep(1);
	}
//			kill(network_pid, SIGTERM);
//			sleep(1);
//			kill(sensor_pid, SIGTERM);
//			sleep(1);
//			writeLog(NORMAL,MAIN_TAG,"Restart Child proces");
//			sleep(1);
//		}
//		else
//			break; 
	
	

	return 0;
}



