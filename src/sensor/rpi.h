
#ifndef _RPI_
#define _RPI_

# include <stdio.h>
# include <unistd.h>
# include <fcntl.h>
# include <stdlib.h>
# include <errno.h>
# include <stdint.h>
#include <string.h>

#include <termios.h>
#include <wiringPi.h>

int serial_fd;
#define RPI_IO_G1 1



#define RLED 24
#define GLED 25


void setupSerial();
void setupLED();
void serialFlush(int fd);
int readData(uint8_t* output);
void sendData(uint8_t* frame,int size);
int requestAndResponseSerial(uint8_t* input,uint8_t* output,int inputSize);
void haltsignal();

#endif
