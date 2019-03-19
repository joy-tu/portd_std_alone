
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    pair.c

    Pair Connection Master/Slave handle routine

    2011-01-10  James Wang
        First release.
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sio.h>
#include <systime.h>
#include <header.h>
#include <config.h>
#include <telnet.h>
#include <debug.h>
#include <datalog.h>
#include <portd.h>
#include <eventd.h>
#include <sysapi.h>
#include <delimit.h>

#define D_SET_CONTROL       5   /* COM Port Option: Set Control */

#define MAX_TMP_LEN 256

#define PAIR_RECONNECT_TIMEOUT  1500        /* reconnect time interval 100 mini-seconds */
#define PAIR_CHECK_CONNECT_TIMEOUT  5000    /* check connect timeout 5 seconds */


extern int portd_terminate;

void *pair_slave_start(void *portno);
void *pair_master_start(void *portno);
static void main_loop(int port, int fd_port, int fd_net, int mode);
static void link_telnet_setting(int fd_net, int mode, long *telflag, int port);
static void link_telnet_checkbreak(int port, int fd_net);
void _pair_telnet_setting(int fd_net, int mode, long *telflag, int port);
int pair_telnet_encode(int fd_net, unsigned char *source, int len, u_int quitkey, long telflag);
int pair_telnet_decode(int fd_net, unsigned char *source, int len, unsigned char *target, long *telflag, char *termcap, int port);

static unsigned long pair_reconnect(int port, unsigned long retry_time);
static void pair_connect(int port);
static unsigned long pair_check_connect(int port, unsigned long check_time);
static void pair_connect_ok(int port);
static void pair_connect_fail(int port);
static unsigned short link_getSrcTcpPort(int port);
static void serial_init(int port);
static int isconnected(int sockfd, fd_set *rd, fd_set *wr, fd_set *ex);

static int pair_fd_net, pair_fd_net_wait;



void *pair_master_start(void *portno)
{
    int fd_port, fd_net = -1, connected, port, iftype;
    char dhost[64];
    struct timeval  time;
    INT16U dport, lport;
    unsigned long  connect_retry_time, connect_check_time;

	port = (int)portno;
    memset(dhost, 0, sizeof(dhost));
    Scf_getTcpClientHost(port, 1, dhost, sizeof(dhost), &dport, &lport);
//printf("dhost=[%s]\n", dhost);
    if (!strlen(dhost) || (dport <= 0))
    {
        while (!portd_terminate)
            usleep(100*1000);
        return (void *)1;
    }

    while ( (fd_port = sio_open(port)) < 0 )
        usleep(1000*1000);

	sio_fifo(port, Scf_getAsyncFifo(port));		/* FIFO */	// for debug
    serial_init(port);  /* Set baud rate, data bits, stop bits, parity and flow control */

    sio_DTR(port, 0);      /* DTR off at init state */
    sio_RTS(port, 0);

    sio_flush(port, 2);
		
    connect_retry_time = 0;
    time.tv_sec = 1;
    time.tv_usec = 0;

    while (!portd_terminate)  /* main while */
    {
        if ((sys_clock_ms() - connect_retry_time) < PAIR_RECONNECT_TIMEOUT)
        {/* When master is not secure-enabled and slave is secure-enabled,
            master will send telnet handshake packet to slave after connected,
            and slave will parse it as unknow protocol and reset connection immediately,
            then master will quit main_loop() and go back here immediately,
            causing quick connect/disconnect.
            So we must sleep to prevent busy connect/disconnect. */
            usleep(PAIR_RECONNECT_TIMEOUT*1000);
        }

        pair_fd_net = -1;
        pair_fd_net_wait = -1;
        connect_retry_time = sys_clock_ms() + PAIR_RECONNECT_TIMEOUT;
        connect_check_time = sys_clock_ms();
        connected = 0;

        /* connect */
        while (!portd_terminate)
        {
            connect_retry_time = pair_reconnect(port, connect_retry_time);
            connect_check_time = pair_check_connect(port, connect_check_time);

            if (pair_fd_net > 0)
            {
                //dbg_printf("connected, fd= %d\r\n", pair_fd_net);
                connected = 1;
                break;
            }
            else
                usleep(1000*1000);
        }

        if (!connected)
            continue; /* to main while */

        fd_net = pair_fd_net;


        if (!portd_terminate)
        {
            /* connect ok if reaches here */
            sio_DTR(port, 1);
            sio_RTS(port, 1);
						
			iftype = Scf_getIfType(port);
			sio_setiftype(port, iftype);
            main_loop(port, fd_port, fd_net, 0);
        }

        close(fd_net);
        fd_net = -1;
    }

    // albert: we didn't pass &ssl to main_loop(), so ssl didn't set to NULL when main_loop() was quit,
    //           so don't call sslFreeConnection() here since we cannot judge whether ssl was freed.
    if (fd_net != -1)
    {
        close(fd_net);
        fd_net = -1;
    }
    sio_close(port);

	port_buffering_check_restart(port);
	
    return (void *)1;
}

void *pair_slave_start(void *portno)
{
    int len, fd_port, fd_listen, fd_net, accepted, flags, yes, port;
    unsigned short sport;
//    unsigned long  arg;
    struct sockaddr_in sin;
    fd_set  rfds;
    struct timeval  time;

	port = (int)portno;
    sport = link_getSrcTcpPort(port);

    while ( (fd_port = sio_open(port)) < 0 )
        sleep(1);

	sio_fifo(port, Scf_getAsyncFifo(port));		/* FIFO */	// for debug
    serial_init(port);  /* Set baud rate, data bits, stop bits, parity and flow control */

    sio_DTR(port, 0);      /* DTR off at init state */
    sio_RTS(port, 0);
    sio_flush(port, 2);

    while ((fd_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        sleep(1);

    yes = 1;
    while (setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        sleep(1);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(sport);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(fd_listen, (struct sockaddr*)&sin, sizeof(sin));

    len = sizeof(sin);
    fd_net = -1;
    while (!portd_terminate)  /* main while */
    {
        accepted = 0;
        listen(fd_listen, 1);
        /* accept */
        while (!portd_terminate)
        {
            FD_ZERO(&rfds);
            FD_SET(fd_listen, &rfds);

            time.tv_sec = 0;
            time.tv_usec = 5000L;
            //printf("select()\n");
            if( select(fd_listen+1, &rfds, NULL, NULL, &time) > 0 )
            {
                if (FD_ISSET(fd_listen, &rfds))
                {
                    if ((fd_net = accept(fd_listen, (struct sockaddr*)&sin, (socklen_t*) &len)) >= 0)
                    {
                        accepted = 1;
                        break;
                    }
                }
                if (accepted)
                    break;
            }
        }

        if (!accepted)
            continue; /* to main while */

        flags = fcntl(fd_net, F_GETFL, 0);
        fcntl(fd_net, F_SETFL, flags | O_NONBLOCK);

        //arg = Scfw_getTcpAliveCheck(port);
        //ioctlsocket(fd_net, FIOTCPALIVE, &arg);
// ===== perry modify start =====
//        arg = 1;
//        setsockopt(fd_net, SOL_SOCKET, SO_KEEPALIVE, (void *)&arg, sizeof(arg));
//        setsockopt(fd_net, IPPROTO_TCP, TCP_NODELAY, (char *)&arg, sizeof(arg));
#ifdef SUPPORT_TCP_KEEPALIVE
            tcp_setAliveTime(port, fd_net);
#endif // SUPPORT_TCP_KEEPALIVE
// ===== perry modify end =====

        if (!portd_terminate)
        {
            /* accept ok if reaches here */
            sio_DTR(port, 1);
            sio_RTS(port, 1);
{
	int iftype;
	iftype = Scf_getIfType(port);
	sio_setiftype(port, iftype);
}
            //dbg_printf("connected, fd_net = %d\r\n", fd_net);
            main_loop(port, fd_port, fd_net, 0);
        }

        close(fd_net);
        fd_net = -1;
    }

  // albert: we didn't pass &ssl to main_loop(), so ssl didn't set to NULL when main_loop() was quit,
  //           so don't call sslFreeConnection() here since we cannot judge whether ssl was freed.
    if (fd_net > 0)
        close(fd_net);

    close(fd_listen);
    sio_close(port);

	port_buffering_check_restart(port);
	
    return (void *)1;
}

int fd_net_pair;

static  void    main_loop(int port, int fd_port, int fd_net, int mode)
{
    static unsigned char tmp[MAX_TMP_LEN], tmp1[MAX_TMP_LEN];
    int     i, maxfd, flags;
    long    telflag;
    int     net_write_flag = 0, port_write_flag = 0, port_buffering_enabled = 0;
    struct timeval time;
    fd_set  rfds, wfds;
    int   (*pair_sio_write)(int port, char *buf, int len);
    int   (*pair_sio_read)(int port, char *buf, int len);
	int serial_data_buffered;

	sys_send_events(EVENT_ID_OPMODE_CONNECT, port << 4);

	if (Scf_getSerialDataLog(port))
	{
		pair_sio_write = log_sio_write;
		pair_sio_read = log_sio_read;
	}
	else
	{
		pair_sio_write = sio_write;
		pair_sio_read = sio_read;
	}

	if (port_buffering_active(port))
	{
		fd_net_pair = fd_net;
		pair_sio_read = buffering_sio_read;
		port_buffering_enabled = 1;
	}
	else
	{
    	sio_flush(port, FLUSH_ALL);
	}

    maxfd = MAX(fd_port, fd_net);

    link_telnet_setting(fd_net, mode, &telflag, port);

    //i = 0;
    //ioctlsocket(fd_net, FIONBIO, (unsigned long*)&i);

    flags = fcntl(fd_net, F_GETFL, 0);
    flags &= ~O_NONBLOCK;
    fcntl(fd_net, F_SETFL, flags);

    /******** MAIN LOOP ********/
    while (!portd_terminate)
    {
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        if ( port_write_flag )
            FD_SET(fd_port, &wfds);
        else
            FD_SET(fd_net, &rfds);

		serial_data_buffered = delimiter_check_buffered(port);
        if ( net_write_flag || serial_data_buffered )
            FD_SET(fd_net, &wfds);

		if (!net_write_flag || port_buffering_enabled)
            FD_SET(fd_port, &rfds);

        time.tv_sec = 0;
        time.tv_usec = 100*1000L;
        if ( select(maxfd+1, &rfds, &wfds, NULL, &time) <= 0 )
        {
            link_telnet_checkbreak(port, fd_net);
            continue;
        }
        link_telnet_checkbreak(port, fd_net);

        if ( FD_ISSET(fd_net, &rfds) )
        {
/*
            if ( sio_ofree(port) < MAX_TMP_LEN )
                port_write_flag = 1;
            else
*/
            {
                if ( (i = recv(fd_net, (char *)tmp, MAX_TMP_LEN, 0)) == 0 )
                {
                    break;  /* link broken */
                }

                if (i > 0)
                {
                    i = pair_telnet_decode(fd_net, tmp, i, tmp1, &telflag, "ansi", port);
                    if ( i > 0 )
                    {
                        int wr, idx, cnt;
                        idx = 0;
                        cnt = 0;
                        while ((wr = pair_sio_write(port, (char*)tmp1+idx, i)) != i)
                        {
                            /* must retry. 07-11-2006, albert */
                            if (++cnt > 20)
                                break;
                            if (wr <= 0)
                                continue;
                            idx += wr;
                            i -= wr;
                            usleep(1000);
                        }
                    }
            /*
             * modify at 07-01-1998 by Yu-Lang Hsu
             */
                    if ( telflag & TFLAG_SENDBRK )
                    {
                        telflag &= ~TFLAG_SENDBRK;
                        sio_break(port, 1);
                        usleep(D_SENDBRK_TIME*1000);
                        sio_break(port, 0);
                    }
                }
            }
        }
        if ( FD_ISSET(fd_net, &wfds) )
        {
			if (serial_data_buffered)
			{
				if ( tcp_ofree(fd_net) >= MAX_TMP_LEN*2 )
				{
					if ((i = buffering_read(port, (char *)tmp, MAX_TMP_LEN)) > 0)
					{
						if (pair_telnet_encode(fd_net, tmp, i, 0xFFFF, telflag) != 0)
							break;      /* link broken */
						//PortdLastTime[port] = sys_clock_s();
					}
				}
			}
            net_write_flag = 0;
        }

        if ( FD_ISSET(fd_port, &rfds) )
        {
			if ( !port_buffering_enabled && (tcp_ofree(fd_net) < MAX_TMP_LEN*2) )
				net_write_flag = 1;
            else if ( (i = pair_sio_read(port, (char *)tmp, MAX_TMP_LEN)) > 0 )
            {
                if (pair_telnet_encode(fd_net, tmp, i, 0xFFFF, telflag) != 0)
                    break;      /* link broken */
            }
        }
        if ( FD_ISSET(fd_port, &wfds) )
        {
            port_write_flag = 0;
        }
    }
    sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);
	port_buffering_check_restart(port);

    /* Wait all data is sent out */
    portd_wait_empty(port, -1, 30000L);
}

static unsigned long pair_reconnect(int port, unsigned long retry_time)
{
    if( (sys_clock_ms() - retry_time) > PAIR_RECONNECT_TIMEOUT )
    {
        if( (pair_fd_net < 0) && (pair_fd_net_wait < 0) )
        {
            pair_connect(port);
        }
        return sys_clock_ms();
    }
    else
        return retry_time;
}

static void pair_connect(int port)
{
    INT16U dport, lport;
    char dhost[64];
    struct sockaddr_in sin;
    int flags, t;

    memset(dhost, 0, sizeof(dhost));
    Scf_getTcpClientHost(port, 1, dhost, sizeof(dhost), &dport, &lport);
    memset(&sin, '\0', sizeof(sin));

    if ((pair_fd_net < 0) && (pair_fd_net_wait < 0))
    {
        if( (pair_fd_net_wait = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0 )
        {
            flags = fcntl(pair_fd_net_wait, F_GETFL, 0);
            fcntl(pair_fd_net_wait, F_SETFL, flags | O_NONBLOCK);


            sin.sin_family = AF_INET;
            sin.sin_port = htons(dport);
            sin.sin_addr.s_addr = inet_addr(dhost);
            t = connect(pair_fd_net_wait, (struct sockaddr*)&sin, sizeof(sin));
            if( (int)t >= 0 )
            {
                pair_connect_ok(port);
            }
        }
    }
}

static unsigned long pair_check_connect(int port, unsigned long check_time)
{
    int rc;
    struct timeval tv;
    fd_set rfds, wfds, efds;

    if( (pair_fd_net < 0) && (pair_fd_net_wait > 0) )
    {
        FD_ZERO(&rfds);
        FD_SET(pair_fd_net_wait, &rfds);
        wfds = efds = rfds;
        tv.tv_sec = 0;
        tv.tv_usec = 5 * 1000L;
        rc = select(pair_fd_net_wait+1, &rfds, &wfds, &efds, &tv);
        if (rc < 0)
        {
            pair_connect_fail(port);
        }
        else if (isconnected(pair_fd_net_wait, &rfds, &wfds, &efds))
        {
            /* connection success */
            pair_connect_ok(port);
        }
        else
        {
            if ((sys_clock_ms() - check_time) < PAIR_RECONNECT_TIMEOUT)
                return check_time;
            else
                pair_connect_fail(port);
        }
    }

    return sys_clock_ms();
}

static void pair_connect_ok(int port)
{

    if( (pair_fd_net < 0) && (pair_fd_net_wait > 0) ) /* Connecting state */
    {
        pair_fd_net = pair_fd_net_wait;
        //t = Scfw_getTcpAliveCheck(port);
        //ioctlsocket(pair_fd_net, FIOTCPALIVE, &t);
// ===== perry modify start =====
#ifdef SUPPORT_TCP_KEEPALIVE
		tcp_setAliveTime(port, pair_fd_net);
#endif // SUPPORT_TCP_KEEPALIVE
// ===== perry modify end =====
    }
}

static void pair_connect_fail(int port)
{
    if( (pair_fd_net < 0) && (pair_fd_net_wait >0) ) /* Connecting state */
    {
        close(pair_fd_net_wait);
        pair_fd_net = -1;
        pair_fd_net_wait = -1;
    }
}

static  int link_line_stat;
static  void    link_telnet_setting(int fd_net, int mode, long *telflag, int port)
{
    _pair_telnet_setting(fd_net, mode, telflag, port);
    link_line_stat = 3;
}

static  void    link_telnet_checkbreak(int port, int fd_net)
{
    unsigned char  tmp[16];
    int ls, ols, n;

    if ( tcp_ofree(fd_net) < 16 )
        return;
    tmp[0] = D_TELNET_IAC;
    n = sio_notify_error(port);
    if ( n & 0x10 ) {
        tmp[1] = D_TELNET_BRK;
      send(fd_net, (char *)tmp, 2, 0);
    }
    else
    {
        ls = sio_lstatus(port) & 3;
        n = sio_getflow(port);
        if ( (n & 1) )      /* do CTS flow control ? */
            ls |= 1;
        if ( (n & 0x10) )   /* do DSR flow control ? */
            ls |= 2;
        ols = link_line_stat;
        if ( ls != ols )
        {
            tmp[7] = D_TELNET_IAC;
            tmp[1] = tmp[8] = D_TELNET_SB;
            tmp[2] = tmp[9] = D_TELNET_COMPORT;
            tmp[3] = tmp[10] = D_SET_CONTROL;
            tmp[5] = tmp[12] = D_TELNET_IAC;
            tmp[6] = tmp[13] = D_TELNET_SE;
            ols ^= ls;
            n = 0;
            if ( (ols & 2) )
            {
                if ( (ls & 2) )
                    tmp[4] = D_SET_DTR_ON;
                else
                    tmp[4] = D_SET_DTR_OFF;
                n += 7;
            }
            if ( (ols & 1) )
            {
                if ( (ls & 1) )
                    tmp[n+4] = D_SET_RTS_ON;
                else
                    tmp[n+4] = D_SET_RTS_OFF;
                n += 7;
            }
            link_line_stat = ls;
            if ( n )
            {
                send(fd_net, (char *)tmp, n, 0);
            }
        }
    }
}

static unsigned short link_getSrcTcpPort(int port)
{
    unsigned short srcport, cmdport;
    Scf_getTcpServer(port, &srcport, &cmdport);
    return srcport;
}


static void serial_init(int port)
{
    int ret;
    int baud, mode, flowctrl, flowmode = 0;

    ret = Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    ret = sio_ioctl(port, baud, mode);    /* Set baud rate, data bits, stop bits and parity */

    switch (flowctrl)
    {
        case 0:        /* None */
            flowmode = 0x00;
            break;
        case 1:        /* RTS/CTS */
            flowmode = F_HW;
            break;
        case 2:        /* XON/XOFF */
            flowmode = F_SW;
            break;
        default:
            flowmode = 0x00;
            break;
    }
	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowmode != F_SW )
			flowmode = F_NONE;
	}
	
    ret = sio_flowctrl(port, flowmode);    /* Set flow control */
}

static int isconnected(int sockfd, fd_set *rd, fd_set *wr, fd_set *ex)
{
    int err;
    int len = sizeof(err);

    errno = 0;              /* assume no error */

    if( !FD_ISSET( sockfd, rd ) && !FD_ISSET( sockfd, wr) )
        return 0;
    if( FD_ISSET(sockfd, rd) && FD_ISSET(sockfd, wr) )
        return 0;
    if( getsockopt( sockfd, SOL_SOCKET, SO_ERROR, &err, (socklen_t*) &len ) < 0 )
        return 0;
    errno = err;
    return err == 0;
}
