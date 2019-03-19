/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    scm.c

    Thread for scm mode.

    2011-09-19  Joy Tu

    First version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <pthread.h>
#include <header.h>
#include <debug.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <portd.h>
#include <sio.h>
#include <config.h>
#include <sysapi.h>
#include <rfc2217.h>
#include <pair.h>
#include <net_config.h>
#include <sysmaps.h>
#include <portd.h>
#include "sercmd.h"
#include "../console/consystem.h"
// perry add region start
#include <ctype.h>
#include <eventd.h>
#include <iwlib.h>
#include <sysWireless.h>

// perry add region end

int Gscm_active = 0;  /* 1 : SCM Running, 0 : SCM Stopping */
int Gscm_online = 0;  /* 1 : Port open, 0 : Port Un-open */
extern int portd_terminate;
//////////////////////////////
#define MIINEPORT_E2 1
#define SIO_MAX_PORTS 1
#define IP_CONFIG_STATIC	0
#define IP_CONFIG_DHCP		1
#define IP_CONFIG_BOOTP	2
#define SCM_HW_TRIGGER 1
#define SCM_SW_TRIGGER 2
#define SCM_BREAK_TRIGGER 3

//#define SUPPORT_NE_SCM  0
#define PARAMTER_NOT_CHECK			0xFF
#define PARAMTER_EXCEPTIONAL_CHECK	0xFE
#define PARAMTER_NOT_SUPPORT		0xFD
#define PARAMTER_RUNNING_ONLY		0xFC

#define DATA_PORT		0
#define CMD_PORT		1
#define CMD_CHANNEL		0xFF
#define	CMD_IO			1

#ifdef SUPPORT_NE_SCM
#define HEAD_NE_COMMAND	'>'
#define HEAD_NE_REPLY	'<'
#define OP_NE_CONFGET	'R'
#define OP_NE_CONFSET	'W'
#define ST_NE_SUCCESS	'Y'
#endif

#define HEAD_INVALID	' '
#define HEAD_COMMAND	'?'
#define HEAD_REPLY		'!'
#define HEAD_COMMAND_M	'$'
#define HEAD_REPLY_M	':'
#define OP_INVALID		'-'
#define OP_GETCONF		'G'
#define OP_SETCONF		'S'
#define OP_RUNNINGCONF	'R'
#define OP_VIEWSTAT		'V'
#define OP_CONTROL		'C'
#define TAIL_CR	'\r'
#define TAIL_LF	'\n'
#define ST_SUCCESS				'0'
#define ST_UNRECOGNIZED_FORMAT	'1'
#define ST_INVALID_OPERATION	'2'
#define ST_INVALID_COMMAND		'3'
#define ST_INCORRECT_PARAMETER	'4'
#define ST_INCOMPLETE_PARAMETER	'5'
#define ST_ENTER_SCM			'E'
#define DBG(x) //printf x

/* perry add region start */
void str_ascii2hex(char *dstr, char *sstr, int slen);

#define SECRUITY_GET_PARAM_NUM_1		1
#define SECRUITY_GET_PARAM_NUM_2		2
#define NETWORK_TYPE_MAX_NUM		1
#define USERNAME_MAX_NUM			2
#define NETWORK_USERNAME			1

#define WEP_MIN_INDEX				1
#define WEP_MAX_INDEX				4
#define WEP_KEY_TOT_NUM			4
#define WEP_TOT_FORMAT			2
enum SECURITY_WEP_FORMAT
{
    WEP_FORMAT_ASCII = 1,
    WEP_FORMAT_HEX
};
enum SECURITY_WEP_KEY_LEN
{
    WEP_KEY_64BIT = 1,
    WEP_KEY_128BIT
};

#define SCAN_CHANNEL_NUM			3

#define TURBO_ROAM_OP_MAX_NUM	1
#define TURBO_ROAM_MAX_CH_NUM	11
#define WLAN_OP_MODE_MAX_NUM	2
#define INFRA_SECURYIT_AUTHE_TOT_NUM		6
#define ADHOC_SECURYIT_AUTHE_TOT_NUM	1
#define PROFILE_NAME_BUF_SIZE		64

#define WLAN_PROF_NAME_LEN		32
#define WLAN_SSID_LENGTH			32

#define W1_WEP_FORMAT_ASCII     1
#define W1_WEP_FORMAT_HEX        2

#define EVENT_MAIL_SEL				0
#define EVENT_TRAP_SEL				1

#define EAP_MODE_TOT_NUM			4
enum SECURITY_EAP_MODE
{
    EAP_TLS_MODE=1,
    EAP_PEAP_MODE,
    EAP_TTLS_MODE,
    EAP_LEAP_MODE,
};

#define SECURITY_ENCRY_TOT_NUM	4
enum SECURITY_ENCRYPTION
{
    SECURITY_ENCRY_DISABLE=1,
    SECURITY_ENCRY_WEP,
    SECURITY_ENCRY_TKIP,
    SECURITY_ENCRY_AES,
};
enum SECURITY_AUTHENTICATION
{
    SECURITY_AUTH_OPEN=1,
    SECURITY_AUTH_SHARED,
    SECURITY_AUTH_WPA,
    SECURITY_AUTH_WPA_PSK,
    SECURITY_AUTH_WPA2,
    SECURITY_AUTH_WPA2_PSK,
};
/*
_config_map map_turbo_roam_channel[] = {
	{0, "N/A"},
	{1, "1"},
	{2, "2"},
	{3, "3"},
	{4, "4"},
	{5, "5"},
	{6, "6"},
	{7, "7"},
	{8, "8"},
	{9, "9"},
	{10, "10"},
	{11, "11"},
	{0},
};
_config_map map_event_status[] = {
	{0, "off"},
	{1, "on"},
	{0},
};
*/
/* perry add region end */

/* Serial Baudrate */
static long serial_baudrate[] =
    {
        50,
        75,
        110,
        134,
        150,
        300,
        600,
        1200,
        1800,
        2400,
        4800,
        7200,
        9600,
        19200,
        38400,
        57600,
        115200,
        230400,
        460800,
        921600
    };

typedef struct _scmd_chk
{
    int		port;
    void	*hook_t;

    int		opened;		/* 1 if sercmd_open() is called. */

    /* Only for character trigger check */
    int		pipe[2];	/* Application interface, 0 for SCM end, 1 for client end. */
    int		fd;			/* Underlying interface. */

    /* Only for break signal check */
    int		break_status;

    int		trigger;	/* Way to trigger SCM */

    int		sercmd_mode;	// 1 if serial command mode is entered.
}
scmd_chk;

typedef struct scm_pkt
{
    char	head;	/* Command/Reply */
    char	op;		/* Action */
    char	cmd[2];	/* Command code */
    char	param[1024];
    char	term;	/* Dummy null-terminated character */
    char	st;		/* status code */

    /* API use */
    int		wdx;
}
scm_pkt, *scm_pkt_t;

scm_pkt	reply_packet;
scm_pkt	cmd_packet;

struct _scm_excep
{
    u_short cmd;
    u_short rcv;
    int (*func)(char opcode, char *cmd, char *param, int paramLen);
};
typedef struct _scm_excep scm_excep;
typedef struct _scm_excep* scm_excep_t;

struct _scm_param
{
    u_short cmd;
    char getParam;
    char setParam;
};
typedef struct _scm_param scm_param;
typedef struct _scm_param* scm_param_t;

#define C(x, y) x | (y<<8)
scm_param cmdParam[] = {
                           // basic
                           {C('B', 'N'), 0, PARAMTER_NOT_CHECK},
                           {C('B', 'P'), 0, PARAMTER_NOT_CHECK},
                           {C('B', 'H'), 0, PARAMTER_NOT_CHECK},
                           {C('B', 'T'), 0, PARAMTER_NOT_CHECK},
                           {C('B', 'A'), 0, 1},
                           {C('B', 'E'), 0, 1},
                           {C('B', 'U'), 0, 1},
                           {C('B', 'L'), 0, 6},
                           {C('B', 'Z'), 0, 1},
                           {C('B', 'S'), 0, PARAMTER_NOT_CHECK},
                           // network
                           {C('N', 'C'), 0, PARAMTER_NOT_CHECK},
                           {C('N', 'I'), 0, PARAMTER_NOT_CHECK},
                           {C('N', 'M'), 0, PARAMTER_NOT_CHECK},
                           {C('N', 'G'), 0, PARAMTER_NOT_CHECK},
                           {C('N', 'D'), 1, 2},
                           {C('N', 'S'), 0, 1},
                           {C('N', 'R'), 0, 2},
                           // serial
                           {C('S', 'A'), 1, 2},
                           {C('S', 'B'), 1, 2},
                           {C('S', 'D'), 1, 2},
                           {C('S', 'S'), 1, 2},
                           {C('S', 'P'), 1, 2},
                           {C('S', 'L'), 1, 2},
                           {C('S', 'F'), 1, 2},
#ifdef MIINEPORT_E2
                           {C('S', 'I'), 1, 2},
#endif // MIINEPORT_E2
                           // op mode
                           {C('O', 'M'), 2, 3},
                           // realcom
                           {C('R', 'A'), 2, 3},
                           {C('R', 'M'), 2, 3},
                           {C('R', 'J'), 2, 3},
                           {C('R', 'D'), 2, 3},
                           // rfc2217
#ifdef MIINEPORT_E2
                           {C('F', 'A'), 2, 3},
                           {C('F', 'P'), 2, 3},
#endif // MIINEPORT_E2
                           // tcp
#ifdef MIINEPORT_E1
                           {C('T', 'O'), 2, 3},
                           {C('T', 'L'), 2, 3},
                           {C('T', 'E'), 2, PARAMTER_EXCEPTIONAL_CHECK},
#endif // MIINEPORT_E1
                           {C('T', 'A'), 2, 3},
                           {C('T', 'J'), 2, 3},
                           {C('T', 'V'), 2, 3},
                           // tcp server
#ifdef MIINEPORT_E1
                           {C('T', 'S'), 2, 3},
                           {C('T', 'W'), 2, PARAMTER_EXCEPTIONAL_CHECK},
                           {C('T', 'R'), 2, 3},
#endif // MIINEPORT_E1
                           {C('T', 'M'), 2, 3},
                           {C('T', 'P'), 2, 3},
#ifdef MIINEPORT_E2
                           {C('T', 'O'), 2, 3},
                           {C('T', 'D'), 2, 3},
#endif // MIINEPORT_E2
                           // tcp client
#ifdef MIINEPORT_E1
                           {C('T', 'N'), 2, 3},
                           {C('T', 'T'), 2, 3},
#endif // MIINEPORT_E1
                           {C('T', 'C'), 2, 3},
                           {C('T', 'I'), 3, 4},
#ifdef MIINEPORT_E2
                           {C('T', 'L'), 3, 4},
#endif // MIINEPORT_E2
                           /* perry region start */
                           //Profile
                           {C('T', 'B'), 0, 1},
                           {C('T', 'H'), 1, 2},
                           /* perry region end */
                           // UDP
#ifdef MIINEPORT_E1
                           {C('U', 'T'), 2, 3},
#endif // MIINEPORT_E1
                           {C('U', 'D'), 3, 4},
                           {C('U', 'P'), 2, 3},
                           // E-modem
#ifdef MIINEPORT_E2
                           /* perry modify start */
                           //	{C('E', 'A'), 2, 3},
                           //	{C('E', 'P'), 2, 3},
                           /* perry modify end */
#endif // MIINEPORT_E2
                           // delimiter
#ifdef MIINEPORT_E1
                           {C('O', 'Y'), 2, 3},
#endif // MIINEPORT_E1
                           {C('O', 'L'), 2, 3},
                           {C('O', 'D'), 2, 6},
                           {C('O', 'T'), 2, 3},
                           {C('O', 'F'), 2, 3},
                           // accessible IP
                           {C('A', 'S'), 0, 1},
                           {C('A', 'I'), 1, 4},
                           // SNMP
                           {C('M', 'S'), 0, 1},
                           {C('M', 'U'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'N'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'L'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'W'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'V'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'E'), 1, 2},
                           {C('M', 'A'), 1, 2},
                           {C('M', 'P'), 1, 2},
                           {C('M', 'M'), 1, 2},
                           {C('M', 'Y'), 1, 2},
                           {C('M', 'I'), 0, PARAMTER_NOT_CHECK},
                           {C('M', 'O'), 0, 1},
                           {C('M', 'C'), 0, PARAMTER_NOT_CHECK},
                           // PIO
#ifdef MIINEPORT_E1
                           {C('P', 'F'), 1, 2},
#endif // MIINEPORT_E1
                           {C('P', 'M'), 1, 2},
                           {C('P', 'S'), 1, 2},
                           /* perry region start */
                           {C('P', 'N'), 0, 1},
                           {C('P', 'P'), 1, 2},
                           {C('P', 'O'), 1, 2},
                           {C('P', 'D'), 1, 2},
                           /* perry region end */
                           // serial command mode
                           {C('C', 'T'), 0, 1},
                           {C('C', 'C'), 0, 3},
                           {C('C', 'B'), 0, 1},
                           {C('C', 'L'), 0, 1},
                           {C('C', 'S'), 0, 1},
                           {C('C', 'U'), 0, 1},
                           // Miscellaneous
                           {C('V', 'A'), 0, PARAMTER_NOT_CHECK},
                           {C('V', 'I'), 0, 1},
                           // overview
                           {C('@', 'S'), PARAMTER_RUNNING_ONLY, PARAMTER_NOT_SUPPORT},
                           {C('@', 'V'), PARAMTER_RUNNING_ONLY, PARAMTER_NOT_SUPPORT},
                           {C('@', 'B'), PARAMTER_RUNNING_ONLY, PARAMTER_NOT_SUPPORT},
                           {C('@', 'M'), PARAMTER_RUNNING_ONLY, PARAMTER_NOT_SUPPORT},

                           // control
                           {C('N', 'P'), PARAMTER_NOT_SUPPORT, 1},
                           {C('S', 'R'), PARAMTER_NOT_SUPPORT, 1},
                           {C('L', 'D'), PARAMTER_NOT_SUPPORT, 1},

                           //network(Wireless)
                           {C('W', 'C'), 0, PARAMTER_NOT_CHECK},
                           {C('W', 'I'), 0, PARAMTER_NOT_CHECK},
                           {C('W', 'M'), 0, PARAMTER_NOT_CHECK},
                           {C('W', 'G'), 0, PARAMTER_NOT_CHECK},

                           //Email Setting
                           {C('I', 'S'), 0, PARAMTER_NOT_CHECK},
                           {C('I', 'A'), 0, 1},
                           {C('I', 'U'), 0, PARAMTER_NOT_CHECK},
                           {C('I', 'P'), 0, PARAMTER_NOT_CHECK},
                           {C('I', 'F'), 0, PARAMTER_NOT_CHECK},
                           {C('I', 'T'), 1, 2},

                           /* perry region start */
                           //Security
                           {C('Q', 'A'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'E'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'P'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'T'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'U'), SECRUITY_GET_PARAM_NUM_2, 3},
                           {C('Q', 'W'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'V'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'L'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'I'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'S'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'F'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'K'), SECRUITY_GET_PARAM_NUM_2, 3},
                           {C('Q', 'R'), SECRUITY_GET_PARAM_NUM_1, 2},
                           {C('Q', 'S'), SECRUITY_GET_PARAM_NUM_1, 2},
                           // event
                           {C('E', 'C'), 1, 2},
                           {C('E', 'W'), 1, 2},
                           {C('E', 'A'), 1, 2},
                           {C('E', 'I'), 1, 2},
                           {C('E', 'P'), 1, 2},
                           {C('E', 'D'), 2, 3},
                           {C('E', 'S'), 2, 3},
                           /* perry region end */
                           {0, 0, 0}
                       };
#if 0
scm_excep scmExcep[] = {
                           {C('B', 'H'), 0, _scm_excep_console},
                           {C('B', 'T'), 0, _scm_excep_console},
                           {C('T', 'W'), 0, _scm_excep_tcp},
                           {C('T', 'E'), 0, _scm_excep_tcp},
                           {NULL, NULL, NULL},
                       };
#endif
#undef C

#if 0
#define C(x, y) x | (y<<8)
scm_excep scmExcep[] = {
                           {C('B', 'H'), 0, _scm_excep_console},
                           {C('B', 'T'), 0, _scm_excep_console},
                           {C('T', 'W'), 0, _scm_excep_tcp},
                           {C('T', 'E'), 0, _scm_excep_tcp},
                           {NULL, NULL, NULL},
                       };
#undef C
#endif
struct scm_io_param
{
    /* Remaining buffer */
    char	*buf;
    int		lbuf;

    scmd_chk	*chk;
};

void scm_main(int (*read)(int, char *, int), int (*write)(int, char *, int), int port, int timeout, int enter_signal);
static void scm_process(int (*write)(int, char *, int), int param, scm_pkt_t pkt);
static void scm_error_reply(int (*write)(int , char *, int), int param, scm_pkt_t pkt, unsigned char err);
static void scm_sw_trigger_check(int arg, u_char *ch);

#ifdef SUPPORT_NE_SCM
static scm_pkt_t scm_prepare_reply(unsigned char head, unsigned char op, unsigned char cmd[2], unsigned char st);
#else
static scm_pkt_t scm_prepare_reply(unsigned char op, char cmd[2], unsigned char st);
#endif // SUPPORT_NE_SCM
static scm_pkt_t scm_get_packet(int (*read)(int , char *, int), int param, u_long timeout);
static void scm_send_packet(int (*write)(int, char *, int), int param, scm_pkt_t pkt);

#ifdef SUPPORT_NE_SCM
static int scm_decodeNeCmd(int (*write)(void *, char *, int), void *param, scm_pkt_t pkt);
static int scm_encodeNeCmd(int (*write)(void *, char *, int), void *param, scm_pkt_t pkt);
#endif // SUPPORT_NE_SCM
//static scmd_chk *Gscmd_chk = NULL;
/**************************************************************/

static unsigned long htol(char * str)
{
    unsigned long	s = 0;
    unsigned char	c;

    while ( 1 )
    {
        c = *str++;
        if ( c < '0' )
            break;	/* break if not hex char, maybe 0 */
        else if ( c <= '9' )
            s = s * 16 + c - '0';
        else if ( c < 'A' )
            break;	/* break if not hex char */
        else if ( c <= 'F' )
            s = s * 16 + c - 'A' + 10;
        else if ( c < 'a' )
            break;	/* break if not hex char */
        else if ( c <= 'f' )
            s = s * 16 + c - 'a' + 10;
        else
            break;	/* break if not hex char */
    }
    return(s);
}

static unsigned int	htoi(char * str)
{
    return( (unsigned int)htol(str) );
}

static int isValidHex(char* param)
{
    if (param==NULL)	return -1;

    if (_strlen(param)<1 || _strlen(param)>2)	return -2;

    if (param[0]<'0' || (param[0]>'9' && param[0]<'A') ||
            (param[0]>'F' && param[0]<'a') || param[0]>'f')
        return -2;

    if (param[1])
    {
        if (param[1]<'0' || (param[1]>'9' && param[1]<'A') ||
                (param[1]>'F' && param[1]<'a') || param[1]>'f')
            return -2;
    }

    return 1;
}

static int isValidNum(char* param,u_int min,u_int max)
{
    int num;

    if (param == NULL)
        return -1;

    if (_strlen(param) < 1)	// length = 0
        return -2;

    num = _atoi(param);

    if (num < min || num > max)
        return -3;

    if (num == 0)
    {	//make sure it is a number not char
        if (param[0] < '0' || param[0] > '9')
            return -4;
    }

    return num;
}
#define isEnableDisable(x) isValidNum((x),0,1)
#define isValidPort(x)	isValidNum((x),0,65535)
#define ERROR_4FF_IP	99
#define ERROR_ZERO_IP	-99
static int isValidNetmask(char* param)
{
    char 	*result;
    char	temp[16];
    int		i,ret;
    u_char	isFind,isZero=0;
    int		validNum[]={255,254,252,248,240,224,192,128,0};
    u_char	count=0;

    if (param==NULL)	return -1;

    if (_strlen(param)<7 || _strlen(param)>15)	return -2;

    _strcpy(temp,param);
    result=strtok(temp,".");
    while (result)
    {
        if ((ret=isValidNum(result,0,255))<0)
            return -3;

        if (isZero && ret!=0)
            return -4;
        else for (i=0,isFind=0;i<sizeof(validNum)/sizeof(int);i++)
            {
                if (ret==validNum[i])
                {
                    if (i!=0)	isZero=1;

                    isFind=1;
                    break;
                }
            }

        if (!isFind)	return -5;

        if (count++ >= 3)
            return 1;	// successful
        result=strtok(NULL,".");
    }
    return -6;
}

static int isValidIP(char* param)
{
    int temp[4];
    char *ptr;
    int	idx=0;

    if (param==NULL)	return -1;

    if (_strlen(param)<7 || _strlen(param)>15)	return -2;

    temp[0]=temp[1]=temp[2]=temp[3]=0;
    ptr=param;
    while (*ptr)
    {
        if (*ptr>='0' && *ptr<='9')	temp[idx]=temp[idx]*10+*ptr-'0';
        else if (*ptr == '.')		idx++;
        else						return -3;

        if (temp[idx]>0xFF)	return -4;
        if (idx>3)			return -5;

        ptr++;
    }

    temp[0]+=temp[1]+temp[2]+temp[3];
    if (temp[0]==1020)
        return ERROR_4FF_IP;
    if (temp[0]==0)
        return ERROR_ZERO_IP;
    return 1;

}
#if 0
// return 1 if valid
static int isValidNetmask(char* param)
{
    char 	*result;
    char	temp[16];
    int		i,ret;
    u_char	isFind,isZero=0;
    int		validNum[]={255,254,252,248,240,224,192,128,0};
    u_char	count=0;

    if (param==NULL)	return -1;

    if (_strlen(param)<7 || _strlen(param)>15)	return -2;

    _strcpy(temp,param);
    result=strtok(temp,".");
    while (result)
    {
        if ((ret=isValidNum(result,0,255))<0)
            return -3;

        if (isZero && ret!=0)
            return -4;
        else for (i=0,isFind=0;i<sizeof(validNum)/sizeof(int);i++)
            {
                if (ret==validNum[i])
                {
                    if (i!=0)	isZero=1;

                    isFind=1;
                    break;
                }
            }

        if (!isFind)	return -5;

        if (count++ >= 3)
            return 1;	// successful
        result=strtok(NULL,".");
    }
    return -6;
}

static int isValidHex(char* param)
{
    if (param==NULL)	return -1;

    if (_strlen(param)<1 || _strlen(param)>2)	return -2;

    if (param[0]<'0' || (param[0]>'9' && param[0]<'A') ||
            (param[0]>'F' && param[0]<'a') || param[0]>'f')
        return -2;

    if (param[1])
    {
        if (param[1]<'0' || (param[1]>'9' && param[1]<'A') ||
                (param[1]>'F' && param[1]<'a') || param[1]>'f')
            return -2;
    }

    return 1;
}
#endif
static unsigned long hexstring2dec(char *str)
{
    unsigned long	ret;

    ret = 0;
    while (*str)
    {
        ret <<= 4;
        if ( *str >= '0' && *str <= '9' )
            ret += (*str - '0');
        else if ( *str >= 'A' && *str <= 'F' )
            ret += ((*str - 'A') + 10);
        else if ( *str >= 'a' && *str <= 'f' )
            ret += ((*str - 'a') + 10);
        str++;
    }
    return(ret);
}

static int ishex(char *buf)
{
    char	ch;

    for (;;)
    {
        ch = *buf++;
        if (ch == 0)
            break;
        if ((ch >= '0') && (ch <= '9'))
            continue;
        if ((ch >= 'a') && (ch <= 'f'))
            continue;
        if ((ch >= 'A') && (ch <= 'F'))
            continue;
        return(0);
    }
    return(1);
}

static short stoh(char *str)
{
    unsigned short val;

    val = 0;

    if (ishex(str))
    {
        val = hexstring2dec(str);
    }
    return val;
}

static int _scm_find_parm_num(char *param, int paramLen)
{
    int i;
    int num;

    if (paramLen == 0)
        return 0;

    num = 1;
    for (i=0; i<paramLen; i++)
    {
        if (param[i] == ';')
            ++num;
    }

    return num;
}
/*
 * return  1: success
 *        -1: not find cmd
 *        -2: parameter number fail
 */
int scm_checkParamNumber(char opcode, char *cmd, char *param, int paramLen)
{
    scm_param_t tmp;
    int parmNum=0;

    tmp = &cmdParam[0];

    while (tmp->cmd)
    {
        if (memcmp((char*)&tmp->cmd, cmd, 2) == 0)
        {
            if ( (opcode == OP_GETCONF)
                    || (opcode == OP_RUNNINGCONF)
                    || (opcode == OP_VIEWSTAT) )
            {
                parmNum = (int)tmp->getParam;
            }
            else if ( (opcode == OP_SETCONF)
                      || (opcode == OP_CONTROL) )
            {
                parmNum = (int)tmp->setParam;
            }

            break;
        }
        tmp++;
    }
#if 1
    if (tmp->cmd == 0)
    {
        return -1;
    }
#endif
    if (parmNum == PARAMTER_NOT_CHECK)
    {
        return 1;
    }
    else if (parmNum == PARAMTER_EXCEPTIONAL_CHECK)
    {
#if 0 //Joy
        scm_excep_t excepTmp = scmExcep;

        while (excepTmp->cmd)
        {
            if (memcmp((char*)&excepTmp->cmd, cmd, 2) == 0)
            {
                if (excepTmp->func)
                    return excepTmp->func(opcode, cmd, param, paramLen);
            }

            ++excepTmp;
        }
#endif
    }
    else if (parmNum == PARAMTER_RUNNING_ONLY)
    {
        if (opcode == OP_GETCONF)
            return -1;

        if (paramLen == 0)
            return 1;
    }
    else if (parmNum == PARAMTER_NOT_SUPPORT)
        return -1;
    else if (parmNum == 0)
    {
        if (paramLen == 0)
            return 1;
    }
    else
    {
        int num;

        if (paramLen == 0)
            return -2;

        num = _scm_find_parm_num(param, paramLen);

        if (num == parmNum)
            return 1;
    }

    return -2;

}


static void scm_error_reply(int (*write)(int , char *, int), int param, scm_pkt_t pkt, unsigned char err)
{
#ifdef SUPPORT_NE_SCM
    pkt = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, err);
#else
    pkt = scm_prepare_reply(pkt->op, pkt->cmd, err);
#endif // SUPPORT_NE_SCM
    scm_send_packet(write, param, pkt);
    /* param = port number */
}

#ifdef SUPPORT_NE_SCM
static scm_pkt_t scm_prepare_reply(unsigned char head, unsigned char op, unsigned char cmd[2], unsigned char st)
#else
static scm_pkt_t scm_prepare_reply(unsigned char op, char cmd[2], unsigned char st)
#endif // SUPPORT_NE_SCM
{
    scm_pkt_t	pkt;

    pkt = &reply_packet;
#ifdef SUPPORT_NE_SCM
    switch (head)
    {
    case HEAD_NE_COMMAND:
        pkt->head = HEAD_NE_REPLY;
        break;
    case HEAD_COMMAND:
    default:
        pkt->head = HEAD_REPLY;
        break;
    }
#else
    pkt->head = HEAD_REPLY;
#endif // SUPPORT_NE_SCM
    pkt->op = op;
    pkt->cmd[0] = cmd[0];
    pkt->cmd[1] = cmd[1];
    pkt->wdx = 0;
    pkt->st = st;

    return pkt;
}

static void scm_send_packet(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    int	i, len, rdx;
    char	tail[] = {TAIL_LF};

    rdx = 0;
    len = pkt->wdx;

#ifdef SUPPORT_NE_SCM
//	if(scm_encodeNeCmd(write, param, pkt) < 0)
    return;
#endif // SUPPORT_NE_SCM
    for (i = 0; i < len; i++)
    {
        if (pkt->param[i] == TAIL_LF)
        {
            if (rdx == 0)
            {
                pkt->head = HEAD_REPLY_M;	/* XXX: Current, the packet here must be reply */
                write(param, &pkt->head, (int)&((scm_pkt_t)0)->param);
                write(param, &pkt->st, sizeof(pkt->st));
                write(param, &pkt->param[0], i + 1);
                pkt->head = HEAD_REPLY;
            }
            else
            {
                pkt->head = HEAD_REPLY_M;
                write(param, &pkt->head, sizeof(pkt->head));
                write(param, &pkt->param[rdx], (i - rdx + 1));
                pkt->head = HEAD_REPLY;
            }

            rdx = i + 1;
        }
    }

    if (rdx == 0)
    {
        write(param, &pkt->head, (int)&((scm_pkt_t)0)->param);
        write(param, &pkt->st, sizeof(pkt->st));
        write(param, &pkt->param[0], i);
        write(param, tail, 1);
    }
    else
    {
        write(param, &pkt->head, sizeof(pkt->head));
        write(param, &pkt->param[rdx], i - rdx);
        write(param, tail, 1);
    }
}

static scm_pkt_t scm_get_packet(int (*read)(int, char *, int), int param, u_long timeout)
{
    scm_pkt_t	pkt;
    int			i;
    int			idx;
    char		ch;
    u_long		t;
    int	baud, mode, flowctrl, ret;

    enum {
        S_INITIAL,
        S_VALID_HEAD,	/* A valid head character is received */
        S_CR,			/* Used in multiple line only, previous CR is received */
        S_CRLF,			/* Used in multiple line only, previous CRLF is received */
        S_LF,			/* Used in multiple line only, previous LF is received */
        S_UNRECOGNIZED,	/* Used in multiple line now, indicate that unknown head character following line tail */
    } stat;

    pkt = &cmd_packet;
    pkt->head = HEAD_INVALID;
    pkt->op = OP_INVALID;
    pkt->cmd[0] = '-';
    pkt->cmd[1] = '-';
    pkt->st = ST_INVALID_OPERATION;

    for (i = 0; i < sizeof(pkt->param); i++)
        pkt->param[i] = '\0';

    pkt->term = '\0';

    idx = 0;

    stat = S_INITIAL;
    t = sys_clock_s();

    while (1)
    {
        if (portd_terminate)
        {
            sys_setSCMStatus(SCM_DATA_MODE);
            pthread_exit(NULL);
        }
        if (timeout)
        {
            if (sys_clock_s() - t >= timeout)
            {
                return NULL;
            }
        }

        i = read(param, &ch, 1);
        if (ret < 0)
        {
            sercmd_open(param);
            Scf_getAsyncIoctl(param, &baud, &mode, &flowctrl);
            sercmd_ioctl(param, baud, mode);
        }
        if (i != 1)
        {
            usleep(100000);//sys_sleep_ms(100);
            continue;
        }
        t = sys_clock_s();

        if (ch == TAIL_CR || ch == TAIL_LF)
        {
            if (idx == 0)	/* stat must be S_INITIAL */
                continue;	/* Ignore leading CR or LF */

            if (stat == S_CR && ch == TAIL_LF)
            {
                if (idx < (int)&((scm_pkt_t)0)->term)
                    ((unsigned char*)pkt)[idx++] = ch;

                stat = S_CRLF;
            }
            else if (stat != S_VALID_HEAD)	/* Caused by error input */
            {
                break;
            }
            else if (pkt->head == HEAD_COMMAND_M)
            {
                if (idx < (int)&((scm_pkt_t)0)->term)
                    ((unsigned char*)pkt)[idx++] = ch;

                if (ch == TAIL_CR)
                    stat = S_CR;
                else
                    stat = S_LF;
            }
            else
            {
                /* done */
                break;
            }
        }
        else switch (stat)
            {
            case S_INITIAL:
                if (ch == HEAD_COMMAND || ch == HEAD_COMMAND_M
#ifdef SUPPORT_NE_SCM
                        || ch == HEAD_NE_COMMAND
#endif // SUPPORT_NE_SCM
                   )

                {
                    idx = 0;	/* Reset to front! */
                    ((unsigned char*)pkt)[idx++] = ch;	/* same as pkt->head */
                    stat = S_VALID_HEAD;
                }
                break;
            case S_UNRECOGNIZED:
            case S_VALID_HEAD:
                if (idx < (int)&((scm_pkt_t)0)->term)
                    ((unsigned char*)pkt)[idx] = ch;
                ++idx;
                break;
            case S_CR:
            case S_LF:
            case S_CRLF:
                pkt->head = ch;
                if (ch == HEAD_COMMAND || ch == HEAD_COMMAND_M
#ifdef SUPPORT_NE_SCM
                        || ch == HEAD_NE_COMMAND
#endif // SUPPORT_NE_SCM
                   )
                {
                    stat = S_VALID_HEAD;
                }
                else
                {
                    stat = S_UNRECOGNIZED;
                }
                break;
            }
    }/* while(getch()) */

    if (stat != S_VALID_HEAD)
    {
        pkt->st = ST_UNRECOGNIZED_FORMAT;
    }
    else if (idx > (int)&((scm_pkt_t)0)->term)
    {
        pkt->st = ST_INCOMPLETE_PARAMETER;
    }
    else if (idx < (int)&((scm_pkt_t)0)->cmd)
    {
        pkt->st = ST_INVALID_OPERATION;
    }
    else if (idx < (int)&((scm_pkt_t)0)->param)
    {
        pkt->st = ST_INVALID_COMMAND;
    }
    else
    {
        pkt->st = ST_SUCCESS;
        pkt->wdx = idx - (int)&((scm_pkt_t)0)->param;
    }
    return pkt;
}

/***************************** scm config function *************************/

static void scm_config_network(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
        // Get or Set eth0 DHCP status
    case 'C':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int btype;

            Scf_getIPConfig(IF_NUM_DEV_ETH0, &btype);
            if (btype == IP_CONFIG_BOOTP)
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", IP_CONFIG_BOOTP + 1);
            else
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", btype);
        }
        else
        {
            int cmp;
            if (pkt->param)
            {
                if (isValidNum(pkt->param, 0, 3)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "2");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "3");
                if (cmp == 0)
                {
                    Scf_setIPConfig(IF_NUM_DEV_ETH0, IP_CONFIG_BOOTP	);
                    /* For Compatible */
                    break;
                }
                Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
            }
            else
            {
                Scf_setIPConfig(IF_NUM_DEV_ETH0, 0);
            }
        }

        break;
#if 1
        // Get or Set eth0 IP Address
    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long ip;
            struct in_addr in;

            Scf_getIfaddr(IF_NUM_DEV_ETH0, &ip);
            in.s_addr = ip;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;

            if (pkt->param)
            {
                if (isValidIP(pkt->param) < 0 || isValidIP(pkt->param) == ERROR_4FF_IP)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                inet_aton(pkt->param, &in);
                Scf_setIfaddr(IF_NUM_DEV_ETH0, in.s_addr);
            }
            else
            {
                Scf_setIfaddr(IF_NUM_DEV_ETH0, 0);
            }
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }

        break;
        // Get or Set eth0 NetMask
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long mask;
            struct in_addr in;

            Scf_getNetmask(IF_NUM_DEV_ETH0, &mask);
            in.s_addr = mask;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;
            int cmp;

            if (pkt->param)
            {
                if (isValidNetmask(pkt->param)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "0.0.0.0");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "255.255.255.255");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                inet_aton(pkt->param, &in);
                Scf_setNetmask(IF_NUM_DEV_ETH0, in.s_addr);
            }
            else
            {
                Scf_setNetmask(IF_NUM_DEV_ETH0, 0);
            }
#if 0
            inet_aton(pkt->param, &in);
            Scf_setNetmask(IF_NUM_DEV_ETH0, in.s_addr);
#endif
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }

        break;
        // Get or Set eth0 Gateway
    case 'G':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long mask;
            struct in_addr in;

            Scf_getGateway(IF_NUM_DEV_ETH0, &mask);
            in.s_addr = mask;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;
            int cmp;
            /* IsValidIP()-> 0.0.0.0 is invalid */
            if (pkt->param)
            {
                if (_strlen(pkt->param))
                {
                    if (isValidIP(pkt->param)<0)
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    cmp = _strcmp(pkt->param, "255.255.255.255");

                    if (cmp == 0)
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    inet_aton(pkt->param, &in);
                    Scf_setGateway(IF_NUM_DEV_ETH0, in.s_addr);
                }
                else
                    Scf_setGateway(IF_NUM_DEV_ETH0, 0);
            }
            else
            {
                Scf_setGateway(IF_NUM_DEV_ETH0, 0);
            }
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }

        break;
    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int index;
            unsigned long addr;
            struct in_addr in;

            index = _atoi(pkt->param);
            Scf_getDNS(index, &addr);
            in.s_addr = addr;

            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            char *result;
            int index;
            int cmp;
            struct in_addr in;

            result=strtok(pkt->param,";");

            if (result == NULL)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(result);
            if (index!=1 && index!=2)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            result=strtok(NULL,";");

            if (result)
            {
                if (isValidIP(result)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(result, "255.255.255.255");

                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                inet_aton(result, &in);
                Scf_setDNS(index, in.s_addr);
            }
            else
            {
                Scf_setDNS(index, 0);
//			scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
//				return;
            }
        }

        break;
#endif
    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}

static void scm_config_basic(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    int http, https, telnet, ssh;
#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'N':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            Scf_getServerName(&rep->param[rep->wdx], 80);
            rep->wdx = _strlen(rep->param);
        }
        else
        {
            if (pkt->param == NULL)
            {
                Scf_setServerName("", 0);
                break;
            }
            if (_strlen(pkt->param) > DCF_SYS_NAME_LEN)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setServerName(pkt->param, _strlen(pkt->param));
        }

        break;
        //Get or Set web console state
    case 'H':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            Scf_getConsoleSetting(&http, &https, &telnet, &ssh);

            if (!http)
            {
                rep->wdx += sprintf(&rep->param[rep->wdx],"0");
            }
            else
            {
                rep->wdx += sprintf(&rep->param[rep->wdx],"1;80");
            }

        }
        else
        {
            char *token;
            int value;

            Scf_getConsoleSetting(&http, &https, &telnet, &ssh);

            token = strtok(pkt->param, ";");
            value = _atoi(token);

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setConsoleSetting(value, https, telnet, ssh);
        }
        break;
        //Get or Set telnet console state

    case 'T':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            Scf_getConsoleSetting(&http, &https, &telnet, &ssh);

            if (!telnet)
            {
                rep->wdx += sprintf(&rep->param[rep->wdx],"0");
            }
            else
            {
                rep->wdx += sprintf(&rep->param[rep->wdx],"1;23");
            }

        }
        else
        {
            char *token;
            int value;

            Scf_getConsoleSetting(&http, &https, &telnet, &ssh);

            token = strtok(pkt->param, ";");
            value = _atoi(token);

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setConsoleSetting(http, https, value, ssh);
        }
        break;
        /* Get or Set Local Time */
    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int year, mon, day, hour, min, sec;

            sys_time_get(&year, &mon, &day, &hour, &min, &sec);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d/%d/%d %d:%d:%d",
                                year, mon, day, hour, min, sec);
        }
        else
        {
            int year, month, date, hour, minute, second;
            char *token;

            token = strtok(pkt->param, ";");

            year = _atoi(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 1, 12) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            month = _atoi(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 1, 31) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            date = _atoi(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 23) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            hour = _atoi(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 59) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            minute = _atoi(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 59) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            second = _atoi(token);

            sys_time_set(year,month,date,hour,minute,second);
        }
        break;
        /* Get or Set Time zone */
    case 'Z':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int zone_index;

            zone_index = Scf_getTimeZoneIndex();

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", zone_index);
        }
        else
        {
            int zone_index;
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 63) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            zone_index = _atoi(token);

            Scf_setTimeZoneIndex(zone_index);
        }
        break;
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[DCF_TIME_SERVER_LEN + 1];

            memset(buf, 0, DCF_TIME_SERVER_LEN + 1);

            Scf_getTimeServer(buf, DCF_TIME_SERVER_LEN+1);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;

            token = strtok(pkt->param, ";");

            if (token == NULL)
            {
                Scf_setTimeServer("", 0);
                break;
            }

            if (_strlen(token) > DCF_TIME_SERVER_LEN)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setTimeServer(token, DCF_TIME_SERVER_LEN);
        }

        break;
        //Get or Set password for Admin
    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char	pwd[20];
            memset(pwd,0,sizeof(pwd));

            Scf_getPassword(0, pwd, sizeof(pwd));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", pwd);
        }
        else
        {
            /* DCF_CON_PSWD_LEN = 16 */
            if (pkt->param == NULL)
            {
                Scf_setPassword(0, "", 0);
                break;
            }

            if (_strlen(pkt->param) > 16)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setPassword(0, pkt->param, _strlen(pkt->param));
        }
        break;
        //Get or Set password for User
    case 'R':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char	pwd[20];
            memset(pwd,0,sizeof(pwd));

            Scf_getPassword(1, pwd, sizeof(pwd));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", pwd);
        }
        else
        {
            /* DCF_CON_PSWD_LEN = 16 */
            if (pkt->param == NULL)
            {
                Scf_setPassword(1, "", 0);
                break;
            }

            if (_strlen(pkt->param) > 16)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setPassword(1, pkt->param, _strlen(pkt->param));
        }
        break;
        // Set to Save & Restart, 1 : restart only, 2: save & restart
#if 0
    case 'R':
        if (pkt->op == OP_SETCONF)
        {
            int pid;
            pid = sys_get_pid(1, DSPORTD_PID_FILE);
            if (pkt->param[0] == '1')
            {
                usleep(100000);
                if (!fork ())
                {
                    //system("ls -al");
                    //system("echo 'fork child'");

                    kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGTERM);
                    system("/usr/bin/portd -p 1");
                }
                else
                {}

            }
            else if (pkt->param[0] == '2')
            {
                usleep(100000);
                sys_reboot(0);
            }
        }
        break;
#endif
    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}

static void scm_config_serial(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    int index,mode;
    long baud;
    char *token;
    int portIdx;
    int	flowctrl, baud_index;
    long baudrate;
#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char	alias[16 + 1];

            memset(alias, 0x0, sizeof(alias));

            Scf_getPortAlias(portIdx, alias, 16 + 1);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", alias);
        }
        else
        {
#if 0
            token = strtok(NULL, ";");

            if (_strlen(token) >= 16)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if (token == NULL)
                Scf_setPortAlias(portIdx, "", 0);
            else
                Scf_setPortAlias(portIdx, token, _strlen(token));
#else
            token = strtok(NULL, ";");
#if 0
            if (token == NULL)
            {
                Scf_setPortAlias(portIdx, "", 0);
                break;
            }
#endif
            if (token)
            {
                if (_strlen(token) > 16)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                Scf_setPortAlias(portIdx, token, _strlen(token));
            }
            else
            {
                Scf_setPortAlias(portIdx, "", 0);
            }
#endif
        }
        break;
    case 'B':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);
            switch (baud_index)
            {
            case BAUD_50:
                baudrate = 50L;
                break;
            case BAUD_75:
                baudrate = 75L;
                break;
            case BAUD_110:
                baudrate = 110L;
                break;
            case BAUD_134:
                baudrate = 134L;
                break;
            case BAUD_150:
                baudrate = 150L;
                break;
            case BAUD_300:
                baudrate = 300L;
                break;
            case BAUD_600:
                baudrate = 600L;
                break;
            case BAUD_1200:
                baudrate = 1200L;
                break;
            case BAUD_1800:
                baudrate = 1800L;
                break;
            case BAUD_2400:
                baudrate = 2400L;
                break;
            case BAUD_4800:
                baudrate = 4800L;
                break;
                //case BAUD_7200:     baudrate = B7200;   break;
            case BAUD_9600:
                baudrate = 9600L;
                break;
            case BAUD_19200:
                baudrate = 19200L;
                break;
            case BAUD_38400:
                baudrate = 38400L;
                break;
            case BAUD_57600:
                baudrate = 57600L;
                break;
            case BAUD_115200:
                baudrate = 115200L;
                break;
            case BAUD_230400:
                baudrate = 230400L;
                break;
            case BAUD_460800:
                baudrate = 460800L;
                break;
            case BAUD_921600:
                baudrate = 921600L;
                break;
            default:
                baudrate = 38400L;
                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%ld", baudrate);
        }
        else if (pkt->op == OP_SETCONF)
        {
            int cmp;
            int num;
            u_int maxBaud;
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            token = strtok(NULL,";");
#if 0//
            if (Spf_queryCapability(DCAP_B921600))
                maxBaud = 921600L;
            else
                maxBaud = 230400L;
#else
            maxBaud = 921600L;
#endif
            if (isValidNum(token, 50, maxBaud) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            cmp = _strcmp(token, "7200");

            if (!cmp)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            baud = atol(token);
            num = sizeof(serial_baudrate) / sizeof(long);
            for (index = 0;index < num; index++)
            {
                if (serial_baudrate[index] == baud)
                    break;
            }
            if (index != num)//find baud rate
            {
                Scf_setAsyncIoctl(portIdx, index, mode, flowctrl);
            }
            else//linear baud rate
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
                //Scf_setAsyncIoctl(portIdx, baud, mode, flowctrl);
            }
        }
        break;

    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int data_bits;

            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            switch ( mode & BIT_DATA_MASK )
            {
//       		case BIT_5: data_bits = 5;     break;
//       		case BIT_6: data_bits = 6;     break;
            case BIT_7:
                data_bits = 7;
                break;
            case BIT_8:
                data_bits = 8;
                break;
            default:
                data_bits = 8;
                break;
            }

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", data_bits);
        }
        else
        {
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);
            token=strtok(NULL,";");

            if (isValidNum(token,7,8)<0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            index = _atoi(token);

            mode &= ~BIT_DATA_MASK;
            switch (index)
            {
            case 5:
                mode |= BIT_5;
                break;
            case 6:
                mode |= BIT_6;
                break;
            case 7:
                mode |= BIT_7;
                break;
            case 8:
                mode |= BIT_8;
                break;
            }
            Scf_setAsyncIoctl(portIdx, baud_index, mode, flowctrl);
        }
        break;

    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int stop_bits;

            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            switch ( mode & BIT_STOP_MASK )
            {
            case STOP_1:
                stop_bits = 0;
                break;
            case STOP_2:
                stop_bits = 2;
                break;
            default:
                stop_bits = 0;
                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", stop_bits);
        }
        else
        {
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);
            token=strtok(NULL,";");
            if (isValidNum(token,0,2)<0)// 0,1,2 is available
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            if (_strcmp(token, "1") == 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            mode &= ~BIT_STOP_MASK;
            index = _atoi(token);

            switch (index)
            {
            case 0:
                mode |= STOP_1;
                break;
            case 1:
            case 2:
                mode |= STOP_2;
                break;
            }
            Scf_setAsyncIoctl(portIdx, baud_index, mode, flowctrl);
        }
        break;

    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int parity;

            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            switch ( mode & BIT_PARITY_MASK )
            {
            case P_NONE:
                parity = 0;
                break;
            case P_ODD :
                parity = 1;
                break;
            case P_EVEN:
                parity = 2;
                break;
//        		case P_MRK :  parity = 3;     break;
//        		case P_SPC:    parity = 4;     break;

            default:
                parity = 0;
                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", parity);
        }
        else
        {
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);
            token=strtok(NULL,";");
            if (isValidNum(token,0, 2)<0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            index = atol(token);

            mode &= ~BIT_PARITY_MASK;
            switch (index)
            {
            case 0:
                mode |= P_NONE;
                break;
            case 1:
                mode |= P_ODD;
                break;
            case 2:
                mode |= P_EVEN;
                break;
            case 3:
                mode |= P_MRK;
                break;
            case 4:
                mode |= P_SPC;
                break;
            }
            Scf_setAsyncIoctl(portIdx, baud_index, mode, flowctrl);
        }
        break;

    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int flow;

            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            switch (flowctrl)
            {
            case 0:     /* None */
                flow = 0;
                break;
            case 1:     /* RTS/CTS */
                flow = 1;
                break;
            case 2:     /* XON/XOFF */
                flow = 2;
                break;
                //	case 3:     /* DTR/DSR */
                //		flowctrl = 0x60;
                //		break;
            default:
                flow = 0;
                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", flow);
        }
        else
        {
            Scf_getAsyncIoctl(portIdx, &baud_index, &mode, &flowctrl);

            token = strtok(NULL,";");
            if (isValidNum(token, 0, 2) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(token);
            flowctrl = index;

            Scf_setAsyncIoctl(portIdx, baud_index, mode, flowctrl);
        }

        break;
    case 'F':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int fifo;

            fifo = Scf_getAsyncFifo(portIdx);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", fifo);
        }
        else
        {
            token=strtok(NULL,";");
            if (isEnableDisable(token)<0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(token);
            Scf_setAsyncFifo(portIdx, index);
        }
        break;
    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int inter;

            inter = Scf_getIfType(portIdx);

            if (inter == 2) /* W1 get 2 is RS485-2 */
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", 1);
            else
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", inter);

        }
        else
        {
            int iftype;

            token = strtok(NULL,";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            index = _atoi(token);
            switch (index)
            {
            case RS232_MODE:
                iftype = RS232_MODE;
                break;
            case RS422_MODE:
                iftype = 2; /* W1 RS485_2 value is 2 */
                break;
            default:
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setIfType(portIdx, iftype);
        }
        break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;

    }

    scm_send_packet(write, param, rep);
}

#define OP_REALCOM CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_REALCOM
#define OP_RFC2217 CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_RFC2217
#define OP_TCPSERVER CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPSERVER
#define OP_TCPCLIENT CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPCLIENT
#define OP_UDP CFG_APPLICATION_SOCKET | CFG_OPMODE_UDP
#define OP_PAIRMASTER CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_MASTER
#define OP_PAIRSLAVE CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_SLAVE
#define OP_EMODEM CFG_APPLICATION_ETH_MODEM
#define DCF_PORT_FLAG_DCH1			0x0001		/* delimit char 1 is set */
#define DCF_PORT_FLAG_DCH2			0x0002		/* delimit char 2 is set */
#define DEL_MODE_NO					1
#define DEL_MODE_1						2
#define DEL_MODE_2						4
#define DEL_MODE_STRIP					8
static void scm_config_op_basic(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     portIdx;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            switch (Scf_getOpMode(portIdx))
            {
#if 1
            case 0		      :
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",0);
                break;
            case OP_REALCOM:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",1);
                break;
            case OP_RFC2217:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",2);
                break;
            case OP_TCPSERVER:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",3);
                break;
            case OP_TCPCLIENT:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",4);
                break;
            case OP_UDP:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",5);
                break;
//			case OP_EMODEM:			rep->wdx += sprintf(&rep->param[rep->wdx],"%d",6);	break;
#endif
            default:
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d",0);
                break;
            }
        }
        else
        {
            token = strtok(NULL,";"); /* mcsc field */
            token = strtok(NULL,";"); /* opmode filed */
            if (isValidNum(token, 0, 5) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            switch (_atoi(token))
            {
            case 0:
                Scf_setOpMode(portIdx, 0);
                break;

            case 1:
                Scf_setOpMode(portIdx, OP_REALCOM);
                break;

            case 2:
                Scf_setOpMode(portIdx, OP_RFC2217);
                break;

            case 3:
                Scf_setOpMode(portIdx, OP_TCPSERVER);
                break;

            case 4:
                Scf_setOpMode(portIdx, OP_TCPCLIENT);
                break;

            case 5:
                Scf_setOpMode(portIdx, OP_UDP);
                break;

            default:
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
        }

        break;

    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            Scf_getDataPacking(portIdx, &flags, (INT8U *)&ch1,(INT8U *) &ch2, &tout, &mode, &packlen);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", packlen);
        }
        else
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            token=strtok(NULL,";"); /* MCSC Index */
            token=strtok(NULL,";"); /* Data Packing Length Value */

            if (isValidNum(token, 0, 1024) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *)&ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            packlen = (u_short)_atoi(token);

            Scf_setDataPacking(portIdx,
                               flags,
                               (unsigned char) ch1,
                               (unsigned char) ch2,
                               tout,
                               mode,
                               packlen);
        }

        break;
    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            rep->wdx += sprintf(&rep->param[rep->wdx],
                                "%d;%02X;%d;%02X",
                                !!(flags&DCF_PORT_FLAG_DCH1),
                                ch1,
                                !!(flags&DCF_PORT_FLAG_DCH2),
                                ch2);

        }
        else
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            token = strtok(NULL,";"); /* MCSC Index */
            token = strtok(NULL,";"); /* Delimiter 1 Flag */

            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            if (atol(token))
                flags |= DCF_PORT_FLAG_DCH1;
            else
                flags &= ~DCF_PORT_FLAG_DCH1;

            token = strtok(NULL,";"); /* Delimiter 1 Char */
            if (isValidHex(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            ch1 = htoi(token);

            token = strtok(NULL,";"); /* Delimiter2 Flag */
            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            if (atol(token))
                flags |= DCF_PORT_FLAG_DCH2;
            else
                flags &= ~DCF_PORT_FLAG_DCH2;

            token = strtok(NULL,";");
            if (isValidHex(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            ch2 = htoi(token);

            Scf_setDataPacking(portIdx,
                               flags,
                               (unsigned char) ch1,
                               (unsigned char) ch2,
                               tout,
                               mode,
                               packlen);
        }

        break;
    case 'T':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;
            int del;

            del = 0;

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);
            switch (mode)
            {
            case DEL_MODE_NO:
                del = 0;

                break;
            case DEL_MODE_1:
                del = 1;

                break;
            case DEL_MODE_2:
                del = 2;

                break;
            case DEL_MODE_STRIP:
                del = 3;

                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", del);
        }
        else
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;
            //int del;

            token = strtok(NULL, ";"); /* MCSC Index */
            token = strtok(NULL, ";"); /* Delimiter Process Index */

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            if (isValidNum(token, 0, 3) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            mode = 1 << _atoi(token);

            Scf_setDataPacking(portIdx,
                               flags,
                               (unsigned char) ch1,
                               (unsigned char) ch2,
                               tout,
                               mode,
                               packlen);
        }

        break;
    case 'F':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%u", tout);
        }
        else
        {
            int flags, mode, packlen;
            char ch1, ch2;
            INT16U tout;

            token = strtok(NULL, ";"); /* MCSC Index */
            token = strtok(NULL, ";"); /* Force Transmit Index */

            if (isValidNum(token, 0, 65535) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_getDataPacking(portIdx,
                               &flags,
                               (INT8U *) &ch1,
                               (INT8U *) &ch2,
                               &tout,
                               &mode,
                               &packlen);

            tout = (u_short)atol(token);

            Scf_setDataPacking(portIdx,
                               flags,
                               (unsigned char) ch1,
                               (unsigned char) ch2,
                               tout,
                               mode,
                               packlen);
        }

        break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);

        return;
    }

    scm_send_packet(write, param, rep);
}
static void scm_config_fix(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
        //Get serial number
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int serial= 0;
            Scf_getSerialNumber(&serial);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", serial);
        }

        break;
        //Get firmware version
    case 'V':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
//		int i;
            char version[64];
            char *token;
            sys_getVersionString(version, sizeof(version));

            token = strtok(version, " ");
#if 0
            for (i = 0; i < 64; i ++)
            {
                if (version[i] == 0x20)
                { /* space = 0x20 */
                    break;
                }
            }
            memset(&version[i], 0, 64 - i);
#endif
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", token);
        }

        break;
        //Get Firmware Build Time
    case 'B':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char *token;
            char version[64];
            sys_getVersionString(version, sizeof(version));

            token = strtok(version, " ");

            token = strtok(NULL, " ");

            token = strtok(NULL, " ");

            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", token);
        }

        break;
        //Get MAC Address
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char mac[80];

            sys_getmacaddr("eth0", mac);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", mac);
        }
        break;

    case 'W':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char mac[80];

            sys_getmacaddr("eth1", mac);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", mac);
        }
        break;

    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}

static void scm_config_op_RealCOM(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     portIdx;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int alive;

            //alive = Scf_getTcpAliveCheck();
            alive = Scf_getPortAliveCheck(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", alive);
        }
        else
        {
            token = strtok(NULL, ";");

            stoh(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 99) <0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setPortAliveCheck(portIdx, atol(token));
            //Scf_setTcpAliveCheck((u_short) atol(token));
        }
        break;
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int maxconn;

            maxconn = Scf_getMaxConns(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", maxconn);
        }
        else
        {
            token = strtok(NULL,";");
            token = strtok(NULL,";");

            if (isValidNum(token, 1, 8) < 0) /* SPEC. is 8 */
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setMaxConns(portIdx, (u_char)atol(token));
        }
        break;
    case 'J':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = Scf_getSkipJamIP(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setSkipJamIP(portIdx, _atoi(token));
        }
        break;
    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = Scf_getAllowDrvCtrl(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setAllowDrvCtrl(portIdx, _atoi(token));
        }
        break;

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}

static void scm_config_op_rfc2217(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     portIdx;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int alive;

            //alive = Scf_getTcpAliveCheck();
            alive = Scf_getPortAliveCheck(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", alive);
        }
        else
        {
            token = strtok(NULL, ";");

            stoh(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 99) <0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setPortAliveCheck(portIdx, atol(token));
            //Scf_setTcpAliveCheck((u_short) atol(token));
        }
#endif
        break;
    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            INT16U tcpport, cmdport;

            Scf_getTcpServer(portIdx, &tcpport, &cmdport);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%u", tcpport);
        }
        else
        {
            u_short lPort = 0, cmdPort = 0;

            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isValidNum(token, 1, 65535) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_getTcpServer(portIdx, &lPort, &cmdPort);
            lPort = _atoi(token);
            Scf_setTcpServer(portIdx, lPort, cmdPort);
        }
        break;

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}

static void scm_config_op_TCP(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     portIdx=0, iTemp, iCnt;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif
#if 0
    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);
#endif
    token = strtok(pkt->param, ";");
    if ( (pkt->cmd[1] != 'B') && (pkt->cmd[1] != 'H') )
    {
        if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        portIdx = _atoi(token);
    }
#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int alive;

            //alive = Scf_getTcpAliveCheck();
            alive = Scf_getPortAliveCheck(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", alive);
        }
        else
        {
            token = strtok(NULL, ";");

            stoh(token);

            token = strtok(NULL, ";");

            if (isValidNum(token, 0, 99) <0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setPortAliveCheck(portIdx, atol(token));
            //Scf_setTcpAliveCheck((u_short) atol(token));
        }
#endif
        break;
    case 'J':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = Scf_getSkipJamIP(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setSkipJamIP(portIdx, _atoi(token));
        }
        break;
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int maxconn;

            maxconn = Scf_getMaxConns(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", maxconn);
        }
        else
        {
            token = strtok(NULL,";");
            token = strtok(NULL,";");

            if (isValidNum(token, 1, 8) < 0) /* SPEC. is 8 */
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setMaxConns(portIdx, (u_char)atol(token));
        }
        break;

    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            INT16U tcpport, cmdport;

            Scf_getTcpServer(portIdx, &tcpport, &cmdport);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%u", tcpport);
        }
        else
        {
            INT16U tcpport, cmdport;

            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isValidPort(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_getTcpServer(portIdx, &tcpport, &cmdport);
            Scf_setTcpServer(portIdx, (u_short)atol(token), cmdport);
        }
        break;

    case 'O':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            INT16U tcpport, cmdport;

            Scf_getTcpServer(portIdx, &tcpport, &cmdport);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%u", cmdport);
        }
        else
        {
            u_short cmd_port, tcpport;

            token = strtok(NULL,";");
            token = strtok(NULL,";");

            if (isValidNum(token, 1, 65535) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_getTcpServer(portIdx, &tcpport, &cmd_port);
            cmd_port = _atoi(token);
            Scf_setTcpServer(portIdx, tcpport, cmd_port);
        }
        break;

    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = Scf_getAllowDrvCtrl(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            token = strtok(NULL,";");
            token = strtok(NULL,";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setAllowDrvCtrl(portIdx, _atoi(token));
        }
        break;

    case 'V':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int time;

            time = Scf_getInactivityTime(portIdx);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", time);
        }
        else
        {
            token=strtok(NULL,";");
            token=strtok(NULL,";");

            if (isValidNum(token,0,65535) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setInactivityTime(portIdx, _atoi(token));
        }
        break;

    case 'C':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int connectionctrl;
            int ret;

            connectionctrl = Scf_getTcpClientMode(portIdx);
            ret = 0;
            switch (connectionctrl)
            {
            case DCF_CLI_MODE_ON_STARTUP | DCF_CLI_MODE_OFF_NONE:
                ret = 0;
                break;
            case DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_NONE:
                ret = 1;
                break;
            case DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_INACT:
                ret = 2;
                break;
            case DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_DSROFF:
                ret = 3;
                break;
            case DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_NONE:
                ret = 4;
                break;
            case DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_DCDOFF:
                ret = 5;
                break;
            case DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_NONE:
                ret = 6;
                break;
            default :
                ret = 0;
                break;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", ret);
        }
        else
        {
            int ret;

            ret = -1;
            token = strtok(NULL,";");
            token = strtok(NULL,";");

            if (isValidNum(token,0,6) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            switch (atol(token))
            {
            case 0:
                ret = DCF_CLI_MODE_ON_STARTUP | DCF_CLI_MODE_OFF_NONE;
                break;
            case 1:
                ret = DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_NONE;
                break;
            case 2:
                ret = DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_INACT;
                break;
            case 3:
                ret = DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_DSROFF;
                break;
            case 4:
                ret = DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_NONE;
                break;
            case 5:
                ret = DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_DCDOFF;
                break;
            case 6:
                ret = DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_NONE;
                break;
            }
            if (ret)
                Scf_setTcpClientMode(portIdx, ret);
        }
        break;
    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int idx;
            char dhost[40+1];
            INT16U dport, lport;

            token=strtok(NULL,";");

            token=strtok(NULL,";");
            if (isValidNum(token, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            idx = _atoi(token) + 1;
            Scf_getTcpClientHost(portIdx,
                                 idx,
                                 dhost,
                                 sizeof(dhost),
                                 &dport,
                                 &lport);

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s:%d",dhost,dport);
        }
        else
        {
            int idx;
            char DIP[DCF_IP_DNS_LEN + 1];
            u_short dport, lport;

            token = strtok(NULL,";"); /* MCSC Index */

            token = strtok(NULL,";"); /* Dest Address Index */

            memset(DIP,0, DCF_IP_DNS_LEN + 1);

            if (isValidNum(token, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            idx = _atoi(token) + 1;

            Scf_getTcpClientHost(portIdx,
                                 idx,
                                 DIP,
                                 sizeof(DIP),
                                 &dport,
                                 &lport);

            token = strtok(NULL,";"); /* IP */
            if (*token==':')	// no destioant ip
            {
                memset(DIP,0, 15);
                token++;
            }
            else
            {
                token = strtok(token,":"); /* IP Port */
                if (memcmp(token,"255.255.255.255", 15) == 0)
                {
                    memset(DIP, 0, 15);
                }
                else
                {
                    memcpy(DIP, token, 15);
                }
                token = strtok(NULL,":");
            }

            if (isValidPort(token)<0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            dport = atol(token);

            Scf_setTcpClientHost(portIdx,
                                 idx,
                                 DIP,
                                 _strlen(DIP),
                                 dport,
                                 lport);

        }
        break;
    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int idx;
            char dhost[40+1];
            INT16U dport, lport;

            token=strtok(NULL,";");

            token=strtok(NULL,";");
            if (isValidNum(token, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            idx = _atoi(token) + 1;
            Scf_getTcpClientHost(portIdx,
                                 idx,
                                 dhost,
                                 sizeof(dhost),
                                 &dport,
                                 &lport);

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d",lport);
        }
        else
        {
            int idx;
            char dhost[40+1];
            INT16U dport, lport;

            token = strtok(NULL,";"); /* MCSC Index */
            token = strtok(NULL,";"); /* Local Port Index */
            if (isValidNum(token, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            idx = _atoi(token) + 1;

            token = strtok(NULL,";"); /* Port Number Index */

            if (isValidNum(token, 1, 65535) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_getTcpClientHost(portIdx,
                                 idx,
                                 dhost,
                                 sizeof(dhost),
                                 &dport,
                                 &lport);
            lport = _atoi(token);

            Scf_setTcpClientHost(portIdx,
                                 idx,
                                 dhost,
                                 sizeof(dhost),
                                 dport,
                                 lport);
        }
        break;
        /* perry add region start */
    case 'B':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            iTemp = Scf_getTurboRoaming();

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", iTemp);
        }
        else
        {// pkt->op == OP_SETCONF
            if (isValidNum(token, 0, TURBO_ROAM_OP_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            iTemp = _atoi(token);
            if ( Scf_setTurboRoaming(iTemp) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            }
        }
        break;

    case 'H':
        if (isValidNum(token, 1, SCAN_CHANNEL_NUM) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            /* perry 2 modify start */
            portIdx = _atoi(token);	// portIdx is scan channel
            iTemp = Scf_getScanChannel(portIdx);	// iTemp is channel number of scan channel "portIdx"
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", iTemp);
        }
        else
        {// pkt->op == OP_SETCONF
            /* perry 3 modify start */
            iCnt = _atoi(token);	// iCnt is scan channel
            token = strtok(NULL, ";");
            if (isValidNum(token, 0, TURBO_ROAM_MAX_CH_NUM) < 0)
            {// channel more than max channel
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            iTemp = _atoi(token);	// iTemp is channel number
            Scf_setScanChannel(iCnt, iTemp);
        }
        break;

        /* perry add region end */

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}

typedef char *			fchar_t;
typedef unsigned char * 	fuchar_t;
typedef unsigned long * 	fulong_t;
int             Ssys_IPtostr(fuchar_t ip, fchar_t str)
{
    int     i, k;
    fchar_t	tmp;

    /* if is space(never setup), I will show out 255.255.255.255 */
    if ( *(fulong_t)ip == 0x20202020L )
        *(fulong_t)ip = 0xFFFFFFFFL;

    tmp = str;
    for ( i=0; i<4; i++, ip++ )
    {
        k = (int)(*ip);
        if ( k >= 100 )
        {
            *tmp++ = (k / 100) + '0';
            k = k % 100;
        }
        if ( (k >= 10) || (k != *ip) )
        {
            *tmp++ = (k / 10) + '0';
            k = k % 10;
        }
        *tmp++ = k + '0';
        *tmp++ = '.';
    }
    tmp--;
    *tmp = 0;
    return((int)(tmp - str));
}

u_long zero2ff(u_long ip)
{
#if 0
    if (ip == 0L)
        return 0xFFFFFFFFL;
#else
    if (ip == 0L)
        return 0x00000000L;
#endif
    else
        return ip;
}
static	char	addr_buffer[16];
fchar_t _inet_ntoa(ulong addr)
{
    Ssys_IPtostr((fuchar_t)&addr, (fchar_t)addr_buffer);
    return(addr_buffer);
}

static void scm_config_op_UDP(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     portIdx;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");
    if (isValidNum(token, 1, SIO_MAX_PORTS) < 0)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    portIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
#if 1
    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int idx;
            int pno;
            INT32U bip, eip;
            char BIP[DCF_IP_DNS_LEN];
            char EIP[DCF_IP_DNS_LEN];
            memset(BIP,0,DCF_IP_DNS_LEN);
            memset(EIP,0,DCF_IP_DNS_LEN);

            token=strtok(NULL,";");

            token=strtok(NULL,";");
            if (isValidNum(token, 1, 4) < 0)
            {
                scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            idx = _atoi(token);
            Scf_getUdpS2E(portIdx, idx, &bip, &eip, &pno);
            strncpy(BIP, _inet_ntoa(zero2ff(bip)),DCF_IP_DNS_LEN);
            strncpy(EIP, _inet_ntoa(zero2ff(eip)),DCF_IP_DNS_LEN);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%s-%s:%d",BIP,EIP,pno);
        }
#if 1
        else
        {
            char *tmp;
            int idx;
            INT32U bip, eip;
            int pno;

            token = strtok(NULL,";"); /* MCSC Index */
            token = strtok(NULL,";"); /* Dest Addr Index */
            if (isValidNum(token, 1, 4) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            idx = _atoi(token);

            Scf_getUdpS2E(portIdx, idx, &bip, &eip, &pno);

            token = strtok(NULL,";"); /* IP Value */

            if (*token == ':')	// no destioant ip
            {
                bip = eip = 0xFFFFFFFFUL;
                token++;
            }
            else
            {
                tmp=strchr(token,'-');
                if (tmp==NULL)
                {
                    token=strtok(token,":");
                    if (isValidIP(token)<0)
                    {
                        scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                        return;
                    }
                    bip = inet_addr(token);
                    eip = 0xFFFFFFFFUL;
                }
                else
                {
                    *tmp='\0';	// bip-eip => bip\0eip
                    //DBG(("token3 = %s", token));
                    if (isValidIP(token)<0)
                    {
                        scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                        return;
                    }
                    bip=inet_addr(token);
                    //DBG(("tmp+1 = %s", tmp+1));
                    token=strtok(tmp+1,":");
                    if (isValidIP(tmp+1)<0)
                    {
                        scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                        return;
                    }
                    eip=inet_addr(tmp+1);
                    //DBG(("bip-eip\r\n"));
                }
                token=strtok(NULL,":");
            }
            //DBG(("pno token = %s\r\n", token));
            if (isValidPort(token)<0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            pno = _atoi(token);
            Scf_setUdpS2E(portIdx, idx, bip, eip, pno);
        }
#endif
        break;
#endif
    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int local_port;             /* local listen port */

            local_port = Scf_getUdpPort(portIdx);

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d",local_port);
        }
        else
        {
            token = strtok(NULL,";"); /* MCSC Index */
            token = strtok(NULL,";"); /* Local Listen Port Index */

            if (isValidPort(token) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setUdpPort(portIdx, (u_short)atol(token));
        }
        break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}

static void scm_config_access(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int     IpIdx;
#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM


    switch (pkt->cmd[1])
    {
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = Scf_getAccessibleIP();
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            if (isEnableDisable(pkt->param) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setAccessibleIP(_atoi(pkt->param));
        }

        break;

    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            u_long	ipaddr, netmask;
            int mode;
            char IPBuf[DCF_IP_DNS_LEN];
            char MaskBuf[DCF_IP_DNS_LEN];

	     memset(IPBuf, 0, DCF_IP_DNS_LEN);
	     memset(MaskBuf, 0, DCF_IP_DNS_LEN);
            token = strtok(pkt->param, ";");
            if (isValidNum(token, 1, 16) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            IpIdx = _atoi(token) - 1;
            Scf_getIPTable(IpIdx, &mode, &ipaddr, &netmask);
            _strcpy(IPBuf, _inet_ntoa(zero2ff(ipaddr)));
            _strcpy(MaskBuf, _inet_ntoa(zero2ff(netmask)));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d;%s;%s", mode, IPBuf,MaskBuf);
        }
        else
        {
            char *token;
//             	int count=0;
// 		int finish = 0;
//		int clean = 0, n;
            int mode;
            u_long	ipaddr, netmask;
            int index;
            int cmp;

            /* Get Access IP Index */
            token = strtok(pkt->param,";");
            index = atol(token);

            Scf_getIPTable(index - 1, &mode, &ipaddr, &netmask);

            if (isValidNum(token, 1, 16) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* Get Disabled Or Enabled */
            token = strtok(NULL, ";");
            if (isEnableDisable(token) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            mode = atol(token);

            /* Get IP */
            token = strtok(NULL, ";");

            if (token)
            {
                if (isValidIP(token) < 0 || isValidIP(token) == ERROR_4FF_IP)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                ipaddr = inet_addr(token);
            }
            else
            {
                ipaddr = 0;
            }
            /* Get NetMask */
            token = strtok(NULL, ";");

            if (token)
            {
                if (isValidNetmask(token)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(token, "0.0.0.0");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                netmask = inet_addr(token);
            }
            else
            {
                netmask = 0;
            }

            Scf_setIPTable(index - 1, mode, ipaddr, netmask);

#if 0
            char *result;
            int count=0;
            int finish = 0;
            int clean = 0, n;
            int mode;
            u_long	ipaddr, netmask;
            int index;

            if (pkt->param == NULL)
            {
                n = 0;
                break;
            }
            else
            {
                n = _strlen(pkt->param);
            }

            if (pkt->param[n-1] == ';')
                clean = 1;

            index = atol(pkt->param);

            Scf_getIPTable(index - 1, &mode, &ipaddr, &netmask);

            result = strtok(pkt->param,";");

            if (isValidNum(result, 1, 16) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            while (result!=NULL && count<4)
            {
                switch (count)
                {
                case 0://index has assigned
                    break;
                case 1://mode
                    if (isEnableDisable(result)<0)
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    mode = atol(result);

                    break;
                case 2://ip
#if 0

#endif
                    if (result)
                    {
                        //DBG(("_strlen(result) = %d\r\n", _strlen(result)));
                        if (_strlen(result))
                        {
                            ipaddr = inet_addr(result);
                        }
                        else
                        {
                            //DBG(("ip clean\r\n"));
                            ipaddr = 0xFFFFFFFFL;
                            finish = 1;
                        }
                    }
                    break;
                case 3://netmask
                    int cmp;

                    if (_strlen(result))
                    {
                        if (isValidNetmask(result)<0)
                        {
                            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                            return;
                        }
                        cmp = _strcmp(pkt->param, "0.0.0.0");
                        if (cmp == 0)
                        {
                            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                            return;
                        }
                        cmp = _strcmp(pkt->param, "255.255.255.255");
                        if (cmp == 0)
                        {
                            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                            return;
                        }
                        netmask = inet_addr(result);
                    }
                    else
                        netmask = 0;
                    break;
                }
                result=strtok(NULL,";");
                count++;
                if (finish)
                    break;
            }
            //DBG(("count = %d\r\n", count));
            if (count<2 && !finish)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            if (count == 2 && clean)
            {
                ipaddr = 0;
                netmask = 0;
            }
            if (count == 3 && clean)
                netmask = 0;

            Scf_setIPTable(index - 1, mode, ipaddr, netmask);
#endif
        }

        break;

    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}

static void scm_config_snmp(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
//    char *token;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
    //token = strtok(pkt->param, ";");

    switch (pkt->cmd[1])
    {
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;

            enable = 	Scf_getSNMPEnable();
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {
            int enable;

            if (isEnableDisable(pkt->param) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            enable = Scf_getSNMPEnable();

            Scf_setSNMPEnable(atol(pkt->param));
        }

        break;
    case 'U':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];

            Scf_getSNMPReadComm(buf, sizeof(buf));
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            if (pkt->param)
            {
                if (_strlen(pkt->param) > DCF_SNMP_CNAME_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSNMPReadComm(pkt->param,_strlen(pkt->param));
            }
            else
            {
                Scf_setSNMPReadComm("", 0);
            }
        }
        break;

    case 'N':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];

            Scf_getSNMPContact(buf, sizeof(buf));
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            if (pkt->param)
            {
                if (_strlen(pkt->param) > DCF_CONTACT_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSNMPContact(pkt->param, _strlen(pkt->param));
            }
            else
            {
                Scf_setSNMPContact("", 0);
            }
        }

        break;

    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];

            Scf_getServerLocation(buf, sizeof(buf));
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            if (_strlen(pkt->param) > DCF_LOCATION_LEN)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setServerLocation(pkt->param, _strlen(pkt->param));
        }

        break;
    case 'W':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];

            Scf_getSNMPWriteComm(buf, sizeof(buf));
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            if (pkt->param)
            {
                if (_strlen(pkt->param) > DCF_SNMP_CNAME_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSNMPWriteComm(pkt->param, _strlen(pkt->param));
            }
            else
            {
                Scf_setSNMPWriteComm("", 0);
            }
        }
        break;
    case 'V':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int opmode, version;

            version = 0;

            opmode = Scf_getSNMPVersion();

            if ((opmode & DCF_SNMP_VERSION_V1) && (opmode & DCF_SNMP_VERSION_V2C) &&
                    (opmode & DCF_SNMP_VERSION_V3))																					// enable v1
                version = 0;
            else if ((opmode & DCF_SNMP_VERSION_V1) && (opmode & DCF_SNMP_VERSION_V2C))																					// enable v2
                version = 1;
            else if ((opmode & DCF_SNMP_VERSION_V3))																					// enable v3
                version = 2;																												// enable v1/v2/v3
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", version);

        }
        else
        {
            int version;

            if (isValidNum(pkt->param, 0, 2) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            switch (_atoi(pkt->param))
            {
            case 0:
                version = DCF_SNMP_VERSION_V1 | DCF_SNMP_VERSION_V2C |
                          DCF_SNMP_VERSION_V3;
                break;
            case 1:
                version = DCF_SNMP_VERSION_V1 | DCF_SNMP_VERSION_V2C;

                break;
            case 2:
                version = DCF_SNMP_VERSION_V3;

                break;
            default:
                version = 0;

                scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
                return;
            }
            Scf_setSNMPVersion(version);
        }

        break;
    case 'E':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            if (_atoi(token))
            {
                /* index 1: Read /Write  */
                Scf_getSNMPRWUser(buf, sizeof(buf));
            }
            else
            {
                /* index 1: Read Only */

                Scf_getSNMPROUser(buf, sizeof(buf));
            }

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;
            int index;

            token = strtok(pkt->param, ";"); /* index */

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            index = _atoi(token);

            token = strtok(NULL, ";"); /* Value */
            if (token)
            {
                if (_strlen(token) > DCF_SNMP_USER_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                if (index) /* Read/ Write */
                    Scf_setSNMPRWUser(token, _strlen(token));
                else /* Write */
                    Scf_setSNMPROUser(token, _strlen(token));
            }
            else
            {
                if (index) /* Read/ Write */
                    Scf_setSNMPRWUser("", 0);
                else /* Write */
                    Scf_setSNMPROUser("", 0);
            }
        }

        break;
    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int mode;
            char *token;
            /* mode-> 0: disabled, 1: MD5, 2: SHA */

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            if (_atoi(token))
            {
                /* index 1: Read /Write  */
                mode = Scf_getSNMPRWAuth();
            }
            else
            {
                /* index 1: Read Only */
                mode = Scf_getSNMPROAuth();
            }
            // enable v1/v2/v3
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", mode);
        }
        else
        {
            int mode, index;
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(pkt->param, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(token);

            token = strtok(NULL, ";"); /* Value */

            if (isValidNum(token, 0, 2) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            mode = _atoi(token);

            if (index) /* Read/ Write */
                Scf_setSNMPRWAuth(mode);
            else /* Write */
                Scf_setSNMPROAuth(mode);


        }

        break;
    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            if (_atoi(token))
            {
                /* index 1: Read /Write  */
                Scf_getSNMPRWPasswd(buf, sizeof(buf));
            }
            else
            {
                /* index 1: Read Only */

                Scf_getSNMPROPasswd(buf, sizeof(buf));
            }

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;
            int index;

            token = strtok(pkt->param, ";"); /* index */

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            index = _atoi(token);

            token = strtok(NULL, ";"); /* Value */
            if (token)
            {
                if (_strlen(token) > DCF_SNMP_PASS_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                if (index) /* Read/ Write */
                    Scf_setSNMPRWPasswd(token, _strlen(token));
                else /* Write */
                    Scf_setSNMPROPasswd(token, _strlen(token));
            }
            else
            {
                if (index) /* Read/ Write */
                    Scf_setSNMPRWPasswd("", 0);
                else /* Write */
                    Scf_setSNMPROPasswd("", 0);
            }
        }

        break;
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int mode;
            char *token;
            /* mode-> 0: disabled, 1: DES, 2: AES */

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            if (_atoi(token))
            {
                /* index 1: Read /Write  */
                mode = Scf_getSNMPRWPrivMode();
            }
            else
            {
                /* index 1: Read Only */
                mode = Scf_getSNMPROPrivMode();
            }
            // enable v1/v2/v3
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", mode);
        }
        else
        {
            int mode, index;
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(pkt->param, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(token);

            token = strtok(NULL, ";"); /* Value */

            if (isValidNum(token, 0, 2) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            mode = _atoi(token);

            if (index) /* Read/ Write */
                Scf_setSNMPRWPrivMode(mode);
            else /* Write */
                Scf_setSNMPROPrivMode(mode);
        }

        break;
    case 'Y':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[80];
            char *token;

            token = strtok(pkt->param, ";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            if (_atoi(token))
            {
                /* index 1: Read /Write  */
                Scf_getSNMPRWPriv(buf, sizeof(buf));
            }
            else
            {
                /* index 1: Read Only */

                Scf_getSNMPROPriv(buf, sizeof(buf));
            }

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;
            int index;

            token = strtok(pkt->param, ";"); /* index */

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            index = _atoi(token);

            token = strtok(NULL, ";"); /* Value */
            if (token)
            {
                if (_strlen(token) > DCF_SNMP_PASS_LEN)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                if (index) /* Read/ Write */
                    Scf_setSNMPRWPriv(token, _strlen(token));
                else /* Write */
                    Scf_setSNMPROPriv(token, _strlen(token));
            }
            else
            {
                if (index) /* Read/ Write */
                    Scf_setSNMPRWPriv("", 0);
                else /* Write */
                    Scf_setSNMPROPriv("", 0);
            }
        }

        break;
    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[DCF_SNMPTRAP_SERVER_LENGTH+1];

            Scf_getSNMPTrap(buf, sizeof(buf));

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;

            token = strtok(pkt->param, ";");
            if (token)
            {
                if (_strlen(token) > DCF_SNMPTRAP_SERVER_LENGTH)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }

                Scf_setSNMPTrap(token, DCF_SNMPTRAP_SERVER_LENGTH);
            }
            else
            {
                Scf_setSNMPTrap("", 0);
            }
        }
        break;

    case 'O':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int version;

            version = Scf_getSNMPTrapVersion();
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", version);
        }
        else
        {
            int version;
            char *token;

            token = strtok(pkt->param, ";"); /* index */

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            version = _atoi(token);
            Scf_setSNMPTrapVersion(version);
        }
        break;

    case 'C':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char buf[DCF_SNMP_COMMUNITY+1];

            memset(buf, 0, DCF_SNMP_COMMUNITY+1);
            Scf_getSNMPTrapCommunity(buf, sizeof(buf));

            rep->wdx +=sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {
            char *token;

            token = strtok(pkt->param, ";");
            if (token)
            {
                if (_strlen(token) > DCF_SNMP_COMMUNITY)
                {
                    scm_error_reply(write,param,pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                Scf_setSNMPTrapCommunity(token, DCF_SNMP_COMMUNITY);
            }
            else
            {
                Scf_setSNMPTrapCommunity("", 0);
            }
        }
        break;

    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}
/*---------------------------------------*
#define DIO_MODE_OUTPUT     0
#define DIO_MODE_INPUT      1
#define DIO_HIGH    1
#define DIO_LOW     0
---------------------------------------*/
static void scm_config_pin(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token, buf[PROFILE_NAME_BUF_SIZE];
    int pinIdx, iTemp, iParam_Byte0;
    struct wpa_config wpa_cfg;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif

    token = strtok(pkt->param, ";");

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'M':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int mode;

            /* 1~8 PIN 1~8,  9 is mean all Pin */
            if (isValidNum(token, 0, 7) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            pinIdx = _atoi(token);

            mode = Scf_getSDioMode(pinIdx);
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", mode);

        }
        else
        {
            int mode;

            /* 1~8 PIN 1~8,  9 is mean all Pin */
            if (isValidNum(token, 0, 7) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            pinIdx = _atoi(token);

            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            mode = _atoi(token);

            if (pinIdx == 7)
            {
//			if ((Scf_getScmTrigger() == 1)  && mode == 0)
                if ((Scf_getScmTrigger() == 1))
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
            }

            Scf_setSDioMode(pinIdx,mode);

        }
        break;
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int state;
            /* 1~8 PIN 1~8,  9 is mean all Pin */
            if (isValidNum(token, 0, 7) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            pinIdx = _atoi(token);

            state = Scf_getSDioState(pinIdx);
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", state);
        }
        else
        {
            int state, mode;

            /* 1~8 PIN 1~8,  9 is mean all Pin */
            if (isValidNum(token, 0, 7) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            pinIdx = _atoi(token);

            token=strtok(NULL,";");

            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            state = _atoi(token);

            mode = Scf_getSDioMode(pinIdx);
#if 1
            if (mode == 0) // input
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
#endif
            Scf_setSDioState(pinIdx, state);

        }
        break;
        /* perry add region start */
    case 'N':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            iTemp = Scf_getNetworkType();

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", iTemp);
        }
        else
        {// pkt->op == OP_SETCONF
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            iParam_Byte0 = _atoi(token);
            if ( Scf_setNetworkType(iParam_Byte0) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            }
        }
        break;

    case 'P':
        if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            iParam_Byte0 = _atoi(token);
            memset(buf, 0, PROFILE_NAME_BUF_SIZE);
            Scf_getProfileName(iParam_Byte0, buf);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", buf);
        }
        else
        {// pkt->op == OP_SETCONF
            iParam_Byte0 = _atoi(token);
            token = strtok(NULL,";");
            if ( token )
            {// profile name is not empty
                if ( WLAN_PROF_NAME_LEN < _strlen(token) )
                {// more than max length
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                if ( Scf_setProfileName(iParam_Byte0, token) < 0 )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                }
            }
            else
            {
                if ( Scf_setProfileName(iParam_Byte0, "") < 0 )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                }
            }
        }
        break;

    case 'O':
        if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            iParam_Byte0 = _atoi(token);
            iTemp = Scf_getProfileOPMode(iParam_Byte0);

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", iTemp);
        }
        else
        {// pkt->op == OP_SETCONF
            iParam_Byte0 = _atoi(token);
            token = strtok(NULL,";");
            if (isValidNum(token, 0, WLAN_OP_MODE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            iTemp = _atoi(token);
            Scf_setProfileOPMode(iParam_Byte0, iTemp);
        }
        break;

    case 'D':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            iParam_Byte0 = _atoi(token);
            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.ssid);
        }
        else
        {// pkt->op == OP_SETCONF
            iParam_Byte0 = _atoi(token);
            token = strtok(NULL,";");
            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {// get config error
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( token )
            {// SSID is not empty
                if ( WLAN_SSID_LENGTH < _strlen(token) )
                {// more than max length
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                _strcpy(wpa_cfg.ssid, token);
            }
            else
            {
                _strcpy(wpa_cfg.ssid, "");
            }

            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg)<0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;
        /* perry add region end */

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;

    }

    scm_send_packet(write, param, rep);
}

static void scm_config_control(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
//    int pinIdx;


#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif
//	token = strtok(pkt->param, ";");
    /* 1~8 PIN 1~8,  9 is mean all Pin */

//	pinIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'T':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int trigger;

            trigger = Scf_getScmTrigger();
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", trigger);
        }
        else
        {
            int trigger;

            token = strtok(pkt->param, ";");
            trigger = _atoi(token);
            if (isValidNum(pkt->param, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            Scf_setScmTrigger(trigger);
        }
        break;
    case 'C':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            u_char scmd[3];

            Scf_getScmChar(&scmd[0], &scmd[1], &scmd[2]);
            rep->wdx += sprintf(&rep->param[rep->wdx],"%02X;%02X;%02X",
                                scmd[0],scmd[1],scmd[2]);
        }
        else
        {
            unsigned char scmd[3];
            int trigger;

            trigger = Scf_getScmTrigger();

            token = strtok(pkt->param,";");
            if (isValidHex(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            scmd[0] = htoi(token);

            token = strtok(NULL,";");
            if (isValidHex(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            scmd[1] = htoi(token);

            token = strtok(NULL,";");
            if (isValidHex(token) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            scmd[2] = htoi(token);
            Scf_setScmChar(scmd[0], scmd[1], scmd[2]);

        }
        break;

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;

    }

    scm_send_packet(write, param, rep);
}

static void scm_config_other(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
//    int pinIdx;


#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif
//	token = strtok(pkt->param, ";");
    /* 1~8 PIN 1~8,  9 is mean all Pin */

//	pinIdx = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int	arp_en, arp_period;

            arp_en = Scf_getGratuitousArp_en();
            Scf_getGratuitousArp_period(&arp_period);
            if (arp_en)
            {
                rep->wdx += sprintf(&rep->param[rep->wdx],"1;%d", arp_period);
            }
            else
                rep->wdx += sprintf(&rep->param[rep->wdx],"0");
        }
        else
        {
            int	arp_en, arp_period;

            token = strtok(pkt->param, ";"); /* ARP Enable Or Disabled */

            if (isValidNum(pkt->param, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            arp_en = _atoi(token);

            token = strtok(NULL,";");/* ARP Period */
            if (token)
            {
                if (isValidNum(token, 10, 1000) < 0)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }

                arp_period = _atoi(token);
                Scf_setGratuitousArp_en(arp_en);
                if (arp_en)
                    Scf_setGratuitousArp_period(arp_period);
            }
            else
            {
                Scf_setGratuitousArp_en(arp_en);
            }
        }
        break;
    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int net_type;
            /* 0:Auto, 1:Dix, 2:Eth0, 3:Eth1*/
            net_type = Scf_getActiveInterface();

            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", net_type);
        }
        else
        {
            int	net_type;

            token = strtok(pkt->param, ";"); /* ARP Enable Or Disabled */

            if (isValidNum(pkt->param, 0, 3) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }

            net_type = _atoi(token);

            Scf_setActiveInterface(net_type);
        }
        break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;

    }

    scm_send_packet(write, param, rep);
}

static void scm_config_wireless(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'B':
        if (pkt->op == OP_GETCONF)
        {
#define BSSID_TEMPPATH1   "/var/bssid_buf1"
#define BSSID_TEMPPATH2   "/var/bssid_buf2"
#define BSSID_TEMPPATH3   "/var/bssid_buf3"
            char cmd[128];
            char bssid[128];
            FILE *fp;

            memset(cmd, 0, sizeof(cmd));
            memset(bssid, 0, sizeof(bssid));
            sprintf(cmd, "wpa_cli status | grep bssid > %s", BSSID_TEMPPATH1);
            system(cmd);
            sprintf(cmd, "awk '/bssid/' %s | awk -F= '{print $2}' > %s", 
                BSSID_TEMPPATH1, BSSID_TEMPPATH2);
            system(cmd);
            sprintf(cmd, "echo -n `cat %s` > %s", BSSID_TEMPPATH2, BSSID_TEMPPATH3);
            system(cmd);

            fp = fopen(BSSID_TEMPPATH3, "r");
            if (fp != NULL)
            {
                fgets(bssid, sizeof(bssid), fp);
                fclose(fp);
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", bssid);
        }
        break;
        
        // Get or Set eth0 DHCP status
    case 'C':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int btype;

            Scf_getIPConfig(IF_NUM_DEV_ETH1, &btype);
            if (btype == IP_CONFIG_BOOTP)
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", IP_CONFIG_BOOTP + 1);
            else
                rep->wdx += sprintf(&rep->param[rep->wdx],"%d", btype);
        }
        else
        {
            int cmp;
            if (pkt->param)
            {
                if (isValidNum(pkt->param, 0, 3)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "2");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "3");
                if (cmp == 0)
                {
                    Scf_setIPConfig(IF_NUM_DEV_ETH1, IP_CONFIG_BOOTP	);
                    /* For Compatible */
                    break;
                }
                Scf_setIPConfig(IF_NUM_DEV_ETH1, _atoi(pkt->param));
            }
            else
            {
                Scf_setIPConfig(IF_NUM_DEV_ETH1, 0);
            }
        }

#endif
        break;
#if 1
        // Get or Set eth0 IP Address
    case 'I':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long ip;
            struct in_addr in;

            Scf_getIfaddr(IF_NUM_DEV_ETH1, &ip);
            in.s_addr = ip;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;

            if (pkt->param)
            {
                if (isValidIP(pkt->param) < 0 || isValidIP(pkt->param) == ERROR_4FF_IP)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }

                inet_aton(pkt->param, &in);
                Scf_setIfaddr(IF_NUM_DEV_ETH1, in.s_addr);
            }
            else
            {
                Scf_setIfaddr(IF_NUM_DEV_ETH1, 0);
            }
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }

#endif
        break;
        // Get or Set eth0 NetMask
    case 'M':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long mask;
            struct in_addr in;

            Scf_getNetmask(IF_NUM_DEV_ETH1, &mask);
            in.s_addr = mask;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;
            int cmp;

            if (pkt->param)
            {
                if (isValidNetmask(pkt->param)<0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "0.0.0.0");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                cmp = _strcmp(pkt->param, "255.255.255.255");
                if (cmp == 0)
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                inet_aton(pkt->param, &in);
                Scf_setNetmask(IF_NUM_DEV_ETH1, in.s_addr);
            }
            else
            {
                Scf_setNetmask(IF_NUM_DEV_ETH1, 0);
            }
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }

#endif
        break;
        // Get or Set eth0 Gateway
    case 'G':
#if 1
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            unsigned long mask;
            struct in_addr in;

            Scf_getGateway(IF_NUM_DEV_ETH1, &mask);
            in.s_addr = mask;
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", inet_ntoa(in));
        }
        else
        {
            struct in_addr in;
            int cmp;
            /* IsValidIP()-> 0.0.0.0 is invalid */
            if (pkt->param)
            {
                if (_strlen(pkt->param))
                {
                    if (isValidIP(pkt->param)<0)
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    cmp = _strcmp(pkt->param, "255.255.255.255");

                    if (cmp == 0)
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    inet_aton(pkt->param, &in);
                    Scf_setGateway(IF_NUM_DEV_ETH1, in.s_addr);
                }
                else
                    Scf_setGateway(IF_NUM_DEV_ETH1, 0);
            }
            else
            {
                Scf_setGateway(IF_NUM_DEV_ETH1, 0);
            }
            //Scf_setIPConfig(IF_NUM_DEV_ETH0, _atoi(pkt->param));
        }


#endif
        break;
#endif
    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}

//// Added by James.
#define STRLEN_LIM_SMTPSERVER       40
#define STRLEN_LIM_MAIL_USERNAME    16
#define STRLEN_LIM_MAIL_PASSWD      16
#define STRLEN_LIM_MAIL_FROM        64
#define STRLEN_LIM_MAIL_TO          64

static void scm_config_mail(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'S':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char str[40+1];
            Scf_getMailServer(str, sizeof(str));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", str);
        }
        else
        {    //set
            token = strtok(pkt->param, ";");

            if (token)
            {
                if (_strlen(token) > STRLEN_LIM_SMTPSERVER)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setMailServer(token, _strlen(token));
            }
            else
            {
                Scf_setMailServer("", 0);
            }
        }
        break;
    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            int enable;
            enable = Scf_getSMTP_Auth();
            rep->wdx +=sprintf(&rep->param[rep->wdx],"%d", enable);
        }
        else
        {    //set
            token = strtok(pkt->param, ";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_setSMTP_Auth(_atoi(token));
        }
        break;
    case 'U':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char str[80];
            Scf_getSMTP_AuthName(str, sizeof(str));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", str);
        }
        else
        {    //set
            token = strtok(pkt->param, ";");

            if (token)
            {
                if (_strlen(token) > STRLEN_LIM_MAIL_USERNAME)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSMTP_AuthName(token, _strlen(token));
            }
            else
            {
                Scf_setSMTP_AuthName("", 0);
            }
        }
        break;
    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char str[80];
            Scf_getSMTP_AuthPass(str, sizeof(str));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", str);
        }
        else
        {    //set
            token = strtok(pkt->param, ";");

            if (token)
            {
                if (_strlen(token) > STRLEN_LIM_MAIL_PASSWD)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSMTP_AuthPass(token, _strlen(token));
            }
            else
            {
                Scf_setSMTP_AuthPass("", 0);
            }
        }
        break;
    case 'F':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char str[80];
            Scf_getSMTP_FromAddr(str, sizeof(str));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", str);
        }
        else
        {    //set
            token = strtok(pkt->param, ";");

            if (token)
            {
                if (_strlen(token)>STRLEN_LIM_MAIL_FROM)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSMTP_FromAddr(token, _strlen(token));
            }
            else
            {
                Scf_setSMTP_FromAddr("", 0);
            }
        }
        break;
    case 'T':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            char str[80];
            if (isValidNum(pkt->param, 1, 4) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            Scf_getSMTP_ToAddr(_atoi(pkt->param), str, sizeof(str));
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", str);
        }
        else
        {    //set
            int index;
            token = strtok(pkt->param, ";");
            if (isValidNum(token, 1, 4) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            index = _atoi(token);

            token = strtok(NULL,";");
            if (token)
            {
                if (_strlen(token) > STRLEN_LIM_MAIL_TO)
                {
                    scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                    return;
                }
                Scf_setSMTP_ToAddr(index, token, _strlen(token));
            }
            else
            {
                Scf_setSMTP_ToAddr(index, "", 0);
            }
        }
        break;
    default:
        scm_error_reply(write,param, pkt, ST_INVALID_COMMAND);
        return;
    }
    scm_send_packet(write, param, rep);
}
////End
/* perry add region start */
static void scm_config_security(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int iParam_Byte0, n, iTemp, iTemp2;
    struct wpa_config wpa_cfg;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif
    token = strtok(pkt->param, ";");
    /* 1~8 PIN 1~8,  9 is mean all Pin */

    iParam_Byte0 = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'A':
        if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            n = Scf_getAuthentication(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {// pkt->op == OP_SETCONF
            token = strtok(NULL,";");

            if ( iParam_Byte0==DWLAN_BSSTYPE_ADHOC )
            {
                n = ADHOC_SECURYIT_AUTHE_TOT_NUM;
            }
            else
            {
                n = INFRA_SECURYIT_AUTHE_TOT_NUM;
            }
            if (isValidNum(token, 1, n) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            Scf_setAuthentication(iParam_Byte0, n, NULL);
        }
        break;

    case 'E':
        if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            n=Scf_getEncryption(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {	// the else some error number may modify
            token = strtok(NULL,";");
            if (isValidNum(token, 1, SECURITY_ENCRY_TOT_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);	// encryption number

            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            iTemp = Scf_getAuthentication(iParam_Byte0);

            switch (iTemp)
            {// iTmep is Authentication
            case SECURITY_AUTH_OPEN: // open system
                if (n==SECURITY_ENCRY_DISABLE)
                {
                    wpa_cfg.auth_alg=DCF_WPA_AUTH_ALG_OPEN;
                    wpa_cfg.wep_tx_keyidx=-1;
                    memset(wpa_cfg.wep_key[0], 0, DCF_WLAN_WEP_KEY_NUM*sizeof(wpa_cfg.wep_key[0]));
                }
                else if ( n==SECURITY_ENCRY_WEP )
                {
                    wpa_cfg.wep_tx_keyidx= 1;
                }
                else if ( n<=SECURITY_ENCRY_TOT_NUM )
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                else
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                wpa_cfg.key_mgmt = DCF_WLAN_KEYMGMT_NONE;
                wpa_cfg.pairwise = 0;
                wpa_cfg.group = 0;
                wpa_cfg.eap = 0;
                wpa_cfg.phase1 = 0;
                wpa_cfg.phase2 = 0;
                memset(wpa_cfg.identity, 0, sizeof(wpa_cfg.identity));
                memset(wpa_cfg.password, 0, sizeof(wpa_cfg.password));
                memset(wpa_cfg.anonymous_identity, 0, sizeof(wpa_cfg.anonymous_identity));
                memset(wpa_cfg.psk, 0, sizeof(wpa_cfg.psk));
                break;
            case SECURITY_AUTH_SHARED: // shared key
                if ( n!=SECURITY_ENCRY_WEP )
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                wpa_cfg.key_mgmt = DCF_WLAN_KEYMGMT_NONE;
                wpa_cfg.pairwise = 0;
                wpa_cfg.group = 0;
                wpa_cfg.eap = 0;
                wpa_cfg.phase1 = 0;
                wpa_cfg.phase2 = 0;
                memset(wpa_cfg.identity, 0, sizeof(wpa_cfg.identity));
                memset(wpa_cfg.password, 0, sizeof(wpa_cfg.password));
                memset(wpa_cfg.anonymous_identity, 0, sizeof(wpa_cfg.anonymous_identity));
                memset(wpa_cfg.psk, 0, sizeof(wpa_cfg.psk));
                break;
            case SECURITY_AUTH_WPA: // WPA
            case SECURITY_AUTH_WPA2: // WPA2
                wpa_cfg.wep_tx_keyidx=-1;
                memset(wpa_cfg.wep_key[0], 0, DCF_WLAN_WEP_KEY_NUM*sizeof(wpa_cfg.wep_key[0]));
                memset(wpa_cfg.psk, 0, sizeof(wpa_cfg.psk));
                if ( n==SECURITY_ENCRY_TKIP )
                {
                    wpa_cfg.pairwise = DCF_WLAN_PAIRWISE_TKIP;
                    wpa_cfg.group = DCF_WLAN_GROUP_TKIP;
                }
                else if ( n==SECURITY_ENCRY_AES )
                {
                    wpa_cfg.pairwise = DCF_WLAN_PAIRWISE_CCMP;
                    wpa_cfg.group = DCF_WLAN_GROUP_CCMP;
                }
                else
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                break;

            case SECURITY_AUTH_WPA_PSK: // WPA-PSK
            case SECURITY_AUTH_WPA2_PSK: // WPA2-PSK
                wpa_cfg.wep_tx_keyidx=-1;
                wpa_cfg.eap = 0;
                wpa_cfg.phase1 = 0;
                wpa_cfg.phase2 = 0;
                memset(wpa_cfg.wep_key[0], 0, DCF_WLAN_WEP_KEY_NUM*sizeof(wpa_cfg.wep_key[0]));
                memset(wpa_cfg.identity, 0, sizeof(wpa_cfg.identity));
                memset(wpa_cfg.password, 0, sizeof(wpa_cfg.password));
                memset(wpa_cfg.anonymous_identity, 0, sizeof(wpa_cfg.anonymous_identity));
                if ( n==SECURITY_ENCRY_TKIP )
                {
                    wpa_cfg.pairwise = DCF_WLAN_PAIRWISE_TKIP;
                    wpa_cfg.group = DCF_WLAN_GROUP_TKIP;
                }
                else if ( n==SECURITY_ENCRY_AES )
                {
                    wpa_cfg.pairwise = DCF_WLAN_PAIRWISE_CCMP;
                    wpa_cfg.group = DCF_WLAN_GROUP_CCMP;
                }
                else
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                break;
            default:
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }

            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            n=Scf_getEapMethod(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            if ( iParam_Byte0!=DWLAN_BSSTYPE_INFRA )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is Authentication
            iTemp = Scf_getAuthentication(iParam_Byte0);
            if ( (iTemp!=SECURITY_AUTH_WPA) && (iTemp!=SECURITY_AUTH_WPA2) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }

            if ( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 1, EAP_MODE_TOT_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);	// encryption number
            wpa_cfg.auth_alg = 0;
            switch (n)
            {
            case EAP_TLS_MODE:
                wpa_cfg.eap = DCF_WLAN_EAP_TLS;
                break;
            case EAP_PEAP_MODE:
                wpa_cfg.eap = DCF_WLAN_EAP_PEAP;
                break;
            case EAP_TTLS_MODE:
                wpa_cfg.eap = DCF_WLAN_EAP_TTLS;
                break;
            case EAP_LEAP_MODE:
                wpa_cfg.eap = DCF_WLAN_EAP_LEAP;
                wpa_cfg.auth_alg = DCF_WPA_AUTH_ALG_LEAP;
                break;
            }
            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

    case 'T':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            n=Scf_getTunnAuth(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            if ( iParam_Byte0!=DWLAN_BSSTYPE_INFRA )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is authentication
            iTemp = Scf_getAuthentication(iParam_Byte0);
            if ( (iTemp!=SECURITY_AUTH_WPA) && (iTemp!=SECURITY_AUTH_WPA2) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is EAP method
            iTemp=Scf_getEapMethod(iParam_Byte0);
            if ( (iTemp!=EAP_PEAP_MODE) && (iTemp!=EAP_TTLS_MODE) )
            {// (EAP method != EAP_PEAP_MODE) && (EAP method != EAP_TTLS_MODE)
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            if ( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, DCF_WLAN_PAHSE2_GTC, DCF_WLAN_PAHSE2_EAP_MSCHAPV2) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            // n is Tunneled authentication
            if ( iTemp==EAP_PEAP_MODE )
            {// EAP method == EAP_PEAP_MODE
                if ( (n<DCF_WLAN_PAHSE2_GTC) ||(n>DCF_WLAN_PAHSE2_MSCHAPV2)  )
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                wpa_cfg.phase2 = n;
            }
            else
            {// EAP method == EAP_TTLS_MODE
                if ( (n<DCF_WLAN_PAHSE2_MSCHAPV2) ||(n>DCF_WLAN_PAHSE2_EAP_MSCHAPV2)  )
                {
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                wpa_cfg.phase2 = n;
            }
            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

    case 'U':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            token = strtok(NULL, ";");
            n = _atoi(token);
            if (isValidNum(token, 1, USERNAME_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( n==NETWORK_USERNAME )
            {
                if ( wpa_cfg.identity == NULL )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.identity);
            }
            else
            {// n==anonymous usename
                if ( wpa_cfg.anonymous_identity == NULL )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
                rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.anonymous_identity);
            }
        }
        else
        {
            if ( iParam_Byte0 != DWLAN_BSSTYPE_INFRA )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is authentication
            iTemp = Scf_getAuthentication(iParam_Byte0);
            if ( (iTemp!=SECURITY_AUTH_WPA) && (iTemp!=SECURITY_AUTH_WPA2) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            if ( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 1, USERNAME_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter Byte1
            iTemp = _atoi(token);
            token = strtok(NULL,";");
            if ( token )
            {// string is not empty
                n =  _strlen(token);
                // n is trring length
                if ( n>DCF_WLAN_WPA_USERNAME_LEN )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
            }
            else
            {// string is empty
                n = 0;
            }
            if (iTemp==NETWORK_USERNAME )
            {
                memset(wpa_cfg.identity, 0 , (DCF_WLAN_WPA_USERNAME_LEN + 1) * sizeof(char));
                if ( n==0 )
                {// username is empty
                    memcpy(wpa_cfg.identity, "", n * sizeof(char));
                }
                else
                {
                    memcpy(wpa_cfg.identity, token, n * sizeof(char));
                }
            }
            else
            {// n==anonymous usename
                // iTemp is EAP method
                iTemp=Scf_getEapMethod(iParam_Byte0);
                if ( iTemp!=EAP_TTLS_MODE )
                {// (EAP method != EAP_TTLS_MODE)
                    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                    return;
                }
                memset(wpa_cfg.anonymous_identity, 0 , (DCF_WLAN_WPA_USERNAME_LEN + 1) * sizeof(char));
                if ( n==0 )
                {// username is empty
                    memcpy(wpa_cfg.anonymous_identity, "", n * sizeof(char));
                }
                else
                {
                    memcpy(wpa_cfg.anonymous_identity, token, n * sizeof(char));
                }
            }
            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

    case 'W':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.password);
        }
        else
        {
            if ( iParam_Byte0 != DWLAN_BSSTYPE_INFRA )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is authentication
            iTemp = Scf_getAuthentication(iParam_Byte0);
            if ( (iTemp!=SECURITY_AUTH_WPA) && (iTemp!=SECURITY_AUTH_WPA2) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            iTemp=Scf_getEapMethod(iParam_Byte0);
            if ( (iTemp<EAP_PEAP_MODE) || (iTemp>EAP_LEAP_MODE) )
            {// (EAP method < EAP_PEAP_MODE) || (EAP method > EAP_LEAP_MODE)
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            if ( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if ( token )
            {// password is not empty
                n =  _strlen(token);
                if ( n>DCF_WLAN_WPA_PASSWORD_LEN )
                {
                    scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                    return;
                }
            }
            else
            {
                n = 0;
            }
            memset(wpa_cfg.password, 0 , (DCF_WLAN_WPA_PASSWORD_LEN + 1) * sizeof(char));
            if ( n==0 )
            {// password is empty
                memcpy(wpa_cfg.password, "", n * sizeof(char));
            }
            else
            {
                memcpy(wpa_cfg.password, token, n * sizeof(char));
            }
            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

    case 'V':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = Scf_getServerCert(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            if ( iParam_Byte0 != DWLAN_BSSTYPE_INFRA )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            // iTemp is authentication
            iTemp = Scf_getAuthentication(iParam_Byte0);
            if ( (iTemp!=SECURITY_AUTH_WPA) && (iTemp!=SECURITY_AUTH_WPA2) )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            iTemp=Scf_getEapMethod(iParam_Byte0);
            if ( (iTemp<EAP_TLS_MODE) || (iTemp>EAP_TTLS_MODE) )
            {// (EAP method < EAP_TLS_MODE) || (EAP method > EAP_TTLS_MODE)
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            Scf_setServerCert(iParam_Byte0, n);

        }
        break;

    case 'L':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = Scf_getWEPkeylen(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n=Scf_getEncryption(iParam_Byte0);
            if ( n!=SECURITY_ENCRY_WEP )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 1, 2) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            Scf_setWEPkeylen(iParam_Byte0, n);
        }
        break;

    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = Scf_getWEPkeyidx(iParam_Byte0);
            if (n == 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
        }
        else
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n=Scf_getEncryption(iParam_Byte0);
            if ( n!=SECURITY_ENCRY_WEP )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, WEP_MIN_INDEX, WEP_MAX_INDEX) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            Scf_setWEPkeyidx(iParam_Byte0, n, NULL);
        }
        break;

    case 'F':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = Scf_getWEPKeyFormat(iParam_Byte0);
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n=Scf_getEncryption(iParam_Byte0);
            if ( n!=SECURITY_ENCRY_WEP )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 1, WEP_TOT_FORMAT) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n = _atoi(token);
            // n is format
            Scf_setWEPKeyFormat(iParam_Byte0, n);
        }
        break;

    case 'K':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            token = strtok(NULL, ";");
            n = _atoi(token);
            if (isValidNum(token, 1, WEP_KEY_TOT_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            if ( wpa_cfg.identity == NULL )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }

            rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.wep_key[n-1]);
        }
        else
        {
            if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            n=Scf_getEncryption(iParam_Byte0);
            if ( n!=SECURITY_ENCRY_WEP )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
            token = strtok(NULL,";");
            if (isValidNum(token, 1, WEP_KEY_TOT_NUM) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            if ( Scf_getWlanConfig(iParam_Byte0, &wpa_cfg) <0 )
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTmep is number of key
            iTemp = _atoi(token);
            token = strtok(NULL,";");
            // n is format
            n = Scf_getWEPKeyFormat(iParam_Byte0);
            // iTemp2 is key length
            iTemp2 =  Scf_getWEPkeylen(iParam_Byte0);
            ;
            if ( iTemp2==WEP_KEY_64BIT )
            {
                iTemp2 = 5;
            }
            else// if( iTemp2==WEP_KEY_128BIT)
            {
                iTemp2 = 13;
            }
            if ( n==WEP_FORMAT_ASCII )
            {// format is WEP_FORMAT_ASCII
                if ( token )
                {// wepkey is not empty
                    if ( _strlen(token)!=iTemp2 )
                    {
                        scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                        return;
                    }
                    memset(wpa_cfg.wep_key[iTemp-1], 0, DCF_WLAN_WEP_KEY_LEN + 1);
                    _strcpy(wpa_cfg.wep_key[iTemp-1], token);
                }
                else
                {// wepkey is empty
                    memset(wpa_cfg.wep_key[iTemp-1], 0, DCF_WLAN_WEP_KEY_LEN + 1);
                    _strcpy(wpa_cfg.wep_key[iTemp-1], "");
                }
            }
            else if ( n==WEP_FORMAT_HEX )
            {// format is WEP_FORMAT_HEX
                if ( token )
                {// wepkey is not empty
                    if ( _strlen(token)!=(iTemp2<<1) )
                    {
                        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                        return;
                    }
                    _strcpy(wpa_cfg.wep_key[iTemp-1], token);
                }
                else
                {
                    memset(wpa_cfg.wep_key[iTemp-1], 0, DCF_WLAN_WEP_KEY_LEN + 1);
                    _strcpy(wpa_cfg.wep_key[iTemp-1], "");
                }
            }
            if ( Scf_setWlanConfig(iParam_Byte0, &wpa_cfg) < 0 )
            {
                scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
                return;
            }
        }
        break;

	case 'R':
		if (isValidNum(token, 0, NETWORK_TYPE_MAX_NUM) < 0)
		{
			scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
			return;
		}
		if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
		{
			scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
			return;
		}
		else
		{
			int i,j;
			n=Scf_getEncryption(iParam_Byte0);
			if ( n!=SECURITY_ENCRY_WEP )
			{
				scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
				return;
			}
			token = strtok(NULL,";");
			if ( token )
			{// passphrase key is not empty
				n = _strlen(token);
			    if ( n>DCF_WLAN_WEP_PASSPHRASE_LEN )
			    {
					scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
					return;
			    }
			}
			else
			{// passphrase key is empty
					scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
					return;
			}
			if( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) < 0 )
			{
				scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
				return;
			}
			
           char wepkey[DCF_WLAN_WEP_KEY_NUM][DCF_WLAN_WEP_KEY_LEN]; 
           int wepkeylen;
			memset(wepkey,0 ,sizeof(wepkey));
			((char *)token)[n] = 0;
			Scf_setWEPKeyFormat(iParam_Byte0, W1_WEP_FORMAT_HEX);
			wepkeylen = ((Scf_getWEPkeylen(iParam_Byte0) - 1) == 0) ? 5 : 13;		
			nwepgen(token, wepkeylen, wepkey);

			for(i = 0 ; i < DCF_WLAN_WEP_KEY_NUM; i++)
			{
				for (j = 0; j < wepkeylen; j++)
					if (wepkey[i][j]) break;
				if (j < wepkeylen) /* not all zero */	
					str_ascii2hex(wpa_cfg.wep_key[i], wepkey[i], wepkeylen);
			}

			if ( Scf_setWlanConfig(iParam_Byte0,&wpa_cfg) < 0 )
           {
				scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
				return;
			}
		}
		break;

		case 'S':
			if( isValidNum(token, DWLAN_BSSTYPE_INFRA, DWLAN_BSSTYPE_INFRA) < 0 )
			{
				scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
				return;
			}
			if( pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF )
			{
				Scf_getWlanConfig(iParam_Byte0, &wpa_cfg);
				rep->wdx += sprintf(&rep->param[rep->wdx],"%s", wpa_cfg.psk);
			}
			else
			{
				// iTemp is authentication
				iTemp = Scf_getAuthentication(iParam_Byte0);
				if ( (iTemp!=SECURITY_AUTH_WPA_PSK) && (iTemp!=SECURITY_AUTH_WPA2_PSK) )
				{
				    scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
				    return;
				}
				if( Scf_getWlanConfig(iParam_Byte0,&wpa_cfg) < 0 )
				{
					scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
					return;
				}
				token = strtok(NULL,";");
				if ( token )
				{// passphrase key is not empty
					n = _strlen(token);	// n is psk passphrase length
				    if ( (n>DCF_WLAN_PSK_PASSPHRASE_LEN) ||(n<8) )
				    {
						scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
						return;
				    }
				}
				else
				{// passphrase key is empty
						scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
						return;
				}

				memset(wpa_cfg.psk, 0 , (DCF_WLAN_PSK_PASSPHRASE_LEN + 1) * sizeof(char));
				memcpy(wpa_cfg.psk, token, n * sizeof(char));

				if ( Scf_setWlanConfig(iParam_Byte0,&wpa_cfg) < 0 )
	           {
					scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
					return;
				}
        	}
		break;
		
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;

    }

    scm_send_packet(write, param, rep);
}

static void scm_config_event(int (*write)(int , char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;
    char *token;
    int iParam_Byte0, n, iTemp;

#ifdef MAXTEST
    print(port,"port = %d\r\nop = %c cmd = %c\r\nparam = %s\r\n",port,pkt->op,pkt->cmd[1],pkt->param);
#endif
    token = strtok(pkt->param, ";");
    /* 1~8 PIN 1~8,  9 is mean all Pin */

    iParam_Byte0 = _atoi(token);

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM

    switch (pkt->cmd[1])
    {
    case 'C':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                n = Scf_getEventMail(EVENT_ID_COLDSTART);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                n = Scf_getEventTrap(EVENT_ID_COLDSTART);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter 1, on(1) or off(0)
            iTemp = _atoi(token);
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                Scf_setEventMail(EVENT_ID_COLDSTART, iTemp);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                Scf_setEventTrap(EVENT_ID_COLDSTART, iTemp);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'W':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                n = Scf_getEventMail(EVENT_ID_WARMSTART);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                n = Scf_getEventTrap(EVENT_ID_WARMSTART);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter 1, on(1) or off(0)
            iTemp = _atoi(token);
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                Scf_setEventMail(EVENT_ID_WARMSTART, iTemp);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                Scf_setEventTrap(EVENT_ID_WARMSTART, iTemp);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'A':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                n = Scf_getEventMail(EVENT_ID_LOGINFAIL);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                n = Scf_getEventTrap(EVENT_ID_LOGINFAIL);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter 1, on(1) or off(0)
            iTemp = _atoi(token);
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                Scf_setEventMail(EVENT_ID_LOGINFAIL, iTemp);
            }
            else if ( iParam_Byte0==EVENT_TRAP_SEL )
            {
                Scf_setEventTrap(EVENT_ID_LOGINFAIL, iTemp);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'I':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                n = Scf_getEventMail(EVENT_ID_IPCHANGED);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter 1, on(1) or off(0)
            iTemp = _atoi(token);
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                Scf_setEventMail(EVENT_ID_IPCHANGED, iTemp);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'P':
        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                n = Scf_getEventMail(EVENT_ID_PWDCHANGED);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // iTemp is parameter 1, on(1) or off(0)
            iTemp = _atoi(token);
            if ( iParam_Byte0==EVENT_MAIL_SEL )
            {
                Scf_setEventMail(EVENT_ID_PWDCHANGED, iTemp);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'D':
        if (isValidNum(token, 1, 1) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }

        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            token = strtok(NULL,";");
            // iTemp is parameter 1, is mail or trap selected
            iTemp = _atoi(token);
            if ( iTemp==EVENT_MAIL_SEL )
            {
                n= Scf_getSerialEvent (iParam_Byte0 , EVENT_ID_DCDCHANGE_MAIL);
            }
            else if ( iTemp==EVENT_TRAP_SEL )
            {
                n= Scf_getSerialEvent (iParam_Byte0 , EVENT_ID_DCDCHANGE_TRAP);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            // iTemp is parameter 1, is mail or trap selected
            iTemp = _atoi(token);

            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // n is parameter 2, on(1) or off(0)
            n = _atoi(token);
            if ( iTemp==EVENT_MAIL_SEL )
            {
                Scf_setSerialEvent(iParam_Byte0 , EVENT_ID_DCDCHANGE_MAIL, n);
            }
            else if ( iTemp==EVENT_TRAP_SEL )
            {
                Scf_setSerialEvent(iParam_Byte0 , EVENT_ID_DCDCHANGE_TRAP, n);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    case 'S':
        if (isValidNum(token, 1, 1) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }

        if (pkt->op == OP_GETCONF || pkt->op == OP_RUNNINGCONF)
        {
            token = strtok(NULL,";");
            // iTemp is parameter 1, is mail or trap selected
            iTemp = _atoi(token);
            if ( iTemp==EVENT_MAIL_SEL )
            {
                n= Scf_getSerialEvent (iParam_Byte0 , EVENT_ID_DSRCHANGE_MAIL);
            }
            else if ( iTemp==EVENT_TRAP_SEL )
            {
                n= Scf_getSerialEvent (iParam_Byte0 , EVENT_ID_DSRCHANGE_TRAP);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            /* perry 2 modify start */
            rep->wdx += sprintf(&rep->param[rep->wdx],"%d", n);
            /* perry 2 modify end */
        }
        else
        {
            token = strtok(NULL,";");
            // iTemp is parameter 1, is mail or trap selected
            iTemp = _atoi(token);

            token = strtok(NULL,";");
            if (isValidNum(token, 0, 1) < 0)
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
            // n is parameter 2, on(1) or off(0)
            n = _atoi(token);
            if ( iTemp==EVENT_MAIL_SEL )
            {
                Scf_setSerialEvent(iParam_Byte0 , EVENT_ID_DSRCHANGE_MAIL, n);
            }
            else if ( iTemp==EVENT_TRAP_SEL )
            {
                Scf_setSerialEvent(iParam_Byte0 , EVENT_ID_DSRCHANGE_TRAP, n);
            }
            else
            {
                scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
                return;
            }
        }
        break;

    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}
/* perry add region end */

void str_ascii2hex(char *dstr, char *sstr, int slen)
{
	int	i;
	char buf[4];

	for (i = 0; i < slen; i++)
	{
		sprintf(buf, "%02x", (*sstr++)&0xff);	
		memcpy(&dstr[i*2], buf, 2);
	}
	dstr[i*2] = 0;	
}

/* perry add region end */

static void scm_config(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
#ifdef MAXTEST
    //sprintf(buf,"scm_config %d cmd = %c\r\n",port,pkt->cmd[0]);
    //ch_write(port,buf,_strlen(buf));
    //print(port,"scm_config %d cmd = %c\r\n",port,pkt->cmd[0]);
#endif
#if 1
    switch (pkt->cmd[0])
    {
    case 'B': /* ?GBN */
        scm_config_basic(write, param, pkt);

        break;
    case 'N': /* ?GNB */
        scm_config_network(write, param, pkt);

        break;
    case 'O': /* ?GOM1;x --->x is 0~2 */
        scm_config_op_basic(write, param, pkt);

        break;
    case 'S': /* ?GSA1 */
        scm_config_serial(write, param, pkt);

        break;
    case '@': /* ?G@S or ?R@S */
        scm_config_fix(write, param, pkt);

        break;
    case 'R': /* ?GRA1;x */
        scm_config_op_RealCOM(write,param,pkt);

        break;
    case 'F': /* ?GFA1;x */
        scm_config_op_rfc2217(write,param,pkt);

        break;
    case 'T': /* ?GTO1;x */
        scm_config_op_TCP(write,param,pkt);

        break;
    case 'U': /* ?GUP1;x , some is ?GUD1;0;0 */
        scm_config_op_UDP(write,param,pkt);

        break;
    case 'A': /* ?GAI1 */
        scm_config_access(write, param, pkt);
        break;

    case 'M': /* ?GMU */
        scm_config_snmp(write, param, pkt);
        break;

    case 'P': /* ?GPM1 */
        scm_config_pin(write, param, pkt);
        break;

    case 'C': /* ?GCT */
        scm_config_control(write, param, pkt);
        break;

    case 'V': /* ?GVA */
        scm_config_other(write,param,pkt);
        break;

    case 'W': /* ?GWI */
        scm_config_wireless(write,param,pkt);
        break;

        /* Add your code here */
//// Added by James.
    case 'I': /* ?GI */
        scm_config_mail(write,param,pkt);
        break;
////End
        /* perry add region start */
    case 'Q': /* ?GQ */
        scm_config_security(write,param,pkt);
        break;

    case 'E': /* ?GE */
        scm_config_event(write,param,pkt);
        break;
        /* perry add region end */
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);

        return;
    }
#else

#endif
}
static void scm_view(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    scm_pkt_t	rep;

#ifdef SUPPORT_NE_SCM
    rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
    rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
    switch (pkt->cmd[0])
    {
#ifdef SUPPORT_LAN_SPEED
    case 'N':
        switch (pkt->cmd[1])
        {
        case 'S':
            switch (Scf_getLanSpeed(0))
            {
            case D_LAN_SPEED_100M_FULL:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "100F");
                break;
            case D_LAN_SPEED_100M_HALF:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "100H");
                break;
            case D_LAN_SPEED_10M_FULL:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "10F");
                break;
            case D_LAN_SPEED_10M_HALF:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "10H");
                break;
            case D_LAN_SPEED_AUTO:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "AUTO");
                break;

            default:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "0");
                break;
            }
            break;
        default:
            scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
            return;
        }
        break;
    case 'W':
        switch (pkt->cmd[1])
        {
        case 'S':
            switch (Scf_getLanSpeed(0))
            {
            case D_LAN_SPEED_100M_FULL:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "100F");
                break;
            case D_LAN_SPEED_100M_HALF:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "100H");
                break;
            case D_LAN_SPEED_10M_FULL:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "10F");
                break;
            case D_LAN_SPEED_10M_HALF:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "10H");
                break;
            case D_LAN_SPEED_AUTO:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "AUTO");
                break;

            default:
                rep->wdx += sprintf(&rep->param[pkt->wdx], "0");
                break;
            }
            break;
        default:
            scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
            return;
        }
        break;
#endif
#if 1
    case 'P':
    {
        int dio;
//#ifdef MIINEPORT_E1
#if 0
        u_char mask = DIO_FUNC_HW | DIO_FUNC_SW;
        u_char func;
#endif

        if ( (pkt->cmd[1] != 'M') && (pkt->cmd[1] != 'S') )
        {
            scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
            return;
        }

        if (isValidNum(pkt->param, 0, MAX_DIO - 1) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }

        dio = _atoi(pkt->param);

//#ifdef MIINEPORT_E1
#if 0
        func = Scf_getDioFunc(dio, mask);	// DIO

        if ( (func & (DIO_HW_DIO | DIO_SW_DIO)) == 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
#endif
        switch (pkt->cmd[1])
        {
        case 'M':
        {
            int mode;
            DIO_GetSingleIO(dio, &mode);
            rep->wdx += sprintf(&rep->param[rep->wdx], "%d", !mode);
        }
        break;
        case 'S':
        {
            int highlow;
            DIO_GetSingleIOStatus(dio, &highlow);
            rep->wdx += sprintf(&rep->param[rep->wdx], "%d", !highlow);
        }
        break;
        }
    }
    break;
#endif
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_send_packet(write, param, rep);
}
static void scm_control(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    switch (((int)pkt->cmd[0] << 8) | (int)pkt->cmd[1])
    {
        /* PING */ //// Added by James
    case (((int)'N' << 8) | (int)'P'):
    {
        scm_pkt_t rep;
        char buf[128];
        char *token;
        FILE *fp;
        u_long t0;
#define PING_TEMPPATH1   "/var/ping.result1"
#define PING_TEMPPATH2   "/var/ping.result2"

        token = strtok(pkt->param, ";");
        /*
        		if (token)
        		{
        			if (isValidIP(token) < 0)
        			{
        				scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        				return;
        			}
        		}
        */
        if(!is_valid_domain_name(token, _strlen(token)))
        {
        	sprintf(buf, "echo Invalid host IP address/name. > %s", PING_TEMPPATH1);
			system(buf);
        }else{
        	sprintf(buf, "/bin/ping -c 1 %s > %s &", token, PING_TEMPPATH1);
        	system(buf);
        }
		
        t0 = sys_clock_ms();
        while ((sys_clock_ms()-t0) < 1000)
        {
            usleep(50*1000);
            fp = fopen(PING_TEMPPATH1, "r");
            if (fp == NULL)
                continue;

            fseek(fp, 0, SEEK_END);
            if (ftell(fp) > 0)
            {
                fclose(fp);
                break;
            }
            fclose(fp);
        }
        // get the time
        sprintf(buf, "awk '/time/' %s | awk '{print $7}' | awk -F= '{print $2}' > %s", PING_TEMPPATH1, PING_TEMPPATH2);
        system(buf);
        usleep(50*1000);

        memset(buf, 0, sizeof(buf));
        fp = fopen(PING_TEMPPATH2, "r");
        if (fp != NULL)
        {
            fgets(buf, sizeof(buf), fp);
            fclose(fp);
        }
        if (_strlen(buf)==0)
            sprintf(buf, "-");
        else
        {
            sprintf(buf, "%d", _atoi(buf)+1);
        }
#ifdef SUPPORT_NE_SCM
        rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
        rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
        rep->wdx += sprintf(&rep->param[rep->wdx],"%s", buf);
        scm_send_packet(write, param, rep);
        return;
    }

    break;
    /* Save & Restart */
    case (((int)'S' << 8) | (int)'R'):
    {
        scm_pkt_t	rep;
        if (pkt->param == NULL)
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
#ifdef SUPPORT_NE_SCM
        if (pkt->head == HEAD_NE_COMMAND)
            --pkt->param[0];
#endif // SUPPORT_NE_SCM
        if (pkt->param[0] == '0') /* Restart Only */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //sys_restart_system();
            usleep(3000000);

            kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGHUP);
            sys_setSCMStatus(SCM_DATA_MODE);
            pthread_exit(NULL);
        }
        else if	(pkt->param[0] == '1') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            sys_setSCMStatus(SCM_DATA_MODE);
            sys_reboot(0);
        }
#if 0
        else if	(pkt->param[0] == '2') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGHUP);
            pthread_exit(NULL);
            /* portd died */
        }
        else if	(pkt->param[0] == '3') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGHUP);
            /* portd restart , scm fail */
        }
        else if	(pkt->param[0] == '4') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            sys_rm_pid(1, DSPORTD_PID_FILE);
            /* fail */
        }
        else if	(pkt->param[0] == '5') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGHUP);
            pthread_exit(NULL);
            /* ok */

        }
        else if	(pkt->param[0] == '6') /* Save & Restart */
        {
#ifdef SUPPORT_NE_SCM
            rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
            rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
            scm_send_packet( write,param,rep);
            //sys_sleep_ms(3000);	/* wait for response send */
            //Scf_saveRestart();
            usleep(3000000);
            //kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGTERM);
            if (!fork ())
            {
                usleep(1000);
                //kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGTERM);
                kill(sys_get_pid(1, DSPORTD_PID_FILE), SIGTERM);
                system("portd -p 1");
                //usleep(1000000);
                //system("portd -p 1");
            }

            pthread_exit(NULL);
            /* ok */
        }
#endif
        else
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
        return;
    }
    break;
    /* Load factory default */
    case (((int)'L' << 8) | (int)'D'):
    {
        scm_pkt_t	rep;

        if (pkt->param == NULL)
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
        if (pkt->param[0] == '0') // keep IP settings
        {
            Scf_LoadDefault(1);
        }
        else if (pkt->param[0] == '1')
        {
            Scf_LoadDefault(0);
        }
        else
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
#ifdef SUPPORT_NE_SCM
        rep = scm_prepare_reply(pkt->head, pkt->op, pkt->cmd, ST_SUCCESS);
#else
        rep = scm_prepare_reply(pkt->op, pkt->cmd, ST_SUCCESS);
#endif // SUPPORT_NE_SCM
        scm_send_packet( write,param,rep);
        return;
    }
    break;
    /* DIO mode */
    case (((int)'P' << 8) | (int)'M'):
    {
        int mode;
        char *token;
        int pinIdx;

        token = strtok(pkt->param, ";");
        /* 1~8 PIN 1~8,  9 is mean all Pin */
        if (isValidNum(token, 0, 7) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        pinIdx = _atoi(token);

        token = strtok(NULL,";");
        if (isValidNum(token, 0, 1) < 0)
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
        mode = _atoi(token);

        if (pinIdx == 0)
        {
            if ((Scf_getScmTrigger() == 1)  && mode==0)
            {
                scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
                return;
            }
        }

        Scf_setSDioMode(pinIdx,mode);
    }
    break;
    /* DIO state*/
    case (((int)'P' << 8) | (int)'S'):
    {
        int state, mode;
        char *token;
        int pinIdx;

        token = strtok(pkt->param, ";");
        /* 1~8 PIN 1~8,  9 is mean all Pin */
        if (isValidNum(token, 0, 7) < 0)
        {
            scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
            return;
        }
        pinIdx = _atoi(token);

        token=strtok(pkt->param,";");

        if (isValidNum(token, 0, 1) < 0)
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }
        state=_atoi(token);

        mode = Scf_getSDioMode(pinIdx);

        if (mode == 1) // input
        {
            scm_error_reply(write,param,pkt,ST_INCORRECT_PARAMETER);
            return;
        }

        Scf_setSDioState(pinIdx, state);
    }
    break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);
        return;
    }

    scm_error_reply(write, param, pkt, ST_INVALID_COMMAND);

}
static void scm_process(int (*write)(int, char *, int), int param, scm_pkt_t pkt)
{
    if (pkt->st != ST_SUCCESS)
    {
        scm_error_reply(write, param, pkt, pkt->st);
        return;
    }
#ifdef SUPPORT_NE_SCM
    if (pkt->head == HEAD_NE_COMMAND)
    {
        int ret;
        if ((ret = scm_decodeNeCmd(write, param,pkt)) != 0)
        {
            char err;
            switch (ret)
            {
            case -1:
                err = ST_INVALID_OPERATION;
                break;
            case -2:
                err = ST_INVALID_OPERATION;
                break;
            case -4:
                err = ST_INCORRECT_PARAMETER;
                break;
            case -3:
            default:
                err = ST_UNRECOGNIZED_FORMAT;
                break;
            }
            if (ret < 0)
                scm_error_reply(write, param, pkt, err);
            return;
        }
    }
#endif

    if ( (pkt->head != HEAD_COMMAND)
#ifdef SUPPORT_NE_SCM
            && (pkt->head != HEAD_NE_COMMAND)
#endif // SUPPORT_NE_SCM
       )
    {
        scm_error_reply(write, param, pkt, ST_UNRECOGNIZED_FORMAT);
        return;
    }
#if 1
    if (scm_checkParamNumber(pkt->op, pkt->cmd, pkt->param, _strlen(pkt->param)) == -2)
    {
        scm_error_reply(write, param, pkt, ST_INCORRECT_PARAMETER);
        return;
    }
    switch (pkt->op)
    {
    case OP_GETCONF:
    case OP_SETCONF:
    case OP_RUNNINGCONF:
        scm_config(write, param, pkt);
        break;
    case OP_VIEWSTAT:
        scm_view(write, param, pkt);
        break;
    case OP_CONTROL:
        scm_control(write, param, pkt);
        break;
    default:
        scm_error_reply(write, param, pkt, ST_INVALID_OPERATION);
        return;
    }
#endif
}

void scm_main(int (*read)(int, char *, int), int (*write)(int, char *, int), int port, int timeout, int enter_signal)
{
    scm_pkt_t	pkt;
    u_long		t;

    if (enter_signal)
    {
#ifdef SUPPORT_NE_SCM
        pkt = scm_prepare_reply(HEAD_COMMAND, OP_INVALID, "--", ST_ENTER_SCM);
#else
        pkt = scm_prepare_reply(OP_INVALID, "--", ST_ENTER_SCM);
#endif // SUPPORT_NE_SCM
        scm_send_packet(write, port, pkt);
    }
    t = sys_clock_s();
#if 1
    while (1)
    {
        pkt = scm_get_packet(read, port, timeout);
        if (pkt == NULL)	/* timeout */
        {
            printf("scm_main timeout return\n");
            /*sys_restart_system();*/
            return;
        }

        scm_process(write, port, pkt);

    }
#endif
}
/*-----------GPIO Define-----------------
#define DIO_PIN_0   0
#define DIO_PIN_1   1
#define DIO_PIN_2   2
#define DIO_PIN_3   3
#define DIO_PIN_4   4
#define DIO_PIN_5   5
#define DIO_PIN_6   6
#define DIO_PIN_7   7
#define DIO_MODE_OUTPUT     0
#define DIO_MODE_INPUT      1
#define DIO_HIGH    1
#define DIO_LOW     0
---------------------------------------*/
static void scm_hw_trigger_check(int port)
{
    int	high;
    int 	ret;

    sys_set_dio_mode(DCF_SERCMD_HW_TRIGGER, 1); /*input:1, output:0*/
    sys_set_dio(DCF_SERCMD_HW_TRIGGER, 1);

    while (1)
    {
        if (portd_terminate)
        {
            sys_setSCMStatus(SCM_DATA_MODE);
            pthread_exit(NULL);
        }
        sys_set_dio_mode(DCF_SERCMD_HW_TRIGGER, 1);
        ret = DIO_GetSingleIOStatus(DCF_SERCMD_HW_TRIGGER, &high);
        if ((ret && (!high)))
            break;

        usleep(50000);
    }
}

static void scm_break_trigger_check(int port)
{
    int error;
    int	baud, mode, flowctrl;

    while (1)
    {
        if (portd_terminate)
        {
            sys_setSCMStatus(SCM_DATA_MODE);
            pthread_exit(NULL);
        }
        error = sercmd_notify_error(port);

        if (error == 0x10)
        {
            break;
        }
        else if (error < 0)
        {
            sercmd_open(port);
            Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
            sercmd_ioctl(port, baud, mode);
        }
        usleep(50000);
    }

}
int Gscm_alarm = 0;

void scm_alarm_handler (int a)
{
    Gscm_alarm = 1;
    //setitimer( ITIMER_REAL, NULL, NULL) ;
}
static void scm_sw_trigger_check(int port, u_char *ch)
{
    char buf;
    int ret, cnt;
    struct itimerval t;
    int	baud, mode, flowctrl;

    t.it_interval.tv_usec = 100000;
    t.it_interval.tv_sec = 0;
    t.it_value.tv_usec = 100000;
    t.it_value.tv_sec = 0;

    signal(SIGALRM, scm_alarm_handler );
    setitimer(ITIMER_REAL, &t, NULL);

    cnt = 0;
    ret = 0;

    while (1)
    {
        if (portd_terminate)
        {
            sys_setSCMStatus(SCM_DATA_MODE);
            pthread_exit(NULL);
        }
        if (!Gscm_online)
        { /* OffLine Trigger Parser */
            //setitimer(ITIMER_REAL, &t, NULL);
            ret = sercmd_read(port, &buf ,1);
            if (ret < 0)
            {

                sercmd_open(port);
                Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
                sercmd_ioctl(port, baud, mode);
            }

            if (Gscm_alarm == 1)
            {
                cnt = 0;
                Gscm_alarm = 0;
            }

            if (ret)
            {
                if (buf == ch[0] && cnt == 0)
                {
                    cnt ++;
                }
                else if (buf == ch[1] && cnt == 1)
                {
                    cnt ++;
                }
                else if (buf == ch[2] && cnt == 2)
                {
                    cnt ++;
                    break;
                }
                else
                {
                    cnt = 0;
                }
            }
        }
        if (Gscm_active)
            break;
        usleep(1000);
    }
    setitimer(ITIMER_REAL, NULL, NULL) ;
}

void trigger_scm (int port)
{
    Gscm_active = 1;
}

int scmd(void * arg)
{
    int    fd, port, first;
    int	baud, mode, flowctrl;
    int    trigger;
    u_char ch[3];

    first = 1;
    port = (int)arg & ~0x8000;
#if 1
    trigger = Scf_getScmTrigger();
    Scf_getScmChar(&ch[0], &ch[1], &ch[2]);
#endif
    if (trigger)
    {/* scm enable */
        if (trigger == SCM_HW_TRIGGER)
        { /* hw trigger */
            fd = sercmd_open (port);
            Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
            sercmd_ioctl(port, baud, mode);
            scm_hw_trigger_check(port);
            Gscm_active = 1;
            sys_setSCMStatus(SCM_CMD_MODE);
            scm_main((int (*)(int,char*,int))sercmd_read,
                     (int (*)(int,char*,int))sercmd_write,
                     port, 300, first);
            first = 0;

        }
        else if (trigger == SCM_SW_TRIGGER)
        { /* sw trigger */
            fd = sercmd_open (port);
            Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
            sercmd_ioctl(port, baud, mode);
            scm_sw_trigger_check(port, ch);
            Gscm_active = 1;
            sys_setSCMStatus(SCM_CMD_MODE);
            scm_main((int (*)(int,char*,int))sercmd_read,
                     (int (*)(int,char*,int))sercmd_write,
                     port, 300, first);

            first = 0;

            /* */
        }
        else if (trigger == SCM_BREAK_TRIGGER)
        { /* break trigger */
            fd = sercmd_open(port);
            Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
            sercmd_ioctl(port, baud, mode);
            scm_break_trigger_check(port);
            Gscm_active = 1;
            sys_setSCMStatus(SCM_CMD_MODE);
            scm_main((int (*)(int,char*,int))sercmd_read,
                     (int (*)(int,char*,int))sercmd_write,
                     port, 300, first);
            first = 0;

        }
    }
    else
    {

        return 0;
    }

    sercmd_close(port);
    sys_reboot(0);

    return 0;
}
