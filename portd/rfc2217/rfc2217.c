/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    rfc2217.c

    Routines to support RFC2217 protocol

    2011-01-05  James Wang
        Porting RFC2217 from NPort 6000.
*/


#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <header.h>
#include <debug.h>
#include <sio.h>
#include <telnet.h>
#include <config.h>
#include <systime.h>
#include <rfc2217.h>
#include <support.h>
#include <portd.h>
#include <sysmaps.h>
#include <datalog.h>
#include <eventd.h>
#include <sysapi.h>

#define D_SERVER_SEND       100
#define D_TELNET_NEWENVOPT  39  /* New Environment Option */

#define LS_DTR  0x01
#define LS_RTS  0x02
#define LS_CTS  0x10
#define LS_DSR  0x20
#define LS_RI   0x40
#define LS_DCD  0x80

#define     TEMP_LEN            1460
#define     FLOW_RESUME_SENT    0
#define     FLOW_SUSPEND_SEND   1
#define     FLOW_RESUME_SEND    2
#ifdef SUPPORT_SERCMD
extern int Gscm_active;
extern int Gscm_online;
#endif
extern int portd_terminate;

static int    net_flowctrl, serial_bufsize;
static char   buffer[TEMP_LEN];
static INT8U out[TEMP_LEN];
static INT8U tx_disable;

static int  SioBreakState;

static void serial_init(int port);
static void rfc2217_main(int port, int fd_port, int fd_net);
static int  rfc2217_write(int port, int fd_net, char *buf, int len);
static int  rfc2217_open_serial(int port);
static void rfc2217wait_empty(int port, unsigned long timeout);

static void rfc2217_telnet_setting(int fd_net, long *telflag, int port);
static void rfc2217_notify(int port, int fd_net);
static long rfc2217_decode(int fd_net, int port, long telflag, INT8U *buf, int len);
static int  rfc2217_option(int fd_net, int port, INT8U *buf, int len, long telflag, INT8U *outp, int *outlen);
static int  rfc2217_command(int fd_net, int port, INT8U *buf, int len, INT8U *outp, int *outlen);
static char rfc2217_com_control(int port, INT8U value);

/*****************************************************************************/

static INT8U telnet_line_mask;
static INT8U telnet_modem_stat;
static INT8U telnet_modem_mask;

static void serial_init(int port)
{
    int ret;
    int baud, mode, flowctrl;

    ret = Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    ret = sio_open(port);
    ret = sio_ioctl(port, baud, mode);    /* Set baud rate, data bits, stop bits and parity */
	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
    ret = sio_flowctrl(port, _sio_mapFlowCtrl(flowctrl));    /* Set flow control */
    sio_close(port);
}

static int rfc2217_open_listener(unsigned short data_port_no)
{
    int fd_listen, yes;
    struct sockaddr_in  sin;

    while ((fd_listen=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0)
        usleep(1000*1000);

    yes = 1;
    while (setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        usleep(1000*1000);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(data_port_no);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    //sys_setTCPAllowFunc(fd_listen, newwork_allow_check);

    while (bind(fd_listen, (struct sockaddr*) &sin, sizeof(sin)) < 0)
        usleep(1000*1000);

    listen(fd_listen, 1);
    return fd_listen;
}

static void rfc2217_close_listener(int *fd)
{
    if (*fd >= 0)
    {
        close(*fd);
        shutdown(*fd, SHUT_RDWR);
        *fd = -1;
    }
}

void *rfc2217_start(void *arg)
{
    int         fd_port, fd_net=0, fd_listen=-1;
    int         port, n, flags;
    //unsigned long   t;
    struct sockaddr_in  sin;
    INT16U      data_port_no, cmd_port_no;
    struct      timeval tv;
    fd_set      rfds;

    port = (int) arg;
    SioBreakState = 0;
    serial_init(port);  /* Set baud rate, data bits, stop bits, parity and flow control */

    Scf_getTcpServer(port, (INT16U *) &data_port_no, (INT16U *) &cmd_port_no);
    net_flowctrl = FLOW_RESUME_SENT;

    while (1)
    {
        if (fd_listen < 0)
            fd_listen = rfc2217_open_listener(data_port_no);

        FD_ZERO(&rfds);
        FD_SET(fd_listen, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 1000*1000;

        if (select(fd_listen+1, &rfds, NULL, NULL, &tv) == 0)
        {
            if (portd_terminate)
            {
                rfc2217_close_listener(&fd_listen);
                break;
            }
            continue;
        }

        if (FD_ISSET(fd_listen, &rfds))
        {
            n = sizeof(struct sockaddr_in);

            fd_net = accept(fd_listen, (struct sockaddr*) &sin, (socklen_t *)&n);
        }
#ifdef SUPPORT_SERCMD
        Gscm_online = 1;
#endif

        // system will do the TCP alive check setting.
#ifdef SUPPORT_TCP_KEEPALIVE
        tcp_setAliveTime(port, fd_net);
#endif // SUPPORT_TCP_KEEPALIVE

        flags = fcntl(fd_net, F_GETFL, 0);
        fcntl(fd_net, F_SETFL, flags | O_NONBLOCK);

        fd_port = rfc2217_open_serial(port);

        delimiter_start(port, fd_port, 1, &fd_net, NULL, rfc2217_write, NULL, 1);

        rfc2217_main(port, fd_port, fd_net);

        rfc2217_close_listener(&fd_listen);

        //======end======
        if (portd_terminate)
            break;
    }
    return NULL;
}

static void rfc2217_main(int port, int fd_port, int fd_net)
{
    int     maxfd, n, fblen, finish;
    int     net_write_flag = 0, port_write_flag = 0;
    struct timeval time;
    fd_set  rfds, wfds;
    long    telflag=0;
    char    flowbuf[10];

	sys_send_events(EVENT_ID_OPMODE_CONNECT, port << 4);
    //DIO_ControlSingleIO(port, 1); /* InUse LED on */

    sio_DTR(port, 1);
    sio_RTS(port, 1);
    maxfd = MAX(fd_net, fd_port);

    time.tv_sec = 0;
    time.tv_usec = (50 * 1000L);
    rfc2217_telnet_setting(fd_net, &telflag, port);
    tx_disable = 0;

    finish = 0;
    /******** MAIN LOOP ********/
    while (!finish)
    {
#ifdef SUPPORT_SERCMD
        if (Gscm_active) {
            usleep(100000);
            continue;
        }
#endif
        if (portd_terminate)
        {
            finish = 1;
            continue;
        }
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        if ( port_write_flag )
            FD_SET(fd_port, &wfds);
        else
            FD_SET(fd_net, &rfds);

        if ( net_write_flag )
            FD_SET(fd_net, &wfds);
        else
        {
            if ( tx_disable == 0 )
                FD_SET(fd_port, &rfds);
        }

        if (select(maxfd+1, &rfds, &wfds, 0, &time) <= 0)
        {
            if ( (telflag & TFLAG_DO_RFC2217) )
            {
                rfc2217_notify(port, fd_net);
            }

            if( (n = delimiter_poll(port)) > 0 )
            {
                time.tv_sec = 0;
                time.tv_usec = 5*1000L;
            }
            else
            {
                time.tv_sec = 0;
                time.tv_usec = 50*1000L;
            }
            continue;
        }

        if ( (telflag & TFLAG_DO_RFC2217) )
            rfc2217_notify(port, fd_net);

        if ( FD_ISSET(fd_port, &wfds) )
        {
            if ((sio_ofree(port) >= (serial_bufsize*3)/4) && net_flowctrl == FLOW_RESUME_SEND)
            {
                net_flowctrl = FLOW_RESUME_SENT;
                memset(flowbuf, 0, 10);
                fblen = 0;
                flowbuf[fblen++] = D_TELNET_IAC;
                flowbuf[fblen++] = D_TELNET_SB;
                flowbuf[fblen++] = D_TELNET_COMPORT;
                flowbuf[fblen++] = D_FLOWCTRL_RESUME;
                flowbuf[fblen++] = D_TELNET_IAC;
                flowbuf[fblen++] = D_TELNET_SE;
                if (send(fd_net, flowbuf, fblen, 0) > 0)
                {
                    /* printf("resume sent\r\n"); */
                }
                port_write_flag = 0;
            }
        }
        if ( FD_ISSET(fd_net, &rfds) )
        {
            if ( sio_ofree(port) < TEMP_LEN )
            {
                // To do: tcp_state is not supported. It must be re-written.
                if ( tcp_state(fd_net) == TCP_CLOSE_WAIT )
                {
                    //rfc2217wait_empty(port, 5000L);
                    break;
                }
                net_flowctrl = FLOW_SUSPEND_SEND;
                net_write_flag = 1;
                port_write_flag = 1;
            }
            else
            {
                if ( (n = recv(fd_net, buffer, TEMP_LEN, 0)) > 0 )
                {
                    telflag = rfc2217_decode(fd_net, port, telflag, (INT8U *)buffer, n);
                    if ( telflag & TFLAG_SENDBRK )
                    {
                        telflag &= ~TFLAG_SENDBRK;
                        sio_break(port, 1);
                        usleep(D_SENDBRK_TIME*1000);
                        sio_break(port, 0);
                    }
                }
                else if (n == 0)
                {
                        break;
                }
            }
        }
        if ( FD_ISSET(fd_net, &wfds) )
        {
            if (net_flowctrl == FLOW_SUSPEND_SEND)
            {
                net_flowctrl = FLOW_RESUME_SEND;
                memset(flowbuf, 0, 10);
                fblen = 0;
                flowbuf[fblen++] = D_TELNET_IAC;
                flowbuf[fblen++] = D_TELNET_SB;
                flowbuf[fblen++] = D_TELNET_COMPORT;
                flowbuf[fblen++] = D_FLOWCTRL_SUSPEND;
                flowbuf[fblen++] = D_TELNET_IAC;
                flowbuf[fblen++] = D_TELNET_SE;
                if (send(fd_net, flowbuf, fblen, 0) > 0)
                {
                }
            }
            net_write_flag = 0;
        }

        if ( FD_ISSET(fd_port, &rfds) )
        {
            if( (n = delimiter_read(port, 0)) < 0 )
            {
                net_write_flag = 1;
            }
            else
            {
                if( n > 0 )
                {
                      time.tv_sec = 0;
                      time.tv_usec = 5*1000L;
                }
                else
                {
                      time.tv_sec = 0;
                      time.tv_usec = 50*1000L;
                }
            }
        }
    } /* end of main loop */

	sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);
    close(fd_net);

    //DIO_ControlSingleIO(port, 0); /* InUse LED off */

    delimiter_stop(port);
    rfc2217wait_empty(port, 5000L);
    sio_DTR(port, 0);
    sio_RTS(port, 0);
    {
      int vv;
      vv = sio_close(port);
    }
    return;
}

static int rfc2217_write(int port, int fd_net, char *buf, int len)
{
    int i, outlen = 0;

    if (fd_net > 0)
    {
        for (i=0; i<len; i++)
        {
            if (buf[i] == 0xff)
            {
                out[outlen++] = 0xff;   /* add an escape char */
                out[outlen++] = buf[i];
            }
            else
            {
                out[outlen++] = buf[i];
            }
            if (outlen == TEMP_LEN || outlen == (TEMP_LEN-1))
            {
                send(fd_net, out, outlen, 0);
                outlen = 0;
            }
        }
        if (outlen)
        {
            send(fd_net, out, outlen, 0);
        }
    }
    return len;
}

static int rfc2217_open_serial(int port)
{
    int fd_port;

    while ( (fd_port = sio_open(port)) < 0 )
        usleep(1000*1000);
    /* don't serial_init(port), let the setting remains same value */
    sio_flush(port, 2);

    //event_sio_init(port);  /* setup DSR, DCD on/off event */

    serial_bufsize = (int)sio_ofree(port);
    return fd_port;
}

static void rfc2217wait_empty(int port, unsigned long timeout)
{
    int     n, i;
    unsigned long   t;
    if ( (n = (int)sio_oqueue(port)) <= 0 ) /* bugfix: < 0 means error port */
        return;

    t = sys_clock_ms();
    while ( 1 )
    {
        if ( (i = (int)sio_oqueue(port)) <= 0 ) /* bugfix: < 0 means error port */
            return;
        if ( n == i )
        {
            if ( (sys_clock_ms() - t) >= timeout )
            {
                sio_flush(port, 2);
                return;
            }
        }
        else
        {
            n = i;
            t = sys_clock_ms();
        }
        usleep(10*1000);
    }
}

static void rfc2217_telnet_setting(int fd_net, long *telflag, int port)
{
    *telflag |= TFLAG_DO_RFC2217;
    telnet_modem_stat = 0xF0;
    telnet_line_mask = 0;
    telnet_modem_mask = 0xFF;
}

static void rfc2217_notify(int port, int fd_net)
{
    INT8U  tmp[16];
    INT8U  ls, ols, c1, c2;
    int n;

    if ( (tcp_ofree(fd_net) < 16) || tx_disable )
        return;

    if ( (c1 = (telnet_line_mask & 0x1E)) != 0 )
    {
        tmp[4] = 0;

        if((n = sio_notify_error(port)) != 0)
        {
            if(n & 0x01)    // parity error
                tmp[4] |= 0x04;
            if(n & 0x02)    // frame error
                tmp[4] |= 0x08;
            if(n & 0x0C)    // overrun / overflow error
                tmp[4] |= 0x02;
            if(n & 0x10)    // break
                tmp[4] |= 0x10;
        }

        tmp[4] &= c1;
        if ( tmp[4] != 0 )
        {
            tmp[0] = D_TELNET_IAC;
            tmp[1] = D_TELNET_SB;
            tmp[2] = D_TELNET_COMPORT;
            tmp[3] = D_NOTIFY_LINESTATE + D_SERVER_SEND;
            tmp[5] = D_TELNET_IAC;
            tmp[6] = D_TELNET_SE;
            send(fd_net, (char *)tmp, 7, 0);
            //dbg_printf("send NOTIFY-LINESTATE= 0x%x\r\n", tmp[4]);
        }
    }
    if ( (c1 = (telnet_modem_mask & 0x0F)) != 0 )
    {
        ls = (INT8U)((sio_lstatus(port) & (S_CTS|S_DSR|S_CD)) << 4); /* we have no RI pin */
#if 0   /* avoid frequently sending notify caused by frequently flow signal change */
        n = sio_getflow(port);
        if ( (n & 1) )      /* if do CTS flow control, the CTS always reports HI */
            ls |= 0x10;
        if ( (n & 0x10) )   /* if do DSR flow control, the DSR always reports HI */
            ls |= 0x20;
#endif
        ols = telnet_modem_stat;
        c2 = ((ols ^ ls) >> 4) & c1;

        if ( (ls != ols) && (c2 != 0) )
        {
            telnet_modem_stat = ls;
            tmp[0] = D_TELNET_IAC;
            tmp[1] = D_TELNET_SB;
            tmp[2] = D_TELNET_COMPORT;
            tmp[3] = D_NOTIFY_MODEMSTATE + D_SERVER_SEND;
            tmp[4] = ls | c2;
            tmp[5] = D_TELNET_IAC;
            tmp[6] = D_TELNET_SE;
            send(fd_net, (char *)tmp, 7, 0);
            //dbg_printf("send NOTIFY-MODEMSTATE= 0x%x\r\n", tmp[4]);
        }
    }
}

static char rfc2217_com_control(int port, INT8U value)
{
    int n;
    int result=0;

    result = value;
    switch ( value )
    {
    case 13:/* Request COM Port Flow Control Setting (inbound) */
    case 0: /* Request COM Port Flow Control Setting (outbound/both) */
        n = sio_getflow(port);
        if (n & 0x60)
            result = 19;
        else if (n & 0x03)
            result = 3;
        else if (n & 0x0C)
            result = 2;
        else
            result = 1;
        break;
    case 14:/* Use No Flow Control (inbound) */
        break;
    case 1: /* Use No Flow Control (outbound/both) */
        sio_flowctrl(port, 0);
        break;
    case 15:/* Use XON/XOFF Flow Control (inbound) */
        break;
    case 2: /* Use XON/XOFF Flow Control (outbound/both) */
        sio_flowctrl(port, 0x0C);
        break;
    case 16:/* Use HARDWARE Flow Control (inbound) */
        break;
    case 3: /* Use HARDWARE Flow Control (outbound/both) */
        sio_flowctrl(port, 0x03);
        break;
    case 4: /* Request BREAK state */
        if (SioBreakState)
            result = 5;
        else
            result = 6;
        break;
    case 5: /* Set Break State ON */
        sio_break(port, 1);
        SioBreakState = 1;
        break;
    case 6: /* Set BREAK State OFF */
        sio_break(port, 0);
        SioBreakState = 0;
        break;
    case 7: /* Request DTR Signal State */
        if ( sio_getDTR(port))
            result = 8;
        else
            result = 9;

        break;
    case 8: /* Set DTR Signal State ON */
        sio_DTR(port, 1);
        break;
    case 9: /* Set DTR Signal State OFF */
        sio_DTR(port, 0);
        break;
    case 10:/* Request RTS Signal State */
        if ( sio_getRTS(port))
            result = 11;
        else
            result = 12;
        break;
    case 11:/* Set RTS Signal State ON */
        sio_RTS(port, 1);
        break;
    case 12:/* Set RTS Signal State OFF */
        sio_RTS(port, 0);
        break;
    case 19:/* Use DSR Flow Control (outbound/both) */
        sio_flowctrl(port, 0x60);
        break;
    default:
        break;
    }

    return(result);
}

static int rfc2217_command(int fd_net, int port, INT8U *buf, int len,
                            INT8U *outp, int *outlen)
{
    int n, m, mode;
    char    signature;
    long    baud;
    int     olen;

    signature = *buf++;
    if ( signature == D_SET_BAUDRATE )
        n = 4;
    else if ( signature == D_FLOWCTRL_SUSPEND ||
          signature == D_FLOWCTRL_RESUME )
        n = 0;
    else
        n = 1;
    if ( len < (n + 3) )
        return(-1);
    m = 0;

    olen = *outlen;
    outp[olen++] = D_TELNET_IAC;
    outp[olen++] = D_TELNET_SB;
    outp[olen++] = D_TELNET_COMPORT;

    switch ( signature )
    {
    case D_SIGNATURE:
         outp[olen++] = D_SIGNATURE;
         memcpy(outp + olen, "MOXA", 4);
         olen += 4;
         break;
    case D_SET_BAUDRATE:
        baud = buf[0] + (buf[1]*0x100L) + (buf[2]*0x10000L) + (buf[3]*0x1000000L);
        baud = ntohl(baud);
        if ( baud == 0 )
            baud = sio_getbaud(port);
        else
            sio_baud(port, baud);

        baud = htonl(baud);
        outp[olen++] = D_SET_BAUDRATE + D_SERVER_SEND;
        outp[olen++] = (INT8U) (baud & 0x000000FF);
        outp[olen++] = (INT8U) ((baud & 0x0000FF00) >> 8);
        outp[olen++] = (INT8U) ((baud & 0x00FF0000) >> 16);
        outp[olen++] = (INT8U) ((baud & 0xFF000000) >> 24);
        break;
    case D_SET_DATASIZE:
        mode = sio_getmode(port);
        if ( buf[0] == 0 )
        {
        }
        else if ( buf[0] >= 5 && buf[0] <= 8 )
        {
            mode &= 0xFC;
            mode |= (buf[0] - 5);
            sio_ioctl(port, -1, mode);
        }
        outp[olen++] = D_SET_DATASIZE + D_SERVER_SEND;
        outp[olen++] = (INT8U) ((mode & 3) + 5);
        break;
    case D_SET_PARITY:
        mode = sio_getmode(port);
        if ( buf[0] == 0 )
        {
        }
        else if ( buf[0] >= 1 && buf[0] <= 5 )
        {
            mode &= 0xC7;
            switch ( buf[0] )
            {
            case 2: mode |= 0x08; break;
            case 3: mode |= 0x18; break;
            case 4: mode |= 0x28; break;
            case 5: mode |= 0x38; break;
            }
            sio_ioctl(port, -1, mode);
        }

        outp[olen++] = D_SET_PARITY + D_SERVER_SEND;
        switch ( (mode & 0x38) )
        {
            case 0x00: outp[olen++] = 1; break; /* NONE */
            case 0x08: outp[olen++] = 2; break; /* ODD */
            case 0x18: outp[olen++] = 3; break; /* EVEN */
            case 0x28: outp[olen++] = 4; break; /* MARK */
            case 0x38: outp[olen++] = 5; break; /* SPACE */
            default:   outp[olen++] = 0; break;
        }
        break;
    case D_SET_STOPSIZE:
        mode = sio_getmode(port);
        if ( buf[0] == 0 )
        {
        }
        else if ( buf[0] >= 1 && buf[0] <= 3 )
        {
            mode &= 0xFB;
            if ( buf[0] != 1 )
                mode |= 0x04;
            sio_ioctl(port, -1, mode);
        }

        outp[olen++] = D_SET_STOPSIZE + D_SERVER_SEND;
        if ( (mode & 4) == 0 )
            outp[olen++] = 1;
        else if ( (mode & 3) == 0 )
            outp[olen++] = 3;
        else
            outp[olen++] = 2;
        break;
    case D_SET_CONTROL:
        outp[olen++] = D_SET_CONTROL + D_SERVER_SEND;
        outp[olen++] = rfc2217_com_control(port, buf[0]);
        break;
    case D_FLOWCTRL_SUSPEND:
        outp[olen++] = D_FLOWCTRL_SUSPEND + D_SERVER_SEND;
        tx_disable = 1;
        break;
    case D_FLOWCTRL_RESUME:
        outp[olen++] = D_FLOWCTRL_RESUME + D_SERVER_SEND;
        tx_disable = 0;
        break;
    case D_LINESTATE_MASK:
        outp[olen++] = D_LINESTATE_MASK + D_SERVER_SEND;
        if (buf[0] == 0xff)
            outp[olen++] = 0xff; /* add a escape char(0xff) if linestate-mask is 0xff */

        outp[olen++] = (INT8U) (buf[0]);
        telnet_line_mask = buf[0];
        //dbg_printf("SET-LINESTATE-MASK reg = 0x%x\r\n", buf[0]);
        break;
    case D_MODEMSTATE_MASK:
        outp[olen++] = D_MODEMSTATE_MASK + D_SERVER_SEND;
        if (buf[0] == 0xff)
            outp[olen++] = 0xff; /* add a escape char(0xff) if modemstate-mask is 0xff */

        outp[olen++] = (INT8U) (buf[0]);
        telnet_modem_mask = buf[0] & 0xBB;    /* 0xbb -> we don't support RI and TERI */
        //dbg_printf("telnet_modem_mask[%d] = %lx\r\n", port, telnet_modem_mask);
        break;
    case D_PURGE_DATA:

        if ( buf[0] >= 1 && buf[0] <= 3 )
        {
/*
            if (buf[0] == 1)
                printf("D_PURGE_DATA -> RX\r\n");
            if (buf[0] == 2)
                printf("D_PURGE_DATA -> TX\r\n");
            if (buf[0] == 3)
                printf("D_PURGE_DATA -> RX/TX\r\n");
*/
            outp[olen++] = D_PURGE_DATA + D_SERVER_SEND;
            outp[olen++] = (INT8U) (buf[0]);
            sio_flush(port, (int)(buf[0] - 1));
        }
        break;
    default:
        return(-2);
    }
    outp[olen++] = D_TELNET_IAC;
    outp[olen++] = D_TELNET_SE;
    *outlen = olen;

    return(n);
}

static int rfc2217_option(int fd_net, int port, INT8U *buf, int len,
                  long telflag, INT8U *outp, int *outlen)
{
    int i, n;

    n = len;
    for ( i=0; i<len; i++ )
    {
        if ( (buf[i] == D_TELNET_IAC) && (buf[i+1] == D_TELNET_SE) )
        {
            n = i + 2;
            if ( (buf[0] == D_TELNET_COMPORT) &&
                 (telflag & TFLAG_DO_RFC2217) )
            {
                rfc2217_command(fd_net, port, buf+1, n, outp, outlen);
            }
            break;
        }
    }
    return(n);
}

static long rfc2217_decode(int fd_net, int port, long telflag, INT8U *buf,
                   int len)
{
    int slen, n;
    INT8U *startp;
    char    tmp[4];
    INT8U  c;
    int outlen;
    int (*port_sio_write)(int port, char *buf, int len);

    port_sio_write = (Scf_getSerialDataLog(port)) ? log_sio_write : sio_write;

    startp = buf;
    slen = 0;

    outlen = 0;
    while ( len-- )
    {
        c = *buf++;
        if ( c != D_TELNET_IAC )
        {
            slen++;
            continue;
        }
        if ( len == 0 )
            break;
        len--;
        c = *buf++;
        tmp[1] = 0;
        if ( c == D_TELNET_IAC )
        {
            slen++;
        }
        else if ( c == D_TELNET_SB )
        {
            n = rfc2217_option(fd_net, port, buf, len, telflag, out, &outlen);
            len -= n;
            buf += n;
        }
        else if ( c == D_TELNET_BRK )
        {
            telflag |= TFLAG_SENDBRK;
        }
        else if ( c == D_TELNET_WILL )
        {
            len--;
            c = *buf++;
            if ( c == D_TELNET_BINARY )
            {
                telflag |= TFLAG_RXBIN;
                if ( (telflag & TFLAG_DOBIN) == 0 )
                {
                    out[outlen++] = D_TELNET_IAC;
                    out[outlen++] = D_TELNET_DO;
                    out[outlen++] = D_TELNET_BINARY;
                }
            }
            else if ( c == D_TELNET_ECHO )
            {
                if ( (telflag & TFLAG_DOECHO) == 0 )
                {
                    /* yes remote can do echo */
                    out[outlen++] = D_TELNET_IAC;
                    out[outlen++] = D_TELNET_DO;
                    out[outlen++] = D_TELNET_ECHO;
                }
            }
            else if ( c == D_TELNET_SGA )
            {
                out[outlen++] = D_TELNET_IAC;
                out[outlen++] = D_TELNET_DO;
                out[outlen++] = D_TELNET_SGA;
            }
            else if ( c == D_TELNET_COMPORT )
            {
                telflag |= TFLAG_DO_RFC2217;
                out[outlen++] = D_TELNET_IAC;
                out[outlen++] = D_TELNET_DO;
                out[outlen++] = D_TELNET_COMPORT;
            }
            else
            {
                out[outlen++] = D_TELNET_IAC;
                out[outlen++] = D_TELNET_DONT;
                out[outlen++] = c;
            }
        }
        else if ( c == D_TELNET_DO )
        {
            len--;
            c = *buf++;
            if ( c == D_TELNET_BINARY )
            {
                telflag |= TFLAG_TXBIN;
                if ( (telflag & TFLAG_WILLBIN) == 0 )
                {
                    out[outlen++] = D_TELNET_IAC;
                    out[outlen++] = D_TELNET_WILL;
                    out[outlen++] = D_TELNET_BINARY;
                }
            }
            else if ( c == D_TELNET_ECHO )
            {
                if ( (telflag & TFLAG_WILLECHO) == 0 )
                {
                    out[outlen++] = D_TELNET_IAC;
                    out[outlen++] = D_TELNET_WONT;
                    out[outlen++] = D_TELNET_ECHO;
                }
            }
            else if ( c == D_TELNET_COMPORT )
            {
                telflag |= TFLAG_DO_RFC2217;
                out[outlen++] = D_TELNET_IAC;
                out[outlen++] = D_TELNET_WILL;
                out[outlen++] = D_TELNET_COMPORT;
            }
            else
            {
                out[outlen++] = D_TELNET_IAC;
                out[outlen++] = D_TELNET_WONT;
                out[outlen++] = c;
            }
        }
        if ( slen )
        {
            port_sio_write(port, (char *)startp, slen);
            slen = 0;
        }
        startp = buf;
    }
    if ( slen )
        port_sio_write(port, (char *)startp, slen);
    if( outlen )
        send(fd_net, out, outlen, 0);
    return(telflag);
}

