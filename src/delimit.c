/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : delimit.c                                                  */
/*****************************************************************************/
#ifndef _DELIMIT_C
#define _DELIMIT_C
#endif

#define TRACE(msg)	//{printf("%s(%d) ", __FUNCTION__, __LINE__); printf msg;}

#include <stdio.h>
#include <stdlib.h>
#include <common.h>
#include <platform.h>
#ifdef LINUX
#include <arpa/inet.h>
#include <sio.h>
#elif defined(ZEPHYR)
#include <posix/sys/socket.h>
#include <posix/arpa/inet.h>
#include <sio/mx_sio.h>
#endif
#include <header.h>
#include <portd.h>
#include <delimit.h>
#include "aspp.h"

#define DK_FORCE_TX_SIZE  DK_BUFFER_SIZE_S2E  /* force transmit if buffered data size exceeds this */

#define min(a,b)  (((a) < (b)) ? (a) : (b))

static fdkparam_t Gdktab=NULL;

int delimiter_init(int port, int has_delimiter, int has_buffering)
{
	fdkparam_t	dp;
	int  tout=0, mode, packlen;
	unsigned char ch1, ch2;
	unsigned int flag=0;
//   	int shmid;
//   	key_t key;

	Gdktab = (fdkparam_t) malloc( sizeof(dkparam) );
	if( Gdktab == (fdkparam_t) NULL )
	{
		printf("Memory not enough.\n");
		exit(EXIT_FAILURE);
	}
	dp = (fdkparam_t) Gdktab;
	memset(dp, 0, sizeof(dkparam));

	dp->port = port;
	dp->s2e_len = dp->e2s_len = 0;
	dp->s2e_rndx = dp->s2e_wndx = dp->s2e_cndx = 0;
	dp->e2s_rndx = dp->e2s_wndx = 0;
	dp->flag = 0;
	dp->sio_read = sio_read;
	dp->sio_write = sio_write;
	dp->s2e_size = DK_BUFFER_SIZE_S2E;

	dp->s2e_buf = malloc(dp->s2e_size);
	if ((dp->s2e_buf == NULL) && (dp->s2e_size > DK_BUFFER_SIZE_S2E))
	{
		printf("No memory for port %d buffering!\n", port);
		dp->s2e_size = DK_BUFFER_SIZE_S2E;
		exit(EXIT_FAILURE);
		//dp->s2e_buf = malloc(dp->s2e_size);
	}

	dp->del_read = delimiter_read_1;
	dp->del_write = delimiter_write_1;

	if (has_delimiter)
	{
		Scf_getDataPacking(port, (int *) &flag, (INT8U *) &ch1, (INT8U *) &ch2, (INT16U *) &tout, &mode, &packlen);

		dp->deli_char1 = ch1;
		dp->deli_char2 = ch2;

		dp->force_tx_waitms = tout;
		dp->packlen = packlen;
		dp->s2e_sentlen = packlen;

		if( flag & 1 )
			dp->flag |= DK_DO_DELI_CH1;

		if( flag & 2 )
			dp->flag |= DK_DO_DELI_CH2;

		if( dp->force_tx_waitms )
			dp->flag |= DK_DO_FORCE_TX;

		dp->deli_process = mode;


	    /* we don't care delimiters if packlen is set to > 0 */
	    if (dp->packlen >0)
	    {
	        dp->flag &= ~(DK_DO_DELI_CH1 | DK_DO_DELI_CH2);
	    }
	}

	if ((((dp->flag&DK_DO_DELIMITER)==0) && (dp->packlen==0)) ||
         (!(dp->flag&DK_DO_DELI_CH1) && (dp->flag&DK_DO_DELI_CH2) && !(dp->flag&DK_DO_FORCE_TX)))
	{
		dp->del_read = delimiter_read_1;
		dp->del_write = delimiter_write_1;
	}
	else
	{
		dp->del_read = delimiter_read_2;
		dp->del_write = delimiter_write_1;
	}



	return 1;
}

void delimiter_exit(int port)
{
	if( Gdktab == (fdkparam_t) NULL )
		return;

	if (Gdktab->s2e_buf != NULL)
    {
		free(Gdktab->s2e_buf);
        Gdktab->s2e_buf = NULL;
    }
	if (Gdktab)
		free(Gdktab);
	
	Gdktab = (fdkparam_t) NULL;
}

void delimiter_start(int port, int fd_port, int max_conns, int fd_net[], int data_sent[],
                     int (*sendfunc)(int, int, char *, int), int (*recvfunc)(int, int, char *, int),
                     int raw_tcp)
{
	fdkparam_t	dp;
//	int iftype;

	dp = (fdkparam_t) Gdktab;
	dp->fd_port = fd_port;
	dp->max_conns = max_conns;
	dp->fd_net = fd_net;
	dp->data_sent = data_sent;
	if( dp->flag & DK_RUN )
		return;

	dp->s2e_cflag = 0;
	dp->s2e_ccnt = 0;
	dp->s2e_cndx = 0;
	dp->s2e_rndx = 0;
	dp->s2e_wndx = 0;
	dp->s2e_len = 0;
	//dp->s2e_sentlen = 0; //vince
	dp->deli_stat = 0;

	dp->sent_to_tcp_len = 0;

	dp->e2s_len = 0;
	dp->e2s_rndx = dp->e2s_wndx = 0;
	dp->sendfunc = sendfunc;
	dp->recvfunc = recvfunc;
	dp->lasttime = 0L;
	dp->flag |= DK_RUN;
	if( raw_tcp )
		dp->flag |= DK_MODE_RAW_TCP;
	else
		dp->flag &= ~DK_MODE_RAW_TCP;
}

void delimiter_stop(int port)
{
	fdkparam_t	dp;

	dp = (fdkparam_t) Gdktab;
	if( (dp->flag & DK_RUN) == 0 )
		return;

	dp->flag &= ~(DK_RUN | DK_MODE_RAW_TCP);
}

int	delimiter_poll(int port)
{
	fdkparam_t	dp = (fdkparam_t) Gdktab;

	delimiter_write(port);

	if( ((dp->flag & DK_RUN) == 0) ||
	    (dp->s2e_len == 0) )
	{
		return 0;
	}

	return dp->s2e_len;
}

int delimiter_read(int port, int send_buffered_data)
{
	fdkparam_t	dp;
	dp = (fdkparam_t) Gdktab;

	if( (dp->flag & DK_RUN) == 0 )
	{
		return -1;
	}

	return dp->del_read(port, send_buffered_data);
}

int delimiter_read_1(int port, int send_buffered_data)
{
	int		n, max;
	fdkparam_t	dp;
	unsigned char	*p;
	dp = (fdkparam_t) Gdktab;
	if (dp->s2e_len == DK_BUFFER_SIZE_S2E)
		return dp->s2e_len;
	p = dp->s2e_buf + dp->s2e_wndx;

	if ( dp->s2e_wndx >= dp->s2e_rndx )
	{
		max = dp->s2e_size - dp->s2e_wndx;
	}
	else
	{
		max = dp->s2e_rndx - dp->s2e_wndx;
	}

	if ( (n = dp->sio_read(port, (char *)p, max)) > 0 )
	{
		dp->lasttime = sys_clock_ms();
		dp->s2e_len += n;
		dp->s2e_wndx += n;

		if ( dp->s2e_wndx == dp->s2e_size )
		{
			dp->s2e_wndx = 0;
		}
		dp->s2e_cflag = 1;
	}

	if ( dp->s2e_len == 0 )
	{
		return -1;
	}
	delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
	return dp->s2e_len;
}

int delimiter_read_2(int port, int send_buffered_data)
{
	int		n, max, f;
	fdkparam_t	dp;
	unsigned char	*p;
	unsigned char	c, d;
	dp = (fdkparam_t) Gdktab;	

	if (dp->s2e_len == DK_BUFFER_SIZE_S2E)
		return dp->s2e_len;

	p = dp->s2e_buf + dp->s2e_wndx;
 
	if ( dp->s2e_wndx >= dp->s2e_rndx )
	{
		max = dp->s2e_size - dp->s2e_wndx;
	}
	else
	{
		max = dp->s2e_rndx - dp->s2e_wndx;
	}

	if ( (n = dp->sio_read(port, (char *)p, max)) > 0 )
	{	
		dp->lasttime = sys_clock_ms();
		dp->s2e_len += n;
		dp->s2e_wndx += n;

		if ( dp->s2e_wndx == dp->s2e_size )
		{
			dp->s2e_wndx = 0;
		}
		dp->s2e_cflag = 1;
	}

    /* move to after delimited check
    if (dp->s2e_len == DK_BUFFER_SIZE_S2E)
    {
	    delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
    }
    */

    /* if length of buffer >= sentlen, we send data. */
    /* sentlen is determined by packlen or delimiter+1, +2. */

	if ((dp->s2e_sentlen > 0 && dp->s2e_len >= dp->s2e_sentlen))
    {
        delimiter_send(port, dp->s2e_sentlen, 0);
        return -1;
    }

    if (dp->s2e_len == DK_BUFFER_SIZE_S2E)
    {
		if (dp->deli_process == DEL_STRIP)		/* strip delimiter */
		{
			f = dp->flag & (DK_DO_DELI_CH1 | DK_DO_DELI_CH2);
		    if (f == DK_DO_DELI_CH1 && dp->s2e_buf[DK_BUFFER_SIZE_S2E-1] == dp->deli_char1)
		    {
				delimiter_send(port, DK_BUFFER_SIZE_S2E-1, 1);
		    }
		    else if (f == (DK_DO_DELI_CH1 | DK_DO_DELI_CH2) && dp->s2e_buf[DK_BUFFER_SIZE_S2E-1] == dp->deli_char2 && dp->s2e_buf[DK_BUFFER_SIZE_S2E-2] == dp->deli_char1)
		    {
				delimiter_send(port, DK_BUFFER_SIZE_S2E-2, 2);
		    }
			else
			{
				delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
			}
		}
		else
		{
	    	delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
		}
	    return -1;
    }

    if (dp->s2e_sentlen != 0 && dp->packlen == 0)
    {
        return -1;
    }

	if ( dp->s2e_len == 0 )
	{
		return -1;
	}

	f = dp->flag & (DK_DO_DELI_CH1 | DK_DO_DELI_CH2);

	if( (dp->flag & DK_DO_DELIMITER) == 0 )
	{
		if (dp->s2e_sentlen == 0) {
			delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
		}
	}
	else if( f & DK_DO_DELI_CH1 )
	{
		do{
			d = dp->deli_stat;
			n = dp->s2e_cndx;
			while( dp->s2e_cflag && (d != 2) && (dp->s2e_ccnt < DK_FORCE_TX_SIZE) )
			{
				c = dp->s2e_buf[n];
				if( d == 0 )
				{
					if( c == dp->deli_char1 )
					{
						d = 1;
						if( f == DK_DO_DELI_CH1 )
						{
							d = 2;
						}
					}
				}
				else
				{
					if( c == dp->deli_char2 )
					{
						d = 2;
					}
					else if ( c == dp->deli_char1 )
					{
						d = 1;
					}
					else
					{
						d = 0;
					}
				}
				n++;
				if ( n == dp->s2e_size )
				{
					n = 0;
				}
				dp->s2e_ccnt++;
				if(n == dp->s2e_wndx)
					dp->s2e_cflag = 0;  /* all data are checked */
			}

			dp->deli_stat = d;

			if( d == 2 )
			{
				if( n > dp->s2e_rndx )
					max = n - dp->s2e_rndx;
				else
					max = DK_BUFFER_SIZE_S2E - dp->s2e_rndx + n;

				if (dp->deli_process == DEL_PLUSONE)	/* delimiter+1 */
				{
			        if (dp->s2e_len >= max+1 || dp->s2e_len >= DK_FORCE_TX_SIZE)
			        {
                        if (delimiter_send(port, max+1, 0) < 0)
                        	break;	/* avoid infinite loop when network is jammed */
			        }
			        else
			        {
    			        dp->s2e_sentlen = dp->s2e_len + 1;
			            dp->s2e_cndx = n;
			            break;
			        }
				}
				else if (dp->deli_process == DEL_PLUSTWO)	/* delimiter+2 */
				{
			        if (dp->s2e_len >= max+2 || dp->s2e_len >= DK_FORCE_TX_SIZE)
			        {
                        if (delimiter_send(port, max+2, 0) < 0)
                        	break;	/* avoid infinite loop when network is jammed */
			        }
			        else
			        {
			            if (dp->s2e_len == max+1)
    			            dp->s2e_sentlen = dp->s2e_len + 1;
    			        else {
	    			        dp->s2e_sentlen = dp->s2e_len + 2;
    			        	}
						if(dp->s2e_sentlen > DK_FORCE_TX_SIZE)
							dp->s2e_sentlen = DK_FORCE_TX_SIZE;
	    			    dp->s2e_cndx = n;
			            break;
			        }
				}
				else if (dp->deli_process == DEL_STRIP)		/* strip delimiter */
				{
			        if (f == DK_DO_DELI_CH1)
			        {
    			        if (delimiter_send(port, max-1, 1) < 0)
    			        	break;	/* avoid infinite loop when network is jammed */
			        }
			        else
			        {
    			        if (delimiter_send(port, max-2, 2) < 0)
    			        	break;	/* avoid infinite loop when network is jammed */
			        }
				}
				else if (dp->deli_process == DEL_NOTHING)	/* do nothing */
				{
					if (delimiter_send(port, max, 0) < 0)
						break;	/* avoid infinite loop when network is jammed */
				}
			}
			else
			{
#if 0	/* use deli_stat to replace this */
				if ( d == 1 )
				{
					if ( n == 0 )
						n = DK_BUFFER_SIZE_S2E - 1;
					else
						n--;
				}
#endif
				dp->s2e_cndx = n;
				/* The length of buffered data may be greater than DK_FORCE_TX_SIZE,
				   each time we only deal with DK_FORCE_TX_SIZE */
				TRACE(("d=%d,cndx=%d,ccnt=%d\r\n", d, dp->s2e_cndx, dp->s2e_ccnt));
				if(dp->s2e_ccnt >= DK_FORCE_TX_SIZE)
				{
					/* no delimiter condition meet in DK_FORCE_TX_SIZE */
					if(delimiter_send(port, DK_FORCE_TX_SIZE, 0) < 0)
						break;	/* avoid infinite loop when network is jammed */
					TRACE(("len=%d,wndx=%d,rndx= %d\r\n", dp->s2e_len, dp->s2e_wndx, dp->s2e_rndx));
				}
			}
		}while(d==2);
	}
	else if(dp->flag & DK_DO_FORCE_TX)
	{
		int send_len = 0;
		TRACE(("send_buffer_date = %d, lasttime = %lu, now = %lu\r\n", send_buffered_data, dp->lasttime, sys_clock_ms()));
		if(send_buffered_data)
		{
			if((sys_clock_ms() - dp->lasttime) > dp->force_tx_waitms)
				send_len = min(dp->s2e_len, DK_FORCE_TX_SIZE);

			TRACE(("s2e_len = %d\r\n", dp->s2e_len));
			/*
			 * If off_buf have data, marge data first.
			 *
			 * Add by Sean Hsu - 07.23.2010
			 */
			//if(dp->s2e_len > 0)
			//	send_len = 0;
		}
		if(!send_len
			&& ( (dp->s2e_len - dp->sent_to_tcp_len) >= DK_FORCE_TX_SIZE) )
			send_len = DK_FORCE_TX_SIZE;
		if(send_len)
		{
			if (dp->s2e_len == 0)
				return -1;	/* all data sent */
			delimiter_send(port, send_len, 0);
			return (dp->s2e_len);
		}
	}

    if (dp->s2e_len == DK_BUFFER_SIZE_S2E)
    {
	    delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
    }

    return -1;
}

int delimiter_write(int port)
{
	fdkparam_t	dp;
	dp = (fdkparam_t) Gdktab;
	if (dp->e2s_len == 0)
		return 0;

	return dp->del_write(port);
}
int	sendback(int port, int fd_net, char *buf, int len)
{
	fdkparam_t	dp;
	unsigned char	*p;
	int n, n2/*, ofree*/;

	dp = (fdkparam_t) Gdktab;
	p = dp->e2s_buf + dp->e2s_rndx;
	n = DK_BUFFER_SIZE_E2S - dp->e2s_rndx;

	if (n > dp->e2s_len)
            n = dp->e2s_len;

	if (n > 0)
	{
	    if ((n2 = send(fd_net, (char*)p, n, 0)) > 0)
//	    if ((n2 = dp->sio_write(port, (char*)p, n)) > 0)
	    {
		    dp->e2s_len -= n2;

		    if (dp->e2s_len == 0)
		    {
			    dp->e2s_rndx = 0;
			    dp->e2s_wndx = 0;
		    }
		    else
		    {
		        dp->e2s_rndx += n2;
			    if (dp->e2s_rndx == DK_BUFFER_SIZE_E2S)
			    {
				    dp->e2s_rndx = 0;
			    }
		    }
    	}
    }
	return dp->e2s_len;
}

int delimiter_write_1(int port)
{
	fdkparam_t	dp;
	unsigned char	*p;
	int n, n2, ofree;

	dp = (fdkparam_t) Gdktab;
	p = dp->e2s_buf + dp->e2s_rndx;
	n = DK_BUFFER_SIZE_E2S - dp->e2s_rndx;

	if (n > dp->e2s_len)
            n = dp->e2s_len;

	ofree = sio_ofree(port);
	if (n > ofree)
            n = ofree;

	if (n > 0)
	{
	    if ((n2 = dp->sio_write(port, (char*)p, n)) > 0)
	    {
		    dp->e2s_len -= n2;

		    if (dp->e2s_len == 0)
		    {
			    dp->e2s_rndx = 0;
			    dp->e2s_wndx = 0;
		    }
		    else
		    {
		        dp->e2s_rndx += n2;
			    if (dp->e2s_rndx == DK_BUFFER_SIZE_E2S)
			    {
				    dp->e2s_rndx = 0;
			    }
		    }
    	}
    }
	return dp->e2s_len;
}

int delimiter_send(int port, int max, int strip)
{
	fdkparam_t      dp;
	unsigned char   *p;
	int	n, merged = 0;
	dp = (fdkparam_t) Gdktab;

	if(dp->sent_to_tcp_len > 0)
		return 0;	/* data was sent to TCP buffer but TCP buffer is not yet cleared, don't process */

	if (dp->s2e_len == 0)
		return 0;

	if(max > (dp->s2e_len - strip))	/* max = 0 when Strip Delimiter is satisfied */
	{
		max = dp->s2e_len - strip;
	}

	if((n = dp->s2e_size - dp->s2e_rndx) >= max)
	{
		p = dp->s2e_buf + dp->s2e_rndx;
	}
	else  /* merge tail and head buffer data */
	{
		p = malloc(max);
		memcpy(p, dp->s2e_buf + dp->s2e_rndx, n);
		memcpy(p + n, dp->s2e_buf, max - n);
		merged = 1;
	}

	if(max > 0)
	{
		n = dp->sendfunc(dp->port, dp->fd_net[0], (char *)p, max);
		if(n == 0)
		{
			n = -1;
		}
	}
	else
		n = 0;

	if ((n) >= 0)	/* n = 0 in case that max = 0 */
	{
		dp->deli_stat = 0;
		dp->s2e_ccnt = 0; /* restart next delimiter finding procedure */
		dp->s2e_sentlen = dp->packlen;

		dp->s2e_len -= (n+strip);
		if (dp->s2e_len == 0)
		{
			dp->s2e_rndx = 0;
			dp->s2e_wndx = 0;
		}
		else
		{
			dp->s2e_rndx += (n+strip);
			if(dp->s2e_rndx >= dp->s2e_size)
			{
				dp->s2e_rndx -= dp->s2e_size;
			}
		}
		if((dp->s2e_cndx = dp->s2e_rndx) == dp->s2e_wndx)
			dp->s2e_cflag = 0;  /* all data are checked. this is called when delimiter+1/2 */
		
	}

	if(merged)
		free(p);

	if (n == -1)
		return -1;

	return dp->s2e_len;
}

int delimiter_recv(int port, int fd_net)
{
	int		n, max;
	fdkparam_t	dp;
	unsigned char	*p;
	int state;

	dp = (fdkparam_t) Gdktab;
	if ((dp->flag & DK_RUN) == 0)
		return 0;

	/*
	 * Connection might has been reset by peer.
	 * If don't check this, program will block because e2s_len is full. 
	 */
	state = tcp_state(fd_net);
	if (state == 0)	// invalid
	{
		return -1;
	}
#ifdef UART_BURN
	if (dp->e2s_len >= DK_BUFFER_SIZE_E2S)
	{
		delimiter_write(port);
		return dp->e2s_len;
	}
#endif	
	p = dp->e2s_buf + dp->e2s_wndx;

	if (dp->e2s_wndx >= dp->e2s_rndx)
	{
		max = DK_BUFFER_SIZE_E2S - dp->e2s_wndx;
	}
	else
	{
		max = dp->e2s_rndx - dp->e2s_wndx;
	}

	if ((n = dp->recvfunc(dp->port, fd_net, (char *)p, max)) > 0)
	{
	    dp->e2s_len += n;
	    dp->e2s_wndx += n;
	    if (dp->e2s_wndx == DK_BUFFER_SIZE_E2S)
	    {
	        dp->e2s_wndx = 0;
	    }
	}
    //else if (n < 0)
       else if (n <= 0)
	    return -1;  /* close connection */

	if (dp->e2s_len == 0)
	    return 0;
#ifdef UART_BURN
	delimiter_write(port);
#else
	sendback(dp->port, dp->fd_net[0], (char *)p, max);
//	dp->sendfunc(dp->port, dp->fd_net[0], (char *)p, max);
#endif
	return dp->e2s_len;
}

void delimiter_flush(int port, int mode)
{
	fdkparam_t	dp;
	dp = (fdkparam_t) Gdktab;

TRACE(("port=%d,mode=%x\n", port, mode));
	/* mode = 0 : FLUSH_RX */
	/*      = 1 : FLUSH_TX */
	/*      = 2 : FLUSH_ALL */
	if( mode != 1 )
	{
		dp->s2e_cflag = dp->s2e_ccnt = dp->deli_stat = 0;
		dp->s2e_rndx = dp->s2e_wndx = dp->s2e_cndx = 0;
		dp->s2e_len = 0;
		dp->s2e_sentlen = dp->packlen; //vince
		dp->sent_to_tcp_len = 0;
	}

	if( mode )
	{
		dp->e2s_rndx = dp->e2s_wndx = 0;
		dp->e2s_len = 0;
	}
}

int delimiter_s2e_len(int port)
{
	fdkparam_t	dp;
	dp = (fdkparam_t) Gdktab;
	return dp->s2e_len;
}

int delimiter_e2s_len(int port)
{
	fdkparam_t	dp;
	dp = (fdkparam_t) Gdktab;
	return dp->e2s_len;
}
