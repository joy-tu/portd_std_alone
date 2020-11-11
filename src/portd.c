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
#include <posix/pthread.h>
#include <header.h>
#include <posix/netinet/in.h>
#include <portd.h>
#include <sio/mx_sio.h>
#include "aspp.h"

struct port_data Gport;

volatile int portd_terminate = 0;
static int portd_init(int port_idx);
static void portd(int port_idx);
static void portd_exit(int port_idx);
extern int delimiter_init(int port, int has_delimiter, int has_buffering);
extern void delimiter_exit(int port);

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
    int ret;

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

static void portd(int port_idx)
{
    struct port_data *ptr;

    ptr = &Gport;

    ptr->thread_id = (pthread_t)NULL;

    switch (ptr->opmode)
    {
        case CFG_APPLICATION_DEVICE_CONTROL:
        {
            if (ptr->application == CFG_OPMODE_REALCOM) // RealCOM
            {
                printf("Start port %d as RealCOM mode\n", port_idx);
		  pthread_create(&ptr->thread_id, NULL, &aspp_start, (void *)port_idx);
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

int tcp_state(int fd)
{
    uint32_t state;
    net_tcp_state(fd, &state);
    return state;
}

int portd_getexitflag(int port)
{
    return !!portd_terminate;
}

void portd_setexitflag(int port, int flag)
{
    portd_terminate = flag;
}
