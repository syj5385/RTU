

#ifndef	_SENSOR_PROTOCOL_H_
#define	_SENSOR_PROTOCOL_H_
#include <stdint.h>
#include "rtu_serial.h"
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t data_mutex; 

void analysisMessage(uint8_t* buf, Frame *dest);
int executeSensorProtocol(Frame *frame);
int executeMessage(Frame *dest); 
int writeFrameToNetwork(int sockfd, Frame* frame);

void initData();
bool checkCorrect(uint8_t* buf, int size);
int sendhalt(Frame* dest);
#endif /*_SENSOR_PROTOCOL_H_ */ 
