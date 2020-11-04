#ifndef _BUILD_IN_CONF_
#define _BUILD_IN_CONF_

#include <stdint.h>

#define CONFIG_NONE            0x0
#define CONFIG_USR_NOT_SET     0x01	// user cannot set this config

struct portd_conf {
	char item_name[256];
	int  val;
	int  min;
	int  max;
	uint32_t flags;
};

struct runtime_config {
	int port;
	/* Serial communication paramters */
	int baud_rate;
	int data_bits;
	int stop_bits;
	int parity;
	int flow_control;
	int interface;
	/* OPmode settings */
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
			SHOW_LOG(stderr, Grun_conf.port, MSG_ERR, "Invalid configuration %s.\n", #name); \
		}\
	}while(0)

/************** Serial communication parameter and opmode configurations **************/
#define MAX_PORT 4

#define OPMODE_TCPSRV    512
#define OPMODE_REALCOM   256

#define BAUD_IDX                 CFG(baud_rate)
#define DATABITS                 CFG(data_bits)
#define STOPBITS                 CFG(stop_bits)
#define PARITY                   CFG(parity)
#define FLOW_CTRL                CFG(flow_control)
#define INTERFACE                CFG(interface)
#define FIFO                     1
#define RTSDTRACT                3
#define SERDATALOG               0
#define PORTBUFF                 0

#define OPMODE                   OPMODE_REALCOM
#define TCP_ALIVE_MIN            CFG(tcp_alive_check_time)
#define INACTTIME                CFG(inactivity_time)
#define MAX_CONN                 CFG(max_connection)
#define SKIPJAMIP                CFG(ignore_jammed_ip)
#define ALLOWDRV                 CFG(allow_driver_control)
#define TCPSERV_BASE_PORT        CFG(tcp_port)
#define TCPSERV_CMDB_PORT        CFG(cmd_port)
#define PACKLEN                  CFG(packet_length)
#define DATAPAK_EN               (CFG(delimiter_1_en) | CFG(delimiter_2_en) << 1)
#define DELIM1                   CFG(delimiter_1)
#define DELIM2                   CFG(delimiter_2)
#define DELIMPROC                CFG(delimiter_process)
#define FORCE_TX                 CFG(force_transmit)

#endif