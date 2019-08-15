[Configurations]
You can use the template configurarion file, tcp_srv.conf.

baud_rate:
	The index of serial baud rate.

data_bits:
	Serial port data bits.

stop_bits:
	Serial port stop bits.

parity:
	Serial port parity.

flow_control:
	Serial port flow control.

interface:
	Serial port interface.

tcp_alive_check_time:
	TCP alive check time for the software.

inactivity_time:
	The time limit for keeping the connection open if no data flows to or from the serial device.

tcp_port:
	The TCP port number that the serial port will use to listen to
	connections, and that other devices must use to contact the serial port.

[How to ues this program]
1. Please use sudo or run as root to execute portd.

2. Usage: sudo ./portd [options]
   Options:
		-p "port number": Start TCP server mode for the specific port.
		-f "config file": Input config file.
		-v:               Show software version.
		-h:               How to use this software.

	ex. sudo ./portd -p 1 -f tcp_srv.conf

3. You can log messages to file.
	ex. sudo ./portd -p 1 -f tcp_srv.conf portd1.log 2>&1