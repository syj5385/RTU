

removeLog(){
	echo "Remove rtu log file"
	rm -fr "$LOGFILE"
	rm -fr "$PREVLOGFILE"
}


removeConf(){
	echo "Remove Rtu config file"
	rm -fr /etc/rtu/rtu.conf
	echo "Remove rtu port.conf file"
	rm -fr /etc/rtu/ports.conf
}

deleteService(){
	rm -fr /usr/sbin/rtu
	rm -fr /usr/sbin/network_proc
	rm -fr /usr/sbin/sensor_proc
	echo "remove rtu server"
	rm -fr /etc/init.d/rtu
	systemctl daemon-reload	
}

test -f /etc/rtu/rtu.conf && {
	# RTU Server is installed
	/etc/init.d/rtu stop
	echo "Stop rtu execute file"
	. /etc/rtu/rtu.conf
	deleteService
	removeLog
	removeConf
	echo "Remove RTU Server"
}||{

	echo "RTU service is not installed"
	echo "exit to uninstall"
	exit
}


