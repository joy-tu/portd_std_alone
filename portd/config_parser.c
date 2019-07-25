#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "../debug.h"
#include "../config.h"

#define TMP_SIZE 2048
#define ENABLE  1
#define DISABLE 0

struct portd_conf config_item[] =
{
	/* Serial communication paramters */
	{"baud_rate",                 15,  0,         18       },
	{"data_bits",                 8,   5,         8        },
	{"stop_bits",                 1,   1,         2        },
	{"parity",                    0,   0,         4        },
	{"flow_control",              1,   0,         2        },
	{"interface",                 0,   0,         3        },
	/* OPmode settings */
	{"tcp_alive_check_time",      7,   0,         99       },
	{"inactivity_time",           0,   0,         65535    },
	{"max_connection",            1,   1,         8        },
	{"ignore_jammed_ip",    DISABLE,   DISABLE,   ENABLE   },
	{"allow_driver_control", ENABLE,   DISABLE,   ENABLE   },
	{"tcp_port",               4001,   1,         65535    },
	{"cmd_port",                966,   1,         65535    },
	{"packet_length",             0,   0,         1024     },
	{"delimiter_1_en",      DISABLE,   DISABLE,   ENABLE   },
	{"delimiter_2_en",      DISABLE,   DISABLE,   ENABLE   },
	{"delimiter_1",               0,   0,         255      },
	{"delimiter_2",               0,   0,         255      },
	{"delimiter_process",         0,   0,         3        },
	{"force_transmit",            0,   0,         65535    },
	{"",                         -1,  -1,         -1       },
};

struct runtime_config Grun_conf;
char Gtty_name[128] = {0};
int get_ttyname(int port)
{
	int tty;

	snprintf(Gtty_name, sizeof(Gtty_name), "/dev/ttyM%d", port -1);

	CONFIG_DEBUG("tty_name: %s\n", Gtty_name);
	if((tty = open(Gtty_name, O_RDONLY)) < 0)
	{
		fprintf(stderr, "TTY error: cannot open the TTY device %s.\n", Gtty_name);
		return -1;
	}

	close(tty);

	return 0;
}

static int get_config(char *key, char* value, char *err_msg, size_t msg_size)
{
	int i;
	char sval[20] = {0};
	long li;

	for (i = 0; strlen(config_item[i].item_name) > 0; i++)
	{
		if (strcmp(key, config_item[i].item_name) == 0)
		{
			strncpy(sval, value, sizeof(sval) - 1);
			break;
		}
	}

	/* The key item is no use, ignore it. */
	if (sval[0] == 0)
	{
		return 0;
	}
	
	li = strtol(sval, NULL, 0);

	/* Check value range */
	if (errno == ERANGE ||
		(li < config_item[i].min || li > config_item[i].max))
	{
		snprintf(err_msg, msg_size, "Value out of range, error at \"%s\"", config_item[i].item_name);
		return -1;
	}

	config_item[i].val = (int)li;

	return 0;
}

int config_parser(char *path)
{
	int line = 0;
	int ret = -1;
	char buff[TMP_SIZE];
	const char *const delim = " \t\r\n";
	char *saveptr = NULL;
	char *key = NULL, *value = NULL;
	char err_msg[128];
	FILE *config;

	CONFIG_DEBUG("********* Start parsing config file *********\n");

	if ((config = fopen(path, "r+")) == NULL)
	{
		snprintf(err_msg, sizeof(err_msg), "Config file: %s not found", path);
		goto EXIT;
	}

	while (fgets(buff, sizeof(buff), config))
	{
		++line;

		if (buff[0] == '#')	// ignore comment
			continue;

		if ((key = strtok_r(buff, delim, &saveptr)) != NULL)
			value = strtok_r(NULL, delim, &saveptr);

		if (!key || !value)
		{
			snprintf(err_msg, sizeof(err_msg), "error at line %d", line);
			goto EXIT;
		}

		if (get_config(key, value, err_msg, sizeof(err_msg)) < 0)
		{
			goto EXIT;
		}
	}

    ret = 0;

EXIT:
	if (config)
		fclose(config);

	if (ret < 0)
		fprintf(stderr, "[Fail to parse config file] %s.\n", err_msg);

#if __CONFIG_DEBUG
	int i;
	for(i = 0; i < sizeof(config_item)/sizeof(config_item[0]); i++)
	{
		if(strlen(config_item[i].item_name) == 0)
			break;

		printf("name: %s val: %d\n", config_item[i].item_name, config_item[i].val);
	}
#endif

	CONFIG_DEBUG("*********************************************\n");

	return ret;
}
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
int load_runtime_conf(void)
{
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