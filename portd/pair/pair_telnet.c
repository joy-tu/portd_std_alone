 
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    pair.c

    Pair Connection Master/Slave handle routine 

    2011-01-10  James Wang
        First release.
*/


#include <sio.h>
#include <sys/socket.h>
#include <string.h>
#include <telnet.h>
#include <config.h>


#define TFLAG_COMPORT       0x20000L /* Receive COM Port Option */
#define TFLAG_WILLCONTROL   0x40000L /* To do line control */
typedef unsigned char   u_char;

static u_char  willdone1[6] =
{
        (u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_SGA,
        (u_char)D_TELNET_IAC,(u_char)D_TELNET_WILL,(u_char)D_TELNET_TERM,
};
static u_char   willdone2[15]=
{
        D_TELNET_IAC, D_TELNET_WILL, D_TELNET_ECHO,
        D_TELNET_IAC, D_TELNET_WILL, D_TELNET_SGA,
        D_TELNET_IAC, D_TELNET_WILL, D_TELNET_BINARY,
        D_TELNET_IAC, D_TELNET_DO,   D_TELNET_ECHO,
        D_TELNET_IAC, D_TELNET_DO,   D_TELNET_BINARY
};
//static int    telnet_line_stat[SIO_MAX_PORTS+1];
static u_char   telnet_set_control;
static void do_com_control(int port, u_char value);

void _pair_telnet_setting(int fd_net, int mode, long *telflag, int port)
{    
    if ( mode == 1 )
    {
      send(fd_net, (char *)willdone1, 6, 0);
        *telflag = 0;
    }
    else
    {
        *telflag = TFLAG_WILLECHO | TFLAG_WILLBIN | TFLAG_DOECHO | TFLAG_DOBIN;
        send(fd_net, (char *)willdone2, 15, 0);
    }
  /*    telnet_line_stat[port] = SIO_LS_DSR | SIO_LS_CTS;   */
}

/*
 *  Encode telnet data, and sent to lan, previous function
 *  must check the lan is canwrite
 */
int     pair_telnet_encode(int fd_net, u_char *source, int len, unsigned int quitkey, long telflag)
{
    int i;
/*  u_char  c, *ptr_net, target_net[MUX_TMP_LEN * 2];   */
    u_char  c, *ptr_net, target_net[512 * 2];

    ptr_net = target_net;
    for ( i=0; i<len; i++ ) {
        *ptr_net++ = c = *source++;
        if ( c == D_TELNET_IAC )
            *ptr_net++ = D_TELNET_IAC;
        /* albert: If port buffering is enabled, buffered data will be 
           send out before telnet protocol handshake is finished once a
           socket connection is established (in main_loop()). In this case
           TFLAG_TXBIN is set after buffered data were sent, thus the
           buffered data were encoded in ASCII mode. 
           Since pair connection is always Binary mode for both peers,
           we will permanently transmit Binary data to avoid encoding
           incorrect buffering data. */
        #if 0
        else if ( c == 13 && (telflag & TFLAG_TXBIN) == 0 )
            *ptr_net++ = 0;
        #endif
        if ( c == quitkey && c != 0 )
            return(-1);
    }
    if ( (i = (int)(ptr_net - target_net)) > 0 ) {
        send(fd_net, (char *)target_net, i, 0);
    }
    return(0);
}

int     pair_telnet_decode(int fd_net, u_char *source, int len,
                  u_char *target, long *telflag,
                  char *termcap, int port)
{
    int     i;
    long        flag;
    u_char *ptr_net;
    u_char *ptr_port;
    u_char      ch;
    u_char      target_net[256];  /* the CMD cannot large then 256! */

    flag = *telflag;
    ptr_port = target;
    ptr_net = (u_char *)target_net;
    while ( len-- ) {
        ch = *source++;
        if ( flag & TFLAG_ENTER ) { /* skip follow enter 0 */
            flag &= ~TFLAG_ENTER;
            if ( ch == 0 )
                continue;
        }
        if ( (flag & (TFLAG_RXMASK)) == 0 ) {
            if ( ch != D_TELNET_IAC ) {
                *ptr_port++ = ch;
                if ( ch == 13 ) /* CR */
                    if ( (flag & TFLAG_RXBIN) == 0 )
                        flag |= TFLAG_ENTER;
            }
            else {
                flag |= TFLAG_IAC;
                continue;
            }
            continue;
        }
        if ( flag & TFLAG_IAC ) {
            flag &= ~TFLAG_IAC;
            switch ( ch )
            {
            case D_TELNET_IAC:  *ptr_port++ = ch;   break;
            case D_TELNET_SB:   flag |= TFLAG_SB;   break;
            case D_TELNET_WILL: flag |= TFLAG_WILL; break;
            case D_TELNET_DO:   flag |= TFLAG_DO;   break;
            case D_TELNET_DONT:
            case D_TELNET_WONT: flag |= TFLAG_WONT; break;
            case D_TELNET_BRK:  flag |= TFLAG_SENDBRK;  break;
            }
            continue;
        }
        if ( flag & TFLAG_WONT ) {  /* do nothing */
            flag &= ~TFLAG_WONT;
            continue;
        }
        if ( flag & TFLAG_DO ) {
            flag &= ~TFLAG_DO;
            if ( ch == D_TELNET_BINARY ) {
                flag |= TFLAG_TXBIN;
                if ( (flag & TFLAG_WILLBIN) == 0 ) {
                    *ptr_net++ = D_TELNET_IAC;
                    *ptr_net++ = D_TELNET_WILL;
                    *ptr_net++ = ch;
                }
            }
            else if ( ch == D_TELNET_ECHO ) {
                if ( (flag & TFLAG_WILLECHO) == 0 ) {
                    *ptr_net++ = D_TELNET_IAC;
                    *ptr_net++ = D_TELNET_WONT;
                    *ptr_net++ = ch;
                }
            }
            else if ( ch != D_TELNET_SGA && ch != D_TELNET_TERM ) {
                *ptr_net++ = D_TELNET_IAC;
                *ptr_net++ = D_TELNET_WONT;
                *ptr_net++ = ch;
            }
            continue;
        }
        if ( flag & TFLAG_WILL ) {
            flag &= ~TFLAG_WILL;
            if ( ch == D_TELNET_BINARY ) {
                flag |= TFLAG_RXBIN;
                if ( (flag & TFLAG_DOBIN) == 0 ) {
                    *ptr_net++ = D_TELNET_IAC;
                    *ptr_net++ = D_TELNET_DO;
                    *ptr_net++ = ch;
                }
            }
            else if ( ch == D_TELNET_ECHO ) {
                if ( (flag & TFLAG_DOECHO) == 0 ) {
                    /* yes remote can do echo */
                    *ptr_net++ = D_TELNET_IAC;
                    *ptr_net++ = D_TELNET_DO;
                    *ptr_net++ = ch;
                }
            }
            else if ( ch != D_TELNET_SGA && ch != D_TELNET_TERM ) {
                *ptr_net++ = D_TELNET_IAC;
                *ptr_net++ = D_TELNET_DONT;
                *ptr_net++ = ch;
            }
            continue;
        }
        if ( flag & TFLAG_SB ) {    /* just answer when ask term */
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
                    } else if ( flag & TFLAG_WILLCONTROL ) {        
                        do_com_control(port, telnet_set_control);   
                    }
                    flag &= ~(TFLAG_SB | TFLAG_SB1 | TFLAG_SB2 |
                        TFLAG_SB3 | TFLAG_WILLTERMIS |
                        TFLAG_WILLCONTROL);
                }
                else
                    flag &= ~TFLAG_SB3;
            }
            else if ( flag & TFLAG_COMPORT ) {
                if ( flag & TFLAG_WILLCONTROL ) {
                    telnet_set_control = ch;
                    flag &= (~TFLAG_COMPORT);
                    flag |= TFLAG_SB2;
                } else if ( ch == D_SET_CONTROL )
                    flag |= TFLAG_WILLCONTROL;
                else {
                    flag &= (~TFLAG_COMPORT);
                    flag |= TFLAG_SB2;
                }
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
            else if ( ch == D_TELNET_COMPORT )
                flag |= TFLAG_COMPORT;
            else if ( ch == D_TELNET_TERM )
                flag |= TFLAG_SB1;
            else
                flag |= TFLAG_SB2;  /* just skip all */
        }
    }
    *telflag = flag;
    if ( (i = (int)(ptr_net - (u_char *)target_net)) != 0 )
    {
        send(fd_net, (char *)target_net, i, 0);
    }
    return((int)(ptr_port - target));
}

static  void    do_com_control(int port, u_char value)
{
    u_char  flag;

	if( Scf_getIfType(port)!=RS232_MODE )
	{
		return;
	}
    if ( port > MAX_PORTS )
        return;
    flag = 0;
    switch ( value )
    {
        case D_SET_DTR_ON:
            flag = 1;
        case D_SET_DTR_OFF:
            sio_DTR(port, flag);
            break;
        case D_SET_RTS_ON:
            flag = 1;
        case D_SET_RTS_OFF:
            sio_RTS(port, flag);
            break;
    }
}

