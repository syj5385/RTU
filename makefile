
# makefile for RTU

CFLAGS = -g

SRC=./src/
NET=$(SRC)network/
SEN=$(SRC)sensor/
ETC=$(SRC)etc/
PROTOCOL=$(SRC)protocol/


MAIN_OBJ=$(SRC)rtu.o $(ETC)config.o $(ETC)log.o
MAIN_TARGET=RTU

NET_OBJ=$(NET)network.o $(NET)tcp.o $(NET)netinfo.o $(PROTOCOL)rtu_serial.o $(ETC)ipc.o $(ETC)config.o $(ETC)log.o $(ETC)linked_list.o
NET_TARGET=network

SEN_OBJ=$(SEN)sensor.o $(ETC)ipc.o $(ETC)config.o $(ETC)log.o
SEN_TARGET=sensor

LIB=pthread
RPI=wiringPi
all : clean build

clean : 
	rm -fr $(MAIN_TARGET) $(MAIN_OBJ)
	rm -fr $(SEN_TARGET) $(SEN_OBJ)
	rm -fr $(NET_TARGET) $(NET_OBJ)  
	rm -fr ./etc/nhsocket ./etc/gateway.txt

build : $(MAIN_OBJ) $(NET_OBJ) $(SEN_OBJ)
	$(CC) $(CFLAGS) -o  $(MAIN_TARGET) $(MAIN_OBJ) -l$(LIB)
	$(CC) $(CFLAGS) -o $(NET_TARGET) $(NET_OBJ) -l$(LIB)
	$(CC) $(CFLAGS) -o $(SEN_TARGET) $(SEN_OBJ) -l$(LIB) -l$(RPI)

removelog : 
	rm -fr ./etc/rtu.log


