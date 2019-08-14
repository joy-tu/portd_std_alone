/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*****************************************************************************/
/* Company      : MOXA Technologies Co., Ltd.                                */
/* Filename     : delimit.c                                                  */
/* Description  :                                                            */
/* Product      : Secured Serial Device Server                               */
/* Programmer   : Shinhsy Shaw                                               */
/* Date         : 2003-07-22                                                 */
/*****************************************************************************/
#ifndef _DELIMIT_C
#define _DELIMIT_C
#endif

#define TRACE(msg)	//{printf("%s(%d) ", __FUNCTION__, __LINE__); printf msg;}

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <config.h>
#include <portd.h>
#include <delimit.h>
#include <sio.h>
#include <datalog.h>
#include "aspp.h"
#include <sys/shm.h>
#include <sys/sem.h>
#include "../message.h"

#define DK_FORCE_TX_SIZE  1024  /* force transmit if buffered data size exceeds this */

#define min(a,b)  (((a) < (b)) ? (a) : (b))

int Scf_getLocallogEnable(int port);

static fdkparam_t Gdktab=NULL;

void port_buffering_start(int port)
{
	Gdktab->flag |= DK_DO_BUFFERING;
}

int port_buffering_active(int port)
{
	return (Gdktab->flag & DK_CAN_BUFFER) ? 1 : 0;
}

void port_buffering_check_restart(int port)
{
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;
	/* DK_RESET_BUFFER was set by portd_restart() */
	if((dp->flag & (DK_DO_BUFFERING | DK_RESET_BUFFER)) == (DK_DO_BUFFERING | DK_RESET_BUFFER))
	{
TRACE(("port=%d\n", port));

		dp->s2e_len = 0;
		dp->s2e_cflag = 0;
		dp->s2e_ccnt = 0;
		dp->s2e_cndx = 0;
		dp->s2e_rndx = 0;
		dp->s2e_wndx = 0;
		dp->s2e_sentlen = 0;
		dp->deli_stat = 0;
		dp->sent_to_tcp_len = 0;
	}
	dp->flag &= ~(unsigned short)DK_RESET_BUFFER;
}

void port_buffering_reset(int port)
{
	/* When the opmode didn't initiate delimiter,
		this global variable will be NULLL.*/
	if(Gdktab != NULL)
		Gdktab->flag |= DK_RESET_BUFFER;
}

void port_buffering_flush(int port)
{
	port_buffering_reset(port);
	port_buffering_check_restart(port);
}

void port_buffering_reset_sent_tcp_len(int port)
{
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;
	dp->sent_to_tcp_len = 0;
}

/*
 When port buffering is enabled, the data in TCP buffer may be reset when connection
 is closed due to network timeout, thus cause buffered data loss.
 So we must make sure the data in TCP buffer was sent to the peer before we send
 new data to TCP buffer. */
int port_buffering_tcp_is_clear(int port, int do_update)
{
	fdkparam_t	dp;
	int i, tcp_is_clear, clear_map, check_map, m, has_connection, fd, oqueue, can_update = 0;

	dp = (fdkparam_t)Gdktab;

	if(!(dp->flag & DK_MODE_RAW_TCP))
		return 1;

	if(dp->sent_to_tcp_len == 0)
		return 1;

	if(dp->flag & DK_IGNORE_JAM_IP)     /* ignore Jam Ip enabled */
		tcp_is_clear = 0;
	else
		tcp_is_clear = 1;

	clear_map = check_map = 0;
	has_connection = 0;

	for(i=0; i<dp->max_conns; i++)
	{
		if((fd = dp->fd_net[i]) > 0)
		{
			/* Albert.20120103: If old connection with TCP buffer queued was forcely disconnected via NPPI timeout,
			 * and then new connection dp->fd_net[i] with initial tcp_oqueue(fd)=0 will make us incorrectly tell that
			 * tcp_is_clear and update delimiter buffer index. So I check (dp->data_sent[i] > 0) and add check_map.
			 * (dp->data_sent[i] > 0) means the data was ever sent to this TCP connection. (dp->data_sent[i] == 0) means
			 * it is not new joined connection and its initial tcp_oqueue(fd)=0 will not affect value of clear_map.
			 * The new joined connection still affect the value of tcp_is_clear becuase we need it to be true
			 * to send buffer data to this new connection.
			 */
			if(tcp_state(fd) == TCP_ESTABLISHED)
			{
				has_connection = 1;
				oqueue = tcp_oqueue(fd);

				if (dp->data_sent[i] > 0)	/* new joined connection is not included in the check_map */
				{
					m = (1 << i);
					check_map |= m;
					if (oqueue == 0)
						clear_map |= m;
				}

				if(dp->flag & DK_IGNORE_JAM_IP)     /* ignore Jam Ip enabled */
				{
					if(oqueue == 0)
					{
						tcp_is_clear = 1;
						TRACE(("tcp_oqueue[%d][%d]=%d sent=%d\r\n", port,i,oqueue,dp->data_sent[i]));
					}
				}
				else
				{
					// Modified to solve when tcp_state = TCP_STATE_CONNECTED, but tcp_oqueue = ENOTCONN,
					// the data will be lost, at 12-01-2009, by Frank Ho, in Ver1.7.59
					if(oqueue > 0/* || tcp_oqueue(fd) == -3*/)
					{
						tcp_is_clear = 0;
					}
					// ... end ...
					else
						TRACE(("tcp_oqueue[%d][%d]=%d sent=%d do_update=%d t=%lu\r\n", port,i,oqueue,dp->data_sent[i],do_update,sys_clock_ms()));
				}
			}
		}
	}

	if(!has_connection)
	{
		//TRACE(("port %d has no connection,max_conns=%d,fd_net[0]=%d,state=%d\r\n", port, dp->max_conns, dp->fd_net[0], tcp_state(dp->fd_net[0])));
		tcp_is_clear = 0;
	}

	if(dp->flag & DK_IGNORE_JAM_IP)     /* ignore Jam Ip enabled */
		can_update = (clear_map & check_map);  /* update delimiter index only when one of non-new connections is cleared */
	else
		can_update = (check_map != 0) & (clear_map == check_map); /* update delimiter index when all non-new connections are clear */

	if ((dp->sent_to_tcp_len > 0) && (check_map == 0))
	{
		/* old connections did not send out dp->sent_to_tcp_len and no old connections exist.
		 * we must reset dp->sent_to_tcp_len so that a call to delimiter_send() can have a chance
		 * to send data to new connection. */
		dp->sent_to_tcp_len = 0;
		TRACE(("*** dp->sent_to_tcp_len=%d chk_m=%x clr_m=%x ***\n", dp->sent_to_tcp_len, check_map, clear_map));
	}

	if(tcp_is_clear && can_update)
	{
		dp->s2e_len -= dp->sent_to_tcp_len;

		if(dp->s2e_len == 0)
		{
			dp->s2e_rndx = 0;
			dp->s2e_wndx = 0;
		}
		else
		{
			dp->s2e_rndx += dp->sent_to_tcp_len;
			dp->s2e_rndx %= dp->s2e_size;
			/*
			if ( dp->s2e_rndx >= dp->s2e_size )
			{
				dp->s2e_rndx -= dp->s2e_size;
			}*/
		}
		if((dp->s2e_cndx = dp->s2e_rndx) == dp->s2e_wndx)
			dp->s2e_cflag = 0;  /* all data are checked. this is called when delimiter+1/2 */

		dp->sent_to_tcp_len = 0;
	}

	if (has_connection)
		TRACE(("port%d [ret=%d] len=%d,wndx=%d,rndx=%d,cndx=%d,cflag=%d,sent_tcp=%d,data_sent[0]=%d,can_update=%d,chk_m=%x,clr_m=%x,update=%d\r\n", port, tcp_is_clear, dp->s2e_len, dp->s2e_wndx, dp->s2e_rndx, dp->s2e_cndx, dp->s2e_cflag, dp->sent_to_tcp_len, dp->data_sent[0], can_update, check_map, clear_map, do_update));

	return tcp_is_clear;
}

#if 0
void print_delimiter_state(int port)
{
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;
	TRACE(("port%d len=%d,wndx=%d,rndx=%d,cndx=%d,cflag=%d,sent_tcp=%d\r\n", port, dp->s2e_len, dp->s2e_wndx, dp->s2e_rndx, dp->s2e_cndx, dp->s2e_cflag, dp->sent_to_tcp_len));
}
#endif

int delimiter_check_buffered(int port)
{
	fdkparam_t dp = (fdkparam_t)Gdktab;

	if(dp->flag & DK_DO_BUFFERING)
	{
// ... Frank Ho remove for Force Transmit + Port Buffering SD card bug, Ver1.7.59 ...
#if 0
		if(dp->flag & DK_DO_FORCE_TX)
		{
			/* avoid busy FD_ISSET(fd_net, &wfds) in aspp.c/raw_tcp.c */
			if(((sys_clock_ms() - dp->lasttime) <= dp->force_tx_waitms) &&
			        (dp->s2e_len < DK_FORCE_TX_SIZE))
			{
//dbg_sio_printf("[%d] %s, dp->s2e_len: %d\r\n", __LINE__, __func__, dp->s2e_len);
				return 0;
			}
		}
#endif
// ... end ...
		/* cflag: data is buffered */
		if(dp->s2e_cflag && (dp->s2e_len > 0))
		{
			/* if packlen is not 0, we must also pack data */
			if(!dp->packlen || (dp->packlen && (dp->s2e_len >= dp->packlen)))
			{
				/* Albert.20120102: Send buffered data only when TCP is clear.
				 * If connection was built and Ethernet cable is disconnected,
				 * we will busy calling "delimiter_read(port, 1)" and have no chance to close connection
				 * via NPPI_NOTIFY timeout mechanism.
				 */
				if (port_buffering_tcp_is_clear(port, -1))
					return 1;
			}
		}
	}
	return 0;
}

void buffering_offline_read(int port)
{
	int	n;
	unsigned char	*p;
	unsigned int max;
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;

	if((dp->flag & DK_DO_BUFFERING))
	{
		if(dp->s2e_len >= dp->s2e_size)
		{
			// don't call sio_flush(), it will cause RTS always high since sio buffer is always clear.
			//sio_flush(port, SIO_FLUSH_RX); /* so that fd_port rfd will not be infinitely selected */
			return;
		}

		p = dp->s2e_buf + dp->s2e_wndx;
		if(dp->s2e_wndx >= dp->s2e_rndx)
		{
			max = dp->s2e_size - dp->s2e_wndx;
		}
		else
		{
			max = dp->s2e_rndx - dp->s2e_wndx;
		}

		if((n = dp->sio_read(port, (char *)p, (int)max)) > 0)
		{
			// ... Frank Ho modified for last time update, Ver1.7.59 ...
			dp->lasttime = sys_clock_ms();
			// ... end ...
			dp->s2e_len += n;
			dp->s2e_wndx += n;
			if(dp->s2e_wndx == dp->s2e_size)
			{
				dp->s2e_wndx = 0;
			}
			dp->s2e_cflag = 1; /* data needs to be checked (data is buffered) */
			TRACE(("port=%d,max=%d,n=%d,len=%d,wndx=%d,rndx=%d,cndx=%d,cflag=%d\r\n", port, max, n, dp->s2e_len, dp->s2e_wndx, dp->s2e_rndx, dp->s2e_cndx, dp->s2e_cflag));
		}
	}
}

/* called by pair.c for buffering without delimiter */
int fd_net_pair = 0;
int buffering_sio_read(int port, char *buf, int len)
{
	int n, do_buffering = 0;
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;

	if(dp->flag & DK_DO_BUFFERING)
	{
		/* cflag: data is buffered in memory, must append data to memory */
		if(!dp->s2e_cflag && (tcp_ofree(fd_net_pair) < len * 2))
			do_buffering = 1;
		if(dp->s2e_cflag || do_buffering)
		{
			buffering_offline_read(port);
			return 0;
		}
	}
	/* connection established and no data buffered in memory or SD,
	   read data from sio port directly */
	n = dp->sio_read(port, (char *)buf, len);
	// ... Frank Ho modified for last time update, Ver1.7.59 ...
	dp->lasttime = sys_clock_ms();
	// ... end ...
	return n;
}

/* called by pair.c for buffering without delimiter */
int buffering_read(int port, char *buf, int len)
{
	int n;
	fdkparam_t	dp;

	dp = (fdkparam_t)Gdktab;

	n = 0;
	/* read buffered data */
	if(dp->s2e_cflag)  /* data buffered in memory */
	{
		if(dp->s2e_len == 0)
			return 0;
#if 0
		if(dp->s2e_len == dp->s2e_size)
			n = dp->s2e_len - dp->s2e_rndx; /* wndx = 0 in this case */
		else
			n = dp->s2e_wndx - dp->s2e_rndx; /* we does't support circular buffering */
#endif
		n = min(dp->s2e_len, len);
		n = min(n, dp->s2e_size - dp->s2e_rndx);
		memcpy(buf, dp->s2e_buf + dp->s2e_rndx, n);
		dp->s2e_rndx += n;
		if(dp->s2e_rndx >= dp->s2e_size)
			dp->s2e_rndx -= dp->s2e_size;
		dp->s2e_len -= n;
		if(dp->s2e_len == 0)
		{
			/* must reset dp->s2e_wndx that (max=size-wndx) != 0 when dp->s2e_len = 0 */
			dp->s2e_rndx = 0;
			dp->s2e_wndx = 0;
			dp->s2e_cflag = 0;
		}
	}

	return n;
}

int delimiter_init(int port, int has_delimiter, int has_buffering)
{
	fdkparam_t	dp;
	int  tout=0, mode, packlen;
	unsigned char ch1, ch2;
	unsigned int flag=0;
    int shmid;
    key_t key;

    /*
     * We don't need shared memory now, because no data log is needed.
     * When one of the processes using the same shared memory died,
     * the other process will exit unexpectedly while using shared memory.
     * If we need to support data log in the future, must fix this issue.
     */
#if 0
	if (Gdktab == NULL)
	{
	    /*
	     * We'll name our shared memory segment
	     * DK_SHM_KEY + port.
	     */
	    key = DK_SHM_KEY + port;

	    /* Create a shared memory segment using the IPC key.  The        */
	    /* size of the segment is a constant.  The specified permissions */
	    /* give everyone read/write access to the shared memory segment. */
	    /* If a shared memory segment already exists for this key,       */
	    /* return an error.                                              */
	    if ((shmid = shmget(key, sizeof(dkparam), 0666)) < 0)
            {
	        if ((shmid = shmget(key, sizeof(dkparam), 0666 | IPC_CREAT | IPC_EXCL )) < 0)
	        {
	            perror("shmget");
	            return 0;
	        }
            }

	    /*
	     * Now we attach the segment to our data space.
	     */
	    if ((Gdktab = (fdkparam_t)shmat(shmid, NULL, 0)) == (fdkparam_t) -1)
	    {
			Gdktab = NULL;
	        perror("shmat");
		return 0;
	    }
    }
#else
	Gdktab = (fdkparam_t) malloc( sizeof(dkparam) );
	if( Gdktab == (fdkparam_t) NULL )
	{
		SHOW_LOG(stderr, port, MSG_ERR, "Memory not enough.\n");
		exit(EXIT_FAILURE);
	}
#endif
	dp = (fdkparam_t) Gdktab;
	memset(dp, 0, sizeof(dkparam));

	dp->port = port;
	dp->s2e_len = dp->e2s_len = 0;
	dp->s2e_rndx = dp->s2e_wndx = dp->s2e_cndx = 0;
	dp->e2s_rndx = dp->e2s_wndx = 0;
	dp->flag = 0;

	if (Scf_getSerialDataLog(port))
	{
		dp->sio_read = log_sio_read;
		dp->sio_write = log_sio_write;
	}
	else
	{
		dp->sio_read = sio_read;
		dp->sio_write = sio_write;
	}

	dp->s2e_size = DK_BUFFER_SIZE_S2E;

	if (has_buffering)
	{
		if(Scf_getPortBuffering(port))
		{
			dp->flag |= DK_CAN_BUFFER;
			dp->s2e_size = ((Scf_getMaxPorts() == 1) ? 20 : 10) * 1024 * 1024;
		}
	}

	dp->s2e_buf = malloc(dp->s2e_size);
	if ((dp->s2e_buf == NULL) && (dp->s2e_size > DK_BUFFER_SIZE_S2E))
	{
		SHOW_LOG(stderr, port, MSG_ERR, "No memory for port %d buffering!\n", port);
		dp->s2e_size = DK_BUFFER_SIZE_S2E;
		exit(EXIT_FAILURE);
		//dp->s2e_buf = malloc(dp->s2e_size);
	}

	if((Scf_getMaxConns(port) > 1) && (Scf_getSkipJamIP(port) == 1))
		dp->flag |= DK_IGNORE_JAM_IP;

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
		dp->del_write = delimiter_write_2;
	}

	dp->iftype = Scf_getIfType(port);
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

	free(Gdktab);
	Gdktab = (fdkparam_t) NULL;
}

void delimiter_start(int port, int fd_port, int max_conns, int fd_net[], int data_sent[],
                     int (*sendfunc)(int, int, char *, int), int (*recvfunc)(int, int, char *, int),
                     int raw_tcp)
{
	fdkparam_t	dp;
	int iftype;

	dp = (fdkparam_t) Gdktab;
	dp->fd_port = fd_port;
	dp->max_conns = max_conns;
	dp->fd_net = fd_net;
	dp->data_sent = data_sent;
	if( dp->flag & DK_RUN )
		return;

	if(dp->flag & DK_DO_BUFFERING)
		;	/* don't reset s2e buffer */
	else
	{
		dp->s2e_cflag = 0;
		dp->s2e_ccnt = 0;
		dp->s2e_cndx = 0;
		dp->s2e_rndx = 0;
		dp->s2e_wndx = 0;
		dp->s2e_len = 0;
		//dp->s2e_sentlen = 0; //vince
		dp->deli_stat = 0;
	}
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

	switch( dp->iftype )
	{
#if defined (w1)
		case 0:	/* RS-232 */
			iftype = RS232_MODE;
			break;
		case 2:	/* RS-485 2-wire */
			iftype = RS485_2WIRE_MODE;
			break;
		default:
			iftype = RS232_MODE;
			break;
#else
		case 0:	/* RS-232 */
			iftype = RS232_MODE;
			break;
		case 1:	/* RS-422 */
			iftype = RS422_MODE;
			break;
		case 2:	/* RS-485 2-wire */
			iftype = RS485_2WIRE_MODE;
			break;
		case 3:	/* RS-485 4-wire */
			iftype = RS485_4WIRE_MODE;
			break;
		default:
			iftype = RS232_MODE;
			break;
#endif
	}
	sio_setiftype(port, iftype);
}

void delimiter_stop(int port)
{
	fdkparam_t	dp;

	dp = (fdkparam_t) Gdktab;
	if( (dp->flag & DK_RUN) == 0 )
		return;

	dp->flag &= ~(DK_RUN | DK_LINK_SIO | DK_MODE_RAW_TCP);
}

int	delimiter_poll(int port)
{
	fdkparam_t	dp = (fdkparam_t) Gdktab;

	delimiter_write(port);

	if( ((dp->flag & DK_RUN) == 0) ||
	    //((dp->flag & DK_DO_FORCE_TX) == 0) ||
	    (dp->s2e_len == 0) /* ||
		(dp->e2s_len == 0) */
	  )
	{
		return 0;
	}

	// ... Frank Ho add 2009.11.27 for Ver1.7.59 ...
	// solve the retransmit data issue in port buffering + delimiter case
	if(dp->flag & DK_DO_BUFFERING)
	{
		if(!port_buffering_tcp_is_clear(port, 2))
			return(dp->s2e_len);
	}
	// ... end ...

	if(dp->flag & DK_DO_FORCE_TX)
	{
		if( (sys_clock_ms() - dp->lasttime) > (unsigned long)(dp->force_tx_waitms) )
		{
			delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
		}
	}

	if( (dp->s2e_sentlen > 0) && (dp->s2e_len >= dp->s2e_sentlen) ) // packet len > 0
	{
		delimiter_send(port, dp->s2e_sentlen, 0);
	}

	return dp->s2e_len;
}

int delimiter_read(int port, int send_buffered_data)
{
	fdkparam_t	dp;
	int n;
	int retry_count=0;
	dp = (fdkparam_t) Gdktab;

TRACE(("dp->flag: 0x%x, send_buffered_data: %d\r\n", dp->flag, send_buffered_data));
	if( (dp->flag & DK_RUN) == 0 )
	{
TRACE(("start buffering_offline_read()\r\n"));
// ... Frank Ho add 2009.11.27 for Ver1.7.59 ...
// solve the retransmit data issue in port buffering + delimiter case
		if(dp->flag & DK_DO_DELIMITER)
		{
			TRACE(("port=%d, send_buffered=%d\r\n", port, send_buffered_data));
			port_buffering_tcp_is_clear(port, 3);
		}
// ... end ...
		buffering_offline_read(port);
TRACE(("s2e_len: %d\r\n", dp->s2e_len));

		return -1;
	}

	if( dp->flag & DK_MODE_RAW_TCP )
	{
		if(dp->flag & DK_DO_BUFFERING)
		{
			TRACE(("port=%d, send_buffered=%d\r\n", port, send_buffered_data));
			if(!port_buffering_tcp_is_clear(port, 4))
			{
				TRACE(("port %d network jammed\r\n", port));
				buffering_offline_read(port);
				return -1;
			}
		}
	}

	if( dp->s2e_len == DK_BUFFER_SIZE_S2E && !(dp->flag & DK_DO_BUFFERING) )
	{/* if DK_DO_BUFFERING, the following while loop will never break because no call
		to port_buffering_tcp_is_clear() and thus dp->s2e_len will not be updated to 0 */
		do
		{
			n = delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);

			if( n < 0 && errno == EAGAIN )
			{
				retry_count++;
				if( retry_count > 10)
				{
					return 0;
				}
			}
			if( n < 0 && errno != EAGAIN )
			{
				return 0;
			}
            usleep(10);
		}
		while( dp->s2e_len != 0);

	}

	return dp->del_read(port, send_buffered_data);
}

int delimiter_read_1(int port, int send_buffered_data)
{
	int		n, max;
	fdkparam_t	dp;
	unsigned char	*p;
	dp = (fdkparam_t) Gdktab;
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
		check_from_ser(n, (char*)p, max);
		dp->lasttime = sys_clock_ms();
		dp->s2e_len += n;
		dp->s2e_wndx += n;

		if ( dp->s2e_wndx == dp->s2e_size )
		{
			dp->s2e_wndx = 0;
		}
		dp->s2e_cflag = 1;
	}

TRACE(("max=%d,n=%d,s2e_len=%d\n", max, n, dp->s2e_len));
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
	    delimiter_send(port, DK_BUFFER_SIZE_S2E, 0);
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
		    check_to_ser(n2, (char*)p, n);
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

int delimiter_write_2(int port)
{
	fdkparam_t	dp;
	unsigned char	*p;
	int n, ofree;

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
	    if ((n = dp->sio_write(port, (char*)p, n)) > 0)
	    {
		    dp->e2s_len -= n;
		    if (dp->e2s_len == 0)
		    {
			    dp->e2s_rndx = 0;
			    dp->e2s_wndx = 0;
		    }
		    else
		    {
		        dp->e2s_rndx += n;
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

TRACE(("max=%d,strip=%d,sent_to_tcp_len=%d\n", max, strip, dp->sent_to_tcp_len));
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
			TRACE(("port %d network jammed, max= %d\r\n", dp->port, max));
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

		if(dp->flag & DK_DO_BUFFERING)
		{
			/* update dp->s2e_len in port_buffering_tcp_is_clear() */
			dp->sent_to_tcp_len = n + strip;
			n = -1;	/* so that delimiter check will be break */
			TRACE(("sent_to_tcp_len=%d\r\n", dp->sent_to_tcp_len));
		}
		else
		{
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
	}
	TRACE(("port=%d,n=%d,len=%d,cndx=%d,rndx=%d,wndx=%d,cflag=%d\r\n", dp->port, n, dp->s2e_len, dp->s2e_cndx, dp->s2e_rndx, dp->s2e_wndx, dp->s2e_cflag));

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

	if (dp->e2s_len >= DK_BUFFER_SIZE_E2S)
	{
		delimiter_write(port);
		return dp->e2s_len;
	}
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

	delimiter_write(port);

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




