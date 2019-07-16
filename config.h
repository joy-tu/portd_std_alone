#ifndef _BUILD_IN_CONF_
#define _BUILD_IN_CONF_

struct portd_conf {
	char item_name[256];
	int  val;
};

struct runtime_config {
	int tcp_alive_check_time;
	int inactivity_time;
	int max_connection;
	int ignore_jammed_ip;
	int allow_driver_control;
	int tcp_port;
	int cmd_port;
	int packet_length;
	int delimiter_1_en;
	int delimiter_2_en;
	int delimiter_1;
	int delimiter_2;
	int delimiter_process;
	int force_transmit;
} Grun_conf;

extern struct runtime_config Grun_conf;
extern char Gtty_name[128];

#define CFG(mbr) (Grun_conf.mbr)
#define LOAD(name) \
	do{\
		if(load_item(#name, &CFG(name)) < 0)\
		{\
			fprintf(stderr,"Invalid configuration %s !\n",#name);\
		}\
		else {\
			fprintf(stderr,"Load %s:%d !\n",#name, CFG(name));\
		}\
	}while(0)

#define MAX_PORT 4
#if 0
#define OPMODE    256 //realcom
#else
#define OPMODE    512 //tcp server
#endif
#define FIFO 1 //FIFO
#define INTERFACE 0 //RS-232
#define SERDATALOG 0
#define RTSDTRACT 3
#define PORTBUFF 0
#define BAUD_IDX 16 //115200
#define SERMODE   3 // 8N1
#define FLOW_CTRL 1 //RTS/CTS
#if 1
#define TCP_ALIVE_MIN CFG(tcp_alive_check_time)
#define INACTTIME     CFG(inactivity_time)
#define	MAX_CONN      CFG(max_connection)
#define SKIPJAMIP     CFG(ignore_jammed_ip)
#define ALLOWDRV      CFG(allow_driver_control)
#define	TCPSERV_BASE_PORT CFG(tcp_port)
#define	TCPSERV_CMDB_PORT CFG(cmd_port)
#define PACKLEN       CFG(packet_length)
#define	DATAPAK_EN    (CFG(delimiter_1_en) | CFG(delimiter_2_en) << 1)
#define	DELIM1        CFG(delimiter_1)
#define	DELIM2        CFG(delimiter_2)
#define	DELIMPROC     CFG(delimiter_process)
#define FORCE_TX      CFG(force_transmit)
#else
#define TCP_ALIVE_MIN 7
#define MAX_CONN  1
#define SKIPJAMIP 0
#define ALLOWDRV 0
#define INACTTIME 0
#define TCPSERV_BASE_PORT 4000
#define PACKLEN 0
#define DATAPAK_EN 0 
#define DELIM1 0
#define DELIM2 0
#define FORCE_TX 0
#define DELIMPROC 1 //Do nothing
#endif
#endif