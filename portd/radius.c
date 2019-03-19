/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    radius.c

    Routines to support Authentication (RADIUS)

    2008-05-27	Kevin Chu
		new release
*/

/*

RADIUS SERVER MUST RUN BEFORE CN2000 BOOT UP!!!!!!
OTHERWYSE CN2000 WILL DISABLE THE RADIUS ACCOUNT SERVICE

							NOW
							------------
Config: RADIUS enable		[Y]			always enable
	RADIUS ACCOUNT enable	[Y]			always enable
	RADIUS Server IP	[xxx.xxx.xxx.xxx]	same as chk_log
	RADIUS backup Server IP [xxx.xxx.xxx.xxx]	no yet :-)
	RETRY time interval	[ 3] sec		3 sec
	RETRY count		[10]			10

[POWER-ON]
	ACCOUNT Request:
		User name				Y
		NAS IP					Y
	if fail, disable RADIUS ACCOUNT 		(no!)
[POWER-OFF]
	ACCOUNT Request:
		User name				Y
		NAS IP					Y

[LOGIN]
	RADIUS Request:
		User name				Y
		NAS IP					Y
		NAS port				Y
		Password				Y
	RADIUS Accept:
		User name				don't care
		NAS IP					don't care
		NAS port				don't care
		Password				don't care
		Session timeout 			Y
		Idle timeout				Y
		Termination action			Y
		State					Y
		Class					Y
		Service type: TELNET/RLOGIN		Y
			REMOTE Server IP:		.

		Service type: FRAME
			Frame type: PPP/SLIP		.
			MTU:				.
			REMOTE IP:			.
			Netmask:			.
	ACCOUNT Request:
		User name				Y
		NAS IP					Y
		NAS port				Y
[LOGOUT]
	RADIUS Request:
		User name				Y
		NAS IP					Y
		NAS port				Y
		Password				Y
		State					Y
	ACCOUNT Request:
		User name				Y
		NAS IP					Y
		NAS port				Y
		Input counts				Y
		Output counts				Y
		Session time				Y

*/

#include <header.h>
#include <config.h>
#include <stdio.h>
#include <portd.h>
#include <sysapi.h>
#include <arpa/inet.h>

#include "radius.h"
#include "md5.h"

extern int Gsys_active_lan;
extern u_long	radius_pid;

extern char PortdUsername[PORTD_USER_LEN+1];
extern char PortdPassword[PORTD_PSWD_LEN+1]; 

#define RESEND_TIME			3	/* 3 Sec */
#define RESEND_MAX_COUNT	5

#define ACT_FAIL			0	/* login fail */
#define ACT_OK				1	/* login ok */
#define ACT_TIMEOUT			-1	/* error happen, must close the link */

#define STATE_IDLE			0
#define STATE_LOGIN			1
#define STATE_ACC_LOGIN 	2
#define STATE_WORKING		3
#define STATE_ACC_LOGOUT	4
#define STATE_LOGOUT		5
#define STATE_CHAP_LOGIN	6
#define STATE_BOOT_UP		7
#define STATE_BOOT_DOWN 	8

#define FLAG_TERMINATE_REQUEST	0x01

#define RADIUS_STATE_LEN	16
#define RADIUS_CLASS_LEN	32

struct	radius_stru
{
	u_char	vector[RADIUS_VECTOR_LEN];
	u_char	pswd[PORTD_PSWD_LEN];
	u_char	id;
	u_char	flag;
	u_short	acc_session_id;
	int		state;
	char	rad_state[RADIUS_STATE_LEN];
	int		rad_state_len;
	char	rad_class[RADIUS_CLASS_LEN];
	int		rad_class_len;
	int		retry;
	u_long	lastTime;
	int		(*func)(int, int);
};

typedef struct	radius_stru *	radius_t;

static	void	radius_boot(int mode);
static	void	radius_tsrv(radius_t rd);
static	void	radius_recv(int fd);
static	int	init(void);
u_long		auth_serverIP;
static	struct	radius_stru	radiusTbl;
int account_flag;
static	int		fd_radius, fd_account;
struct sockaddr_in to_radius, to_account;
static	u_char		radius_id;
static	char		radius_key[16+1];
static	int		radius_key_len;
static	u_short		total_login;
//static	proc_t		auth_task;
static	int		radius_enable_account;

extern int portd_terminate;
int portno;

void radius(u_long arg)
{
	int fd_max;
	radius_t	rd;
	fd_set		rfds;
	struct timeval 	time;

	portno = (int)arg;
	//printf("rd%d: radius!!\r\n", portno);
	arg = arg;
	if (init() < 0)
		return;
	radius_up();
	if (fd_radius > fd_account)
		fd_max = fd_radius;
	else
		fd_max = fd_account;
	time.tv_sec = 1;
	time.tv_usec = 0;
	//printf("main_loop\r\n");
	while (portd_terminate == 0)
	{
		FD_ZERO(&rfds);
		FD_SET(fd_radius, &rfds);
		FD_SET(fd_account, &rfds);
		if( select(fd_max+1, &rfds, NULL, NULL, &time) <= 0 )
		{
			rd = &radiusTbl;
			if (rd->state != STATE_IDLE &&
			   	rd->state != STATE_WORKING)
			{
				if ((sys_clock_s() - rd->lastTime) >= RESEND_TIME)
					radius_tsrv(rd);
			}	
			continue;
		}
		if (FD_ISSET(fd_radius, &rfds))
			radius_recv(fd_radius);
		if (FD_ISSET(fd_account, &rfds))
			radius_recv(fd_account);	
	}
	//printf("rd%d: radius end!!\r\n", portno);
}

void		radius_login(int port, int (*func)(int, int), int mode)
{
	radius_t	rd;
	//printf("rt: radius_login\r\n");
	rd = &radiusTbl;
	if ( auth_serverIP == 0xFFFFFFFFL ) {
		func(port, -2);
		return;
	}
	if ( rd->state == STATE_LOGIN || rd->state == STATE_ACC_LOGIN ||
	     rd->state == STATE_WORKING || rd->state == STATE_CHAP_LOGIN )
		return;
	if ( mode == 4 )
		rd->state = STATE_CHAP_LOGIN;
	else
		rd->state = STATE_LOGIN;
	rd->lastTime = sys_clock_s() - RESEND_TIME;
	rd->retry = 0;
	rd->func = func;
	rd->flag = 0;
	rd->rad_state_len = 0;
	rd->rad_class_len = 0;
	//sys_ThreadResume(radius_pid);
}
void radius_logout(int port, int (*func)(int, int))
{
	radius_t	rd;
	//printf("rt: radius_logout\r\n");
	rd = &radiusTbl;
	if ( auth_serverIP == 0xFFFFFFFFL ) {
		func(port, ACT_OK);
		return;
	}
	if ( rd->state == STATE_IDLE || rd->state == STATE_LOGIN ||
	     rd->state == STATE_CHAP_LOGIN ) {
		rd->state = STATE_IDLE;
		func(port, ACT_OK);
		return;
	}

	if ( rd->state == STATE_ACC_LOGOUT || rd->state == STATE_LOGOUT )
		return;
	if ( rd->flag & FLAG_TERMINATE_REQUEST )
		rd->state = STATE_LOGOUT;
	else if ( radius_enable_account )
		rd->state = STATE_ACC_LOGOUT;
	else {
		rd->state = STATE_IDLE;
		func(port, ACT_OK);
		return;
	}
	rd->lastTime = sys_clock_s() - RESEND_TIME;
	rd->retry = 0;
	rd->func = func;
	//sys_ThreadResume(radius_pid);
}


void		radius_up(void)
{
	//printf("radius_up\r\n");
	radius_boot(PW_ACCOUNTING_ON);		/* RADIUS ACCOUNT login */
}

/*
 *	Radius system boot up or down
 */
static	void	radius_boot(int mode)
{
	radius_t	rd;
	//printf("radius_boot\r\n");
	rd = &radiusTbl;
	if ( auth_serverIP == 0xFFFFFFFFL )
	{
		radius_enable_account = 0;
		return;
	}

	/*
	 * RADIUS Accounting can be disable
	 */
	if ( account_flag == 0 )
	{
		radius_enable_account = 0;
		return;
	}

	if ( mode == PW_ACCOUNTING_ON )
	{
		radius_enable_account = 2;	/* 2 mean checking */
	}
}

/* MD5 function */
void md5_calc(unsigned char *output, unsigned char *input, unsigned int inlen)
{
	MD5_CONTEXT 	context;

	MD5Init(&context);
	MD5Update(&context, input, inlen);
	MD5Final(output, &context);
}

static	void	radius_tsrv(radius_t rd)
{
	int		i, port, len;
	u_long		val;
	u_char		buf[256], md5buf[16], *ptr;
	radius_hdr_t	rd_hdr;
	u_long myip;
	char ipaddr[16];
    char active_if[10];
	port = 1;

	//printf("rd: radius_tsrv\r\n");
	//printf("rd: rd->retry: %d\r\n", rd->retry);
	//printf("rd: rd->state: %d\r\n", rd->state);
	if ( rd->retry > RESEND_MAX_COUNT )
	{
		if ( rd->state == STATE_BOOT_UP || rd->state == STATE_BOOT_DOWN )
		{
			if ( rd->state == STATE_BOOT_UP )
			{
				/* No account server! Disable account report */
				/* Or I can try backup server.... */
			/*
				radius_enable_account = 0;
			*/
			}
			if ( rd->state == STATE_BOOT_DOWN )
				radius_enable_account = 0;

		} 
		else
			rd->func(port, ACT_TIMEOUT);
		rd->state = STATE_IDLE;
		return;
	}
	
	if ( rd->retry == 0 )
	{ 	/* first time, need prepare package */
		rd->id = radius_id++;	/* if a port is waiting too long, and
					   				radius_id loop to same ID, will
					   				make problem, but ID total can has 256
					   				, 16 port cannot retry so fast, so
					   				I don't care */
		if ( rd->state == STATE_LOGIN || rd->state == STATE_LOGOUT )
		{
			for ( i=0; i<RADIUS_VECTOR_LEN; i+=4 )
				*(u_long *)&rd->vector[i] = rand();
			ptr = buf;
			ptr = memcpy(ptr, radius_key, radius_key_len);
			ptr += radius_key_len;
			ptr = memcpy(ptr, rd->vector, RADIUS_VECTOR_LEN);
			ptr += RADIUS_VECTOR_LEN;

			md5_calc(md5buf, buf, ptr - buf);
			for ( i=0; i<PORTD_PSWD_LEN; i++ )
				rd->pswd[i] = PortdPassword[i] ^ md5buf[i];
		}
	}
	
	rd->retry++;
	rd->lastTime = sys_clock_s();

	rd_hdr = (radius_hdr_t)buf;
	rd_hdr->id = rd->id;
	ptr = &buf[RADIUS_HDR_LEN];

	/* NAS IP attrib */
	*ptr++ = PW_NAS_IP_ADDRESS;
	*ptr++ = sizeof(u_long) + 2;
	sys_getActiveNetworkName(active_if);
	sys_getmyip(active_if, ipaddr);
	myip = inet_addr(ipaddr);
	memcpy(ptr, (char *) &myip, 4);
	ptr += sizeof(u_long);

	if ( rd->state != STATE_BOOT_UP && rd->state != STATE_BOOT_DOWN )
	{
		/* User name attrib */
		*ptr++ = PW_USER_NAME;
		len = strlen(PortdUsername);
		*ptr++ = len + 2;
		memcpy(ptr, PortdUsername, len);
		ptr+= len;

		/* NAS port attrib */
		*ptr++ = PW_NAS_PORT;
		*ptr++ = sizeof(u_long) + 2;
		val = htonl(port);
		memcpy(ptr, (char *)&val, 4);
		ptr += sizeof(u_long);
	}
#if 0
	if ( rd->state == STATE_ACC_LOGOUT ) {
		/* Input counts attrib */
		*ptr++ = PW_ACCT_INPUT_OCTETS;
		*ptr++ = sizeof(u_long) + 2;
		val = htonl(PortdTotalRxCnt[port]);
		memcpy(ptr, (char *)&val, 4);
		ptr += sizeof(u_long);
		/* Output counts attrib */
		*ptr++ = PW_ACCT_OUTPUT_OCTETS;
		*ptr++ = sizeof(u_long) + 2;
		val = htonl(PortdTotalTxCnt[port]);
		memcpy(ptr, (char *)&val, 4);
		ptr += sizeof(u_long);
		/* Session time attrib */
		*ptr++ = PW_ACCT_SESSION_TIME;
		*ptr++ = sizeof(u_long) + 2;
		val = htonl(PortdTotalTime[port]);
		memcpy(ptr, (char *)&val, 4);
		ptr += sizeof(u_long);
		/* terminate cause */
		*ptr++ = PW_ACCT_TERMINATE_CAUSE;
		*ptr++ = sizeof(u_long) + 2;
		val = htonl(PortdTerminateCause[port]);
		memcpy(ptr, (char *)&val, 4);
		ptr += sizeof(u_long);
	}
#endif
	
	switch ( rd->state )
	{
	case STATE_LOGIN:
	case STATE_CHAP_LOGIN:
		rd_hdr->code = PW_ACCESS_REQUEST;
		if ( rd->state == STATE_CHAP_LOGIN ) {
#if 0
			ps = &Gppp_state[port];
			/* CHAP CHALLENGE attrib */
			*ptr++ = PW_CHAP_CHALLENGE;
			*ptr++ = 2 + 16;
			memcpy(ptr, ps->challenge, 16);
			ptr+=16;

			/* CHAP PASSWORD */
			*ptr++ = PW_CHAP_PASSWORD;
			*ptr++ = 2 + 1 + 16;
			*ptr++ = ps->chap_ident;
			memcpy(ptr, PortdPassword[port], 16);
			ptr+=16;
#endif
		}
		else
		{
			/* User pswd attrib */
			*ptr++ = PW_USER_PASSWORD;
			len = PORTD_PSWD_LEN;
			*ptr++ = len + 2;
			memcpy(ptr, rd->pswd, len);
			ptr += len;
		}
		break;
	case STATE_BOOT_UP:
	case STATE_BOOT_DOWN:
	case STATE_ACC_LOGIN:
	case STATE_ACC_LOGOUT:
		rd_hdr->code = PW_ACCOUNTING_REQUEST;

		/* account status attrib */
		*ptr++ = PW_ACCT_STATUS_TYPE;
		*ptr++ = sizeof(u_long) + 2;
		if ( rd->state == STATE_BOOT_UP )
			val = htonl(PW_ACCOUNTING_ON);
		else if ( rd->state == STATE_BOOT_DOWN )
			val = htonl(PW_ACCOUNTING_OFF);
		else if ( rd->state == STATE_ACC_LOGIN )
			val = htonl(PW_STATUS_START);
		else
			val = htonl(PW_STATUS_STOP);
		memcpy(ptr, (char *) &val, 4);
		ptr += sizeof(u_long);

		/* account session ID attrib */
		*ptr++ = PW_ACCT_SESSION_ID;
		*ptr++ = 8 + 2;
		if ( rd->state == STATE_BOOT_UP || rd->state == STATE_ACC_LOGIN )
			rd->acc_session_id = total_login;
		//sprintf((char*)ptr, "%02d%06d", port, rd->acc_session_id);
		ptr += 8;

		/*
		 * This Callback-Number attribute added by Yu-Lang Hsu at
		 * 07-30-1998 for RADIUS server logging.
		 * The Framed-IP-Address attribute added at 09-22-1998.
		 */
		if ( rd->state == STATE_ACC_LOGIN )
		{
#if 0
		    opmode = PortdOPMode[port] & D_VCTYPE_TYPEMASK;
		    if ( (opmode == D_VCTYPE_PPP) || (opmode == D_VCTYPE_PPPD) ) 
		    {
				ps = &Gppp_state[port];
				*ptr++ = PW_FRAMED_IP_ADDRESS;
				*ptr++ = 6;
				val = ps->dst;
				memcpy(ptr, (char *)&val, 4);
				ptr += 4;
		    }
		    if ( (PortdServiceType[port] == PW_CALLBACK_FRAMED) &&
				 (PortdCallBackString[port][0] != 0) ) 
			{
				*ptr++ = PW_LOGIN_CALLBACK_NUMBER;
				len = strlen(PortdCallBackString[port]);
				*ptr++ = len + 2;
				memcpy(ptr, PortdCallBackString[port], len);
				ptr+=len;
		    }
#endif
		}

		/* class attrib */
		if ( rd->rad_class_len ) {
			*ptr++ = PW_CLASS;
			len = rd->rad_class_len;
			*ptr++ = len + 2;
			ptr = memcpy(ptr, rd->rad_class, len);
		}

		break;
	case STATE_LOGOUT:
		rd_hdr->code = PW_ACCESS_REQUEST;

		/* User pswd attrib */
		*ptr++ = PW_USER_PASSWORD;
		len = PORTD_PSWD_LEN;
		*ptr++ = len + 2;
		memcpy(ptr, rd->pswd, len);
		ptr+=len;

		/* state attrib */
		if ( rd->rad_state_len ) {
			*ptr++ = PW_STATE;
			len = rd->rad_state_len;
			*ptr++ = len + 2;
			memcpy(ptr, rd->rad_state, len);
			ptr += len;
		}
		break;
	}

	len = ptr - buf;
	rd_hdr->length = htons(len);
	
	if ( rd->state == STATE_ACC_LOGIN || rd->state == STATE_ACC_LOGOUT ||
	     rd->state == STATE_BOOT_UP || rd->state == STATE_BOOT_DOWN )
	{
		memset(rd_hdr->vector, 0, RADIUS_VECTOR_LEN);
		memcpy(ptr, radius_key, radius_key_len);
		md5_calc(md5buf, buf, len + radius_key_len);
		memcpy(rd_hdr->vector, md5buf, RADIUS_VECTOR_LEN);
		memcpy(rd->vector, md5buf, RADIUS_VECTOR_LEN);
		sendto(fd_account, buf, len, 0, (struct sockaddr *)&to_account, sizeof(struct sockaddr_in));
	}
	else
	{
		memcpy(rd_hdr->vector, rd->vector, RADIUS_VECTOR_LEN);
		//printf("sendto!\r\n");
		sendto(fd_radius, buf, len, 0, (struct sockaddr *)&to_radius, sizeof(struct sockaddr_in));
	}
}

static	void	radius_recv(int fd)
{
	int		i, len, port=0;
	u_long		val;
	u_char		*ptr, buf[256], md5buf[16];
	u_char		old_vector[RADIUS_VECTOR_LEN];
	struct sockaddr_in from;
	radius_t	rd;
	radius_hdr_t	rd_hdr;
	u_char		code, r_len;
	
	//printf("radius_recv\r\n");
	i = sizeof(from);
	len = recvfrom(fd, (char *)buf, 256, 0, (struct sockaddr *)&from, (socklen_t *)&i);
	if ( len < RADIUS_HDR_LEN )
		return;
	if ( len > 256 - radius_key_len )
		return;
	if ( auth_serverIP != from.sin_addr.s_addr )
		return;
	rd_hdr = (radius_hdr_t)buf;

	rd = &radiusTbl;
	if ( (rd->state != STATE_IDLE) && (rd->state != STATE_WORKING) && (rd->id == rd_hdr->id) )
	{
		/* verify frame */
		memcpy(old_vector, rd_hdr->vector, RADIUS_VECTOR_LEN);
		memcpy(rd_hdr->vector, rd->vector, RADIUS_VECTOR_LEN);
		memcpy(&buf[len], radius_key, radius_key_len);
		md5_calc(md5buf, buf, len + radius_key_len);
		memcpy(rd_hdr->vector, old_vector, RADIUS_VECTOR_LEN);
		if (memcmp(old_vector, md5buf, RADIUS_VECTOR_LEN) != 0)
			return;
	}	
	else
		return;

	switch (rd_hdr->code)
	{
		case PW_ACCESS_ACCEPT :
			switch (rd->state)
			{
				case STATE_LOGIN:
				case STATE_CHAP_LOGIN:
					total_login++;
					/* decode attrib code */
					len -= RADIUS_HDR_LEN;
					ptr = &buf[RADIUS_HDR_LEN];
					while ( len >= 2 )
					{
						code = *ptr++;
						r_len = *ptr++;
						memcpy((char *)&val, ptr, 4);

						switch ( code )
						{
							case PW_SESSION_TIMEOUT:
#if 0
								PortdSessionTime[port] = htonl(val);
#endif
								break;
							case PW_IDLE_TIMEOUT:
#if 0								
								PortdIdleTime[port] = htonl(val);
#else

#endif
								break;
							case PW_STATE:
								i = r_len - 2;
								if ( i <= RADIUS_STATE_LEN )
								{
									memcpy(rd->rad_state, ptr, i);
									rd->rad_state_len = i;
								}
								break;
							case PW_TERMINATION_ACTION:
								if ( htonl(val) == PW_RADIUS_REQUEST )
									rd->flag |= FLAG_TERMINATE_REQUEST;
								break;
							case PW_SERVICE_TYPE:
#if 0								
								PortdServiceType[port] = htonl(val);
#endif
								break;
							case PW_CLASS:
								i = r_len - 2;
								if ( i <= RADIUS_CLASS_LEN )
								{
									memcpy(rd->rad_class, ptr, i);
									rd->rad_class_len = i;
								}
								break;
							case PW_FRAMED_PROTOCOL:
#if 0								
								PortdFrameType[port] = htonl(val);
#endif
								break;
							case PW_FRAMED_IP_ADDRESS:
#if 0								
								/* I don't change the bit order!! */
								PortdFrameIP[port] = val;
#endif
								break;
							case PW_FRAMED_IP_NETMASK:
#if 0								
								/* I don't change the bit order!! */
								PortdFrameMask[port] = val;
#endif
								break;
							case PW_FRAMED_ROUTING:
								break;
							case PW_FRAMED_MTU:
#if 0								
								PortdFrameMTU[port] = htonl(val);
#endif
								break;
							case PW_FRAMED_COMPRESSION:
#if 0
								PortdFrameCMP[port] = htonl(val);
#endif
								break;
							case PW_LOGIN_IP_HOST:
#if 0
								/* I don't change the bit order!! */
								PortdLoginHost[port] = val;
#endif
								break;
							case PW_LOGIN_SERVICE:
#if 0
								PortdLoginType[port] = htonl(val);
#endif
								break;
							case PW_LOGIN_PORT:
#if 0
								PortdLoginTCPPort[port] = htonl(val);
#endif
								break;
							case PW_LOGIN_CALLBACK_NUMBER:
#if 0								
								memcpy(PortdCallBackString[port], ptr,
									r_len - 2);
								PortdCallBackString[port][r_len-2] = 0;
#endif								
								break;
							case PW_REPLY_MESSAGE:
							case PW_FRAMED_ROUTE:
							default:
								break;
						}
						ptr += r_len - 2;
						len -= r_len;
					}
					if ( radius_enable_account )
					{
						rd->state = STATE_ACC_LOGIN;
						rd->lastTime = sys_clock_s();
						rd->retry = 0;
						radius_tsrv(rd);
					}
					else
					{
						rd->state = STATE_WORKING;
						rd->func(port, ACT_OK);
					}
					break;
				case STATE_LOGOUT:
					if ( radius_enable_account )
					{
						rd->state = STATE_ACC_LOGOUT;
						rd->lastTime = sys_clock_s();
						rd->retry = 0;
						radius_tsrv(rd);
					}
					else
					{
						rd->func(port, ACT_OK);
						rd->state = STATE_IDLE;
					}
					break;
			}
			break;
		case PW_ACCESS_REJECT :
			/****** Want to add:
			   get the reply message and show out to
			   user, or send by PPP frame....
			   Just add the string to the return function argment
			*****!!*****/
			switch (rd->state)
			{
				case STATE_LOGIN:
				case STATE_CHAP_LOGIN:
					rd->func(port, ACT_FAIL);
					rd->state = STATE_IDLE;
					break;
				case STATE_LOGOUT:
					rd->func(port, ACT_FAIL);
					rd->state = STATE_IDLE;
					break;
			}
			break;
		case PW_ACCOUNTING_RESPONSE:
			switch (rd->state)
			{
				case STATE_BOOT_UP:
					rd->state = STATE_WORKING;
					radius_enable_account = 1;
					break;
				case STATE_BOOT_DOWN:
					rd->state = STATE_IDLE;
					radius_enable_account = 0;
					break;
				case STATE_ACC_LOGIN:
					rd->func(port, ACT_OK);
					rd->state = STATE_WORKING;
					if ( radius_enable_account == 2 )
					{
						radiusTbl.state = STATE_WORKING;
						radius_enable_account = 1;
					}
					break;
				case STATE_ACC_LOGOUT:
					rd->func(port, ACT_OK);
					rd->state = STATE_IDLE;
					break;
			}
	}
}


static	int	init(void)
{
	
	//printf("rd%d: init\r\n", portno);
	radius_t	rd;
	struct sockaddr_in 	sin;
	int radius_udp_port, acct_udp_port;
	int t;	
	char 		serverip[DCF_IP_DNS_LEN+1];

	auth_serverIP = 0xFFFFFFFFL;
	Scf_getRadius(serverip, radius_key,  &radius_udp_port, &account_flag);
	//printf("rd%d: server: %s, key: %s, port: %d, accounting: %d\n",portno, serverip, radius_key, radius_udp_port, account_flag);
	radius_udp_port = (radius_udp_port) ? RADIUS_NEW_UDP_PORT : RADIUS_UDP_PORT;
	if( strlen(serverip) == 0 )
	{
		return -1;
	}
	Ssys_getServerIp(serverip, &auth_serverIP, 4000);
	//printf("rd%d: auth_serverIP: 0x%x\n",portno, (int)auth_serverIP);
	if ( auth_serverIP == 0xFFFFFFFFL )
		return -1;
	radius_key_len = strlen(radius_key);
	if ( radius_udp_port == RADIUS_NEW_UDP_PORT )
	{
		acct_udp_port = RADIUS_NEW_ACC_UDP_PORT;
	}
	else
	{
		/* radius_udp_port = RADIUS_UDP_PORT; */
		acct_udp_port = RADIUS_ACC_UDP_PORT;
	}
	rd = &radiusTbl;
	rd->state = STATE_IDLE;
	radius_id = 0;
	total_login = 0;
	while ( (fd_radius = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
		sleep(1000);
	t = 1;
	while (setsockopt(fd_radius, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) == -1)
		sleep(1);
	sin.sin_family = AF_INET;
	sin.sin_port = radius_udp_port + (portno - 1)*2;
	sin.sin_addr.s_addr = INADDR_ANY;
	while (bind(fd_radius, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    	sleep(1);
	while ( (fd_account = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
		sleep(1000);
	t = 1;
	while (setsockopt(fd_account, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) == -1)
		sleep(1);
	sin.sin_family = AF_INET;
	sin.sin_port = acct_udp_port + (portno - 1)*2;
	sin.sin_addr.s_addr = INADDR_ANY;
	while (bind(fd_account, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    	sleep(1);
	to_radius.sin_family = AF_INET;
	to_radius.sin_port = htons(radius_udp_port);
	to_radius.sin_addr.s_addr = auth_serverIP;
	to_account.sin_family = AF_INET;
	to_account.sin_port = htons(acct_udp_port);
	to_account.sin_addr.s_addr = auth_serverIP;

	return 0;
}

