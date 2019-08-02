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

max_connection:
	This configuration specifies the maximum number of connections that will be accepted by the serial port.

ignore_jammed_ip:
	How an unresponsive IP address is handled when there are simultaneous connections to the serial port. 

allow_driver_control:
	How the port will proceed if driver control commands are received from
	multiple hosts that are connected to the port. 

tcp_port:
	The TCP port number that the serial port will use to listen to
	connections, and that other devices must use to contact the serial port.

cmd_port:
	The TCP port number for listening to SSDK commands from the host.

packet_length:
	The maximum amount of data that is allowed to accumulate in the serial
	port buffer before sending. 

delimiter_1_en and delimiter_2_en:
	Enable Delimiter 1 to control data packing with a single character; enable both Delimiter 1 and 2
	to control data packing with two characters received in sequence.

delimiter_1 and delimiter_2:
	Define special delimiter character(s) for data packing.

delimiter_process:
	How data is packed when delimiter characters are received.
	(Note: This configuration has no effect if Delimiter 1 is not enabled.)

	Do nothing:
		Data accumulated in the serial port’s buffer will be packed, including delimiters.
	Delimiter + 1:
		One additional character must be received before the data in the serial port’s buffer is packed.
	Delimiter + 2:
		Two additional characters must be received before the data in the serial port’s buffer is packed.
	Strip Delimiter:
		Data accumulated in the serial port’s buffer will be packed, but the delimiter character(s) will be stripped from the data

force_transmit
	Controls data packing by the amount of time that elapses between bits of data.
	(Note: When using this field, make sure that Inactivity time is disabled or set to a larger value.
	 Otherwise the connection may be closed before the data in the buffer can be transmitted.)

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