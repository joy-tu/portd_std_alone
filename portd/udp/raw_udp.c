/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : raw_udp.c                                                  */
/* Description  :                                                            */
/* Product      : Secured Serial Device Server                               */
/* Programmer   : Shinhsy Shaw                                               */
/* Date         : 2003-07-22                                                 */
/*****************************************************************************/

#include    <stdio.h>
#include    <stdlib.h>
#include    <unistd.h>  // sleep()
#include    <config.h>
#include    <portd.h>
#include    <sio.h>
#include    "raw_udp.h"
#include    <support.h>


#define SUPPORT_MULTICAST
#define ISMULTICASE(ipaddr)  ((ipaddr & 0xF0000000) == 0xE0000000 ? 1 : 0)
#ifdef SUPPORT_SERCMD
extern int Gscm_active;
extern int Gscm_online;
#endif
extern PORT_DATA Gport;
extern int portd_terminate;

void udp_main(int port, int fd_port, int fd_net);
int udp_write(int port, int fd_net, char* buf, int len);
int udp_read(int port, int fd_net, char* buf, int len);

static unsigned long bcastip;   /* broadcast ip */
static unsigned long lip;   /* local ip */
static int lport;           /* local port */

void *raw_udp_start(void* portno)
{
    PPORT_DATA ptr;
    PRAW_UDP_SERIAL detail;
    int fd_net = -1, fd_port = -1;
    struct sockaddr_in  sin;
    //struct ip_mreq mreq;
    int i;
    int on, yes;
    int bindfa = 0;
    int port;
#ifdef SUPPORT_SERIALTOS
    int enable, value;
#endif // SUPPORT_SERIALTOS
#ifdef SUPPORT_MULTICAST
    struct ip_mreq mreq;
#endif // SUPPORT_MULTICAST

    port = (int)portno;

    ptr = &Gport;
    detail = (PRAW_UDP_SERIAL) ptr->detail;
    portd_terminate = 0;
#ifdef SUPPORT_SERCMD
    Gscm_online = 1;
#endif
    while(1)
    {
        for(i = 0; i < UDP_MAX_CONNECT; i++)
        {
            unsigned long bip, eip;
            int pno;

            Scf_getUdpS2E(port, i+1, (INT32U *)&bip, (INT32U *)&eip, &pno);
            detail->dip_begin[i] = ntohl(bip);
            detail->dip_end[i]   = ntohl(eip);
            detail->dip_port[i]  = pno;

            if((detail->dip_begin[i] > detail->dip_end[i])   // if end address < begin address, and end != 0, exchange
                && (detail->dip_end[i] != 0))
            {
                u_long tmp;
                tmp = detail->dip_begin[i];
                detail->dip_begin[i] = detail->dip_end[i];
                detail->dip_end[i] = tmp;
            }

            // if begin IP is 0x0L, and end ip is not 0x0L, set end ip to begin ip.
            if((detail->dip_begin[i] == 0) && (detail->dip_end[i] != 0))
            {
                detail->dip_begin[i] = detail->dip_end[i];
                detail->dip_end[i] = 0;
            }
            if(detail->dip_begin[i] == 0xFFFFFFFF)
                detail->dip_begin[i] = 0;
            if(detail->dip_end[i] == 0xFFFFFFFF)
                detail->dip_end[i] = 0;
        }

        bcastip = portd_getbcast_ip();

        while((fd_port = sio_open(port)) < 0)
            usleep(1000);

        sio_fifo(port, Scf_getAsyncFifo(port));     /* FIFO */
        sio_DTR(port, 0);                           /* DTR off at init state */
        sio_RTS(port, 0);                           /* RTS off at init state */
        sio_flush(port, FLUSH_ALL);

        while((fd_net = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
            usleep(1000);

        // enable the ability of the process to send broadcast messages.
        on = 1;
        setsockopt(fd_net, SOL_SOCKET, SO_BROADCAST, &on, sizeof(int));
        yes = 1;
        setsockopt(fd_net, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

#ifdef SUPPORT_SERIALTOS
        /* serial tos */
        Scf_getSerialPortTos(port, &enable, &value);
        if(enable)
        {
            setsockopt(fd_net, IPPROTO_IP, IP_TOS, (int *)&value, sizeof(value));
        }
#endif // SUPPORT_SERIALTOS

        detail->local_port = Scf_getUdpPort(port);


        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                       /* host byte order                  */
        sin.sin_port = htons(detail->local_port);           /* short, network byte order           */
        sin.sin_addr.s_addr = htonl(INADDR_ANY);

        lip = portd_getlocal_ip();
        lport = detail->local_port;

        while(bind(fd_net, (struct sockaddr *) &sin, sizeof(sin)) == -1)
        {
            usleep(1000);
            if(portd_terminate)
            {
                bindfa = 1;
                break;
            }
        }

#ifdef SUPPORT_MULTICAST
        // use setsockopt() to request that the kernel join a multicast group
        for(i = 0; i < UDP_MAX_CONNECT; i++)
        {
            /*
             * Only join begin ip to multicast group.
             */
            if(ISMULTICASE(detail->dip_begin[i]))   // multicast address
            {
                mreq.imr_multiaddr.s_addr = htonl(detail->dip_begin[i]);
                mreq.imr_interface.s_addr = INADDR_ANY;
                if(setsockopt(fd_net, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                    printf("join group 0x%lx fail, errno=%d\r\n", detail->dip_begin[i], errno);
            }
            /*
             * TODO:
             * neet to setting multicast TTL?
             * Default multicast TTL is 1.
             */
        }
#endif // SUPPORT_MULTICAST

        if(!bindfa)
        {
            //            sio_DTR(port, 1);                 /* DTR on */  move to after set flow ctrl
            //            sio_RTS(port, 1);                 /* RTS on */
            udp_main(port, fd_port, fd_net);
        }

        if(fd_net >= 0)
        {
            close(fd_net);
            fd_net = -1;
        }

        sio_DTR(port, 0);                   /* DTR off  */
        sio_RTS(port, 0);                   /* RTS off  */
        sio_flush(port, FLUSH_ALL);
        sio_close(port);
        if(portd_terminate)
            break;
    }
    return NULL;
}

void udp_main(int port, int fd_port, int fd_net)
{
    int maxfd, n;
    int baud, mode, flowctrl;
    int net_write_flag = 0, port_write_flag = 0;
    struct timeval tv;
    fd_set rfds, wfds;
    PPORT_DATA ptr;
    PRAW_UDP_SERIAL detail;

    //    portd_terminate = 0;  move to raw_udp_start()
    ptr = &Gport;
    detail = (PRAW_UDP_SERIAL) ptr->detail;
    detail->finish = 0;

    Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    sio_ioctl(port, baud, mode);        /* Set baud rate, data bits, stop bits and parity */

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
        //  case 3:   /* DTR/DSR */
        //      flowctrl = 0x60;
        //      break;
    default:
        flowctrl = F_NONE;
        break;
    }
	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
    sio_flowctrl(port, flowctrl);           /* Set flow control */

    sio_DTR(port, 1);                   /* DTR on */
    sio_RTS(port, 1);                   /* RTS on */

    if(fd_net > fd_port)
        maxfd = fd_net;
    else
        maxfd = fd_port;

    delimiter_start(port, fd_port, 1, &fd_net, NULL, udp_write, udp_read, 0);
    
    tv.tv_sec = 0L;
    tv.tv_usec = 100 * 1000L;
    /******** MAIN LOOP ********/
    while(!portd_terminate && !detail->finish)
    {
#ifdef SUPPORT_SERCMD
        if (Gscm_active) {
            usleep(100000);
            continue;
        }
#endif
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);
        if(port_write_flag)
        {
            FD_SET(fd_port, &wfds);
        }
        else
        {
            FD_SET(fd_net, &rfds);
        }

        if(net_write_flag)
        {
            FD_SET(fd_net, &wfds);
        }
        else
        {
            FD_SET(fd_port, &rfds);
        }

        if(select(maxfd + 1, &rfds, &wfds, 0, &tv) <= 0)
        {
            //          usleep(1);
            if(delimiter_poll(port) > 0)
            {
                tv.tv_sec = 0;
                tv.tv_usec = 20 * 1000L;
            }
            else
            {
                tv.tv_sec = 0L;
                tv.tv_usec = 100 * 1000L;
            }

            continue;
        }

        if(FD_ISSET(fd_net, &rfds))
        {
            n = delimiter_recv(port, fd_net);
            if(n < 0)
            {
                detail->finish = 1;
            }
            else
                port_write_flag = 1;
        }

        if(FD_ISSET(fd_net, &wfds))
        {
            net_write_flag = 0;
        }

        if(FD_ISSET(fd_port, &rfds))
        {
            if((n = delimiter_read(port, 0)) >= 0)
            {
                net_write_flag = 1;
            }
            else
            {
                tv.tv_sec = 0;
                tv.tv_usec = 50 * 1000L;
            }
        }

        if(FD_ISSET(fd_port, &wfds))
        {
            port_write_flag = 0;
        }

    } /* end of main loop */

#ifdef SUPPORT_MULTICAST
    {
        // leave multicast group
        struct ip_mreq mreq;
        int i;

        for(i = 0; i < UDP_MAX_CONNECT; i++)
        {
            if(ISMULTICASE(detail->dip_begin[i]))   // multicast address
            {
                mreq.imr_multiaddr.s_addr = htonl(detail->dip_begin[i]);
                mreq.imr_interface.s_addr = INADDR_ANY;
                if(setsockopt(fd_net, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                    printf("leave group 0x%lx fail, erron=%d\r\n", detail->dip_begin[i], errno);
            }
        }
    }
#endif

    delimiter_stop(port);

    portd_wait_empty(port, fd_port, 5000);  /* timeout: 5 seconds */
}

int udp_write(int port, int fd_net, char* buf, int len)
{
    PPORT_DATA ptr;
    PRAW_UDP_SERIAL detail;
    int i;
    unsigned long ipaddr, addr;
    struct sockaddr_in sin;

    ptr = &Gport;
    detail = (struct raw_udp_serial *) ptr->detail;

    for(i = 0; i < UDP_MAX_CONNECT; i++)
    {
        if((detail->dip_begin[i] == 0) && (detail->dip_end[i] == 0)) // both empty
            continue;

        ipaddr = detail->dip_begin[i];
        do
        {
            usleep(1);     // Let executive can give other task, to avoid lock.
            addr = ipaddr & 0x000000FFL;
            if((ipaddr == 0xffffffff) ||
                (ipaddr == bcastip) ||
                (ipaddr && ((ipaddr & 0xC0000000L) != 0xC0000000L)) ||
                (addr && (addr != 0x000000FFL)))
            {
                sin.sin_family = AF_INET;
                sin.sin_addr.s_addr = ntohl(ipaddr);
                sin.sin_port = htons(detail->dip_port[i]);
                sendto(fd_net, buf, len, 0, (struct sockaddr *) &sin, sizeof(sin));
            }

            ipaddr++;
        }
        while(ipaddr   <= detail->dip_end[i]);
    }

    return(len);
}

int udp_read(int port, int fd_net, char* buf, int len)
{
    int n;
    int numbytes;
    struct sockaddr_in sin;

    n = sizeof(sin);
    if((numbytes = recvfrom(fd_net, buf, len, 0, (struct sockaddr *) &sin, (socklen_t *)&n)) == -1)
        return 0;

    if((lip == sin.sin_addr.s_addr) && (lport ==  htons(sin.sin_port)))
        return 0;

    return numbytes;
}

