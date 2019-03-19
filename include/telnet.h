/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    telnet.h

    Telnet structure definitions

    2011-01-05  James Wang
        Porting telnet.h from NPort 6000.
*/

#ifndef _TELNET_H
#define _TELNET_H

/* termflag define */
#define TFLAG_WILLECHO      0x0001  /* telnet will echo */
#define TFLAG_WILLBIN       0x0002  /* telnet will binary */
#define TFLAG_DOECHO        0x0004  /* telnet do echo */
#define TFLAG_DOBIN         0x0008  /* telnet do binary */
#define TFLAG_TXBIN         0x0010  /* telnet transmit binary data */
#define TFLAG_RXBIN         0x0020  /* telnet receive binary data */
#define TFLAG_RXMASK        0x20FFC0    /* rx data need process mask */
#define TFLAG_IAC           0x0040  /* telnet IAC received */
#define TFLAG_SB            0x0080  /* telnet SB received */
#define TFLAG_SB1           0x0100  /* telnet SB received */
#define TFLAG_SB2           0x0200  /* telnet SB received */
#define TFLAG_SB3           0x0400  /* telnet SB received */
#define TFLAG_WILL          0x0800  /* telnet WILL received */
#define TFLAG_DO            0x1000  /* telnet DO received */
#define TFLAG_WONT          0x2000  /* telnet WONT/DONT received */
#define TFLAG_ENTER         0x4000  /* telnet ENTER received */
#define TFLAG_WILLTERMIS    0x8000  /* telnet ENTER received */
#define TFLAG_SENDBRK       0x10000 /* telnet Send Break Signal */
#define TFLAG_CRLF2CR       0x20000
#define TFLAG_CRLF2LF       0x40000
#define TFLAG_LF2CR         0x80000
#define TFLAG_LF2CRLF       0x100000
#define TFLAG_DONT          0x200000
#define TFLAG_RFC2217       0x400000
#define TFLAG_DO_RFC2217    0x800000
#define TFLAG_CRLF2         0x60000 /* CR-LF map to some other keys flags */
#define TFLAG_LF2           0x180000    /* LF map to some other keys flags */

#define D_SENDBRK_TIME      400 /* 400 mini-second */

/* TELNET command code defines */
#define D_TELNET_SE     240 /* End of subnegotiation parameters */
#define D_TELNET_NOP    241 /* No operation */
#define D_TELNET_MARK   242 /* data stream portion of a Synch */
#define D_TELNET_BRK    243 /* NVT break character */
#define D_TELNET_IP     244 /* The Interrupt Process function */
#define D_TELNET_AO     245 /* The Abort output function */
#define D_TELNET_AYT    246 /* The Are You There function */
#define D_TELNET_EC     247 /* The Erase character function */
#define D_TELNET_EL     248 /* The Erase Line function */
#define D_TELNET_GA     249 /* The Go ahead signal */
#define D_TELNET_SB     250 /* subnegotiation of option */
#define D_TELNET_WILL   251 /* confirmation option */
#define D_TELNET_WONT   252 /* refusal to perform */
#define D_TELNET_DO     253 /* request other party to perform */
#define D_TELNET_DONT   254 /* stop performing */
#define D_TELNET_IAC    255 /* Data Byte 255 */


/* TELNET option code defines */
#define D_TELNET_BINARY     0   /* binary transmission */
#define D_TELNET_ECHO       1   /* echo option */
#define D_TELNET_RCP        2   /* prepare to reconnect */
#define D_TELNET_SGA        3   /* suppress go ahead */
#define D_TELNET_NAMS       4   /* negotiate approximate message size */
#define D_TELNET_STATUS     5   /* status option */
#define D_TELNET_TIME       6   /* timing mark */
#define D_TELNET_RCTE       7   /* remote control transmssion & echo */
#define D_TELNET_NAOL       8   /* negotiate about output line-width */
#define D_TELNET_NAOP       9   /* negotiate about output page-size */
#define D_TELNET_NAOCRD     10  /* neg. about output CR disposition */
#define D_TELNET_NAOHTS     11  /* neg. about output horizontal tabs */
#define D_TELNET_NAOHTD     12  /* output horizontal TAB disposition */
#define D_TELNET_NAOFFD     13  /* output formfree disposition */
#define D_TELNET_NAOVTS     14  /* output vertical tabstops */
#define D_TELNET_NAOVTD     15  /* output vertical TAB disposition */
#define D_TELNET_NAOLFD     16  /* output linefree disposition */
#define D_TELNET_ASCII      17  /* extened ASCII */
#define D_TELNET_LOGOUT     18  /* logout option */
#define D_TELNET_BM         19  /* revised TELNET byte marco option */
#define D_TELNET_DET        20  /* Telnet data entry terminal */
#define D_TELNET_SUPDUP     21  /* SUPDUP option */
#define D_TELNET_SUPOUT     22  /* SUPDUP-OUTPUT option */
#define D_TELNET_SLOC       23  /* send location */
#define D_TELNET_TERM       24  /* terminal type */
#define D_TELNET_EOR        25  /* end of record */
#define D_TELNET_TUID       26  /* TACACS user identification Telnet */
#define D_TELNET_OUTMRK     27  /* output marking Telnet */
#define D_TELNET_TERMLOC    28  /* Terminal Location Number */
#define D_TELNET_3270       29  /* Telnet 3270 Regime */
#define D_TELNET_X3PAD      30  /* X.3 PAD */
#define D_TELNET_WINSIZE    31  /* Negotiate About Window Size */
#define D_TELNET_SPEED      32  /* Terminal Speed */
#define D_TELNET_FLOWCTL    33  /* Remote Flow Control */
#define D_TELNET_LINEMODE   34  /* Linemode */
#define D_TELNET_XDISP      35  /* X Display Location */
#define D_TELNET_ENVIR      36  /* Environment Option */
#define D_TELNET_AUTH       37  /* Authentication Option */
#define D_TELNET_CRYPT      38  /* Encryption Option */
#define D_TELNET_COMPORT    44  /* COM Port Option */
#define D_TELNET_EXOPL      255 /* extended options list */
#define D_TELNET_IS         0
#define D_TELNET_SEND       1

#define D_SIGNATURE         0   /* COM Port Option: Signature */
#define D_SET_BAUDRATE      1   /* COM Port Option: Set Baud Rate */
#define D_SET_DATASIZE      2   /* COM Port Option: Set Data Bits */
#define D_SET_PARITY        3   /* COM Port Option: Set Parity */
#define D_SET_STOPSIZE      4   /* COM Port Option: Set Stop Bits */
#define D_SET_CONTROL       5   /* COM Port Option: Set Control */
#define D_NOTIFY_LINESTATE  6   /* COM Port Option: Notify LineState */
#define D_NOTIFY_MODEMSTATE 7   /* COM Port Option: Notify ModemState */
#define D_FLOWCTRL_SUSPEND  8   /* COM Port Option: FlowCtrl Suspend */
#define D_FLOWCTRL_RESUME   9   /* COM Port Option: FlowCtrl Resume */
#define D_LINESTATE_MASK    10  /* COM Port Option: LineState Mask */
#define D_MODEMSTATE_MASK   11  /* COM Port Option: ModemState Mask */
#define D_PURGE_DATA        12  /* COM Port Option: Purge Data */

#define D_SET_DTR_ON        8   /* Set Control: Set DTR ON */
#define D_SET_DTR_OFF       9   /* Set Control: Set DTR OFF */
#define D_SET_RTS_ON        11  /* Set Control: Set RTS ON */
#define D_SET_RTS_OFF       12  /* Set Control: Set RTS OFF */


void    telnet_setting(int fd_net, int mode, int *telflag);
int     telnet_encode(int fd_net, u_char *source, int len, u_int quitkey, int telflag);
int     telnet_decode(int fd_net, u_char *source, int len, u_char *target, int *telflag, char *termcap);
void    telnet_checkbreak(int port, int fd_net);

#endif
