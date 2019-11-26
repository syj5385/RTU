#!/bin/sh

CONFFILE=/etc/rtu/rtu.conf
PORTCONF=/etc/rtu/ports.conf
SCRIPT=rtu_script



logAndconf(){
	mkdir -p /etc/rtu
	mkdir -p /var/log/rtu
	cp etc/ports.conf $PORTCONF
	cp etc/rtu.conf $CONFFILE
	. $CONFFILE
	sudo mkdir -p /etc/rtu
	echo ''  > "$LOGFILE"
}

compile(){
	echo "compile RTU project"
	make all
	cp RTU /usr/sbin/rtu
	cp network /usr/sbin/network_proc
	cp sensor /usr/sbin/sensor_proc
	make clean
}


setService(){
	echo "set RTU process As default program"
	echo "This program will be started automatically when booting this system"
	cp etc/rtu_script /etc/init.d/rtu
	chmod +x /etc/init.d/rtu
	chown root /etc/init.d/rtu
	chgrp root /etc/init.d/rtu
	update-rc.d rtu defaults
	systemctl daemon-reload
	sudo service rtu restart
#/etc/init.d/rtu start
}

logAndconf
compile
#while read LINE
#	if [["$LINE" =~ "SERVERNAME"]];then
#		echo "find server name"
#	fi
#done < $CONFFIL
#setService
