#include <header.h>
#include <sio.h>
#include <portd.h>
#include <config.h>
#include <net_config.h>
#include <stdio.h>
#include "radius.h"
#include "../console/console.h"

#define KEYIN_TIMEOUT	120000L  /* 2 minutes, keyin data */
#define PFLAG_OK		1
#define PFLAG_FAIL		2
#define PFLAG_TIMEOUT	4
#define PFLAG_NOSERVER	8
#define PFLAG_RESULT	(PFLAG_OK | PFLAG_FAIL | PFLAG_TIMEOUT | PFLAG_NOSERVER)

static	u_char		pwd_flag;

char	Gterm_user[]="\r\nlogin: ";
char	Gterm_pswd[]="password: ";
char	Gterm_CRLF[]="\r\n";
char	Gterm_errpwd[]="\r\nLogin incorrect !\r\n";
char	Gterm_errto[]="\r\nLogin timed out after 120 seconds !\r\n";
char	Gterm_errato[]="\r\nAuthentication server connection timed out !\r\n";
char	Gterm_chkauth[]="\r\nChecking authorization, Please wait ...";

extern int portd_terminate;
extern unsigned long PortdLastTime;
extern char PortdUsername[PORTD_USER_LEN+1];
extern char PortdPassword[PORTD_PSWD_LEN+1]; 

static int portd_logout_back(int port, int ok)
{
	PortdUsername[PORTD_USER_LEN] = ok;
	return(0);
}

void portd_logout(int port)
{
	u_long	t;

	//printf("rt: portd_logout\r\n");
	PortdUsername[PORTD_USER_LEN] = 0xFF;
	radius_logout(port, portd_logout_back);
	t = sys_clock_s();
	while ( (sys_clock_s() - t) < 35 ) 	/* wait 35 sec */
	{	
		if ( PortdUsername[PORTD_USER_LEN] != 0xFF )
			break;
		sleep(1);
	}
	//printf("rt: portd_logout ok\r\n");
}

/*
 *	Maybe asking result from old chk_log or RADIUS, so don't care
 *	timeout...
 */
static	int	rpwd_back(int port, int ok)
{
	if (pwd_flag & PFLAG_RESULT)
		return(0);
	if (ok == 1)				/* password check OK */
		pwd_flag |= PFLAG_OK;
	else if (ok == 0)			/* password check fail */
		pwd_flag |= PFLAG_FAIL;
	else if (ok == -1)
		pwd_flag |= PFLAG_TIMEOUT;
	else
		pwd_flag |= PFLAG_NOSERVER;
	return(0);
}

int	str_no_space_len(char *str, int len)
{
	int	i, j;

	for ( j = 0; j < len; j++ )
		if ( str[j] != 0x20 )
			break;
	if ( (j == len) || (str[j] == 0) )
		return(0);
	for ( i = j; i < len; i++ )
		if ( str[i] <= 0x20 )
			break;
	return(i - j);
}

int	config_pswd(char *user, char *buff)
{
	int		i, n, m, ulen;
	char username[DCF_LOCAL_NAME_LEN + 1];
	char password[DCF_LOCAL_PSWD_LEN + 1];
	char	*p;

	ulen = strlen(user);
	for (i = 0; i < DCF_MAX_USERTABLES; i++)
	{
		Scf_getUserTable(i, username, password);

		n = str_no_space_len(username, DCF_LOCAL_NAME_LEN);
		if ( (n <= 0) || (ulen != n) || strncmp(user, username, n) )
			continue;
		p = password;
		n = DCF_LOCAL_PSWD_LEN;
		if ( (m = str_no_space_len(p, n)) > 0 )
			strncpy(buff, p, m);
		buff[m] = 0;
		return(m);
	}
	return(-1);
}

int	lpswdchk(char *user, char *pswd, int pswd_len)
{
	char	buf[32];
	if ( (config_pswd(user, buf) != pswd_len) || (strncmp(buf, (char*)pswd, pswd_len) != 0) )
		return(D_FAILURE);
				return(D_SUCCESS);
}

/*
 *	read data from tcp port, terminaled by CR, and ignore LF
 *	This routine is slow, so only use when user typing !!!
 *	tout:  timeout
 *	eflag: bit 0	0: don't echo   1: echo
 *	       bit 1			1: detect PPP frame
 *	       bit 2			1: detect SLIP frame
 *	       bit 3			1: need to detect ascterm special key
 *	return: >0	length received
 *		100	PPP detected
 *		200	SLIP detected
 *		==0	length = 0
 *		-1	timeout
 *		-2	async disconnected or force exit
 *		-3	escape key checked
 */
int		net_linput(int fd_net, int port, char *buf, int len, u_long tout, int eflag)
{
	int		i, ndx = 0;
	fd_set		rfds;
	struct timeval time;
	u_long		t;
	u_char		ch;

	time.tv_usec = 0;
	time.tv_sec = 2;
	t = sys_clock_ms();

	while ((sys_clock_ms() - t) <= tout) 
	{
		FD_ZERO(&rfds);
		FD_SET(fd_net, &rfds);

		if (portd_terminate != 0)
			return(-2);
		if (select(fd_net+1, &rfds, NULL, NULL, &time) <= 0) 
			continue;

		PortdLastTime = sys_clock_s();

		if (FD_ISSET(fd_net, &rfds)) 
		{
			if ((i = recv(fd_net, &ch, 1, 0)) <= 0) 
			{
				if (i == 0) 
				{
					//radius_terminate_cause(port, PW_LOST_CARRIER);
					return(-2);
				}
				continue;
			}

			PortdLastTime = sys_clock_s();
#if 0			
			if (eflag & 2) 		/* ppp header check */
			{	
			/* RFC 1662 said follow frame is PPP */
			/*	0x7E	  0xFF	    0x03 0xC0 0x21 */
			/*	0x7E	  0xFF 0x7D 0x23 0xC0 0x21 */
			/*	0x7E 0x7d 0xFD 0x7D 0x23 0xC0 0x21 */
			/*	1	2    3	  4    5    6	 7 */
				if (ch == 0x7E) 
				{
					ppp_flag = 1;
					continue;
				}
				else if (ppp_flag == 1 && ch == 0x7D) 
				{
					ppp_flag = 2;
					continue;
				}
				else if (ppp_flag == 1 && ch == 0xFF) 
				{
					ppp_flag = 3;
					continue;
				}
				else if (ppp_flag == 2 && ch == 0xFD) 
				{
					ppp_flag = 3;
					continue;
				}
				else if (ppp_flag == 3 && ch == 0x7D) 
				{
					ppp_flag = 4;
					continue;
				}
				else if (ppp_flag == 4 && ch == 0x23) 
				{
					ppp_flag = 5;
					continue;
				}
				else if (ppp_flag == 3 && ch == 0x03) 
				{
					ppp_flag = 5;
					continue;
				}
				else if (ppp_flag == 5 && ch == 0xC0) 
				{
					ppp_flag = 6;
					continue;
				}
				else if (ppp_flag == 6 && ch == 0x21) 
				{
					return(100);
				}
				else
					ppp_flag = 0;
			}
			
			if (eflag & 4) 		/* slip header check */
			{	
				if (ch == 0xC0) 
				{
					slip_flag = 1;
					continue;
				}
				else if (slip_flag == 1 && ch >= 0x45 && ch <= 0x4F ) 
					return(200);
				else
					slip_flag = 0;
			}
			
			if (eflag & 8) 		/* ascii term check */
			{	
				if (term_key_check(port, ch) < 0) 
				{
					radius_terminate_cause(port, PW_USER_REQUEST);
					return(-3);
				}
			}
#endif			
			if (ch == 8) 			/* BS */
			{		
				if (ndx != 0) 
				{
					--ndx;
					if (eflag)
						send(fd_net, "\010 \010", 3, 0);
				}
			} 
			else if ( ch == 10 ) 		/* LF */
			{	
			} 
			else if ( ch == 13 ) 		/* CR */
			{
				send(fd_net, "\015\012", 2, 0);
				buf[ndx] = 0;
				return(ndx);
			} 
			else if (ch >= 32) 			/* char readable ? */
			{	
				if (ndx < len) 
				{
					buf[ndx++] = ch;
					buf[ndx] = 0;	/* some other function want read the string... */
					if (eflag)
						send(fd_net, &ch, 1, 0);
				}
			}
		}
	}
	//radius_terminate_cause(port, PW_LOST_SERVICE);
	return(-1);
}


/*
 *	Enter and check remote password
 *	return: 0	success
 *		1	detect PPP frame
 *		-1	error
 */
int		net_chk_pswd(int fd_net, int port, int flag, int local)
{
	int	i;
	u_long	t, t1;

	t = sys_clock_ms();	
	while (((sys_clock_ms() - t) <= KEYIN_TIMEOUT) && (portd_terminate == 0)) 
	{
		send(fd_net, Gterm_CRLF, strlen(Gterm_CRLF), 0);
		send(fd_net, Gterm_user, strlen(Gterm_user), 0);
		if ((i = net_linput(fd_net, port, PortdUsername,	PORTD_USER_LEN,			
				(KEYIN_TIMEOUT - (sys_clock_ms() - t)), 1 | flag)) < 0)
			break;
		
		if (i == 100)				/* PPP detected */
			return(1);
			
		if (i == 0) 
		{	
			usleep(500*1000);	
			continue;
		}
		
		send(fd_net, Gterm_pswd, strlen(Gterm_pswd), 0);
		if ((i = net_linput(fd_net, port, PortdPassword, PORTD_PSWD_LEN,
				KEYIN_TIMEOUT - (sys_clock_ms() - t), 0)) < 0)
			break;

		/* Allow superuser login */
/*		
		config_consolepswd(pwd);
		if (strcmp(PortdUsername[port], "AsyncServerADM") == 0 && strcmp(PortdPassword[port], pwd) == 0) 
		{
			pwd_flag[port] = PFLAG_OK;
		} 
		else if ( local ) 
*/
		if (local)	
		{
			if (lpswdchk(PortdUsername, PortdPassword, i) == D_SUCCESS)	
				pwd_flag = PFLAG_OK;
			else
				pwd_flag = PFLAG_FAIL;	
		} 
		else 
		{		
			send(fd_net, Gterm_chkauth, strlen(Gterm_chkauth), 0);
			pwd_flag = 0;		
			radius_login(port, rpwd_back, 1);
		}

		/* wait remote password check result */
		t1 = sys_clock_ms();
		while (sys_clock_ms() - t1 <= 40000L && portd_terminate == 0) 
		{
			if ( pwd_flag & PFLAG_RESULT )
				break;
			usleep(500*1000);
		}

		if (pwd_flag & PFLAG_OK) 
		{
			send(fd_net, Gterm_CRLF, strlen(Gterm_CRLF), 0);
			return(0);
		}
		else if (pwd_flag & PFLAG_FAIL)
			send(fd_net, Gterm_errpwd, strlen(Gterm_errpwd), 0);
		else if (pwd_flag & PFLAG_TIMEOUT)
			send(fd_net, Gterm_errato, strlen(Gterm_errato), 0);
		else 
		{
			send(fd_net, Gterm_errato, strlen(Gterm_errato), 0);
			usleep(100*1000);
			return(-1);
		}
	}
	
	if ((sys_clock_ms() - t) > KEYIN_TIMEOUT) 
	{
		//radius_terminate_cause(port, PW_LOST_SERVICE);
		sio_write(port, Gterm_errto, strlen(Gterm_errto));
		usleep(100*1000);
	}
	return(-1);
}
