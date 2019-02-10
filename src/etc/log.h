
/*
* 1. Intiizlie Log File
* 2. Read Config File
*
*
*
*/

#ifndef _LOG_H_
#define _LOG_H_

#include <stdlib.h>
#include "../define.h"

/* Definition of LOG Type */
#define NORMAL	'D'
#define ERROR	'E'
#define WARNING 'W'

/* shared memory key */
#define		SHM_KEY		0x12345


int openLogFile();
void closeLogFile();
void writeFinishLog();
void writeLog(const char type, const char* TAG, const char* content);


#endif /*_LOG_H_*/

