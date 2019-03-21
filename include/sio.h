
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _SIO_H
#define _SIO_H

#ifndef CROSS
#define MAX_PORTS           4
#else
#if defined(imx_EVB)
#define MAX_PORTS           1
#elif defined(w1)
#define MAX_PORTS           1
#elif defined(w2x50a)
#define MAX_PORTS           2
#elif defined(ia5x50aio)
#define MAX_PORTS           2
#else
#error unknow module
#endif

#endif // CROSS

/* error code */
#define SIO_OK              0
#define SIO_BADPORT         -1      /* no such port or port not opened          */
#define SIO_OUTCONTROL      -2      /* can't control the board                  */
#define SIO_NODATA          -4      /* no data to read or no buffer to write    */
#define SIO_OPENFAIL        -5      /* no such port or port has be opened       */
#define SIO_RTS_BY_HW       -6      /* RTS can't set because H/W flowctrl       */
#define SIO_BADPARM         -7      /* bad parameter                            */
#define SIO_WIN32FAIL       -8      /* call win32 function fail, please call    */
                                    /* GetLastError to get the error code       */
#define SIO_BOARDNOTSUPPORT -9      /* Does not support this board              */
#define SIO_FAIL            -10     /* PComm function run result fail           */
#define SIO_ABORTWRITE      -11     /* write has blocked, and user abort write  */
#define SIO_WRITETIMEOUT    -12     /* write timeoue has happened               */
#define SIO_EAGAIN          -13     /* EAGAIN               */
#define SIO_EWOULDBLOCK     -14     /* EWOULDBLOCK               */


/*      BAUD rate setting       */
#define BAUD_50         0x00
#define BAUD_75         0x01
#define BAUD_110        0x02
#define BAUD_134        0x03
#define BAUD_150        0x04
#define BAUD_300        0x05
#define BAUD_600        0x06
#define BAUD_1200       0x07
#define BAUD_1800       0x08
#define BAUD_2400       0x09
#define BAUD_4800       0x0A
#define BAUD_7200       0x0B
#define BAUD_9600       0x0C
#define BAUD_19200      0x0D
#define BAUD_38400      0x0E
#define BAUD_57600      0x0F
#define BAUD_115200     0x10
#define BAUD_230400     0x11
#define BAUD_460800     0x12
#define BAUD_921600     0x13

/* MODE setting */
#define BIT_5           0x00
#define BIT_6           0x01
#define BIT_7           0x02
#define BIT_8           0x03
#define BIT_DATA_MASK   0x03

#define STOP_1          0x00
#define STOP_2          0x04
#define BIT_STOP_MASK   0x04

#define P_NONE          0x00
#define P_ODD           0x08
#define P_EVEN          0x18
#define P_MRK           0x28
#define P_SPC           0x38
#define BIT_PARITY_MASK 0x38

/* Flow control setting */
#define F_NONE          0x00    /* No flow control          */
#define F_CTS           0x01    /* CTS flow control         */
#define F_RTS           0x02    /* RTS flow control         */
#define F_HW            0x03    /* Hardware flowcontrol     */
#define F_TXSW          0x04    /* Tx XON/XOFF flow control */
#define F_RXSW          0x08    /* Rx XON/XOFF flow control */
#define F_SW            0x0C    /* Sofeware flowcontrol     */

/* Flush */
#define FLUSH_RX        0x00
#define FLUSH_TX        0x01
#define FLUSH_ALL       0x02

/* MODEM CONTROL setting */
#define C_DTR           0x01
#define C_RTS           0x02

/* MODEM LINE STATUS */
#define S_CTS           0x01
#define S_DSR           0x02
#define S_RI            0x04
#define S_CD            0x08

/* Interface */
#define RS232_MODE          0
#define RS422_MODE          1
#define RS485_2WIRE_MODE    2
#define RS485_4WIRE_MODE    3


struct sio_data {
    unsigned int  iftype;
    unsigned int  charmode;
    unsigned int  flowctrl;
    unsigned int  lstatus;
    unsigned long  baudrate;
    unsigned long  tx_cnt;
    unsigned long  rx_cnt;
    unsigned long  up_txcnt;
    unsigned long  up_rxcnt;
    int fifo;
};
typedef struct sio_data SIODATA;


/* basic function phototype */
int sio_open(int port);
int sio_close(int port);
int sio_ioctl(int port, int baud, int mode);
int _sio_mapFlowCtrl(int flowctrl);
int sio_flowctrl(int port, int mode);
int sio_flush(int port, int func);
int sio_DTR(int port, int mode);
int sio_RTS(int port, int mode);
int sio_getDTR(int port);
int sio_getRTS(int port);
int sio_lctrl(int port, int mode);
int sio_setxonxoff(int port, unsigned char xon, unsigned char xoff);
int sio_baud(int port, long speed);
int sio_getch(int port);
int sio_read(int port, char *buf, int len);
#ifdef SUPPORT_SERCMD
int sio_read_ex(int port, char *buf, int len);
#endif
int sio_read_timeout(int port, char *buf, int len, int timeout_ms);
int sio_putch(int port, int term);
int sio_write(int oprt, char *buf, int len);
int sio_lstatus(int port);
long sio_iqueue(int port);
long sio_oqueue(int port);
long sio_getbaud(int port);
unsigned int sio_getmode(int port);
unsigned int sio_getflow(int port);
int sio_notify_status(int port, int *msr, int *hold);
int sio_notify_error(int port);
int sio_break(int port, int mode);
int sio_fifo(int port, int mode);
int sio_xon(int port);
int sio_xoff(int port);

int sio_getiftype(int port);
int sio_setiftype(int port, int iftype);
int sio_getsiodata(int port, SIODATA *siodata);

long sio_ofree(int port);
int sio_breakctl(int port, int flag);

/* 2012/08/10 Develop for RealCOM Mode Win32API ActXon/ActXoff */
int sio_setxon(int port);
int sio_setxoff(int port);

#define SERIAL_PORT_INTERNAL_FLAG 1
#define SERIAL_PORT_EXTERNAL_FLAG 2
int _sio_getPortType(int port);

struct __port_status_ext {
    union {
        unsigned int irq;
        int portno;
    }_config;
    unsigned int  c_flag;
    unsigned int  i_flag;
    unsigned long mmio;
    unsigned int frame_error_cnt;
    unsigned int parity_error_cnt;
    unsigned int overrun_error_cnt;
    unsigned int break_cnt;
    int rts;
    int dtr;
};
typedef struct __port_status_ext _port_status_ext;
typedef struct __port_status_ext *_port_status_ext_t;
int sio_getStatus(int port, _port_status_ext_t ext_status);

#endif
