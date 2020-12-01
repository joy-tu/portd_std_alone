#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "debug.h"
#include "../config.h"

#define ENABLE  1
#define DISABLE 0
struct runtime_config Grun_conf;

struct portd_conf config_item[] =
{
	/* Serial communication paramters */
	{"baud_rate",                 15,  0,         18       , CONFIG_NONE},
	{"data_bits",                 8,   5,         8        , CONFIG_NONE},
	{"stop_bits",                 1,   1,         2        , CONFIG_NONE},
	{"parity",                    0,   0,         4        , CONFIG_NONE},
	{"flow_control",              1,   0,         2        , CONFIG_NONE},
	{"interface",                 0,   0,         3        , CONFIG_NONE},
	/* OPmode settings */
	{"tcp_alive_check_time",      7,   0,         99       , CONFIG_NONE},
	{"inactivity_time",           0,   0,         65535    , CONFIG_NONE},
	{"max_connection",            1,   1,         8        , CONFIG_USR_NOT_SET},
	{"ignore_jammed_ip",    DISABLE,   DISABLE,   ENABLE   , CONFIG_USR_NOT_SET},
	{"allow_driver_control", ENABLE,   DISABLE,   ENABLE   , CONFIG_USR_NOT_SET},
	{"tcp_port",               4001,   1,         65535    , CONFIG_NONE},
	{"cmd_port",                966,   1,         65535    , CONFIG_USR_NOT_SET},
	{"packet_length",             0,   0,         1024     , CONFIG_USR_NOT_SET},
	{"delimiter_1_en",      DISABLE,   DISABLE,   ENABLE   , CONFIG_USR_NOT_SET},
	{"delimiter_2_en",      DISABLE,   DISABLE,   ENABLE   , CONFIG_USR_NOT_SET},
	{"delimiter_1",               0,   0,         255      , CONFIG_USR_NOT_SET},
	{"delimiter_2",               0,   0,         255      , CONFIG_USR_NOT_SET},
	{"delimiter_process",         0,   0,         3        , CONFIG_USR_NOT_SET},
	{"force_transmit",            0,   0,         65535    , CONFIG_USR_NOT_SET},
	{"",                         -1,  -1,         -1       , CONFIG_NONE},
};

int load_item(char *name, int *val)
{
	int i;
	for(i = 0; i < sizeof(config_item)/sizeof(config_item[0]); i++)
	{
		if(strcmp(name, config_item[i].item_name) == 0)
		{
			*val = config_item[i].val;
			return 0;
		}
	}
	return -1;
}
int load_runtime_conf(int port)
{
	Grun_conf.port = port;
	/* Serial communication paramters */
	LOAD(baud_rate);
	LOAD(data_bits);
	LOAD(stop_bits);
	LOAD(parity);
	LOAD(flow_control);
	LOAD(interface);
	/* OPmode settings */
	LOAD(tcp_alive_check_time);
	LOAD(inactivity_time);
	LOAD(max_connection);
	LOAD(ignore_jammed_ip);
	LOAD(allow_driver_control);
	LOAD(tcp_port);
	LOAD(cmd_port);
	LOAD(packet_length);
	LOAD(delimiter_1_en);
	LOAD(delimiter_2_en);
	LOAD(delimiter_1);
	LOAD(delimiter_2);
	LOAD(delimiter_process);
	LOAD(force_transmit);
	return 0;
}
