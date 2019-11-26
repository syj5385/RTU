#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "define.h"
#include "etc/log_util.h"
#include "etc/config.h"

#define	PORT	1

int server_port = -1; 
const char* MAIN_TAG = "MAIN";
int initializeRTU();
void initializeChildProcess();
void startChildProcess();

pid_t network_pid, sensor_pid;
int network_status, sensor_status;


/*
 *  Process Signal initialize And Handler Definition
 */
static sigset_t pset;
/* signal Handler */
static void sigHandler(int signo){

	int status; 

	if(signo == SIGINT){
		LOG_WARNING("SIGINT signal Receive");
		kill(network_pid, SIGTERM);
		kill(sensor_pid, SIGTERM);
		exit(0);
	}
	if(signo == SIGTSTP){
		LOG_WARNING("SIGTSTP signal Received");
		kill(0,SIGTERM);
		exit(0);
	}
	if(signo == SIGTERM){
		LOG_WARNING("Main process get a SIGTERM signal");
		kill(0, SIGTERM);
		pid_t pid1 = waitpid(-1, &status, 0);
		pid_t pid2 = waitpid(-1, &status, 0);
		exit(0);
	}
}

static void initializeSignalHandler(){
	sigemptyset(&pset);
	sigaddset(&pset,SIGQUIT);
	sigprocmask(SIG_BLOCK, &pset, NULL);

	if(signal(SIGINT,sigHandler) == SIG_ERR){
		exit(1);
	}
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

	LOGsetInfo(LOG_PATH,basename(argv[0]));

	LOG_TRACE("Start RTU");
	initializeSignalHandler();


	while(1){
		startChildProcess();
		pid_t pid = waitpid(-1, &status, 0);
		status = (status >> 8)& 0xff; 
		
		if(pid == sensor_pid){
			printf("I will kill network process\n");
			kill(network_pid, SIGTERM);
			pid_t newChild = waitpid(network_pid, &status, 0);
		}
		else if(pid == network_pid){
			printf("I will kill sensor process\n");
			kill(sensor_pid, SIGTERM);
			pid_t newChild = waitpid(network_pid, &status, 0);
		}

		if(status == 0)
			break;


		usleep(10000);
		LOG_WARNING("Start new child process");
	}
	
	

	exit(0);
}



