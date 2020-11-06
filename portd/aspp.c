/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : aspp.c                                                     */
/* Description  :                                                            */
/* Product      : Secured Serial Device Server                               */
/* Programmer   : Shinhsy Shaw                                               */
/* Date         : 2003-07-22                                                 */
/*****************************************************************************/
#ifndef _ASPP_C
#define _ASPP_C
#endif

#include <header.h>
//#include <config.h>
#include <common.h>
#include <sio.h>
#include <portd.h>
#include "aspp.h"
#include <fcntl.h>
#include <debug.h>
#include <stdio.h>
//#include <support.h>
#include <eventd.h>
#include "../message.h"

#define TMP_LEN 	512
#ifdef SUPPORT_SERCMD
extern int Gscm_active;
extern int Gscm_online;
#endif
// for DSCI command 0x14 (dsc_GetNetstat), because we do not have tcp_state function now.
aspp_socket_stat Gaspp_socket_stat[DCF_MAX_SERIAL_PORT][TCP_LISTEN_BACKLOG];

extern struct port_data Gport;
//extern int portd_terminate[MAX_PORTS];

static int aspp_notify_data(int port, unsigned char* buf);

//#define DEBUG_BURNIN

#ifdef DEBUG_BURNIN
/* To debug burn-in, either define BURNIN_NEW or BURNIN_OLD. */
/* Don't try to define both. */
#define BURNIN_NEW
//#define BURNIN_OLD

#include <stdio.h>
#define DBG_ASPP(...) \
	do { \
		char tmpbuf[128]; \
		int ii; \
		ii = sprintf(tmpbuf, "printf \""); \
		ii += sprintf(&tmpbuf[ii], __VA_ARGS__); \
		sprintf(&tmpbuf[ii], "\" >> /var/log/debug"); \
		system(tmpbuf); \
	} while(0)

#define PATTERN_UPPER   "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC000000000"
#define PATTERN_LOWER   "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabc000000000"


int end_debug_to_net = 0;
int cnt_to_net = 0;
int off_to_net = 0;
char pattern_to_net[2][65]={PATTERN_UPPER, PATTERN_LOWER};

int end_debug_from_net = 0;
int cnt_from_net = 0;
int off_from_net = 0;
char pattern_from_net[2][65]={PATTERN_UPPER, PATTERN_LOWER};

int end_debug_to_ser = 0;
int cnt_to_ser = 0;
int off_to_ser = 0;
char pattern_to_ser[2][65]={PATTERN_UPPER, PATTERN_LOWER};

int end_debug_from_ser = 0;
int cnt_from_ser = 0;
int off_from_ser = 0;
char pattern_from_ser[2][65]={PATTERN_UPPER, PATTERN_LOWER};

unsigned char pattern_old[256];
#endif

/* Dummy function for compile & link */
static int sys_send_events(int event_id, int context);
static int sys_send_events(int event_id, int context) 
{
	return 0;
}

void check_to_net(int checklen, char* buf, int buflen)
{
#ifdef DEBUG_BURNIN
#ifdef BURNIN_NEW
    static int i;
    if (end_debug_to_net)
        return;
    
    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [to net] buffer overflow\n");
        end_debug_to_net = 1;
    }

    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_to_net[cnt_to_net%2][off_to_net]&0xff))
        {
            DBG_ASPP("\nBurn-in: [to net] data error, check cnt(%d), idx(%d), 0x%x != 0x%x\n",
                cnt_to_net, off_to_net, buf[i]&0xff, pattern_to_net[cnt_to_net%2][off_to_net]&0xff);
            end_debug_to_net = 1;
            return;
        }
        off_to_net++;

        if (off_to_net == 55)
            sprintf(pattern_to_net[cnt_to_net%2]+off_to_net, "%09d", cnt_to_net);
        else if (off_to_net >= 64)
        {
            cnt_to_net++;
            off_to_net = 0;
        }
    }
#elif defined(BURNIN_OLD)
    static int i;
    if (end_debug_to_net)
        return;

    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [to net] buffer overflow\n");
        end_debug_to_net = 1;
    }
    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_old[off_to_net]&0xff))
        {
            DBG_ASPP("\nBurn-in: [to net] data error, received(0x%x) != should(0x%x)\n",
                buf[i]&0xff, pattern_old[off_to_net]);
            end_debug_to_net = 1;
            return;
        }
        off_to_net ++;
        if (off_to_net >= 256)
            off_to_net = 0;
    }

#endif
#else
    return;
#endif
}

void check_from_net(int checklen, char* buf, int buflen)
{
#ifdef DEBUG_BURNIN
#ifdef BURNIN_NEW
    static int i;
    if (end_debug_from_net)
        return;
    
    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [from net] buffer overflow\n");
        end_debug_from_net = 1;
    }

    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_from_net[cnt_from_net%2][off_from_net]&0xff))
        {
            DBG_ASPP("\nBurn-in: [from net] data error, check cnt(%d), idx(%d), 0x%x != 0x%x\n",
                cnt_from_net, off_from_net, buf[i]&0xff, pattern_from_net[cnt_from_net%2][off_from_net]&0xff);

            end_debug_from_net = 1;
            return;
        }
        off_from_net++;

        if (off_from_net == 55)
            sprintf(pattern_from_net[cnt_from_net%2]+off_from_net, "%09d", cnt_from_net);
        else if (off_from_net >= 64)
        {
            cnt_from_net++;
            off_from_net = 0;
        }
    }
#elif defined(BURNIN_OLD)
    static int i;
    if (end_debug_from_net)
        return;

    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [from net] buffer overflow\n");
        end_debug_from_net = 1;
    }
    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_old[off_from_net]&0xff))
        {
            DBG_ASPP("\nBurn-in: [from net] data error, received(0x%x) != should(0x%x)\n",
                buf[i]&0xff, pattern_old[off_from_net]);
            end_debug_from_net = 1;
            return;
        }
        off_from_net ++;
        if (off_from_net >= 256)
            off_from_net = 0;
    }
#endif
#else
    return;
#endif
}


void check_to_ser(int checklen, char* buf, int buflen)
{
#ifdef DEBUG_BURNIN
#ifdef BURNIN_NEW
    static int i;
    if (end_debug_to_ser)
        return;
    
    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [to ser] buffer overflow\n");
        end_debug_to_ser = 1;
    }

    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_to_ser[cnt_to_ser%2][off_to_ser]&0xff))
        {
            DBG_ASPP("\nBurn-in: [to ser] data error, check cnt(%d), idx(%d), 0x%x != 0x%x\n",
                cnt_to_ser, off_to_ser, buf[i]&0xff, pattern_to_ser[cnt_to_ser%2][off_to_ser]&0xff);
            end_debug_to_ser = 1;
            return;
        }
        off_to_ser++;

        if (off_to_ser == 55)
            sprintf(pattern_to_ser[cnt_to_ser%2]+off_to_ser, "%09d", cnt_to_ser);
        else if (off_to_ser >= 64)
        {
            cnt_to_ser++;
            off_to_ser = 0;
        }
    }
#elif defined(BURNIN_OLD)
    static int i;
    if (end_debug_to_ser)
        return;

    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [to ser] buffer overflow\n");
        end_debug_to_ser = 1;
    }
    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_old[off_to_ser]&0xff))
        {
            DBG_ASPP("\nBurn-in: [to ser] data error, received(0x%x) != should(0x%x)\n",
                buf[i]&0xff, pattern_old[off_to_ser]);
            end_debug_to_ser = 1;
            return;
        }
        off_to_ser ++;
        if (off_to_ser >= 256)
            off_to_ser = 0;
    }
#endif
#else
    return;
#endif
}

void check_from_ser(int checklen, char* buf, int buflen)
{
#ifdef DEBUG_BURNIN
#ifdef BURNIN_NEW
    static int i;
    if (end_debug_from_ser)
        return;
    
    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [from ser] buffer overflow\n");
        end_debug_from_ser = 1;
    }

    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_from_ser[cnt_from_ser%2][off_from_ser]&0xff))
        {
            DBG_ASPP("\nBurn-in: [from ser] data error, check cnt(%d), idx(%d), 0x%x != 0x%x\n",
                cnt_from_ser, off_from_ser, buf[i]&0xff, pattern_from_ser[cnt_from_ser%2][off_from_ser]&0xff);

            end_debug_from_ser = 1;
            return;
        }
        off_from_ser++;

        if (off_from_ser == 55)
            sprintf(pattern_from_ser[cnt_from_ser%2]+off_from_ser, "%09d", cnt_from_ser);
        else if (off_from_ser >= 64)
        {
            cnt_from_ser++;
            off_from_ser = 0;
        }
    }
#elif defined(BURNIN_OLD)
    static int i;
    if (end_debug_from_ser)
        return;

    if (checklen > buflen)
    {
        DBG_ASPP("\nBurn-in: [from ser] buffer overflow\n");
        end_debug_from_ser = 1;
    }
    for (i=0; i<checklen; i++)
    {
        if ((buf[i]&0xff) != (pattern_old[off_from_ser]&0xff))
        {
            DBG_ASPP("\nBurn-in: [from ser] data error, received(0x%x) != should(0x%x)\n",
                buf[i]&0xff, pattern_old[off_from_ser]);
            end_debug_from_ser = 1;
            return;
        }
        off_from_ser ++;
        if (off_from_ser >= 256)
            off_from_ser = 0;
    }
#endif
#else
    return;
#endif
}


void *aspp_start(void* arg)
{
    struct port_data *ptr;
    ASPP_SERIAL *detail;
    int is_driver;
    int port;
    int i;

	is_driver = ((int)arg & 0x8000) ? 0 : 1;
    port = (int)arg & ~0x8000;

    memset(Gaspp_socket_stat[0], 0x0, TCP_LISTEN_BACKLOG * sizeof(aspp_socket_stat));
    ptr = &Gport;

    detail = (struct aspp_serial *) ptr->detail;
#ifdef DISABLE_LINUX_SYN_BACKLOG
	detail->backlog = Scf_getMaxConns(port)+1;
#else
	detail->backlog = Scf_getMaxConns(port);
#endif
    detail->ctrlflag = 0;

    if (Scf_getSkipJamIP(port))
        detail->ctrlflag |= CTRLFLAG_SKIPJAM;

    if (Scf_getAllowDrvCtrl(port))
        detail->ctrlflag |= CTRLFLAG_ALLOWDRV;

    if (!is_driver)	/* TCP Server Mode */
        Scf_getTcpServer(port, (u_short *) &detail->data_port_no, (u_short *) &detail->cmd_port_no);
    else
    {
        detail->data_port_no = port + ASPP_DATA_BASE_PORT1-1;
        detail->cmd_port_no = port + ASPP_CMD_BASE_PORT1-1;
    }

    detail->fd_cmd_listen = detail->fd_data_listen = -1;

    /* Command Port */
//#ifdef MAKE_SUPPORT_CMDPORT
    aspp_open_cmd_listener(detail);
//#endif

    /* Data Port */
    aspp_open_data_listener(detail);

	if (port_buffering_active(port))
	{
		aspp_open_serial(port);
		sio_DTR(port, 1);		 	/* DTR on */
		sio_RTS(port, 1);			/* RTS on */
		port_buffering_start(port);
	}
	else
	{
		/* open/close serial port so that serial settings monitor web page can 
		show setting values instead of default values */
		aspp_open_serial(port);
		sio_close(port);
		sio_DTR(port, 0);		 	/* DTR off */
		sio_RTS(port, 0);			/* RTS off */
	}

    while (1)
    {
        aspp_main(port, is_driver);

		port_buffering_check_restart(port);

        for (i=0; i<detail->backlog; i++)
        {
            if ((detail->flag[i] & FLAG_DATA_UP) && (detail->fd_data[i] > 0)) {
                aspp_close_data(port, i);
	    }

            if ((detail->flag[i] & FLAG_CMD_UP) && (detail->fd_cmd[i] > 0)) {
                aspp_close_cmd(port, i);
	    }
        }

		if (!port_buffering_active(port)){
        	aspp_close_serial(port);
		}

        //if (portd_terminate[port-1])
        if(portd_getexitflag(port))
            break;
    }
    aspp_close_data_listener(detail);
    aspp_close_cmd_listener(detail);
    return NULL;
}

void aspp_open_data_listener(ASPP_SERIAL *detail)
{
    int yes;
    struct sockaddr_in sin;
    int port = Gport.port_idx;

    if (detail->fd_data_listen != -1)
        return;

    if ((detail->fd_data_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP data port %d socket error.\n", detail->data_port_no);
        exit(EXIT_FAILURE);
    }


    sin.sin_family = AF_INET;                       /* host byte order */
    sin.sin_port = htons((short)detail->data_port_no);		/* short, network byte order        */
    sin.sin_addr.s_addr = INADDR_ANY;
    /*sin.sin_len = sizeof(sin);*/

/*
    {
        int data=0;
        optlen = sizeof(data);
        getsockopt(detail->fd_data_listen, SOL_SOCKET, SO_RCVBUF, (void *)&data, (socklen_t *)&optlen);
        printf("get SO_RCVBUF data = %d\n", data);

        optlen = sizeof(data);
        getsockopt(detail->fd_data_listen, SOL_SOCKET, SO_SNDBUF, (void *)&data, (socklen_t *)&optlen);
        printf("get SO_SNDBUF data = %d\n", data);

        data = DCF_SOCK_BUF;
        while(setsockopt(detail->fd_data_listen, SOL_SOCKET, SO_RCVBUF, (char *)&data, sizeof(data)) == -1)
            usleep(100000);

        data = DCF_SOCK_BUF;
        while(setsockopt(detail->fd_data_listen, SOL_SOCKET, SO_SNDBUF, (char *)&data, sizeof(data)) == -1)
            usleep(100000);
    }
*/
    yes = 1;
    if (setsockopt(detail->fd_data_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP data port %d socket error.\n", detail->data_port_no);
        exit(EXIT_FAILURE);
    }

#if 0
    /* TODO: set reuse port. */
    yes = 1;
    while (setsockopt(detail->fd_data_listen, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
        usleep(100000);
#endif

    if (bind(detail->fd_data_listen, (struct sockaddr *) &sin, sizeof(sin)) == -1)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP data port %d socket error, please check if the port has been used.\n",
                    detail->data_port_no);
        exit(EXIT_FAILURE);
    }

#ifdef DISABLE_LINUX_SYN_BACKLOG
	if (listen(detail->fd_data_listen, 1) == -1)
#else
	if (listen(detail->fd_data_listen, detail->backlog) == -1)
#endif
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP data port %d socket error.\n", detail->data_port_no);
        exit(EXIT_FAILURE);
    }
}

void aspp_open_cmd_listener(ASPP_SERIAL *detail)
{
    int yes=1;
    struct sockaddr_in sin;
    int port = Gport.port_idx;

    if (detail->fd_cmd_listen != -1)
        return;

    if ((detail->fd_cmd_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP command port %d socket error.\n", detail->cmd_port_no);
        exit(EXIT_FAILURE);
    }

    sin.sin_family = AF_INET;                       /* host byte order                  */
    sin.sin_port = htons((short)detail->cmd_port_no);      /* short, network byte order        */
    sin.sin_addr.s_addr = INADDR_ANY;
/*
    {
        int data;
        data = DCF_SOCK_BUF;
        while(setsockopt(detail->fd_cmd_listen, SOL_SOCKET, SO_RCVBUF, (char *)&data, sizeof(data)) == -1)
            usleep(100000);
        data = DCF_SOCK_BUF;
        while(setsockopt(detail->fd_cmd_listen, SOL_SOCKET, SO_SNDBUF, (char *)&data, sizeof(data)) == -1)
            usleep(100000);
    }
*/
    yes = 1;
    if (setsockopt(detail->fd_cmd_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP command port %d socket error.\n", detail->cmd_port_no);
        exit(EXIT_FAILURE);
    }

#if 0
    /*
     * TODO:
     * set reuse port.
     */
    yes = 1;
    while (setsockopt(detail->fd_cmd_listen, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) == -1)
        usleep(100000);
#endif

    if (bind(detail->fd_cmd_listen, (struct sockaddr *) &sin, sizeof(sin)) == -1)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP command port %d socket error, please check if the port has been used.\n",
                    detail->cmd_port_no);
        exit(EXIT_FAILURE);
    }

#ifdef DISABLE_LINUX_SYN_BACKLOG
	if (listen(detail->fd_cmd_listen, 1) == -1)
#else
	if (listen(detail->fd_cmd_listen, detail->backlog) == -1)
#endif
    {
        SHOW_LOG(stderr, port, MSG_ERR, "TCP command port %d socket error.\n", detail->cmd_port_no);
        exit(EXIT_FAILURE);
    }
}


void aspp_close_data_listener(ASPP_SERIAL *detail)
{
    if (detail->fd_data_listen >= 0)
    {
        close(detail->fd_data_listen);
        shutdown(detail->fd_data_listen, SHUT_RDWR);
        detail->fd_data_listen = -1;
    }
}

void aspp_close_cmd_listener(ASPP_SERIAL *detail)
{
    if (detail->fd_cmd_listen >= 0)
    {
        close(detail->fd_cmd_listen);
        shutdown(detail->fd_cmd_listen, SHUT_RDWR);
        detail->fd_cmd_listen = -1;
    }
}

void aspp_setup_fd(int port, struct timeval *tv, fd_set *rfds, fd_set *wfds, int *maxfd, int do_port_buffering, int serial_buffered)
{
    int i;
    //fd_set rfd, wfd;
    struct port_data *ptr;
    ASPP_SERIAL *detail;
    ptr = &Gport;
    detail = ptr->detail;

    FD_ZERO(rfds);
    FD_ZERO(wfds);

    if (detail->serial_flag)
    {
        *maxfd = detail->fd_port;
        if (detail->port_write_flag)
            FD_SET(detail->fd_port, wfds);

        if (!detail->net_write_flag || do_port_buffering)
            FD_SET(detail->fd_port, rfds);
    }
    else
        *maxfd = 0;

    if ((detail->connect_count < detail->backlog) && (detail->fd_data_listen >= 0))
    {
        FD_SET(detail->fd_data_listen, rfds);
        *maxfd = MAX(detail->fd_data_listen, *maxfd);
    }

    if ((detail->cmd_connect_count < detail->backlog) && (detail->fd_cmd_listen >= 0))
    {
        FD_SET(detail->fd_cmd_listen, rfds);
        *maxfd = MAX(detail->fd_cmd_listen, *maxfd);
    }
    for (i=0; i< detail->backlog; i++)
    {
        /* data channel */
        if (detail->flag[i] & FLAG_DATA_UP)
        {
            if (!detail->port_write_flag)
                FD_SET(detail->fd_data[i], rfds);

            if (detail->net_write_flag || serial_buffered)
                FD_SET(detail->fd_data[i], wfds);
        }	
#if 0		
            if ((detail->fd_cmd[i] > 0) && (tcp_state(detail->fd_cmd[i]) != TCP_ESTABLISHED)) {

                aspp_close_cmd(port, i);
            }
#endif			
        /* cmd channel */
        if (detail->flag[i] & FLAG_CMD_UP)
        {
            FD_SET(detail->fd_cmd[i], rfds);
            if ((detail->flag[i] & FLAG_FLUSH_DATA) || (detail->pollflag[i]))
            {
                (*tv).tv_sec = 0;
                (*tv).tv_usec = 10*1000L;
            }
        }

        if (detail->flag[i] & FLAG_DATA_UP)
            *maxfd = MAX(detail->fd_data[i], *maxfd);

        if (detail->flag[i] & FLAG_CMD_UP)
            *maxfd = MAX(detail->fd_cmd[i], *maxfd);
    }

    //*rfds = rfd;
    //*wfds = wfd;
}

void aspp_main(int port, int is_driver)
{
    struct port_data *ptr;
    ASPP_SERIAL *detail;
    char cmdbuf[CMD_LEN];
    int i, n, maxfd, realtty, ci;
    unsigned long idletime;
    struct timeval tv;
    fd_set rfds, wfds;
	int port_buffering_flag = 0, serial_data_buffered;

    ptr = &Gport;
    //portd_terminate[port-1] = 0;
    portd_setexitflag(port, 0);
    detail = ptr->detail;
    detail->finish = 0;
    detail->notify_lastpoll = sys_clock_ms();
    //detail->fd_port = -1;
    detail->mode = is_driver;
    detail->notify_flag = 0;
    memset(&(detail->notify_buf), '\0', sizeof(detail->notify_buf));
    detail->break_count = 0;
    detail->connect_count = 0;
    detail->cmd_connect_count = 0;
    detail->serial_flag = 0;
    detail->net_write_flag = 0;
    detail->port_write_flag = 0;
    detail->old_hold = 0;
    detail->old_msr = 0;
#ifdef DISABLE_LINUX_SYN_BACKLOG
    realtty = ((detail->backlog-1) > 1)? 0 : 1;
#else
    realtty = (detail->backlog > 1)? 0 : 1;
#endif

	if (port_buffering_active(port))
	{
		detail->serial_flag = 1;
		port_buffering_flag = 1;
	}
	else
		detail->fd_port = -1;  /* was set in aspp_start() if port buffering is enabled*/

    for (i=0; i< detail->backlog; i++)
    {
        detail->fd_data[i] = -1;
        detail->fd_cmd[i] = -1;
        detail->flag[i] = 0;
        detail->pollflag[i] = 0;
        detail->polltime[i] = 0L;
        detail->flush_wait_time[i] = 0L;
        detail->last_time[i] = 0L;		/* last send/recv time (second)*/
        detail->notify_count[i] = 0;		/* for NPPI */
        detail->id[i] = 0;
        detail->state[i] = 0;
        memset(&detail->peer[i], '\0', sizeof(struct sockaddr_in));

        //@@ add by Kevin
        Gaspp_socket_stat[0][i].local_ip = htonl(INADDR_ANY);
        Gaspp_socket_stat[0][i].local_port = htons((short)detail->data_port_no);;
        Gaspp_socket_stat[0][i].socket_type = 1;        // 0:udp, 1:tcp
        //Gaspp_socket_stat[port][i].tcp_state = TCPS_LISTEN;
        Gaspp_socket_stat[0][i].serial_port = port;
        //@@ end
    }

    n = Scf_getInactivityTime(port);

    if (n == 0)
        idletime = 0xFFFFFFFFL;
    else
        idletime = (unsigned long) n;

    tv.tv_sec = 0;
    tv.tv_usec = 5*1000L;

    /******** MAIN LOOP ********/
    //while (!portd_terminate[port-1] && !detail->finish)
    while( !portd_getexitflag(port) && !detail->finish )
    {
#ifdef SUPPORT_SERCMD
        if (Gscm_active)
        {
            usleep(100000);
            continue;
        }
#endif
#if 1
        static int count=0;
        static unsigned long t=0;
        static int sleep_flag=0;

        if((sys_clock_ms()-t) > 1000)
        {
            //printf("count = %d\r\n", count);
            if(count > 200)
                sleep_flag = 2;
            else if(sleep_flag > 0)
                sleep_flag--;
            count = 0;
            t = sys_clock_ms();
        }
        count++;
#endif

    	serial_data_buffered = delimiter_check_buffered(port);
    	
        aspp_setup_fd(port, &tv, &rfds, &wfds, &maxfd, port_buffering_flag, serial_data_buffered);

        for (ci=0; ci < detail->backlog; ci++)
        {
            if (realtty)
            {
                /* Aspp Command => Wait Oqueue */
                if (detail->pollflag[ci])
                {
                    long len = sio_oqueue(port) + aspp_tcp_iqueue(detail->fd_data[ci]);
                    if ((detail->polltime[ci] <= sys_clock_ms()) || (len == 0L))
                    {
                        detail->polltime[ci] = 0;
                        detail->pollflag[ci] = 0;
                        cmdbuf[0] = D_ASPP_CMD_NOT_OFREE;
                        cmdbuf[1] = 2;
                        /**(int *)(&tmp[2]) = (int) len;*/
                        cmdbuf[2] = (unsigned char) (len & 0x00FF);
                        cmdbuf[3] = (unsigned char) ((len & 0xFF00) >> 8);
                        send(detail->fd_cmd[ci], cmdbuf, 4, 0);

                        tv.tv_sec = 0;
                        tv.tv_usec = 5*1000L;
                    }
                }
            }
        }
        if (select(maxfd+1, &rfds, &wfds, NULL, &tv) <= 0)
        {
            if (detail->serial_flag)
            {
                detail->notify_flag = aspp_notify_data(port, (unsigned char *) &(detail->notify_buf[1]));
                if (detail->notify_flag > 0)
                {
                    if (detail->notify_buf[1] & 0x10)
                        detail->break_count++;
                    detail->data_status |= (detail->notify_buf[1] & 0x0F);
                }

                if ((n = delimiter_poll(port)) > 0)
                {
                    tv.tv_sec = 0;
                    tv.tv_usec = 5*1000L;
                }
                else
                {
                    tv.tv_sec = 0L;
                    tv.tv_usec = 10*1000L;
                }
				/* Bugfix for w1 a-test (jperf-2.0.2) start */
				for(ci=0; ci < detail->backlog; ci++)
                {
                    if (detail->fd_data[ci] >= 0 && tcp_state(detail->fd_data[ci]) == 0)
                    {
                        if((detail->flag[ci] & FLAG_CMD_UP) && (detail->fd_data[ci] >= 0)) {
                            	aspp_close_cmd(port, ci);
                        }

                        aspp_flush_data(port, detail->fd_port, detail->fd_data[ci], 2);
                        if (detail->fd_data[ci] != -1){
                        	aspp_close_data(port, ci);
                        }
                        sio_flush(port, FLUSH_ALL);
                        sio_fifo(port, Scf_getAsyncFifo(port));                   /* FIFO */                        
                    }
                }
				/* Bugfix for w1 a-test (jperf-2.0.2) end */

                /* Inactivity time */
                if (!detail->mode)	/* detail->mode ==> 1 : driver mode, 0 : TCP Server Mode */
                {
                    aspp_check_inactivity(port, n, idletime);
                }
                else
                {
                    for (ci=0; ci < detail->backlog; ci++)
                    {
						if (!(detail->flag[ci] & FLAG_CMD_UP)) /* Albert.20120102: add */
							continue;
                        if (realtty)
                        {
                            /* ASPP command => Flush */
                            if (detail->flag[ci] & FLAG_FLUSH_DATA)
                            {
                                unsigned long t;
                                int x = 0;
                                if (detail->flag[ci] & FLAG_FLUSH_RXDATA)
                                    x |= 1;
                                if (detail->flag[ci] & FLAG_FLUSH_TXDATA)
                                    x |= 2;

                                x--;
								
                                if (aspp_flush_data(port, detail->fd_port, detail->fd_data[ci], x) == 1)
                                    t = 0;
                                else
                                    t = sys_clock_ms() - detail->flush_wait_time[ci];

                                if (t >= 0L)
                                {
                                    aspp_flush_reply(realtty, cmdbuf, detail->fd_data[ci]);

                                    send(detail->fd_cmd[ci], cmdbuf, 6, 0);
                                    detail->flag[ci] &= ~FLAG_FLUSH_DATA;

                                    tv.tv_sec = 0;
                                    tv.tv_usec = 5*1000L;
                                }
                            }
                        } /* end of if(realtty) */

                        /* NPPI event notify and alive polling */
                        if ((detail->flag[ci] & FLAG_SET_NOTIFY))
                        {
                            unsigned long  t;

                            t = sys_clock_ms() - detail->last_time[ci];

                            if ((t > 40000) || ((detail->notify_count[ci] > 0) && ((sys_clock_ms() - detail->last_time[ci]) >= 3000)))
                            {
                                if (detail->notify_count[ci] > 3)
                                {
                                    detail->notify_count[ci] = 0;
					if (detail->fd_cmd[ci] != -1){
                                    aspp_close_cmd(port, ci);
					}
					if (detail->fd_data[ci] != -1){
                                    aspp_close_data(port, ci);	/* Albert.20120102 */
					}
                                }
                                else
                                {
                                    detail->id[ci]++;
                                    if (detail->id[ci] == 0)
                                        detail->id[ci]++;
                                    cmdbuf[0] = D_ASPP_CMD_ALIVE;
                                    cmdbuf[1] = 1;
                                    cmdbuf[2] = detail->id[ci];
                                    aspp_update_lasttime(port);
                                    detail->notify_count[ci]++;
                                    send(detail->fd_cmd[ci], cmdbuf, 3, 0);
                                }
                            }
                        }

                        if (detail->notify_flag > 0)
                        {
                            detail->notify_buf[3] |= 0x80;		/* 0x80, 0 for old version, 1 for new version */
                            detail->notify_buf[0] = D_ASPP_RSP_NOTIFY;
                            send(detail->fd_cmd[ci], detail->notify_buf, 4, 0);
                            detail->break_count = 0;
                            detail->data_status = 0;
                        }
                    } /* end of for(ci=0; ci < detail->backlog; ci++) */
                } /* end of if(detail->mode) */
            } /* if(detail->serial_flag) */
	    else
	    {
                tv.tv_sec = 0;
                tv.tv_usec = 100*1000L;
	    }
            continue;
        } /* if(select(maxfd+1, &rfds, &wfds, &efds, &tv) <= 0) */

        /* Accept new cmd channel */
        if (detail->fd_cmd_listen >= 0)
        {
            if (FD_ISSET(detail->fd_cmd_listen, &rfds))
            {
#ifdef DISABLE_LINUX_SYN_BACKLOG
                int cmd_port_index = -1;
                cmd_port_index = aspp_accept_cmd(port);

                //fprintf(stderr, "[Debug] %s, %d...detail->connect_count is %d, detail->cmd_connect_count is %d, detail->backlog is %d\n", __FUNCTION__, __LINE__, detail->connect_count, detail->cmd_connect_count, detail->backlog);

                if(detail->cmd_connect_count > (detail->backlog-1))
                {
                	//fprintf(stderr, "[Debug] %s, %d...close cmd port %d, cmd_port_index %d\n", __FUNCTION__, __LINE__, port, cmd_port_index);
                	aspp_close_cmd(port, cmd_port_index);
                }

                //fprintf(stderr, "[Debug] %s, %d...detail->connect_count is %d, detail->cmd_connect_count is %d, detail->backlog is %d\n", __FUNCTION__, __LINE__, detail->connect_count, detail->cmd_connect_count, detail->backlog);
#else
                aspp_accept_cmd(port);
#endif
            }
        }

        /* Accept new data channel */
        if (detail->fd_data_listen >= 0)
        {
            if (FD_ISSET(detail->fd_data_listen, &rfds))
            {
                if (detail->connect_count == 0 && !detail->serial_flag)
                {
                    detail->serial_flag = aspp_open_serial(port);
#ifdef DEBUG_BURNIN
end_debug_from_net = 0;
cnt_from_net = 0;
off_from_net = 0;
end_debug_to_net = 0;
cnt_to_net = 0;
off_to_net = 0;
end_debug_from_ser = 0;
cnt_from_ser = 0;
off_from_ser = 0;
end_debug_to_ser = 0;
cnt_to_ser = 0;
off_to_ser = 0;
for (i=0; i<256; i++)
    pattern_old[i] = (unsigned char)i;
system("rm -f /var/log/debug");
#endif


#ifdef SUPPORT_SERCMD
                    Gscm_online = 1;
#endif
                }
#ifdef DISABLE_LINUX_SYN_BACKLOG
                int data_port_index = -1;
                data_port_index = aspp_accept_data(port);

                //fprintf(stderr, "[Debug] %s, %d...detail->connect_count is %d, detail->cmd_connect_count is %d, detail->backlog is %d\n", __FUNCTION__, __LINE__, detail->connect_count, detail->cmd_connect_count, detail->backlog);

                if(detail->connect_count > (detail->backlog-1))
                {
                	//fprintf(stderr, "[Debug] %s, %d...close data port %d, data_port_index %d\n", __FUNCTION__, __LINE__, port, data_port_index);
                	aspp_close_data(port, data_port_index);
                }

                //fprintf(stderr, "[Debug] %s, %d...detail->connect_count is %d, detail->cmd_connect_count is %d, detail->backlog is %d\n", __FUNCTION__, __LINE__, detail->connect_count, detail->cmd_connect_count, detail->backlog);
#else
                aspp_accept_data(port);
#endif

            }
        }

        // The serial_flag check must be below the FD_ISSET(fd_cmd, &rfds).
        // Because command port might be connected alone. We must handle disconnection.
        if (!detail->serial_flag)
            continue;

        for (i=0; i< detail->backlog; i++)
        {
            if ((detail->flag[i] & FLAG_CMD_UP) && (detail->fd_cmd[i] > 0))
            {
                if (FD_ISSET(detail->fd_cmd[i], &rfds))
                {
                    int j;

                    if ((j = recv(detail->fd_cmd[i], cmdbuf, CMD_LEN, 0)) > 0)
                    {
						if (detail->serial_flag)
                            if ((j = aspp_command(port, i, cmdbuf, j)) > 0)
                                send(detail->fd_cmd[i], cmdbuf, j, 0);
                    }
/* open close test fail, so mask this code */
/*
                    else if (j == 0)
                        aspp_close_cmd(port, i);
*/

/*	2013/04/15 bugfix: it will cause real com can't access COMPort
	after nport is idle and the following day */
                    else if (j <= 0) {
			   if (detail->fd_cmd[i] != -1) {
                        	aspp_close_cmd(port, i);
			   }
#if 1
			if (detail->mode == 1) { /* If RealCOM Mode */
			   if (detail->fd_data[i] != -1) {
                        	aspp_close_data(port, i);
			   }

			   if (detail->connect_count == 0) {
                            detail->finish = 1;
			   }
			}
#endif
                    	}

                }
            }
        }

        for (i=0; i< detail->backlog; i++)
        {
            if ((detail->flag[i] & FLAG_DATA_UP) && (detail->fd_data[i] > 0))
            {
                if (FD_ISSET(detail->fd_data[i], &rfds))
                {
                    int x;
			
                    x = delimiter_recv(port, detail->fd_data[i]);
                    aspp_update_lasttime(port);
                    if (x < 0)
                    {
                        if (detail->flag[i] & FLAG_CMD_UP) {
				            if (detail->fd_cmd[i] != -1) {
                                aspp_close_cmd(port, i);
                            }
                        }
                        if (detail->fd_data[i] != -1) {
                            aspp_close_data(port, i);
                        }
                        if (detail->connect_count == 0)
                            detail->finish = 1;
                    }
                    else
                    {
                        if (sio_ofree(port) < TMP_LEN)
                            detail->port_write_flag = 1;
                    }
                }

                if (FD_ISSET(detail->fd_data[i], &wfds))
                {
        			if (serial_data_buffered)
        			{

						delimiter_read(port, 1);
						aspp_update_lasttime(port);
        			}
					if (detail->net_write_flag) {
                    	if (delimiter_s2e_len(port) > 0)
                    	{

							if (delimiter_send(port, DK_BUFFER_SIZE_S2E, 0) == 0)
                        	{	/* Albert.20120102: in case sent_to_tcp_len > 0, set net_write_flag = 0 to have delimiter_poll() update s2e_len & sent_to_tcp_len. */
                        		detail->net_write_flag = 0;
                        	}
                    	}
                    	else
                        	detail->net_write_flag = 0;
					}		
                }
            }
        }

        if (FD_ISSET(detail->fd_port, &rfds))
        {
#if 1
            if(sleep_flag)
            {
                if(sio_iqueue(port) < 1000)
                    usleep(2000);
            }
#endif
			if (delimiter_read(port, 0) >= 0)
                detail->net_write_flag = 1;
            else
            {
                tv.tv_sec = 0;
                tv.tv_usec = 5*1000L;
            }
        }

        if (FD_ISSET(detail->fd_port, &wfds))
        {
            if (delimiter_e2s_len(port) > 0)
                delimiter_write(port);
            else
                detail->port_write_flag = 0;
        }
    } /* End of main loop */
#ifdef SUPPORT_SERCMD
    Gscm_online = 0;
#endif
    delimiter_stop(port);
    portd_wait_empty(port, detail->fd_port, 3000);
}


int aspp_check_inactivity(int port, int n, unsigned long idletime)
{
    int i;
    int result = 0;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    if ((n == 0) && (idletime != 0xFFFFFFFFL))
    {
        for (i=0; i< detail->backlog; i++)
        {
            unsigned long t;
            t = sys_clock_ms() - detail->last_time[i];
            if ((detail->port_write_flag == 0) && (detail->net_write_flag == 0) && (t > (idletime)))
            {
                if (detail->flag[i] & FLAG_CMD_UP) {
                    aspp_close_cmd(port, i);
		}

                if (detail->flag[i] & FLAG_DATA_UP)
                {
                    aspp_close_data(port, i);
                    if (detail->connect_count == 0)
                        result = 1;
                }
            }
        }
    }
    return result;
}



int	aspp_sendfunc(int port, int fd_net, char *buf, int len)
{
    int nbytes = 0, max_nbytes = 0;
    int i=0, realtty;
    int flush_start_rx = 0;
    struct port_data *ptr;
    ASPP_SERIAL *detail;
    int minofree=DCF_SOCK_BUF;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

#ifdef DISABLE_LINUX_SYN_BACKLOG
    realtty = ((detail->backlog-1) > 1)? 0 : 1;
#else
    realtty = (detail->backlog > 1)? 0 : 1;
#endif

    if (!realtty)
    {
        for (i=0; i< detail->backlog; i++)
        {
            if (detail->flag[i] & FLAG_CMD_UP)
            {
                if (detail->flag[i] & FLAG_FLUSH_RXDATA)
                    flush_start_rx = 1;
            }

            /* if not set CTRLFLAG_SKIPJAM, check all tcp_ofree has buffer to send. */
            if(!(detail->ctrlflag & CTRLFLAG_SKIPJAM) && (detail->flag[i] & FLAG_DATA_UP))
            {
                int ofree = tcp_ofree(detail->fd_data[i]);
                minofree = minofree < ofree ? minofree : ofree;
            }

        }
    }

    if (flush_start_rx == 0 && minofree >= len)	// if tcp_ofree < send len, not to send.
    {
        for (i=0; i<detail->backlog; i++)
        {
            if (detail->flag[i] & FLAG_DATA_UP)
            {
                if (detail->ctrlflag & CTRLFLAG_SKIPJAM && !realtty)
                {
                    int ofree;
                    ofree = tcp_ofree(detail->fd_data[i]);
                    if(ofree < len)
                        continue;
                }

                if ((nbytes = send(detail->fd_data[i], buf, len, 0)) < 0)
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        nbytes = send(detail->fd_data[i], buf, len, 0);
                    }
                }
                detail->data_sent[i] = nbytes;	/* Albert.20120103: add */
                check_to_net(nbytes, buf, len);

                max_nbytes = MAX(nbytes, max_nbytes);
                aspp_update_lasttime(port);
            }
        }
    }
    else
        detail->last_time[0] = sys_clock_ms();
    usleep(10);

    return max_nbytes;
}



int aspp_recvfunc(int port, int fd_net, char *buf, int len)
{
    int nbytes;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    nbytes = recv(fd_net, buf, len, 0);
    //SHOW_LOG(stderr, port, MSG_ERR, "Joy %s-%d, nbytes=%d\r\n", __func__, __LINE__, nbytes);

    check_from_net(nbytes, buf, len);

    if (nbytes == -1)
        nbytes = 0;

    if (nbytes == 0)
        return -1;

    return nbytes;
}

int aspp_command(int port, int conn, char *buf, int len)
{
    unsigned char	cmd, setok;
    char	*tmp;
    int		i, n, rsp, realtty;
    unsigned char	datalen;
    int	notify;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

#ifdef DISABLE_LINUX_SYN_BACKLOG
    realtty = ((detail->backlog-1) > 1)? 0 : 1;
#else
    realtty = (detail->backlog > 1)? 0 : 1;
#endif

    i = 0;
    rsp = 0;
    while (i < len)
    {
        cmd = buf[i++];
        if (i >= len)
            break;
        datalen = buf[i++];
        if ((i + datalen) > len)
            break;
        setok = 0;
        notify = 0;
        tmp = &buf[i];

        switch (cmd)
        {
        case D_ASPP_CMD_IOCTL:
            if (datalen == 2)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                {
                    int ioctl_baud, ioctl_mode;
                    ioctl_baud = aspp_convert_baud((int) tmp[0]);
                    ioctl_mode = aspp_convert_parity((int) tmp[1]);
                    sio_ioctl(port, ioctl_baud, ioctl_mode);
                    setok = 1;
                }
                else
                    setok = 1;
            }
            break;
        case D_ASPP_CMD_FLOWCTL:
            if (datalen == 4)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                {
                    int fctrl_mode;
                    fctrl_mode = aspp_convert_flow((int) tmp[0], (int) tmp[1], (int) tmp[2], (int) tmp[3]);
                    sio_flowctrl(port, fctrl_mode);
                }
                setok = 1;
            }
            break;
        case D_ASPP_CMD_LCTRL:
            if (datalen == 2)
            {
                int lctrl_mode = 0;
                if (tmp[0])	lctrl_mode |= C_DTR;
                if (tmp[1])	lctrl_mode |= C_RTS;
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                    sio_lctrl(port, lctrl_mode);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_LSTATUS:
            if (datalen == 0)
            {
                int result;
                if ((result = sio_lstatus(port)) >= 0)
                {
                    notify = 1;

                    buf[rsp++] = D_ASPP_CMD_LSTATUS;
                    buf[rsp++] = 3;
                    buf[rsp++] = (result & S_DSR)? 1 : 0;	/* DSR */
                    buf[rsp++] = (result & S_CTS)? 1 : 0;	/* CTS */
                    buf[rsp++] = (result & S_CD)? 1 : 0;	/* DCD */
                }
            }
            break;
        case D_ASPP_CMD_FLUSH:
            if (datalen == 1)
            {
                if (realtty){
                    aspp_flush_data(port, detail->fd_port, detail->fd_data[conn], tmp[0]);
                }
				setok = 1;
            }
            break;
        case D_ASPP_CMD_IQUEUE:
            if (datalen == 0)
            {
                long iq_len;
                iq_len = sio_iqueue(port);

                buf[rsp++] = D_ASPP_CMD_IQUEUE;
                buf[rsp++] = 2;
                buf[rsp++] = (unsigned char) (iq_len & 0x000000FF);
                buf[rsp++] = (unsigned char) (iq_len >> 8);
            }
            break;
        case D_ASPP_CMD_OQUEUE:
            if (datalen == 0)
            {
                long oq_len;
                oq_len = sio_oqueue(port);

                buf[rsp++] = D_ASPP_CMD_OQUEUE;
                buf[rsp++] = 2;
                buf[rsp++] = (unsigned char) (oq_len & 0x000000FF);
                buf[rsp++] = (unsigned char) (oq_len >> 8);
            }
            break;
        case D_ASPP_CMD_BAUDRATE :
            if (datalen == 4)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                {
                    unsigned long speed = tmp[0] + (tmp[1]*0x100L) + (tmp[2]*0x10000L) + (tmp[3]*0x1000000L);

                    if ((speed < 50L) || (speed > 921600L))
                        setok = 2;
                    else
                    {
                        setok = 1;
                        sio_baud(port, speed);
                    }
                }
                else
                    setok = 1;
            }
            break;
        case D_ASPP_CMD_XONXOFF:
            if (datalen == 2)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                    sio_setxonxoff(port, tmp[0], tmp[1]);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_RESET:
            if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
            {
                if ((datalen == 3) && (tmp[0] == '1') && (tmp[1] == '6') && (tmp[2] == 'a'))
                    /* JE: I don't reset this port, it means nothing! */
                    setok = 1;
            }
            break;
        case D_ASPP_CMD_BREAKON:
            if (datalen == 0)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                    sio_break(port, 1);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_BREAKOFF:
            if (datalen == 0)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                    sio_break(port, 0);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_BREAKSEND:
            if (datalen == 2)
            {
                n = htons(*(unsigned short *)tmp);
                if ((n >= 10) && (n <= 1000))
                {
                    if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                        //sio_sendbreak(port, (int) n);
						sio_break(port, 1);
						usleep(n * 1000);
						sio_break(port, 0);
                    setok = 1;
                }
            }
            break;
        case D_ASPP_CMD_NOTIFYON :
            if (datalen == 0)
            {
                detail->flag[conn] |= FLAG_SET_NOTIFY;
                setok = 1;
                notify = 1;
            }
            break;
        case D_ASPP_CMD_NOTIFYOFF :
            if (datalen == 0)
            {
                detail->flag[conn] &= (~FLAG_SET_NOTIFY);
                setok = 1;
                notify = 0;
            }
            break;
        case D_ASPP_RSP_ALIVE :
            if (datalen == 1)
            {
                aspp_update_lasttime(port);
                detail->notify_count[conn] = 0;
            }
            break;
        case D_ASPP_CMD_SETPORT:
            if ((datalen >= 8) && (datalen < (8 + PORTD_USER_LEN)))
            {
                int result;
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                {
                    int setport_baud, setport_mode;
                    int setport_fctrl;

                    setport_baud = aspp_convert_baud((int) tmp[0]);
                    setport_mode = aspp_convert_parity((int) tmp[1]);

                    sio_ioctl(port, setport_baud, setport_mode);
                    setport_fctrl = aspp_convert_flow((int) tmp[4], (int) tmp[5], (int) tmp[6], (int) tmp[7]);
                    sio_flowctrl(port,setport_fctrl);

                    sio_DTR(port, (int) tmp[2]);
                    sio_RTS(port, (int) tmp[3]);
                }

                result = sio_lstatus(port);

                if (result >= 0)
                {
                    notify = 1;

                    buf[rsp++] = D_ASPP_CMD_SETPORT;
                    buf[rsp++] = 3;
                    buf[rsp++] = (result & S_DSR)? 1 : 0;	/* DSR */
                    buf[rsp++] = (result & S_CTS)? 1 : 0;	/* CTS */
                    buf[rsp++] = (result & S_CD)? 1 : 0;	/* DCD */
                    detail->flag[conn] |= FLAG_SET_NOTIFY;
                }
            }
            break;

        case D_ASPP_CMD_NOT_OFREE :
            if (datalen == 4)
            {
                unsigned long		tm;

                tm = tmp[0] + (tmp[1]*0x100L) + (tmp[2]*0x10000L) + (tmp[3]*0x1000000L);

                if (tm == 0)
                    tm = 1;

                if (realtty)
                {
                    detail->polltime[conn] = (unsigned long) (tm + sys_clock_ms()-50);
                    detail->pollflag[conn] = 1;
                    /* don't setok */
                }
                else
                {
                    buf[rsp++] = D_ASPP_CMD_NOT_OFREE;
                    buf[rsp++] = 2;
                    *(unsigned short *) (&buf[rsp++]) = 0;
                    rsp++;
                }
            }
            break;
        case D_ASPP_CMD_SET_TXFIFO :
            if (datalen == 1)
            {
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                {
                    if (*tmp <= 1)
                        sio_fifo(port, 0);		/* Disable FIFO */
                    else
                        sio_fifo(port, 1);		/* Enable FIFO */
                }
                setok = 1;
            }
            break;
        case D_ASPP_CMD_DSR_SEN :
            if (datalen == 1)
            {
                /*
                 * TODO:
                 * support sio_dsrsensitivity
                 */
                if (realtty || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
                    ;//sio_dsrsensitivity(port, (int) tmp[0]);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_SETXON :
            if (datalen == 0 || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
            {
                /*
                 * TODO:
                 * sio_actxon
                 */
                if (realtty)
                    sio_setxon(port);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_SETXOFF :
            if (datalen == 0 || (detail->ctrlflag & CTRLFLAG_ALLOWDRV))
            {
                /*
                 * TODO:
                 * sio_actxoff
                 */
                if (realtty)
                    sio_setxoff(port);
                setok = 1;
            }
            break;
        case D_ASPP_CMD_FLUSH_START :
            if (datalen == 5)
            {
                unsigned long	val;

                n = (int)tmp[0];
                val = tmp[1] + (tmp[2]*0x100L) + (tmp[3]*0x10000L) + (tmp[4]*0x1000000L);

                if (realtty)
                {
                    sio_flush(port, n);
                    delimiter_flush(port, FLUSH_TX);

                    detail->flush_wait_time[conn] = sys_clock_ms() + val;

                    if (n != 1)
                        detail->flag[conn] |= FLAG_FLUSH_RXDATA;
                    if (n)
                        detail->flag[conn] |= FLAG_FLUSH_TXDATA;
                }
                else
                {
                    buf[rsp++] = D_ASPP_CMD_FLUSH_START;
                    buf[rsp++] = 4;

                    *(unsigned short *) (&buf[rsp++]) = 0;
                    rsp++;
                    *(unsigned short *) (&buf[rsp++]) = 0;
                    rsp++;
                }

            }
            break;
        case D_ASPP_CMD_FLUSH_STOP :
            if (datalen == 0)
            {
                if (realtty)
                {
//					sys_flush_SocketData(conn->fd_data, 0, 0);
                }
                detail->flag[conn] &= ~FLAG_FLUSH_DATA;
                setok = 1;
            }
            break;
        case D_ASPP_CMD_BREAK_COUNT :	/* Get Break Count	*/
            if (datalen == 0)
            {

                buf[rsp++] = D_ASPP_CMD_BREAK_COUNT;
                buf[rsp++] = 2;
                buf[rsp++] = (unsigned char) (detail->break_count >> 8);
                buf[rsp++] = (unsigned char) (detail->break_count & 0x00FF);
                detail->break_count = 0;
            }
            break;
        case D_ASPP_CMD_DATA_STATUS :	/* Get Data Status	*/
            if (datalen == 0)
            {
                buf[rsp++] = D_ASPP_CMD_DATA_STATUS;
                buf[rsp++] = 1;
                buf[rsp++] = (unsigned char) (detail->data_status & 0x0F);
                detail->data_status = 0;
            }
            break;
        }

        if (setok == 1)
        {
            buf[rsp++] = cmd;
            buf[rsp++] = 'O';
            buf[rsp++] = 'K';
        }
        else if (setok == 2)
        {
            buf[rsp++] = cmd;
            buf[rsp++] = 'N';
            buf[rsp++] = 'A';
        }

        if (detail->mode && notify)
        {
            detail->notify_flag = aspp_notify_data(port, (unsigned char *) &(detail->notify_buf[1]));
            if (detail->notify_flag > 0)
            {
                if (detail->notify_buf[1] & 0x10)
                    detail->break_count++;
                detail->data_status |= (detail->notify_buf[1] & 0x0F);
            }
            if (detail->notify_flag > 0)
            {
                detail->notify_buf[3] |= 0x80;		/* 0x80, 0 for old version, 1 for new version */
                detail->notify_buf[0] = D_ASPP_RSP_NOTIFY;
                detail->break_count = 0;
                detail->data_status = 0;
                buf[rsp++] = D_ASPP_RSP_NOTIFY;
                buf[rsp++] = detail->notify_buf[1];
                buf[rsp++] = detail->notify_buf[2];
                buf[rsp++] = detail->notify_buf[3];
            }
        }
        i += datalen;
    }

    return(rsp);
}

int aspp_convert_baud(int baud)
{
    int result;

    switch (baud)
    {
    case D_ASPP_IOCTL_B300:
        result = BAUD_300;
        break;
    case D_ASPP_IOCTL_B600:
        result = BAUD_600;
        break;
    case D_ASPP_IOCTL_B1200:
        result = BAUD_1200;
        break;
    case D_ASPP_IOCTL_B2400:
        result = BAUD_2400;
        break;
    case D_ASPP_IOCTL_B4800:
        result = BAUD_4800;
        break;
    case D_ASPP_IOCTL_B7200:
        result = BAUD_7200;
        break;
    case D_ASPP_IOCTL_B9600:
        result = BAUD_9600;
        break;
    case D_ASPP_IOCTL_B19200:
        result = BAUD_19200;
        break;
    case D_ASPP_IOCTL_B38400:
        result = BAUD_38400;
        break;
    case D_ASPP_IOCTL_B57600:
        result = BAUD_57600;
        break;
    case D_ASPP_IOCTL_B115200:
        result = BAUD_115200;
        break;
    case D_ASPP_IOCTL_B230400:
        result = BAUD_230400;
        break;
    case D_ASPP_IOCTL_B460800:
        result = BAUD_460800;
        break;
    case D_ASPP_IOCTL_B921600:
        result = BAUD_921600;
        break;
    case D_ASPP_IOCTL_B150:
        result = BAUD_150;
        break;
    case D_ASPP_IOCTL_B134:
        result = BAUD_134;
        break;
    case D_ASPP_IOCTL_B110:
        result = BAUD_110;
        break;
    case D_ASPP_IOCTL_B75:
        result = BAUD_75;
        break;
    case D_ASPP_IOCTL_B50:
        result = BAUD_50;
        break;
    default:
        result = BAUD_38400;
        break;
    }
    return result;
}

int aspp_convert_parity(int mode)
{
    int parity, result;

    parity = (int) (mode & 0x38);
    result = (int) (mode & 0x07);

    switch (parity)
    {
    case D_ASPP_IOCTL_NONE:
        result |= P_NONE;
        break;
    case D_ASPP_IOCTL_EVEN:
        result |= P_EVEN;
        break;
    case D_ASPP_IOCTL_ODD:
        result |= P_ODD;
        break;
    case D_ASPP_IOCTL_MARK:
        result |= P_MRK;
        break;
    case D_ASPP_IOCTL_SPACE:
        result |= P_SPC;
        break;
    default:
        result |= P_NONE;
        break;
    }
    return result;
}

int aspp_convert_flow(int cts, int rts, int stx, int srx)
{
    int result = F_NONE;
    if (cts)	result |= F_CTS;		/* CTS flow control */
    if (rts)	result |= F_RTS;		/* RTS flow control */
    if (stx)	result |= F_TXSW;		/* Tx XON/XOFF flow control */
    if (srx)	result |= F_RXSW;		/* Rx XON/XOFF flow control */
    return result;
}

int	aspp_flush_data(int port, int fd_port, int fd_data, int mode)
{
    sio_flush(port, mode);		/* mode = 0 : FLUSH_RX */
    /*      = 1 : FLUSH_TX */
    /*      = 2 : FLUSH_ALL */
    if (mode != 0)		/* Clean tcp iqueue */
        aspp_tcp_flush_iqueue(port, fd_data);

	delimiter_flush(port, mode);
    return 1;
}

int	aspp_flush_reply(int realtty, char * buf, int fd_data)
{
    buf[0] = D_ASPP_CMD_FLUSH_START;
    buf[1] = 4;
    if (realtty)
    {
        int len = aspp_tcp_iqueue(fd_data);
        buf[2] = (unsigned char) (len & 0x00FF);
        buf[3] = (unsigned char) ((len & 0xFF00) >> 8);
        *(unsigned short *)(&buf[4]) = 0;	/* can't get tcp oqueue from linux kernel */
    }
    else
    {
        *(unsigned short *)(&buf[2]) = 0;
        *(unsigned short *)(&buf[4]) = 0;
    }
    return 6;
}


int aspp_accept_data(int port)
{
    struct port_data *ptr;
    int i, n, on=1, find=0;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;
    i = 0;
    while (!find)
    {
        if (((detail->flag[i] & FLAG_DATA_UP)== 0) && (detail->fd_data[i] <= 0))
        {
#ifdef SUPPORT_LOG_CONNECT
            log_opmode_connection info;
#endif // SUPPORT_LOG_CONNECT
#ifdef SUPPORT_SERIALTOS
            int enable;
#endif // SUPPORT_SERIALTOS

            int value;
//          int data=0;

            n = sizeof(struct sockaddr_in);

            // check accept, if fail, return.
            /* Becouse next connect form client maybe connected in backlog,
             * but at accept this connection is be disconnect,
             * it will accept fail, and return -1.
             * So need to check accept return value, if accept fail, exit this functin, wait next accept.
             */
            if( (detail->fd_data[i] = accept(detail->fd_data_listen, (struct sockaddr *) &detail->peer[i], (socklen_t*)&n)) < 0)
            {
                // accept fail
                dbg_printf("accept data fail: %d\r\n", errno);
                if(detail->connect_count == 0) {
                    aspp_close_serial(port);
		}
                break;
            }
            detail->flag[i] |= FLAG_DATA_UP;
            detail->data_sent[i] = 0;	/* Albert.20120103: add */

            if (detail->connect_count == 0)
            {
                detail->notify_flag = 0;
                memset(&(detail->notify_buf), '\0', 5);
                detail->break_count = 0;
                sio_DTR(port, 1);		 	/* DTR on */
                sio_RTS(port, 1);			/* RTS on */
                delimiter_start(port, detail->fd_port, detail->backlog, detail->fd_data, detail->data_sent, aspp_sendfunc, aspp_recvfunc, 1);
            }

			sys_send_events(EVENT_ID_OPMODE_CONNECT, port << 4);

            on = 1;
            setsockopt(detail->fd_data[i], IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
/*
            data = DCF_SOCK_BUF;
            setsockopt(detail->fd_data[i], SOL_SOCKET, SO_RCVBUF, (char *)&data, sizeof(data));
            data = DCF_SOCK_BUF;
            setsockopt(detail->fd_data[i], SOL_SOCKET, SO_SNDBUF, (char *)&data, sizeof(data));
*/

#ifdef SUPPORT_TCP_KEEPALIVE
            tcp_setAliveTime(port, detail->fd_data[i]);
#endif // SUPPORT_TCP_KEEPALIVE

            /* serial tos */
#ifdef SUPPORT_SERIALTOS
            Scf_getSerialPortTos(port, &enable, &value);
            if(enable)
            {
                setsockopt(detail->fd_data[i], IPPROTO_IP, IP_TOS, (int *)&value, sizeof(value));
            }
#endif // SUPPORT_SERIALTOS

            /* non-block */
            on = 1;
            ioctl(detail->fd_data[i], FIONBIO, &on);

            /* set socket low-water to 2 */
            value = 2;
            setsockopt(detail->fd_data[i], SOL_SOCKET, SO_SNDLOWAT, &value, sizeof(value));
#if 0
            /* linksio */
            value = detail->fd_port;
            setsockopt(detail->fd_data[i], SOL_SOCKET, SO_LINKSIO, &value, sizeof(value));
#endif
            detail->last_time[i] = sys_clock_ms();
            detail->connect_count++;
            find = 1;

#ifdef SUPPORT_LOG_CONNECT
            info.port_id = port + 1;
            *(in_addr_t*)info.ip = detail->peer[i].sin_addr.s_addr;
            Slog_put(DLOG_OPMODE_CONNECT, &info, sizeof(info));
#endif // SUPPORT_LOG_CONNECT

            //@@ add by Kevin
            Gaspp_socket_stat[0][i].remote_ip = htonl(detail->peer[i].sin_addr.s_addr);
            Gaspp_socket_stat[0][i].remote_port = htons(detail->peer[i].sin_port);
            //Gaspp_socket_stat[port-1][i].tcp_state = TCPS_ESTABLISHED;
            //@@ end
        }
        else
            i++;
        if (i == detail->backlog)
            break;
    }

#ifdef DISABLE_LINUX_SYN_BACKLOG
	return i;
#else
	return find;
#endif

}

int aspp_accept_cmd(int port)
{
    struct sockaddr_in	sin;
    struct port_data *ptr;
    int i, n, on=1, find=0;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;
    i = 0;
    while (!find)
    {
        if (((detail->flag[i] & FLAG_CMD_UP)== 0) && (detail->fd_cmd[i] <= 0))
        {
#ifdef SUPPORT_SERIALTOS
            int enable;
#endif // SUPPORT_SERIALTOS
            int value;
            n = sizeof(struct sockaddr_in);

            // check accept, if fail, return.
            // Reason is same as accept data port.
            if((detail->fd_cmd[i] = accept(detail->fd_cmd_listen, (struct sockaddr *) &sin, (socklen_t*)&n)) < 0)
            {
                dbg_printf("accept cmd fail: %d\r\n", errno);
                break;
            }
            detail->flag[i] |= FLAG_CMD_UP;
/*
            on = DCF_SOCK_BUF;
            setsockopt(detail->fd_cmd[i], SOL_SOCKET, SO_RCVBUF, (char *)&on, sizeof(on));
            on = DCF_SOCK_BUF;
            setsockopt(detail->fd_cmd[i], SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof(on));
*/
            on = 1;
            setsockopt(detail->fd_cmd[i], IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

#ifdef SUPPORT_TCP_KEEPALIVE
            tcp_setAliveTime(port, detail->fd_cmd[i]);
#endif // SUPPORT_TCP_KEEPALIVE

#ifdef SUPPORT_SERIALTOS
            /* serial tos */
            Scf_getSerialPortTos(port, &enable, &value);
            if(enable)
            {
                setsockopt(detail->fd_cmd[i], IPPROTO_IP, IP_TOS, (int *)&value, sizeof(value));
            }
#endif // SUPPORT_SERIALTOS

            on = 1;
            ioctl(detail->fd_cmd[i], FIONBIO, &on);

            /* set socket low-water to 2 */
            value = 2;
            setsockopt(detail->fd_cmd[i], SOL_SOCKET, SO_SNDLOWAT, &value, sizeof(value));

            detail->cmd_connect_count++;
            find = 1;
        }
        else
            i++;
        if (i == detail->backlog)
            break;
    }

#ifdef DISABLE_LINUX_SYN_BACKLOG
	return i;
#else
	return find;
#endif

}


void aspp_close_data(int port, int index)
{
    struct port_data *ptr;
    ASPP_SERIAL *detail;
    //u_long value=0;
#ifdef SUPPORT_LOG_CONNECT
    log_opmode_connection	info;
#endif // SUPPORT_LOG_CONNECT
    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    //setsockopt(detail->fd_data[index], SOL_SOCKET, SO_UNLINKSIO, &value, sizeof(value));

    detail->connect_count--;

#ifdef SUPPORT_LOG_CONNECT
    info.port_id = port + 1;
    *(in_addr_t*)info.ip = detail->peer[index].sin_addr.s_addr;
    Slog_put(DLOG_OPMODE_DISCONNECT, &info, sizeof(info));
#endif // SUPPORT_LOG_CONNECT

    close(detail->fd_data[index]);
    detail->flag[index] &= ~FLAG_DATA_UP;
    detail->data_sent[index] = 0;
    detail->fd_data[index] = -1;
    memset(&detail->peer[index], '\0', sizeof(struct sockaddr_in));

	sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);

    //@@ add by Kevin
    Gaspp_socket_stat[0][index].remote_ip = 0;
    Gaspp_socket_stat[0][index].remote_port = 0;
    //Gaspp_socket_stat[port-1][index].tcp_state = TCPS_LISTEN;
    //@@ end
}

void aspp_close_cmd(int port, int index)
{
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    detail->cmd_connect_count--;
    close(detail->fd_cmd[index]);
 
    detail->flag[index] &= ~(FLAG_CMD_UP|FLAG_SET_NOTIFY);	/* Albert.20120102: add FLAG_SET_NOTIFY */
    detail->fd_cmd[index] = -1;
}

int aspp_open_serial(int port)
{
    int	baud, mode, flowctrl;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    if ((detail->fd_port = sio_open(port)) < 0)
    {
        SHOW_LOG(stderr, port, MSG_ERR,
                    "Fail to open serial port %d, please check if the serial port has been opened.\n", port);
        exit(EXIT_FAILURE);
    }

    sio_DTR(port, 0);             /* DTR off at init state */
    sio_RTS(port, 0);             /* RTS off at init state */
    sio_flush(port, FLUSH_ALL);

    sio_fifo(port, Scf_getAsyncFifo(port));		/* FIFO */
    Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    sio_ioctl(port, baud, mode);      /* Set baud rate, data bits, stop bits and parity */

	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
    sio_flowctrl(port, _sio_mapFlowCtrl(flowctrl)); /* Set flow control */

    return 1;
}


void aspp_close_serial(int port)
{
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    sio_flush(port, FLUSH_ALL);
    sio_flowctrl(port, F_NONE); /* Set None flow */

#ifdef SUPPORT_CONNECT_GOESDOWN
    {
        int rtsdtr;
        rtsdtr = Scf_getRtsDtrAction(port);
        sio_DTR(port, rtsdtr & 2);
        sio_RTS(port, rtsdtr & 1);
    }
#endif // SUPPORT_CONNECT_GOESDOWN
    sio_close(port);
    detail->serial_flag = 0;
}


void aspp_update_lasttime(int port)
{
    int i;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    for (i=0; i< detail->backlog; i++)
    {
        if (detail->flag[i] & FLAG_DATA_UP)
            detail->last_time[i] = sys_clock_ms();
    }
}


int aspp_tcp_iqueue(int fd)
{
    int bytes;
    ioctl(fd, FIONREAD, &bytes);
    return bytes;
}

void aspp_tcp_flush_iqueue(int port, int fd)
{
    char buf[1024];
    int len;
    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    while ((len = aspp_tcp_iqueue(fd)) > 0)
    {
        if (len > 1024)
            len = 1024;

        recv(fd, buf, len, 0);
    }
}

//int old_msr=0;
//int old_hold=0;
static int aspp_notify_data(int port, unsigned char* buf)
{
    int msr, hold, msr_delt;
    int error;
    int result=0;

    struct port_data *ptr;
    ASPP_SERIAL *detail;

    ptr = &Gport;
    detail = (struct aspp_serial *) ptr->detail;

    if( (sys_clock_ms() - detail->notify_lastpoll) > 100)
    {
        msr = hold = result = msr_delt = 0;
        sio_notify_status(port, &msr, &hold);

        error = sio_notify_error(port);

        if(msr != detail->old_msr)
        {
            result = 1;
            msr_delt = (msr >> 4) ^ (detail->old_msr >> 4);
            detail->old_msr = msr;
            error |= 0x20;
        }

        if(hold != detail->old_hold)
        {
            result = 1;
            detail->old_hold = hold;
        }

        if(error)
            result = 1;

        if(result)
        {
            buf[0] = (char)error;
            buf[1] = (char)(msr|msr_delt);
            buf[2] = (char)hold;
        }
        detail->notify_lastpoll = sys_clock_ms();
    }
    return result;
}

