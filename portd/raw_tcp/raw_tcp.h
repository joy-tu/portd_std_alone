/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : raw_tcp.h                                                  */
/* Description  :                                                            */
/* Product      : Secured Serial Device Server                               */
/* Programmer   : Shinhsy Shaw                                               */
/* Date         : 2003-07-17                                                 */
/*****************************************************************************/
#ifndef _RAW_TCP_H_
#define _RAW_TCP_H_
#endif

#ifdef	_RAW_TCP_C
#define	RAW_TCP_EXTERN
#else
#define	RAW_TCP_EXTERN extern
#endif

void * raw_tcp_start(void* portno);
void raw_tcp_main(int port);
int	raw_tcp_write(int port, int fd_net, char *buf, int len);
int raw_tcp_dns_ask_ip(char *dns_ip, u_long *ip);
unsigned long raw_tcp_reconnect(int port, u_long retry_time);
void raw_tcp_connect(int port, int index);
u_long raw_tcp_check_connect(int port, u_long check_time);
void raw_tcp_connect_ok(int port, int index);
void raw_tcp_connect_fail(int port, int index);
void raw_tcp_close(int port, int index);
int raw_tcp_check_count(int port);



#define	TCP_CLIENT_CLOSED		0
#define	TCP_CLIENT_CONNECTING	1
#define TCP_CLIENT_CONNECTED	2

#define RAW_TCP_RECONNECT_TIMEOUT	2000		/* reconnect time interval 100 mini-seconds */
#define RAW_TCP_CHECK_CONNECT_TIMEOUT	5000	/* check connect timeout 5 seconds */

