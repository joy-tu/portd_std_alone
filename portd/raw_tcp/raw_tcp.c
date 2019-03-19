/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : raw_tcp.c                                                  */
/* Description  :                                                            */
/* Product      : Secured Serial Device Server                               */
/* Programmer   : Shinhsy Shaw                                               */
/* Date         : 2003-07-22                                                 */
/*****************************************************************************/
#ifndef _RAW_TCP_C
#define _RAW_TCP_C
#endif


#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>  // sleep()
#include    <config.h>
#include    <portd.h>
#include    "raw_tcp.h"
#include    <sio.h>
#include    <fcntl.h>
#include    <netinet/tcp.h>
#include    <sysmaps.h>
#include    <sysapi.h>
#include    <support.h>
#include	<eventd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef SUPPORT_SERCMD
extern int Gscm_active;
extern int Gscm_online;
#endif
static int isconnected(int sockfd, fd_set *rd, fd_set *wr, fd_set *ex);

extern PORT_DATA Gport;
extern int portd_terminate;

int dport[DCF_MAX_SERIAL_PORT][TCP_CLIENT_MAX_CONNECT];

int raw_tcp_read(int port, int fd_net, char *buf, int len);
int raw_tcp_write(int port, int fd_net, char *buf, int len);

void * raw_tcp_start(void* portno)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    int i;
    int port;

    port = (int)portno;
    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    memset(detail, 0, sizeof(detail));

  	if (!detail->port_opened)
  	{
	    while((detail->fd_port = sio_open(port)) < 0)
	        sleep(1);
    	detail->port_opened = 1; // port will not be closed when port buffering is enabled
    }

    sio_fifo(port, Scf_getAsyncFifo(port));     /* FIFO */

  	if (port_buffering_active(port))
  	{
    	port_buffering_start(port);
  	}
  	else
  	{
	    sio_DTR(port, 0);             /* DTR off at init state */
	    sio_RTS(port, 0);             /* RTS off at init state */
	    sio_flush(port, FLUSH_ALL);
    }

    /* startup_mode : bit 1 : TCP connect on Startup            */
    /*                bit 2 : TCP connect on Any Character  */
    /*                bit 3 : TCP connect on DSR on */
    /*                bit 4 : TCP connect on DCD on */
    detail->startup_mode = Scf_getTcpClientMode(port);

    for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
    {
        detail->fd_net[i] = -1;
        detail->fd_net_wait[i] = -1;
        detail->portno[i] = 0;
        dport[port-1][i] = 0;
        Scf_getTcpClientHost(port, i+1, detail->dest_ip_dns[i], sizeof(detail->dest_ip_dns[i]), (u_short*)&detail->portno[i], (u_short*)&dport[port-1][i]);

        //printf("IP = %s, port = %d(%d)\r\n", detail->dest_ip_dns[i], detail->portno[i], dport[port-1][i]);
    }

    // Ignore jammed IP
    detail->ctrlflag = 0;
    if(Scf_getSkipJamIP(port))
        detail->ctrlflag |= CTRLFLAG_SKIPJAM;
    // end Ignore jammed IP
    // add by Sean 2008.09.18
    delimiter_start(port, detail->fd_port, TCP_CLIENT_MAX_CONNECT, detail->fd_net, detail->data_sent, raw_tcp_write, raw_tcp_read, 1);
    // end add

    raw_tcp_main(port);

	port_buffering_check_restart(port);

    for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
    {
        if(detail->fd_net[i] >= 0)
            close(detail->fd_net[i]);
        if(detail->fd_net_wait[i] >= 0)
            close(detail->fd_net_wait[i]);
        detail->fd_net[i] = -1;
        detail->fd_net_wait[i] = -1;
    }

  	if (!port_buffering_active(port))
  	{
	    sio_DTR(port, 0);             /* DTR off  */
	    sio_RTS(port, 0);             /* RTS off  */
	    sio_flush(port, FLUSH_ALL);
	    sio_close(port);
	    detail->fd_port = -1;
    	detail->port_opened = 0;
    }

    sighup_handler(1);

    pthread_exit(NULL);
    return NULL;
}

void raw_tcp_main(int port)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    int maxfd, n, i;
    int net_write_flag = 0, port_write_flag = 0;
	int port_buffering = port_buffering_active(port);
	int serial_data_buffered;
    struct timeval tv;
    fd_set rfds, wfds;
    int baud, mode, flowctrl;
    unsigned long idletime, lasttime;
    unsigned long t;
    unsigned long connect_retry_time = sys_clock_ms() + RAW_TCP_RECONNECT_TIMEOUT;
    unsigned long connect_check_time = sys_clock_ms();
    //  int data_buffering =  (int) Scf_getDataBufferingEnable(port);

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    portd_terminate = 0;

    Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    sio_ioctl(port, baud, mode);      /* Set baud rate, data bits, stop bits and parity */

    switch(flowctrl)
    {
    case 0:     /* None */
        flowctrl = F_NONE;
        break;
    case 1:     /* RTS/CTS */
        flowctrl = F_HW;
        break;
    case 2:     /* XON/XOFF */
        flowctrl = F_SW;
        break;
        //      case 3:     /* DTR/DSR */
        //          flowctrl = 0x60;
        //          break;
    default:
        flowctrl = F_NONE;
        break;
    }
	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
    sio_flowctrl(port, flowctrl); /* Set flow control */

    sio_DTR(port, 1);             /* DTR on */
    sio_RTS(port, 1);             /* RTS on */

    n = Scf_getInactivityTime(port);
    /* Connect on = any, Disconnect by = inactivity time, and time must be non zero */
    /* with this case, we would look at inactivity time */
    if((detail->startup_mode & DCF_CLI_MODE_ON_ANYCHAR) &&
            (detail->startup_mode & DCF_CLI_MODE_OFF_INACT) && (n != 0))
    {
        idletime = (u_long)n;
    }
    else
    {
        idletime = 0xFFFFFFFFL;
    }

	if (!port_buffering)
    	sio_flush(port, FLUSH_ALL);

    while((portd_terminate == 0) && ((detail->startup_mode & DCF_CLI_MODE_ON_MASK) != DCF_CLI_MODE_ON_STARTUP))
    {
        if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_ANYCHAR)              /* try to connect on waiting Any Character */
        {
            if(sio_iqueue(port) != 0)
                break;
        }
        else if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_DSRON)         /* DSR ON */
        {
            if(sio_lstatus(port) & S_DSR)
            {
                sio_flush(port, FLUSH_ALL);
                break;
            }
        }
        else if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_DCDON)         /* DCD ON */
        {
            if(sio_lstatus(port) & S_CD)
            {
                sio_flush(port, FLUSH_ALL);
                break;
            }
        }
        usleep(1000);
        continue;
    }

    if(portd_terminate)
        return;

    detail->count = 0;

    lasttime = sys_clock_ms();
    tv.tv_sec = 0;
    tv.tv_usec = 100;

    /******** MAIN LOOP ********/
    while(!portd_terminate)
    {
#ifdef SUPPORT_SERCMD
        if (Gscm_active) {
            usleep(100000);
            continue;
        }
#endif
        maxfd = detail->fd_port;
        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
            if((detail->fd_net[i] > 0) && (detail->fd_net_wait[i] > 0))
                maxfd = MAX(detail->fd_net[i], maxfd);

        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        if(port_write_flag)
            FD_SET(detail->fd_port, &wfds);
        else
        {
            for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
                if((detail->fd_net[i] > 0) && (detail->fd_net_wait[i] > 0))
                    FD_SET(detail->fd_net[i], &rfds);
        }

    	serial_data_buffered = delimiter_check_buffered(port);

        if(net_write_flag || serial_data_buffered )
        {
            for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
                if((detail->fd_net[i] > 0) && (detail->fd_net_wait[i] > 0))
                    FD_SET(detail->fd_net[i], &wfds);
        }
		/* bugfix: NPort can't buffer data to 10M in tcp client mode when port 
           			buffering is enabled.   */
#if 0		
        else
        {
            if(port_buffering || detail->count > 0)
                FD_SET(detail->fd_port, &rfds);
        }
#else
		if(port_buffering || (detail->count > 0 && !net_write_flag))
        	FD_SET(detail->fd_port, &rfds);
#endif

        if(select(maxfd + 1, &rfds, &wfds, 0, &tv) <= 0)
        {
            connect_retry_time = raw_tcp_reconnect(port, connect_retry_time);
            connect_check_time = raw_tcp_check_connect(port, connect_check_time);

            if((n = delimiter_poll(port)) > 0)
            {
                tv.tv_sec = 0;
                tv.tv_usec = 5 * 1000L;
            }
            else
            {
                tv.tv_sec = 1;
                tv.tv_usec = 0;
            }

            if((n == 0) && (idletime != 0xFFFFFFFFL))
            {
                /* idle time out */
                t = sys_clock_ms() - lasttime;
                if((port_write_flag == 0) && (net_write_flag == 0) && (t >= idletime))
                {
                    for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
                    {
                        if(detail->fd_net[i] > 0)
                            raw_tcp_close(port, i);
                    }

                    if(raw_tcp_check_count(port) == 0)
                        portd_terminate = 1;
                }

                t = idletime - t;

                if(tv.tv_sec == 1)
                    n = 1000;
                else
                    n = 5;

                if((int)t <= 0)
                {
                    tv.tv_sec = 0;
                    tv.tv_usec = 0;
                    continue;
                }

                if((int)t < n)
                {
                    tv.tv_sec = 0;
                    tv.tv_usec = t * 1000L;
                }
            }

            /* disconnect by */
            if((detail->startup_mode & DCF_CLI_MODE_OFF_MASK) != DCF_CLI_MODE_OFF_NONE)
            {
                if((detail->startup_mode & DCF_CLI_MODE_OFF_MASK) == DCF_CLI_MODE_OFF_DSROFF)        /* DSR OFF */
                {
                    if((sio_lstatus(port) & S_DSR) == 0)
                    {
                        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
                        {
                            if(detail->fd_net[i] > 0)
                                raw_tcp_close(port, i);
                        }

                        if(raw_tcp_check_count(port) == 0)
                            portd_terminate = 1;
                    }
                }
                else if((detail->startup_mode & DCF_CLI_MODE_OFF_MASK) == DCF_CLI_MODE_OFF_DCDOFF)        /* DCD OFF */
                {
                    if((sio_lstatus(port) & S_CD) == 0)
                    {
                        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
                        {
                            if(detail->fd_net[i] > 0)
                                raw_tcp_close(port, i);
                        }

                        if(raw_tcp_check_count(port) == 0)
                            portd_terminate = 1;
                    }
                }
            }
            continue;
        }

        connect_retry_time = raw_tcp_reconnect(port, connect_retry_time);
        connect_check_time = raw_tcp_check_connect(port, connect_check_time);
#ifdef SUPPORT_SERCMD
        Gscm_online = 1;
#endif

        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
        {
            if(detail->fd_net[i] > 0)
            {
                if(FD_ISSET(detail->fd_net[i], &rfds))
                {
                    n = delimiter_recv(port, detail->fd_net[i]);
                    lasttime = sys_clock_ms();

                    if(n < 0)
                    {
                        raw_tcp_close(port, i);
                        if(raw_tcp_check_count(port) == 0)
                        {
#if 0
                            if(!data_buffering)
                                portd_terminate = 1;
                            else if(delimiter_s2e_len(port) == 0)
                                portd_terminate = 1;
                            else
                                lasttime = sys_clock_ms();
#else
                            if(delimiter_s2e_len(port) == 0)
                                portd_terminate = 1;
                            else
                                lasttime = sys_clock_ms();
#endif
                        }
                    }
                    else
                    {
                        if(n > 0)
                            port_write_flag = 1;
                    }
                }

                if(FD_ISSET(detail->fd_net[i], &wfds))
                {
        			if (serial_data_buffered)
        			{
				    	delimiter_read(port, 1);
				    }
				    if (net_write_flag)
				    {
	                    // after connect fail, when connect success resend.
                    	if (delimiter_s2e_len(port) > 0)
                    	{
                        	if (delimiter_send(port, DK_BUFFER_SIZE_S2E, 0) == 0)
                        	{	/* Albert.20120102: in case sent_to_tcp_len > 0, set net_write_flag = 0 to have delimiter_poll() update s2e_len & sent_to_tcp_len. */
                        		net_write_flag = 0;
                        	}
                    	}
                    	else
                        	net_write_flag = 0;
	                    // end
                    }
                }
            }
        }

        if(FD_ISSET(detail->fd_port, &rfds))
        {
            lasttime = sys_clock_ms();
            if((n = delimiter_read(port, 0)) >= 0)     //not set delimiter
                net_write_flag = 1;
            else
            {
                //lasttime = sys_clock_ms();
                if(n == -1)     //set delimter
                {
                    tv.tv_sec = 0;
                    tv.tv_usec = 50 * 1000L;
                }
                else            //no data need to send
                {
                    tv.tv_sec = 0L;
                    tv.tv_usec = 100 * 1000L;
                }
            }
        }

        if(FD_ISSET(detail->fd_port, &wfds))
            port_write_flag = 0;
    } /* end of main loop */
    delimiter_stop(port);

    portd_wait_empty(port, detail->fd_port, 5000);  /* timeout: 5 seconds */
}

int raw_tcp_write(int port, int fd_net, char *buf, int len)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    int nbytes = 0, max_nbytes = 0;
    int i, send_it = 0;
    int minofree = DCF_SOCK_BUF;

    fd_net = fd_net;        /* avoid compiler warning message */
    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;

    if(!(detail->ctrlflag & CTRLFLAG_SKIPJAM))
    {
        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
        {
            if(detail->fd_net[i] > 0)
            {
                int ofree = tcp_ofree(detail->fd_net[i]);
                minofree = minofree < ofree ? minofree : ofree;
            }
        }
    }
    usleep(10);

    if(minofree < len)
        return 0;

    for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
    {
        if(detail->fd_net[i] <= 0 || (detail->fd_net_wait[i] <= 0))
            continue;

        if(detail->ctrlflag & CTRLFLAG_SKIPJAM) // Ignore jammed IP Enable
            send_it = (tcp_ofree(detail->fd_net[i]) > len) ? 1 : 0;
        else
            send_it = 1;

        if(send_it)
        {
            if((detail->fd_net[i] > 0) && (detail->fd_net_wait[i] > 0))
            {
//printf("%s(%d): %lu\r\n", __func__, __LINE__, sys_clock_ms());
                nbytes = send(detail->fd_net[i], buf, len, 0);
                if((nbytes < 0) && (errno == EAGAIN))
                {
                    /* retry to make burnin OK. */
                    nbytes = send(detail->fd_net[i], buf, len, 0);
                }
                detail->data_sent[i] = nbytes;	/* Albert.20120103: add */

                max_nbytes = MAX(nbytes, max_nbytes);
            }
        }
    }
    return max_nbytes;
}

int raw_tcp_read(int port, int fd_net, char *buf, int len)
{
    int nbytes;

    nbytes = recv(fd_net, buf, len, 0);

    if(nbytes == -1)
        nbytes = 0;

    if(nbytes == 0)
        return -1;
    return nbytes;
}

unsigned long raw_tcp_reconnect(int port, unsigned long retry_time)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    int i;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;

    if((sys_clock_ms() - retry_time) > RAW_TCP_RECONNECT_TIMEOUT)
    {
        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
        {
            if((detail->fd_net[i] < 0) && (detail->fd_net_wait[i] < 0) &&
                    (strlen(detail->dest_ip_dns[i]) > 7) && (detail->portno[i] > 0))
            {
                /* try to connect, but we must make sure that connecton status is available. */
                /* If it is not available, we return and try to connect until next raw_tcp_reconnect(). */

                if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_ANYCHAR)              /* try to connect on waiting Any Character */
                {
                    if(sio_iqueue(port) == 0)
                        return retry_time;
                }
                else if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_DSRON)         /* DSR ON */
                {
                    if(!(sio_lstatus(port) & S_DSR))
                        return retry_time;
                }
                else if((detail->startup_mode & DCF_CLI_MODE_ON_MASK) == DCF_CLI_MODE_ON_DCDON)         /* DCD ON */
                {
                    if(!(sio_lstatus(port) & S_CD))
                        return retry_time;
                }

                /* connecton status is available. */
                raw_tcp_connect(port, i);
            }
        }
        return sys_clock_ms();
    }
    else
        return retry_time;
}


void raw_tcp_connect(int port, int index)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    struct sockaddr_in sin;
    int i = 0;
    u_long inaddr;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    if((detail->fd_net[index] < 0) && (detail->fd_net_wait[index] < 0))
    {
        if((i = Ssys_getServerIp(detail->dest_ip_dns[index], &inaddr, 5000)) < 0)
            return;

        if((detail->fd_net_wait[index] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) > 0)
        {
            int on = 1;

            ioctl(detail->fd_net_wait[index], FIONBIO, &on);
            on = 1;
            setsockopt(detail->fd_net_wait[index], IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
            on = 1;
            setsockopt(detail->fd_net_wait[index], SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));

            memset(&sin, 0, sizeof(sin));
            sin.sin_family = AF_INET;               /* host byte order */
            sin.sin_port = htons(dport[port-1][index]);
            sin.sin_addr.s_addr = INADDR_ANY;
            while(bind(detail->fd_net_wait[index], (struct sockaddr *) &sin, sizeof(sin)) < 0)
            {
                perror("BIND:");
                printf("port = %d\r\n", sin.sin_port);
                sleep(1);
            }

            detail->peer[index].sin_family = AF_INET;
            detail->peer[index].sin_port = htons(detail->portno[index]);
            detail->peer[index].sin_addr.s_addr = inaddr;

            if(connect(detail->fd_net_wait[index], (struct sockaddr *) &detail->peer[index], sizeof(sin)) >= 0)
                raw_tcp_connect_ok(port, index);
            else
            {
                do
                {
                    if((errno != EINPROGRESS) && (errno != 0))
                    {
                        close(detail->fd_net_wait[index]);
                        detail->fd_net_wait[index] = -1;
                        usleep(50000);
                        break;
                    }
                    else
                    {
                        struct timeval tv;
                        fd_set rdevents, wrevents;
                        int rc;

                        FD_ZERO(&rdevents);
                        FD_ZERO(&wrevents);
                        FD_SET(detail->fd_net_wait[index], &rdevents);
                        wrevents = rdevents;
                        tv.tv_sec = 0;
                        tv.tv_usec = 5 * 1000L;
                        rc = select(detail->fd_net_wait[index] + 1, &rdevents, &wrevents, NULL, &tv);
                        if(rc < 0)
                        {
                            raw_tcp_connect_fail(port, index);
                            break;
                        }
                        else if(isconnected(detail->fd_net_wait[index], &rdevents, &wrevents, NULL))
                        {
                            /* connection success */
                            raw_tcp_connect_ok(port, index);
                            break;
                        }
                        else
                            usleep(50000);
                    }
                    i++;
                }
                while(i < 3);
            }
        }
    }
}

static int isconnected(int sockfd, fd_set *rd, fd_set *wr, fd_set *ex)
{
    if(tcp_state(sockfd) ==  TCP_ESTABLISHED)  // ESTABLISHED
        return 1;

    return 0;
}

/*
 * State
 *      Close       => fd_net < 0, fd_net_wait < 0
 *      Connecting  => fd_net < 0, fd_net_wait > 0
 *      Connected   => fd_net > 0, fd_net_wait > 0
 */

unsigned long raw_tcp_check_connect(int port, unsigned long check_time)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    int i;
    struct timeval tv;
    fd_set rdevents, wrevents, exevents;
    int rc;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;

    for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
    {
        if((detail->fd_net[i] < 0) && (detail->fd_net_wait[i] > 0))
        {
            FD_ZERO(&rdevents);
            FD_SET(detail->fd_net_wait[i], &rdevents);
            wrevents = exevents = rdevents;
            tv.tv_sec = 0;
            tv.tv_usec = 5 * 1000L;
            rc = select(detail->fd_net_wait[i] + 1, &rdevents, &wrevents, &exevents, &tv);
            if(rc < 0)
            {
                raw_tcp_connect_fail(port, i);
            }
            else if(isconnected(detail->fd_net_wait[i], &rdevents, &wrevents, &exevents))
            {
                /* connection success */
                raw_tcp_connect_ok(port, i);
                continue;
            }
            else
            {
                if((sys_clock_ms() - check_time) < RAW_TCP_RECONNECT_TIMEOUT)
                    return check_time;
                else
                    raw_tcp_connect_fail(port, i);
            }
        }
    }

    return sys_clock_ms();
}

void raw_tcp_connect_ok(int port, int index)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    /* serial tos */
#ifdef SUPPORT_SERIALTOS
    int enable;
#endif // SUPPORT_SERIALTOS
    int value;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;

    if((detail->fd_net[index] < 0) && (detail->fd_net_wait[index] > 0))   /* Connecting state */
    {
        detail->fd_net[index] = detail->fd_net_wait[index];
        if(detail->startup_mode)
        {
            /* TCP connect on Startup */
            int on = 1;

#ifdef SUPPORT_LOG_CONNECT
            log_opmode_connection   info;  /* event log */
#endif // SUPPORT_LOG_CONNECT

            setsockopt(detail->fd_net[index], IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

            /* set socket receiver and send buffer size */
            on = DCF_SOCK_BUF;
            setsockopt(detail->fd_net[index], SOL_SOCKET, SO_RCVBUF, (char *)&on, sizeof(on));
            on = DCF_SOCK_BUF;
            setsockopt(detail->fd_net[index], SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof(on));

#ifdef SUPPORT_TCP_KEEPALIVE
            tcp_setAliveTime(port, detail->fd_net[index]);
#endif // SUPPORT_TCP_KEEPALIVE

#ifdef SUPPORT_SERIALTOS
            /* serial tos */
            Scf_getSerialPortTos(port, &enable, &value);
            if(enable)
            {
                setsockopt(detail->fd_net[index], IPPROTO_IP, IP_TOS, &value, sizeof(value));
            }
#endif // SUPPORT_SERIALTOS

            /* set socket low-water to 2 */
            value = 2;
            setsockopt(detail->fd_net[index], SOL_SOCKET, SO_SNDLOWAT, &value, sizeof(value));


            /* event log */
#ifdef SUPPORT_LOG_CONNECT
            info.port_id = port + 1;
            info.flag = 0;
            *(in_addr_t*)info.ip = detail->peer[index].sin_addr.s_addr;
            Slog_put(DLOG_OPMODE_CONNECT, &info, sizeof(info));
#endif // SUPPORT_LOG_CONNECT
        }
        /*  move to tcp_main before - Sean 2008.09.18
                if( detail->count == 0 )
                    delimiter_start(port, detail->fd_port, detail->fd_net[index], raw_tcp_write, raw_tcp_read, 1);
        */
        detail->data_sent[index] = 0;	/* Albert.20120103: add */
        detail->count++;
	sys_send_events_ex(EVENT_ID_OPMODE_CONNECT, port << 4, 
		(ptr->application | ptr->opmode),
		detail->peer[index].sin_addr.s_addr, detail->portno[index]);
		//inet_ntoa(*(struct in_addr*)&bcast)
    }
}

void raw_tcp_connect_fail(int port, int index)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    if((detail->fd_net[index] < 0) && (detail->fd_net_wait[index] > 0))   /* Connecting state */
    {
        close(detail->fd_net_wait[index]);
        detail->fd_net[index] = -1;
        detail->fd_net_wait[index] = -1;
    }
}


void raw_tcp_close(int port, int index)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;
    struct sockaddr_in sock;

    memset(&sock, 0, sizeof(sock));
    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    if(detail->fd_net[index] > 0)
    {
#ifdef SUPPORT_LOG_CONNECT
        log_opmode_connection   info;  /* event log */
#endif // SUPPORT_LOG_CONNECT
#if 1
	{
	struct linger linger;
	linger.l_onoff = 1;
	linger.l_linger = 0;

	setsockopt(detail->fd_net[index], SOL_SOCKET, SO_LINGER,
	           (char *) &linger, sizeof(linger));
	}
#endif
        close(detail->fd_net[index]);
        detail->fd_net[index] = -1;
        detail->fd_net_wait[index] = -1;
        detail->data_sent[index] = 0;	/* Albert.20120103: add */

#ifdef SUPPORT_LOG_CONNECT
        info.port_id = port + 1; // event log
        info.flag = 0;
        *(in_addr_t*)info.ip = detail->peer[index].sin_addr.s_addr;
        Slog_put(DLOG_OPMODE_DISCONNECT, &info, sizeof(info));
#endif // SUPPORT_LOG_CONNECT
	sys_send_events_ex(EVENT_ID_OPMODE_DISCONNECT, port << 4, 
		(ptr->application | ptr->opmode),
		detail->peer[index].sin_addr.s_addr, detail->portno[index]);
//		sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);

        detail->count--;
    }
}

int raw_tcp_check_count(int port)
{
    struct port_data *ptr;
    struct raw_tcp_serial *detail;

    ptr = (struct port_data *) &Gport;
    detail = (struct raw_tcp_serial *) ptr->detail;
    if(detail->count == 0)
    {
        int i, j = 0;
        for(i = 0; i < TCP_CLIENT_MAX_CONNECT; i++)
            if(detail->fd_net_wait[i] > 0)
                j++;
        return j;
    }
    else
        return detail->count;
}
