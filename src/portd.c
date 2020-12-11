/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    portd.c
*/

#ifndef _PORTD_C
#define _PORTD_C
#endif

#include <stdio.h>
#include <platform.h>
#ifdef LINUX
#include <pthread.h>
#include <netinet/in.h>
#include <sio.h>
#elif defined(ZEPHYR)
#include <posix/pthread.h>
#include <posix/netinet/in.h>
#include <sio/mx_sio.h>
#include <net/mx_net.h>
#endif
#include <sysfakeconf.h>
#include <common.h>
#include <header.h>
#include <portd.h>
#include "aspp.h"

struct port_data Gport;

volatile int portd_terminate = 0;
static int portd_init(int port_idx);
static void portd(int port_idx);
static void portd_exit(int port_idx);
extern int delimiter_init(int port, int has_delimiter, int has_buffering);
extern void delimiter_exit(int port);
int load_runtime_conf(int port);
#ifdef LINUX
int main(int argc, char *argv[]) {
	portd_start();
	return 0;
}
#endif
int portd_start(void)
{
	int port_idx=1;

	load_runtime_conf(port_idx);
	if (port_idx <= 0) {
		printf("Invalid port specified!\n");
		return -1;
    	}

	while( 1 ) {
		if (!portd_init(port_idx)) {
			sleep(1);    /* sleep 1 second */
			continue;
		}
		portd_terminate = 0;
		portd(port_idx);

		portd_exit(port_idx);
	}

	return 0;
}

static int portd_init(int port_idx)
{
    struct port_data *ptr;
    int val;
    //int ret;

    ptr = &Gport;
    ptr->port_idx = port_idx;

    val = (int) Scf_getOpMode(ptr->port_idx);

    ptr->opmode = val & 0xff00;
    ptr->application = val & 0xff;

    ptr->detail = NULL;

    switch (ptr->opmode)
    {
        case CFG_APPLICATION_DEVICE_CONTROL:
        {
            if (ptr->application == CFG_OPMODE_REALCOM) // RealCOM
            {
		  delimiter_init(port_idx, 1, 1);
                ptr->detail = (struct aspp_serial*) malloc(sizeof(struct aspp_serial));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(struct aspp_serial));
            }

            break;
        }
        case CFG_APPLICATION_SOCKET:
        {
            if (ptr->application == CFG_OPMODE_TCPSERVER) // TCP Server
            {
                delimiter_init(port_idx, 1, 1);
                ptr->detail = (struct aspp_serial*) malloc(sizeof(struct aspp_serial));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(struct aspp_serial));
            }
            break;
        }			
        default:
            return 0;
            break;
    }

    return 1;
}
#ifdef ZEPHYR
#define PORTD_STACK_SIZE 4096 * 10 
K_THREAD_STACK_DEFINE(portd_stack, PORTD_STACK_SIZE);
#endif
void *aspp_start_tcpecho(void *arg);

static void portd(int port_idx)
{
    struct port_data *ptr;
#ifdef ZEPHYR
    pthread_attr_t portd_attr;
#endif
    ptr = &Gport;

    ptr->thread_id = (pthread_t)NULL;

    switch (ptr->opmode)
    {
        case CFG_APPLICATION_DEVICE_CONTROL:
        {
            if (ptr->application == CFG_OPMODE_REALCOM) // RealCOM
            {
#ifdef LINUX            
                printf("Start port %d as RealCOM mode\n", port_idx);
		  pthread_create(&ptr->thread_id, NULL, &aspp_start, (void *)port_idx);
#elif defined(ZEPHYR)
		 (void)pthread_attr_init(&portd_attr);
		 (void)pthread_attr_setstack(&portd_attr, &portd_stack,
				    PORTD_STACK_SIZE);
		  pthread_create(&ptr->thread_id, &portd_attr, &aspp_start, (void *)port_idx);
#endif		  
            }
            break;
        }
        case CFG_APPLICATION_SOCKET:
        {
            if (ptr->application == CFG_OPMODE_TCPSERVER) // TCP Server
            {
                printf("Start port %d as TCP server mode\n", port_idx);
                pthread_create(&ptr->thread_id, NULL, &aspp_start, (void *)(port_idx|0x8000));
            }
            break;
        }
        default:
            return;
            break;
    }

    while (1)
    {
        sleep(1);                    /* always sleep */
    }
}


static void portd_exit(int port_idx)
{
    struct port_data *ptr;

    ptr = &Gport;
	
    if (ptr->detail) {
        free(ptr->detail);
        ptr->detail = NULL;
    }

    delimiter_exit(port_idx);
}

#define TCP_INFO_MODE_GET_STATE     0
#define TCP_INFO_MODE_GET_OQUEUE    1
#ifdef LINUX
static int __tcp_info(int fd, int mode);
int tcp_oqueue(int fd)
{
    return __tcp_info(fd, TCP_INFO_MODE_GET_OQUEUE);
}
int tcp_ofree(int fd)
{
#if 1
    int txq;
    txq = __tcp_info(fd, TCP_INFO_MODE_GET_OQUEUE);
    return DCF_SOCK_BUF - txq;
#else
    return 4096;
#endif
}
/**
 * \brief
 * \param fd
 * \retval -1 Open proc file fail.
 * \retval -2 fd < 0.
 */
int tcp_state(int fd)
{
    int retry = 3;
    int ret = 0;

    while(retry)
    {
        if((ret = __tcp_info(fd, TCP_INFO_MODE_GET_STATE)) != 0)
        {
            break;
        }
        
        usleep(1000); /* Connection missing, wait 1ms  */
        retry --;
    }
    
    return ret;
}

static int __tcp_info(int fd, int mode)
{
    FILE *fp;
    char name[64];
    char buf[1024];
    int count, line;
    int ino;

    if(fd < 0)
    {
        return -2;
    }

    sprintf(name, "/proc/%d/fd/%d", getpid(), fd);

    count = readlink(name, buf, sizeof(buf));
    buf[count] = 0;

    sscanf(buf, "socket:[%d]", &ino);

    fp = fopen("/proc/net/tcp", "r");
    if(fp == NULL)
        return -1;

    line = 0;
    while(fgets(buf, sizeof(buf), fp))
    {
        //int d;
        //char local_addr[64], rem_addr[64];
        //int local_port, rem_port;
        int state;
        unsigned long rxq, txq;
        //int timer_run;
        //unsigned long  time_len, retr;
        //int uid;
        //int timeout;
        unsigned long inode;
        //char more[512];
        int num;

        if(line)
        {
            //num = sscanf(buf,
            //    "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
            //    &d, local_addr, &local_port, rem_addr, &rem_port, &state,
            //    &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);

            num = sscanf(buf,
                "%*d: %*64[0-9A-Fa-f]:%*X "
                "%*64[0-9A-Fa-f]:%*X %X "
                "%lX:%lX %*X:%*X "
                "%*X %*d %*d %ld\n",
                &state, &txq, &rxq, &inode);
            if(ino == inode)
            {
                fclose(fp);
                switch(mode)
                {
                case TCP_INFO_MODE_GET_STATE:
                    return state;
                case TCP_INFO_MODE_GET_OQUEUE:
                    return txq;
                }
                return 0;
            }
        }
        ++line;
    }

    fclose(fp);
    return 0;
}
#elif defined(ZEPHYR)
int tcp_state(int fd)
{
    uint32_t state;
    net_tcp_state(fd, &state);
    return state;
}
int tcp_ofree(int fd)
{
#if 1
    int txq;
    
    net_tcp_oqueue(fd, &txq);

    return DCF_SOCK_BUF - txq;
#else
    return 4096;
#endif
}
int tcp_oqueue(int fd)
{
    int txq;

    net_tcp_oqueue(fd, &txq);

    return txq;
}
#endif /* #ifdef LINUX */
void portd_wait_empty(int port, int fd_port, unsigned long timeout)
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
        usleep(200000);
    }
}

int portd_getexitflag(int port)
{
    return !!portd_terminate;
}

void portd_setexitflag(int port, int flag)
{
    portd_terminate = flag;
}


