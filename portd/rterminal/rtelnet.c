/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
   	rtelnet.c

	RTelnet port handle routine

    2008-05-27	Kevin Chu
		new release    
*/

/*
	under TELNETD mode, we don't care the DCD/DSR is on or not !
	so, if the DCD fall to off, we don't stop it's connection
	If not transmit any data in inactive time, disconnect the
	link
*/
#include <header.h>
#include <config.h>
#include <sysapi.h>
#include <sio.h>
#include <portd.h>
#include <stdio.h>
#include <support.h>
#include <datalog.h>
#include <eventd.h>
#include <telnet.h>

#ifdef SUPPORT_SECURE_MODE
#include <dropbear-0.44/sshSocket.h>
#include <dropbear-0.44/Queue.h>

extern void checktimeouts(struct sshsession *ses);
extern void write_packet(struct sshsession *ses);
extern void read_packet(struct sshsession *ses);
extern void process_packet(struct sshsession *ses);
int cr_lf_translation(char *src, char *dest, int len, char keymap);
#endif

extern int portd_terminate;
extern unsigned long PortdLastTime;
#define	TMP_LEN		256

void	rtelnet_start(void* arg);
void	rtelnet_monitor(int port, char *buf);
static	void	rtelnet_main_loop(int port, int fd_port, int fd_net, int raw);
extern	int		net_chk_pswd(int fd_port, int port, int flag, int local);
extern  void	portd_logout(int port);

#ifdef SUPPORT_SECURE_MODE
static struct sshsession *rtelnetSSH[SIO_MAX_PORTS];
#endif

/* opmode 0: telnet, 1: raw, 2: ssh */
void	rtelnet_start(void* arg)
{
	int		len, fd_port, fd_net = -1, fd_listen;
	int		tcpport, keymap;
	u_long		t;
	struct sockaddr_in sin;
	int	baud, mode, flowctrl;
	int port, opmode;

	opmode = ((int)arg & 0x8000) ? 1 : 0;
    port = (int)arg & ~0x8000;
	//printf("rt: rtelnet_start: %d, %d\r\n", port, opmode);
#ifdef SUPPORT_SECURE_MODE	
	rtelnetSSH[port] = NULL;
#endif
	PortdLastTime = sys_clock_s();

	while ((fd_port = sio_open(port)) < 0)
		sleep(1);
	sio_fifo(port, Scf_getAsyncFifo(port)); 	 
	Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
	sio_ioctl(port, baud, mode);	
	flowctrl = _sio_mapFlowCtrl(flowctrl);
	if( Scf_getIfType(port) != 0x00 )
	{
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
	sio_flowctrl(port, flowctrl);
	sio_flush(port, FLUSH_ALL);
	sio_DTR(port, 0);
	sio_RTS(port, 0);
	
	while ( (fd_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
		sleep(1);
	t = 1;
	while (setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(t)) == -1)
		sleep(1);
	Scf_getRTerminal(port, &tcpport, &keymap);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(tcpport);	
	sin.sin_addr.s_addr = INADDR_ANY;
	while (bind(fd_listen, (struct sockaddr *)&sin, sizeof(sin)) == -1)
    	sleep(1);
	listen(fd_listen, 1);

	len = sizeof(sin);	
	//printf("rt: first loop\r\n");
	while ( portd_terminate == 0 )
	{
		
		t = 1;
		ioctl(fd_listen, FIONBIO, &t);	
		if ( (fd_net = accept(fd_listen, (struct sockaddr *)&sin, (socklen_t*)&len)) < 0 ) 
		{
            sio_flush(port, FLUSH_ALL);
			usleep(100000);
			continue;
		}
		//printf("rt: accept ok\r\n");

		t = 0;
		ioctl(fd_listen, FIONBIO, &t);
		
#ifdef SUPPORT_TCP_KEEPALIVE
        tcp_setAliveTime(port, fd_net);
#endif

		PortdLastTime = sys_clock_s();
		
		rtelnet_main_loop(port, fd_port, fd_net, opmode);

		if (fd_net > 0)
		{
			if (opmode != 2)
				sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);
			//printf("close fd_net\r\n");
			close(fd_net);
		}
		
		/* Wait all data is sent out */
		//printf("rt: portd_wait_empty\r\n");
		portd_wait_empty(port, fd_port, 30000); /* timeout: 30 seconds */
		//printf("rt: portd_wait_empty end\r\n");
	}
	//printf("rt: leave first loop\r\n");

	PortdLastTime = sys_clock_s();
	close(fd_listen);
	sio_close(port);
	//printf("rt: rtelnet_end: %d, %d\r\n", port, opmode);
}

static	void	rtelnet_main_loop(int port, int fd_port, int fd_net, int opmode)
{
	int		i = 0, maxfd, telflag, count, auth_type;
	int		net_write_flag = 0, port_write_flag = 0, tcpport, keymap;
	 struct timeval time;

	fd_set		rfds, wfds;
#ifdef	SUPPORT_LF_XCHG
	char		tmp[TMP_LEN], tmp1[TMP_LEN * 2];
#else
	char		tmp[TMP_LEN], tmp1[TMP_LEN];
#endif
	u_long		t, data_time = 0;
	char *		term;
#ifdef SUPPORT_SECURE_MODE
	struct sshsession *ssh = NULL;
	int dataallowed;
#endif
	int   (*rtelnet_sio_write)(int port, char *buf, int len);
	int   (*rtelnet_sio_read)(int port, char *buf, int len);
	int n;
	unsigned long idletime;

	//printf("rt: rtelnet_main_loop\r\n");
	if (Scf_getSerialDataLog(port))
	{
		rtelnet_sio_write =  log_sio_write;
		rtelnet_sio_read =  log_sio_read;
	}
	else
	{
		rtelnet_sio_write =  sio_write;
		rtelnet_sio_read =  sio_read;   
	}
	
	n = Scf_getInactivityTimeMin(port);
    if (n == 0)
        idletime = 0xFFFFFFFFL;
    else
        idletime = (unsigned long)(n * 60);
	
	term = "ansi";
	
	sio_DTR(port, 1);
	sio_RTS(port, 1);
	
	maxfd = fd_port;
	if (fd_net > maxfd)
		maxfd = fd_net;
	
	time.tv_sec = 2;
	time.tv_usec = 0;
	
	Scf_getRTerminal(port, &tcpport, &keymap);

#ifdef SUPPORT_SECURE_MODE
	if (opmode == 2)
	{
		sshSvrSesInit(&ssh, fd_net, port);
		/* call log_system_port_connect() in newchannel() */
		rtelnetSSH[port] = ssh;
	} 
	else
#endif  
	if ( opmode == 0) 
	{
		telnet_setting(fd_net, 0, &telflag);

		i = keymap;
		if ( i == DCF_CRLF_TO_CR )		/* CR-LF map to CR */
			telflag |= TFLAG_CRLF2CR;
		else if ( i == DCF_CRLF_TO_LF )	/* CR-LF map to LF */
			telflag |= TFLAG_CRLF2LF;
/*			
#ifdef	SUPPORT_LF_XCHG
		i = (int)(pm->op_flags & D_RTELNET_LF_FLAGS);
		if ( i == D_RTELNET_LF2CR )
			telflag |= TFLAG_LF2CR;
		else if ( i == D_RTELNET_LF2CRLF )
			telflag |= TFLAG_LF2CRLF;
#endif
*/

		auth_type = Scf_getAuthType(port);
		if ( auth_type == D_AUTH_LOCAL || auth_type == D_AUTH_RADIUS ) 
		{
			count = 0;
			FD_ZERO(&rfds);
			while ( count++ < 100 ) 
			{
				FD_SET(fd_net, &rfds);
				if (select(fd_net+1, &rfds, NULL, NULL, &time) <= 0)
					break;
				if ( (i = recv(fd_net, tmp, TMP_LEN, 0)) <= 0 )
					break;
				telnet_decode(fd_net, (u_char*)tmp, i, (u_char*)tmp1, &telflag, term);
			}
		}
		
		i = 100;
		count = 0;
		do 
		{
			if ( auth_type != D_AUTH_LOCAL && auth_type != D_AUTH_RADIUS )
				break;
			i = net_chk_pswd(fd_net, port, 0, auth_type & 1);
			count++;
		} while ( i > 0 && count < 3 );
		
		if ( i < 0 || count >= 3 ) 
		{
			shutdown(fd_net, 2);
			usleep(100000);
            //portd_terminate = 1;		
			return;
		} 
		else if ( i == 0 ) 
		{
			send(fd_net, "Login O.K.\r\n", 12, 0);			
			sio_flush(port, FLUSH_ALL);
		}
	}
	else 
	{
		telflag = 0;    
	}
	
	if (opmode != 2)  /* call log_system_port_connect() in newchannel() */
		sys_send_events(EVENT_ID_OPMODE_CONNECT, port << 4);
 
	t = sys_clock_s();
	/******** MAIN LOOP ********/

	//printf("rt: second loop\r\n");
	while ( portd_terminate == 0 )
	{
		FD_ZERO(&rfds); 
		FD_ZERO(&wfds);

#ifdef SUPPORT_SECURE_MODE
    if (opmode == 2)
    {
  	  if (sshSesIsClosed(ssh))  // closed by checktimeousts() or checkclose()
      {  
        SSH_FREE(ssh);
  	    break;         
      }
      FD_SET(fd_net, &rfds);
      if (sshCanSetNetWfd(ssh))
      {           
        FD_SET(fd_net, &wfds);
      }
      sshSetChannelFds(ssh, fd_port, &rfds, &wfds);    
    }
    else
#endif
    {
  		if ( port_write_flag )	
  			FD_SET(fd_port, &wfds);
  		else			
  			FD_SET(fd_net, &rfds);
  			
  		if ( net_write_flag )	
  			FD_SET(fd_net, &wfds);
  		else			
  			FD_SET(fd_port, &rfds);
		}
			
		if ( select(maxfd+1, &rfds, &wfds, NULL, &time) <= 0 ) 
		{
#ifdef SUPPORT_SECURE_MODE
      if ( opmode == 2 )
      {
		if ( (sys_clock_s() - t) > idletime )
					break;
        /* check for auth timeout, rekeying required etc */
        checktimeouts(ssh);
        continue;
      }
      else
#endif
			/* Send AYT(are you there) every 3 mins, to check the	connection is ok or not */
			if ( opmode == 0 ) 
			{
				if ( telflag & TFLAG_ENTER ) 
				{
					telflag &= ~TFLAG_ENTER;
					if ( telflag & TFLAG_CRLF2LF ) 
					{
						tmp[0] = 13;
						rtelnet_sio_write(port, tmp, 1);
					}
				}
				telnet_checkbreak(port, fd_net);
				if ( (sys_clock_s() - t) > idletime)
					break;
			} 
			else if ( (port_write_flag == 0) && (net_write_flag == 0) &&
							((sys_clock_s() - PortdLastTime) > idletime) ) 
		 	{
				if ( (sys_clock_s() - data_time) > idletime )
					break;
			}
			continue;
		}

		if ( FD_ISSET(fd_net, &wfds) )
		{
#ifdef SUPPORT_SECURE_MODE
      if (opmode == 2)
        write_packet(ssh);      		
      else        
#endif          
  			net_write_flag = 0;		
		}
			  
		if ( opmode == 0 )
			telnet_checkbreak(port, fd_net);
		t = sys_clock_s();
		if ( FD_ISSET(fd_net, &rfds) ) 
		{
			if (sio_ofree(port) < TMP_LEN)
				port_write_flag = 1;
#ifdef SUPPORT_SECURE_MODE
      else if (opmode == 2)
      {
        read_packet(ssh);       
      }
#endif
			else if ( (i = recv(fd_net, tmp, TMP_LEN, 0)) > 0 ) 
			{
				{

					i = telnet_decode(fd_net, (u_char*)tmp, i, (u_char*)tmp1, &telflag, term);
					if ( i > 0 ) 
					{
						rtelnet_sio_write(port, tmp1, i);
						PortdLastTime = sys_clock_s();
						data_time = sys_clock_s();
					}
				}
				
				/*
			 	* modify at 07-01-1998 by Yu-Lang Hsu
			 	*/
				if ( telflag & TFLAG_SENDBRK ) 
				{
			    telflag &= ~TFLAG_SENDBRK;
				sio_break(port, 1);
				usleep(D_SENDBRK_TIME * 1000);
				sio_break(port, 0);	    
				}
			}
			else if ( i == 0 )
				break;		/* link broken */
		}

#ifdef SUPPORT_SECURE_MODE
    if ( opmode == 2)
    {
      /* Process the decrypted packet. After this, the read buffer
       * will be ready for a new packet */                                                             
      process_packet(ssh);   
      dataallowed = sshSesDataAllowed(ssh);
    }
#endif  
				
		if ( FD_ISSET(fd_port, &rfds) ) 
		{
#ifdef SUPPORT_SECURE_MODE
      if (opmode == 2)      
        i = sshCh0BufMax(ssh);      
      else
#endif      
        i = tcp_ofree(fd_net);
        
			if (i < TMP_LEN*2)
				net_write_flag = 1;				
			else if ( (i = rtelnet_sio_read(port, tmp, TMP_LEN)) > 0 ) 
			{
#ifdef SUPPORT_SECURE_MODE
        if ( opmode == 2 )
          sshWriteCh0Buf(ssh, tmp, i);
        else  
#endif
					telnet_encode(fd_net, (u_char*)tmp, i, 0xFFFF, telflag);
                    PortdLastTime = sys_clock_s();
				
			}			
		}
		
		if ( FD_ISSET(fd_port, &wfds) )
		{
#ifdef SUPPORT_SECURE_MODE
			if ( opmode == 2 )
			{
				if( sio_ofree(port) >= TMP_LEN )
				{
				  if ((i = sshReadCh0Buf(ssh, tmp, TMP_LEN)) > 0)   // writechannel()
				  { 
				    if (keymap == DCF_CRLF_TO_CRLF) /* no translation */
				      rtelnet_sio_write(port, tmp, i);   
				    else
				    {
				      i = cr_lf_translation(tmp, tmp1, i, keymap);
				      rtelnet_sio_write(port, tmp1, i);           
				    }  
				  }
				}
			}
			else
#endif		
			  port_write_flag = 0;
		}	

#ifdef SUPPORT_SECURE_MODE
	  if (opmode == 2)
	  {
		  /* now handle any of the channel-closing type stuff */
      sshCheckCh0Close(ssh);
	  }    
#endif      
	}
	//printf("rt: leave second loop\r\n");

#ifdef SUPPORT_SECURE_MODE
  if (opmode == 2)
    SSH_FREE(ssh);	
#endif

  portd_logout(port); /* radius logout */

  
	/* Wait all data is sent out */
	//printf("rt: portd_wait_empty\r\n");
  	portd_wait_empty(port, fd_port, 30000); /* timeout: 30 seconds */
	//printf("rt: portd_wait_empty end\r\n");


}

#ifdef SUPPORT_SECURE_MODE
char *rtelnet_get_cipher_name(int port)
{
	return sshCipherName(rtelnetSSH[port]);
}

int cr_lf_translation(char *src, char *dest, int len, char keymap)
{
  char ch, *ptr_dest = dest;

  while (len > 0) 
  {    	
    ch = *src++;
    len--;
    if ((len > 0) && (ch == 0x0D)) /* CR */
    { 
      if (*src == 0x0A) /* LF */
      {
        if (keymap == DCF_CRLF_TO_CR)
        {
          src++;  len--;
          *ptr_dest++ = 0x0D;
          continue;
        }
        if (keymap == DCF_CRLF_TO_LF)
        {
          src++;  len--;
          *ptr_dest++ = 0x0A;
          continue;
        }          
      }
    }      
    *ptr_dest++ = ch;
  }

  return (int)(ptr_dest - dest);
}
#endif
