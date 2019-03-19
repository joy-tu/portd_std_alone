/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    sercmd.c

    API-sercmd for scm thread.

    2011-09-19  Joy Tu

    First version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
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

int sercmd_open(int port)
{
	int fd;

	while ((fd = sio_open(port)) < 0) {
		printf("scm_open fail\n");
       	usleep(100000);
	}

   	return fd;
}

int sercmd_close(int port)
{
	return sio_close(port);
}

int sercmd_read(int port, char *buf, int len)
{
	return sio_read_ex(port, buf, len);
}

int sercmd_write(int port, char *buf, int len)
{
	return sio_write(port, buf, len);
}

int sercmd_ioctl(int port, int baud, int mode)
{
	return sio_ioctl(port, baud, mode);
}

int sercmd_notify_error(int port)
{
	return  sio_notify_error(port);
}

