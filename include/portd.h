/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _PORTD_H_
#define _PORTD_H_

#include <pthread.h>
#include <netinet/in.h>
#include <support.h>

#define DISABLE_LINUX_SYN_BACKLOG 1

#define DSPORTD_PID_FILE "/run/portd%02d.pid"

#define PORTD_USER_LEN  56
#define MAX_CONNECT     8
#ifdef DISABLE_LINUX_SYN_BACKLOG
#define TCP_LISTEN_BACKLOG  MAX_CONNECT+1
#else
#define TCP_LISTEN_BACKLOG  MAX_CONNECT
#endif
#define DK_BUFFER_SIZE_S2E  1024

#define CFG_APPLICATION_DISABLED        0
#define CFG_APPLICATION_DEVICE_CONTROL  0x100
#define CFG_OPMODE_REALCOM              0
#define CFG_OPMODE_RFC2217              1
#define CFG_APPLICATION_SOCKET          0x200
#define CFG_OPMODE_TCPSERVER            0
#define CFG_OPMODE_TCPCLIENT            1
#define CFG_OPMODE_UDP                  2
#define CFG_APPLICATION_PAIR_CONNECTION 0x300
#define CFG_OPMODE_PAIR_MASTER          0
#define CFG_OPMODE_PAIR_SLAVE           1
#define CFG_APPLICATION_ETH_MODEM       0x400
#define CFG_APPLICATION_RTERMINAL       0x600


#define DEL_NOTHING    1
#define DEL_PLUSONE    2
#define DEL_PLUSTWO    4
#define DEL_STRIP      8

#define CTRLFLAG_SKIPJAM		0x01	/* Skip jammed connection 				*/
#define CTRLFLAG_ALLOWDRV		0x02	/* Allow host's driver control command  */

#ifdef SUPPORT_RTERMINAL_MODE
#define D_AUTH_NONE			0x00		/* Authentication: none    */
#define D_AUTH_LOCAL		0x01		/* Authentication: local   */
#define D_AUTH_RADIUS		0x02		/* Authentication: RADIUS  */

#define	DCF_CRLF_TO_CRLF		0x0000
#define	DCF_CRLF_TO_CR			0x0001
#define	DCF_CRLF_TO_LF			0x0002

#define PORTD_PSWD_LEN	16
#endif


#define SUPPORT_TCP_KEEPALIVE

void sighup_handler(int sig);

void 	port_buffering_start(int port);
int		port_buffering_active(int port);
void 	port_buffering_check_restart(int port);
void 	port_buffering_reset(int port);
void 	port_buffering_flush(int port);
void 	buffering_offline_read(int port);
int 	buffering_sio_read(int port, char *buf, int len);
int 	buffering_read(int port, char *buf, int len);
long 	port_buffering_len(int port);
int		port_buffering_tcp_is_clear(int port, int do_update);
int   	delimiter_check_buffered(int port);

void delimiter_start(int port, int fd_port, int max_conns, int fd_net[], int data_sent[],
                     int (*sendfunc)(int, int, char *, int), int (*recvfunc)(int, int, char *, int),
                     int raw_tcp);
int delimiter_read_1(int port, int send_buffered_data);
int delimiter_read_2(int port, int send_buffered_data);
int delimiter_write_1(int port);
int delimiter_write_2(int port);
void delimiter_flush(int port, int mode);
int delimiter_poll(int port);
int delimiter_recv(int port, int fd_net);
int delimiter_send(int port, int max, int strip);
int delimiter_s2e_len(int port);
int delimiter_e2s_len(int port);
int delimiter_read(int port, int send_buffered_data);
int delimiter_write(int port);
void delimiter_stop(int port);


#define DCF_MAX_SERIAL_PORT 1
#define DCF_SOCK_BUF    4096
int tcp_oqueue(int fd);
int tcp_ofree(int fd);
int tcp_state(int fd);
void portd_wait_empty(int port, int fd_port, unsigned long timeout);
unsigned long portd_getlocal_ip(void);
unsigned long portd_getbcast_ip(void);


typedef struct port_data
{
    int         port_idx;
    int         opmode;
    int         application;
    void        *detail;
    pthread_t   thread_id;
#ifdef SUPPORT_SERCMD
    pthread_t   thread_scm_id;
#endif
#ifdef SUPPORT_RTERMINAL_MODE
	pthread_t   thread_radius_id;
#endif
} PORT_DATA, *PPORT_DATA;

typedef struct aspp_serial
{
    int fd_port;
    int fd_data_listen;
    int fd_cmd_listen;
    int finish;

    int data_port_no,   /* data port number => 950, 966... or 4001, 4002... */
        cmd_port_no;    /* cmd port number => 966, 967..., only for driver mode */

    int fd_data[TCP_LISTEN_BACKLOG];
    int fd_cmd[TCP_LISTEN_BACKLOG];

    int data_sent[TCP_LISTEN_BACKLOG];	/* for port buffering */

    int mode;       /* driver = 1, tcp_server = 0 */
    int ctrlflag;   /* bit(0): ignore jam ip, bit(1): allow driver control. */
    int backlog;
    int notify_flag;
    char notify_buf[5];

    int break_count;
    unsigned char data_status;
    int connect_count;  /* data channel connection count */
    int cmd_connect_count;
    unsigned long notify_lastpoll;
    int serial_flag;            /* 0 : serial port not open */
    							/* 1 : serial port is opened */
    int net_write_flag,
        port_write_flag;

    int flag[TCP_LISTEN_BACKLOG];
    int pollflag[TCP_LISTEN_BACKLOG];

    int old_msr, old_hold;  /* used for save the notified status */

    unsigned long polltime[TCP_LISTEN_BACKLOG];
    unsigned long flush_wait_time[TCP_LISTEN_BACKLOG];
    unsigned long last_time[TCP_LISTEN_BACKLOG];        /* last send/recv time (second)*/
    int notify_count[TCP_LISTEN_BACKLOG];   /* for NPPI */
    int id[TCP_LISTEN_BACKLOG];
    int state[TCP_LISTEN_BACKLOG];
    struct sockaddr_in peer[TCP_LISTEN_BACKLOG];

    int linksioflag;

    //    SSL_CTX* ctx;
    //    X509*    client_cert;
    //    SSL*     ssl[TCP_LISTEN_BACKLOG];
    //    SSL_METHOD *meth;
} ASPP_SERIAL, *PASPP_SERIAL;


#define TCP_CLIENT_MAX_CONNECT  4
#define DCF_IP_DNS_LEN          40
#define UDP_MAX_CONNECT         4

typedef struct raw_tcp_serial
{
	int port_opened;
    int fd_port;
    int fd_net[TCP_CLIENT_MAX_CONNECT];
    int data_sent[TCP_CLIENT_MAX_CONNECT];	/* for port buffering */
    int fd_net_wait[TCP_CLIENT_MAX_CONNECT];
    char dest_ip_dns[TCP_CLIENT_MAX_CONNECT][DCF_IP_DNS_LEN+1];
    unsigned long dest_ip[TCP_CLIENT_MAX_CONNECT];
    int portno[TCP_CLIENT_MAX_CONNECT];
    struct sockaddr_in peer[TCP_CLIENT_MAX_CONNECT];
    int startup_mode;
    int count;
    int ctrlflag;
} RAW_TCP_SERIAL;

typedef struct raw_udp_serial
{
    int     fd_net;
    unsigned long dip_begin[UDP_MAX_CONNECT];
    unsigned long dip_end[UDP_MAX_CONNECT];
    int     dip_port[UDP_MAX_CONNECT];
    int     local_port;             /* local listening port */
    int     finish;
} RAW_UDP_SERIAL, *PRAW_UDP_SERIAL;


int portd_getexitflag(int port);
void portd_setexitflag(int port, int flag);

#ifdef SUPPORT_TCP_KEEPALIVE
void tcp_setAliveTime(int port, int fd);
#endif // SUPPORT_TCP_KEEPALIVE

#endif	/* _PORTD_H_ */
