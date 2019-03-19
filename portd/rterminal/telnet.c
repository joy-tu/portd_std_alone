/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    telnet.c

    Routines to support telnet encode/decode

    2008-05-27	Kevin Chu
		new release
*/

/************************************************************************/
/* Name 	: TELNET.C						*/
/* Description	: Telnet encode/decode routine				*/
/* Product	: CN2000 V2.x						*/
/* Module	: AP-Portd-Terminal					*/
/* Authod	: JE							*/
/* Date 	: 02-04-96						*/
/************************************************************************/
#include 	<portd.h>
#include 	<stdio.h>
#include 	<telnet.h>
#include 	<string.h>
#include 	<sio.h>

static	u_char	willdone0[12] = {
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_BINARY,
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_SGA,
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_TERM,
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_DO,(u_char)D_TELNET_BINARY,
};
static	u_char	willdone1[6] = {
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_SGA,
		(u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_TERM,
};
static u_char	willdone2[]={
		D_TELNET_IAC, D_TELNET_WILL, D_TELNET_ECHO,
		D_TELNET_IAC, D_TELNET_WILL, D_TELNET_SGA,
		D_TELNET_IAC, D_TELNET_WILL, D_TELNET_BINARY,
		/*D_TELNET_IAC, D_TELNET_DO,   D_TELNET_ECHO,*/
		D_TELNET_IAC, D_TELNET_DO,   D_TELNET_BINARY
};

void		telnet_setting(int fd_net, int mode, int *telflag)
{
	fd_set		rfds;
	struct timeval time;
	//printf("rt: telnet_setting\r\n");
	if ((mode == 1) || (mode == 2)) 
	{
		/* waiting 200 mini-seconds or remote data input */
		time.tv_sec = 0;
		time.tv_usec = 200*1000;
		FD_ZERO(&rfds);
		FD_SET(fd_net, &rfds);
		select(fd_net+1, &rfds, NULL, NULL, &time);
		if (mode == 2) /* Binary-Terminal mode */
		{	
			send(fd_net, (char *)willdone0, 12, 0);
			*telflag = TFLAG_WILLBIN | TFLAG_DOBIN;
		} 
		else /* ASCII-Terminal mode */
		{
			send(fd_net, (char *)willdone1, 6, 0);
			*telflag = 0;
		}
	}
	else /* Rtelnet mode */
	{
		//printf("telnet_setting_send\r\n");
		send(fd_net, (char *)willdone2, sizeof(willdone2), 0);
		*telflag = TFLAG_WILLECHO | TFLAG_WILLBIN /*|	TFLAG_DOECHO*/ | TFLAG_DOBIN;
	}
}

/*
 *	Encode telnet data, and sent to lan, previous function
 *	must check the lan is canwrite
 */
#define TELNET_BUF_LEN 256 
int		telnet_encode(int fd_net, u_char *source, int len, u_int quitkey, int telflag)
{
	int	i, n;
	u_char	c, *ptr_net, target_net[TELNET_BUF_LEN*2];

	while (len > 0) 
	{
		if (len > TELNET_BUF_LEN)
			n = TELNET_BUF_LEN;
		else
			n = len;
		len -= n;
		ptr_net = target_net;
		for ( i=0; i<n; i++ ) 
		{
			*ptr_net++ = c = *source++;
			if ( c == D_TELNET_IAC )
				*ptr_net++ = D_TELNET_IAC;
			else if ( c == 13 && (telflag & TFLAG_TXBIN) == 0 )
				*ptr_net++ = 0;
			if ( c == quitkey && c != 0 )
				return(-1);
		}
		if ( (i = (int)(ptr_net - target_net)) != 0 )
			send(fd_net, (char *)target_net, i, 0);
	}
	return(0);
}

int		telnet_decode(int fd_net, u_char *source, int len, u_char *target,
			int *telflag, char *termcap)
{
	int	i, flag, oldlen;
	u_char	ch, *ptr_net, *ptr_port;
	u_char	target_net[256];	/* the CMD cannot large then 256! */
	
	//printf("rt: telnet_decode\r\n");
	oldlen = len;
	flag = *telflag;
	ptr_port = target;
	ptr_net = target_net;
	
	while ( len-- ) 
	{
		ch = *source++;
		if ( flag & TFLAG_ENTER ) /* skip follow enter 0 */
		{	
			flag &= ~TFLAG_ENTER;
			if ( (ch == 0) && ((flag & TFLAG_RXBIN) == 0) )
				continue;
			if ( (ch == 10) && (flag & TFLAG_CRLF2CR) )
				continue;
			if ( (ch != 10) && (flag & TFLAG_CRLF2LF) )
				*ptr_port++ = 13;
		} 
		else if ( (ch == 10) && ((flag & TFLAG_IAC) == 0) ) 
		{
			if ( (flag & TFLAG_LF2CR) ) 
			{
				*ptr_port++ = 13;
				continue;
			} 
			else if ( (flag & TFLAG_LF2CRLF) ) 
			{
				*ptr_port++ = 13;
				*ptr_port++ = 10;
				continue;
			}
		}
		if ( (flag & (TFLAG_RXMASK)) == 0 ) 
		{
			if ( ch != D_TELNET_IAC ) 
			{
				*ptr_port++ = ch;
				if ( ch == 13 ) {	/* CR */
					if (((flag & TFLAG_RXBIN) == 0) || (flag & TFLAG_CRLF2)) 
					{
						flag |= TFLAG_ENTER;
						if ( flag & TFLAG_CRLF2LF )
							ptr_port--;
					}
				}
			}
			else 
			{
				flag |= TFLAG_IAC;
				continue;
			}
			continue;
		}
		
		if ( flag & TFLAG_IAC ) 
		{
			flag &= ~TFLAG_IAC;
			switch ( ch )
			{
				case D_TELNET_IAC:	*ptr_port++ = ch;	break;
				case D_TELNET_SB:	flag |= TFLAG_SB;	break;
				case D_TELNET_WILL:	flag |= TFLAG_WILL;	break;
				case D_TELNET_DO:	flag |= TFLAG_DO;	break;
				case D_TELNET_DONT:	flag |= TFLAG_DONT;	break;
				case D_TELNET_WONT:	flag |= TFLAG_WONT;	break;
				case D_TELNET_BRK:	flag |= TFLAG_SENDBRK;	break;
			}
			continue;
		}
		
		if ( flag & TFLAG_DONT ) 
		{
			flag &= ~TFLAG_DONT;
#ifdef	SUPPORT_LF_XCHG
			if ( ch == D_TELNET_BINARY ) 
			{
				flag &= ~TFLAG_TXBIN;
				if ( (flag & TFLAG_WILLBIN) ) 
				{
					flag &= ~TFLAG_WILLBIN;
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_WONT;
					*ptr_net++ = ch;
				}
			}
#endif
			continue;
		}
		if ( flag & TFLAG_WONT ) 
		{	/* do nothing */
			flag &= ~TFLAG_WONT;
#ifdef	SUPPORT_LF_XCHG
			if ( ch == D_TELNET_BINARY ) 
			{
				flag &= ~TFLAG_RXBIN;
				if ( (flag & TFLAG_DOBIN) ) 
				{
					flag &= ~TFLAG_DOBIN;
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_DONT;
					*ptr_net++ = ch;
				}
			}
#endif
			continue;
		}
		if ( flag & TFLAG_DO ) 
		{
			flag &= ~TFLAG_DO;
			if ( ch == D_TELNET_BINARY ) 
			{
				flag |= TFLAG_TXBIN;
				if ( (flag & TFLAG_WILLBIN) == 0 ) 
				{
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_WILL;
					*ptr_net++ = ch;
				}
			}
			else if ( ch == D_TELNET_ECHO ) 
			{
				if ( (flag & TFLAG_WILLECHO) == 0 ) 
				{
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_WONT;
					*ptr_net++ = ch;
				}
			}
			else if ( ch != D_TELNET_SGA && ch != D_TELNET_TERM ) 
			{
				*ptr_net++ = D_TELNET_IAC;
				*ptr_net++ = D_TELNET_WONT;
				*ptr_net++ = ch;
			}
			continue;
		}
		if ( flag & TFLAG_WILL ) 
		{
			flag &= ~TFLAG_WILL;
			if ( ch == D_TELNET_BINARY ) 
			{
				flag |= TFLAG_RXBIN;
				if ( (flag & TFLAG_DOBIN) == 0 ) 
				{
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_DO;
					*ptr_net++ = ch;
				}
			}
			else if ( ch == D_TELNET_ECHO ) 
			{			
				if ( (flag & TFLAG_DOECHO) == 0 ) 
				{
					/* yes remote can do echo */
					*ptr_net++ = D_TELNET_IAC;
					*ptr_net++ = D_TELNET_DO;
					*ptr_net++ = ch;
				}
			}
			else if ( ch != D_TELNET_SGA && ch != D_TELNET_TERM ) 
			{
				*ptr_net++ = D_TELNET_IAC;
				*ptr_net++ = D_TELNET_DONT;
				*ptr_net++ = ch;
			}
			continue;
		}
		if ( flag & TFLAG_SB ) {	/* just answer when ask term */
			if ( flag & TFLAG_SB3 ) {
				if ( ch == D_TELNET_SE ) {
					if ( flag & TFLAG_WILLTERMIS ) {
						*ptr_net++ = D_TELNET_IAC;
						*ptr_net++ = D_TELNET_SB;
						*ptr_net++ = D_TELNET_TERM;
						*ptr_net++ = D_TELNET_IS;
						ptr_net = (u_char *)stpcpy((char * __restrict__)ptr_net, termcap); 
						*ptr_net++ = D_TELNET_IAC;
						*ptr_net++ = D_TELNET_SE;
					}
					flag &= ~(TFLAG_SB | TFLAG_SB1 | TFLAG_SB2 |
					     TFLAG_SB3 | TFLAG_WILLTERMIS);
				}
				else
					flag &= ~TFLAG_SB3;
			}
			else if ( flag & TFLAG_SB2 ) {
				if ( ch == D_TELNET_IAC )
					flag |= TFLAG_SB3;
			}
			else if ( flag & TFLAG_SB1 ) {
				if ( ch == D_TELNET_SEND )
					flag |= TFLAG_WILLTERMIS;
				flag |= TFLAG_SB2;
			}
			else if ( ch == D_TELNET_TERM )
				flag |= TFLAG_SB1;
			else
				flag |= TFLAG_SB2;	/* just skip all */
		}
	}
	
	if ( (flag & TFLAG_ENTER) && (oldlen < TELNET_BUF_LEN) ) 
	{
		if ( (flag & TFLAG_CRLF2LF) )
			*ptr_port++ = 13;
		flag &= ~TFLAG_ENTER;
	}
	*telflag = flag;
	if ( (i = (int)(ptr_net - target_net)) != 0 )
		send(fd_net, (char *)target_net, i, 0);
	return((int)(ptr_port - target));
}


void	telnet_checkbreak(int port, int fd_net)
{
	u_char	tmp[4];
	int n;
	
	n = sio_notify_error(port);
	if (n & 0x10)
	{
		tmp[0] = D_TELNET_IAC;
		tmp[1] = D_TELNET_BRK;
		send(fd_net, tmp, 2, 0);
	}
}
