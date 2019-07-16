#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../config.h"

#define TMP_SIZE 2048
#define ENABLE  1
#define DISABLE 0
char *config_buff;
struct portd_conf config_item[] =
{
	{"tcp_alive_check_time",      7},
	{"inactivity_time",           0},
	{"max_connection",            1},
	{"ignore_jammed_ip",    DISABLE},
	{"allow_driver_control", ENABLE},
	{"tcp_port",               4001},
	{"cmd_port",                966},
	{"packet_length",             0},
	{"delimiter_1_en",      DISABLE},
	{"delimiter_2_en",      DISABLE},
	{"delimiter_1",               0},
	{"delimiter_2",               0},
	{"delimiter_process",         0},
	{"force_transmit",            0},
	{"", -1},
};

struct runtime_config Grun_conf;
char Gtty_name[128];
int get_ttyname(void)
{
	char *ptr = strstr(config_buff, "tty_name");
	char buf[128];
	int tty;

	if(ptr == NULL)
	{
		fprintf(stderr,"Doesn't found TTY config\n");
		exit(1);
	}

	sscanf(config_buff, "%s %s", buf, Gtty_name);

	if((tty = open(Gtty_name, O_RDONLY)) < 0)
	{
		fprintf(stderr,"TTY error: cannot open the TTY device %s.\n", Gtty_name);
		exit(1);
	}
	close(tty);
	return 0;
}

int get_config(void)
{
	int i;

	for(i = 0; i < sizeof(config_item)/sizeof(config_item[0]); i++)
	{
		char buf[sizeof(config_item[0].item_name)];
		int val;
		char *ptr = strstr(config_buff, config_item[i].item_name);

		if(ptr == NULL)
			continue;
		//printf("+name: %s val: %d\n",buf, val);
		sscanf(ptr, "%s %d", buf, &val);
		//printf("-name: %s val: %d\n",buf, val);
		config_item[i].val = val;
	}
	for(i = 0; i < sizeof(config_item)/sizeof(config_item[0]); i++)
	{
		if(strlen(config_item[i].item_name) == 0)
			break;
		printf("=name: %s val: %d\n",
			config_item[i].item_name, 
			config_item[i].val);
	}
	return 1;
}

int config_parser(char *path)
{
	int ret = 0;
	char *tmp;
	FILE *config = fopen(path,"r+");

	if(config == NULL)
		return 0;

	config_buff = (char *) malloc(TMP_SIZE);

	if(config_buff == NULL)
		return 0;

	tmp = config_buff;
	while((tmp = fgets(tmp, TMP_SIZE, config)))
	{
		tmp += strlen(tmp);
	}
	//printf("%s", config_buff);
	get_ttyname();
	get_config();
    ret = 1;
	
	free(config_buff);
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