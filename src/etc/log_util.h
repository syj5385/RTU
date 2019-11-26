#ifndef	_LOG_UTIL_H_
#define	_LOG_UTIL_H_

#include "../define.h"


#define	LOG_IS_TRACE	(LOG_LEVEL >= LOG_LVL_TRACE)
#define LOG_IS_DEBUG	(LOG_LEVEL >= LOG_LVL_DEBUG)
#define LOG_IS_INFO	(LOG_LEVEL >= LOG_LVL_INFO)
#define LOG_IS_WARNING	(LOG_LEVEL >= LOG_LVL_WARNING)
#define LOG_IS_ERROR	(LOG_LEVEL >= LOG_LVL_ERROR)
#define	LOG_IS_FATAL	(LOG_LEVEL >= LOG_LVL_FATAL)

int LOGsetInfo(const char* dir, const char *prefix);
int LOGlogging(char log_type, const char *src_file, const char *func, int line_no, const char* fmt, ...);
int LOGsetLevel(int log_lvl);
int LOGgetLevel(void); 

#define LOG_CALL(func)\
	func;\

#define	LOG_TRACE(...)\
	do{\
		setenv("LOG_LEVEL","TRACE",1);\
		if(LOG_IS_TRACE){\
			LOGlogging('T',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		}\
	}while(0)

#define LOG_DEBUG(...) \
	do{\
		if(LOG_IS_DEBUG){ \
			LOGlogging('D',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		} \
	}while(0)


#define LOG_INFO(...) \
	do{\
		if(LOG_IS_INFO){ \
			LOGlogging('I',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		} \
	}while(0)


#define LOG_WARNING(...) \
	do{\
		if(LOG_IS_WARNING){ \
			LOGlogging('W',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		} \
	}while(0)


#define LOG_ERROR(...) \
	do{\
		if(LOG_IS_ERROR){ \
			LOGlogging('E',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		} \
	}while(0)


#define LOG_FATAL(...) \
	do{\
		if(LOG_IS_FATAL){ \
			LOGlogging('F',__FILE__,__func__, __LINE__, __VA_ARGS__);\
		} \
	}while(0)

#endif
