
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include <netinet/in.h>

#define IF_ACTIVE_FILE		"/var/run/ifactive"

#ifdef ia5x50aio

#define IF_NAME_DEV_ETH0	    "br0"
#define IF_NAME_DEV_SW0	    "sw0"
#define IF_NAME_DEV_SW1	    "sw1"
#define IF_NAME_DEV_SW2	    "sw2"
#define IF_NAME_DEV_SW3	    "sw3"
#define IF_NAME_DEV_SW4	    "sw4"
#else
#define IF_NAME_DEV_ETH0	    "eth0"
#endif

#ifdef WIRELESS_WLAN
#define IF_NAME_DEV_ETH1	"wlan0"
#else
#define IF_NAME_DEV_ETH1	"eth1"
#endif // WIRELESS_WLAN

// SUPPORT_BRIDGE
#define IF_NAME_DEV_BR0     "br0"

// To modify default IP, the following are alse need to be modified.
// 1. /etc/network/interfaceDefault
// 2. /usr/share/udhcpc/default.script
// 3. /usr/local/rc.bootp
// The 2 and 3 is in userdisk.
#define ETHERNET_DEFAULT_IP "192.168.126.254"
#define WIRELESS_DEFAULT_IP "192.168.127.254"


#define IF_NUM_DEV_ETH0		0
#define IF_NUM_DEV_ETH1		1
#define SYS_WARMSTART_FLAG  "/configData/.sys_reboot"
#define SYS_IPCHANGE_FLAG  "/configData/.sys_ipchange"
#define SYS_SCMSTATUS_FLAG  "/var/run/scm_status"
#define WLAN_PROFILE_ACTIVE "/var/run/profile.active"

#define WEB_SERVER_PID      "/var/run/devsvr/goahead.pid"
#define WPA_SUPPLICANT_PID  "/var/run/devsvr/wpa_supplicant.pid"
#define LSTATUS_PID	        "/var/run/devsvr/lstatus.pid"

#define SSL_CERT_PATH       "/configData/etc/server.pem"
#define SSL_CERT_ETH        "/configData/etc/ethCert.pem"
#define SSL_CERT_WLAN       "/configData/etc/wlanCert.pem"
#define SSL_CERT_IMPORT_FLAG "import"

#define SITE_SURVEY_FLAG	"/var/run/sitesurvey"

#define SYSTEM_IMPORT_SYS           "/var/tmp/import_sys"
#define SYSTEM_IMPORT_IP            "/var/tmp/import_ip"
#define SYSTEM_IMPORT_DNS           "/var/tmp/import_dns"
#define SYSTEM_IMPORT_ACCESSIBLEIP  "/var/tmp/import_accessible"
#define SYSTEM_IMPORT_TIME          "/var/tmp/import_time"
#define SYSTEM_IMPORT_WPA_ADHOC     "/var/tmp/import_wpa_adhoc"
#define SYSTEM_IMPORT_WPA_INFRA     "/var/tmp/import_wpa_infra"

#define RESET_NETWORK       0x00000001L
#define RESET_WIRELESS      0x00000002L
#define RESET_IPTABLE       0x00000004L
#define RESET_SNMP          0x00000008L
#define RESET_WPA_ADHOC     0x00000010L
#define RESET_WPA_INFRA     0x00000020L
#define RESET_WEB           0x00000040L

#define RESET_IF_ETH0		1
#define RESET_IF_ETH1		2
#define RESET_IF_ALL		(RESET_IF_ETH0|RESET_IF_ETH1)

#define SCM_DATA_MODE       1
#define SCM_CMD_MODE        2

//#ifdef w2x50a
#if defined(w2x50a) || defined (ia5x50aio)
#define XMODEM_CHECKSUM 0
#define XMODEM_CRC 1
#define XMODEM_CRC1K 2
#define SYS_LOG_NO 512
#endif

enum
{
    STATUS_POWER,
    STATUS_ETH,
    STATUS_ALL,
    STATUS_LED,
    STATUS_DI0,
    STATUS_DI1,
    STATUS_DO0,
    STATUS_DO1
};
int cmpFile(char *filename1, char *filename2);
int cmpDir(char *dirname1, char *dirname2);
int dirExist(char *dirname);
//#elif defined(w2x50a) ||defined(ia5x50aio)
#ifdef w2x50a
#define SYSTEM_EXPORT_NAME	"NPortIAW5x50A_IO.txt"
#else
#define SYSTEM_EXPORT_NAME	"NPortIA5x50A_IO.txt"
#endif
#define MMC_MOUNT				"/mmc"
//#define MMC_TARGET				MMC_MOUNT"/config-"PRODUCT_NICKNAME
#ifdef w2x50a
#define MMC_TARGET				MMC_MOUNT"/config-iaw5x50aio"
#else
#define MMC_TARGET				MMC_MOUNT"/config-ia5x50aio"
#endif
#define MMC_GSD_DIR				MMC_TARGET"/gsd"
//#define MMC_VERSION_FILE		MMC_TARGET"/version"
#define MMC_LOG_DIR				MMC_TARGET"/log"
#define MMC_LOG_FILE			MMC_LOG_DIR"/log"
#define MMC_CONFIG_FILE			MMC_MOUNT"/"SYSTEM_EXPORT_NAME

#define __MTC
#ifdef __MTC
#define CONFIG_SIZE				((int)(16384*1.5))

#define TIME_REC_STA(start) gettimeofday(&(start), NULL)

#define TIME_REC_END(start, stop, diff) do{ gettimeofday(&(stop), NULL); \
	(diff).tv_sec += (stop).tv_sec - (start).tv_sec; \
	(diff).tv_usec += (stop).tv_usec - (start).tv_usec; \
}while(0)
#include <stdio.h>
#define tdbg_printf(...) \
	do { \
		char tmpbuf[128]; \
		int ii; \
		ii = sprintf(tmpbuf, "printf \""); \
		ii += sprintf(&tmpbuf[ii], __VA_ARGS__); \
		sprintf(&tmpbuf[ii], "\" >> /var/log/debug"); \
		system(tmpbuf); \
	} while(0)

#define TIME_REC_RESULT(diff,times) if(times!=0) do { \
	unsigned long time = (diff).tv_sec*1000000L+(diff).tv_usec; \
	tdbg_printf("[Time record] %s = %ld (us)\n", #diff ,time); \
	tdbg_printf("[Time  avg ]  %s = %ld (us)\n\n", #diff ,(time/(times))); \
		(diff).tv_sec = (diff).tv_usec = 0; \
	}while(0)
#else
#define CONFIG_SIZE				16384
#endif
//
// System API protection
//
char* _strcpy(char *dest, const char *src);
size_t _strlen(const char *str);
int _atoi(const char *str);
int _strcmp(const char* s1, const char* s2);
////
#if defined(w2x50a) || defined (ia5x50aio)
int sys_log_clear(void);
int sys_log_write(char* buf, int len);
int sys_log_read(char* buf, int len);
int sys_rx_xmodem(char *filename, char *filepath, int protocol);
int sys_tx_xmodem(char *filename, int protocol);
#endif
u_int __sys_get_pid(char *filename);
int __sys_save_pid(char* filename);
unsigned int sys_get_pid(int port, const char* pidfile);
int sys_save_pid(int port, const char* pidfile);
void sys_rm_pid(int port, const char* pidfile);
int sys_daemon_init(void);
int Ssys_getServerIp(char* host, unsigned long *inaddr, int timeout);
int sys_getmacaddr(const char *interface, char *addr);
int sys_getmyip(const char *interface, char *addr);
int sys_getmynetmask(const char *interface, char *addr);
int sys_getmybroadcast(const char *interface, char *addr);
char *sys_getGateway(char *ifname);
int sys_getActiveProfile(void);
void sys_setEventFlag(unsigned long old_ip);
int sys_send_mtc_msg(int idx, int cond, char *tag, char *act, char *inact);
//int sys_send_events(int event_id, int context);
int sys_send_events_ex(int event_id, int context, unsigned long opmode, int ip, int port);
void sys_reload_eventd();
int sys_miscd_send(int event_id, int value);
void sys_reload_miscd();
int sys_gethostTableEntry(int index, char *host, int host_size, char *ip, int ip_size);
int sys_sethostTableEntry(char host[16][40], char ip[16][32]);
void sys_getVersionExt(int *main_ver, int *sub_ver, int *ext_ver);
void sys_getVersionString(char *buf, int size);
void sys_reset_function(int flag);
void restart_if(int ifdev, int restart);
void sys_reboot(int mode);
int sys_getActiveNetworkPort(void);
void sys_setActiveNetworkPort(int ifnum);
int sys_getActiveNetworkName(char* buf);
int sys_getActiveIP(char *addr);
int sys_getActiveNetmask(char *addr);
int sys_getActiveBcase(char *addr);
int sys_checkEthLink(void);
void sys_CreateSNMPConf();
int sys_getSiteSurveyFlag(void);
void sys_setSiteSurveyFlag(int flag);
#if defined(w2x50a) || defined (ia5x50aio)
char* _chk_interface(char *interface);
#endif

#ifdef SUPPORT_DIO
int	DIO_GetSingleIO(int io, int *mode);
int	DIO_GetSingleIOStatus(int io, int *highlow);
#endif // SUPPORT_DIO

struct _netstat_table
{
    int protocol;
    struct sockaddr_in local_addr;
    struct sockaddr_in rem_addr;
    int state;
    unsigned long txq;
    unsigned long rxq;
    int timer_run;
    unsigned long time_len;
    unsigned long retr;
    int uid;
    int timeout;
    unsigned long inode;
    char more[512];
};
typedef struct _netstat_table _NETSTAT_TABLE;
typedef struct _netstat_table* _NETSTAT_TABLE_T;

struct port_status
{
	int port;
	unsigned long mmio;
	int irq;
	unsigned long total_tx;
	unsigned long total_rx;
	unsigned long tx;
	unsigned long rx;
	unsigned long fr;
	unsigned long pr;
	unsigned long _or;	// or; Avoid alternative tokens in C++
	unsigned long br;
	unsigned long ospeed;
	unsigned long ispeed;
	unsigned int c_flag;
	unsigned int i_flag;
	int fifo;
	int DSR;
	int DTR;
	int RTS;
	int CTS;
	int DCD;
};
typedef struct port_status PORT_STATUS;
typedef struct port_status* PORT_STATUS_T;

#ifdef CROSS
#define PROC_DRIVER "/proc/tty/driver/ttymxc"
#else
#define PROC_DRIVER "ttymxc"
#endif

#define _PATH_PROCNET_TCP     "/proc/net/tcp"
#define _PATH_PROCNET_UDP     "/proc/net/udp"

#define NETSTAT_TCP 0x0001
#define NETSTAT_UDP 0x0002
int sys_getnetstat(int flags, _NETSTAT_TABLE nt[], int nt_size);

#define S2N_IP_LEN 16
int sys_getS2NStatus(int port, char ip[8][S2N_IP_LEN]);
int sys_getPortStatus(int port, PORT_STATUS *p_status);


#ifdef SUPPORT_DIO
/* DIO function */
#define MAX_DIO     8

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
#define DCF_ACTIVE_INTERFACE    DIO_PIN_6
#define DCF_SERCMD_HW_TRIGGER   DIO_PIN_7

int sys_set_dio_mode(int pin, int mode);
int sys_set_dio(int pin, int value);
int sys_get_dio(int pin);
int sys_get_dio_mode(int pin);

#define DIOD_RELOAD_FUNC    1
#define DIOD_RELOAD_ALL     2
#define DIOD_RESTART        3
int sys_reload_diod(int sig);

#endif // SUPPORT_DIO

int sys_getSCMStatus();
int sys_setSCMStatus(int flag);

/* Configuration Import/Export */
int Scf_configurePlainTextExport(char * buffer, int bufsize, int *o_size);
int Scf_configurePlainTextImport(char * buffer, int bufsize, int ip_config);
int Scf_configureExport(char * buffer, int bufsize, int *o_size);
int Scf_configureImport(char * buffer, int bufsize, int ip_config);
void encdecConfigItem(int mode, char *item, char *value, char *dst);
void pwXor2Hex(unsigned char *src, unsigned int srclen, char *dst);
void hexXor2Pw(unsigned char *src, unsigned int srclen, char *dst);



char * sys_getCountryCode(void);

int sys_get_froam_status(int *enable, int *numOfFreq, int freq[3]);

// SUPPORT_STATIC_GARP
int check_ip(char *buf, int size);
int check_mac(char *buf, int size);

void sys_wireless_log(const char *fmt, ...);
void sys_clear_wireless_log(void);
int sys_read_wireless_log(char *buf, int len);

/* The ipc configuration is described below (ram_file) */
int sys_set_di_cnt_start_en(int di_port, int val);
int sys_get_di_cnt_start_en(int di_port);
int sys_set_di_cnt_status(int di_port, int val);
int sys_get_di_cnt_status(int di_port);
int sys_set_di_status(int di_port, int val);
int sys_get_di_status(int di_port);
int sys_recv_di_changed(void);
int sys_send_di_changed(int di_port);
int sys_recv_mtc_changed(void);
int sys_send_mtc_changed(int tbl_idx);
int sys_recv_mtc_pulse_def_changed(void);
int sys_send_mtc_pulse_def_changed(int tbl_idx);
int sys_get_mtc_agent_current_value(char *tag_id, char *cval, int size);
int sys_sync_mtc_agent_current_value(void);
int sys_set_di_rst_flag(int di_port, int val);
int sys_get_di_rst_flag(int di_port);
int sys_set_do_status(int do_port, int val);
int sys_get_do_status(int do_port);
int sys_set_do_pulse_cnt_start_en(int do_port, int val);
int sys_get_do_pulse_cnt_start_en(int do_port);
int sys_recv_do_changed(void);
int sys_send_do_changed(int do_port);
int sys_set_safe_status(int val);
int sys_get_safe_status(void);
void sys_enter_safe_status(void);
int sys_set_di_burnin_err_cnt(int di_port);
int sys_get_di_burnin_err_cnt(int di_port);
int sys_clear_di_burnin_err_cnt();
int Get_IOStatus(int kind, void *status);

char *sys_getSDInfo(char *buf, int bufSize);

int sys_mount_sd(int minMBytes);
int sys_umount_sd(void);

int sys_detectSD(void);
int sys_fmtSD(void);
int sys_config_export_to_file(char *filename,int *o_size);
int sys_config_export_to_sd(int *o_size);
int sys_config_import_from_sd(int ip_config);

/* Get the IP from domain name */
int dns_lookup(char *hostname, char *actual_ip, unsigned int ip_len);
/* Check the domain name is valid or not*/
int is_valid_domain_name(char *hostname, int name_len);
/* Escape double quote for string */
int escape_quote_in_string(char *str, char *new_buf, int new_buf_len);

#endif /* _SYSAPI_H_ */
