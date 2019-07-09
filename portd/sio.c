
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    sio.c

    Serial I/O API for MiiNEPort W1.

    2011-01-04  James Wang
        developing...
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <signal.h>
#include <sysapi.h>
#ifdef __CYGWIN__
#include <termios.h>
#else
#include <linux/serial.h>
#endif // __CYGWIN__

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <header.h>
#include <debug.h>
#include <config.h>
#include <sio.h>
#include <portd.h>
#include <support.h>

#ifdef SUPPORT_SERCMD
#include <scm.h>
#endif
//
// MOXA ioctls
//

#define MOXA                    0x400
#define MOXA_GETSTATUS          (MOXA + 65)
#define MOXA_SET_OP_MODE        (MOXA + 66)
#define MOXA_GET_OP_MODE        (MOXA + 67)
#define MOXA_RST_CNT            (MOXA + 69)
#define MOXA_OQUEUE             (MOXA + 70)
#define MOXA_SETBAUD            (MOXA + 71)
#define MOXA_GETBAUD            (MOXA + 72)
#define MOXA_MON                (MOXA + 73)
#define MOXA_LSTATUS            (MOXA + 74)
#define MOXA_MON_EXT            (MOXA + 75)
#define MOXA_SET_BAUD_METHOD    (MOXA + 76)
#define MOXA_GET_WRITEROOM      (MOXA + 77)
#define MOXA_SET_FIFO           (MOXA + 80)
#define MOXA_SET_FIFO_MU150     (MOXA + 102)
#define MOXA_ACT_XON_MU150	(MOXA + 103)
#define MOXA_ACT_XOFF_MU150	(MOXA + 104)
#define MOXA_MON_EXT_2          (MOXA + 150)
#define	MOXA_SET_SPECIAL_BAUD_RATE	(MOXA + 0x64)
#define	MOXA_GET_SPECIAL_BAUD_RATE	(MOXA + 0x65)

#define TTY_NAME "/dev/ttyr00"

#ifdef SUPPORT_INTERNAL_UART
#define SERIAL_PROC_FILE        "/proc/tty/driver/ttymxc"
#endif // SUPPORT_INTERNAL_UART
#define SUPPORT_EXTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
#define D_MONITOR_INFO_PORT     "/dev/tty"
#endif // SUPPORT_EXTERNAL_UART

struct __port_map {
    int flag;
    int portno;
};
typedef struct __port_map _port_map;
typedef struct __port_map* _port_map_t;
_port_map port_map[] = {
#ifdef SUPPORT_INTERNAL_UART
    {SERIAL_PORT_INTERNAL_FLAG, 0}, // port = 1
#endif // SUPPORT_INTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
    {SERIAL_PORT_EXTERNAL_FLAG, 0}, // port = 2
    {SERIAL_PORT_EXTERNAL_FLAG, 1}, // port = 3
    {SERIAL_PORT_EXTERNAL_FLAG, 2}, // port = 4
    {SERIAL_PORT_EXTERNAL_FLAG, 3}, // port = 5
#endif // SUPPORT_EXTERNAL_UART
};
#define NUMBER_OF_PORTS() (sizeof(port_map)/sizeof(_port_map))

struct _portinfo
{
    int  fd;
    _port_map_t map;
};
typedef struct _portinfo PortInfo;
typedef struct _portinfo* PortInfo_t;

int sio_errno;

static PortInfo GPortInfo;

static int uart_buffer_size;
static char port_buf[32];

#ifdef CROSS
static char* __get_port_node_name(int port)
{
    _port_map_t p;


    port--;
    if( (port < 0) || (port > NUMBER_OF_PORTS()) )
        return NULL;

    p = &port_map[port];

    switch(p->flag)
    {
#ifdef SUPPORT_INTERNAL_UART
    case SERIAL_PORT_INTERNAL_FLAG:
        sprintf(port_buf, "/dev/ttyr0%d", p->portno);
        break;
#endif // SUPPORT_INTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
    case SERIAL_PORT_EXTERNAL_FLAG:
        sprintf(port_buf, "/dev/ttyUSB%d", p->portno);
        printf("open /dev/ttyUSB%d \r\n", p->portno);
        break;
#endif // SUPPORT_EXTERNAL_UART
    default:
        return NULL;
    }

    return port_buf;
}
#endif // CROSS

//
// Static functions.
//
static PortInfo_t getPort(int port)
{
#if 0
    port--;
    if( (port < 0) || (port > NUMBER_OF_PORTS()) )
        return -1;
#endif
    return &GPortInfo;
}

static _port_map_t _getPortMap(int port)
{
    port--;
    if( (port < 0) || (port > NUMBER_OF_PORTS()) )
        return NULL;

    return &port_map[port];
}

int _sio_getPortType(int port)
{
    _port_map_t map;
    map = _getPortMap(port);
    if(map == NULL)
        return -1;
    return map->flag;
}

//
// Sio APIs.
//
int sio_open(int port)
{
    PortInfo_t ptr;
    struct termios options;
    //char fname[256];
    char *fname;
	int inter;

    if ((ptr = getPort(port)) == NULL)
        return SIO_BADPORT;
        printf("==sk== %s:%d\r\n",__FUNCTION__,__LINE__);
#ifdef CROSS
    fname = __get_port_node_name(port);
        printf("==sk== %s:%d\r\n",__FUNCTION__,__LINE__);
    if(fname == NULL)
        return SIO_BADPORT;
#else
    fname = TTY_NAME;
#endif

    if ((ptr->fd=open(fname, O_RDWR | O_NOCTTY)) < 0)
    {
        sio_errno = errno;
        return SIO_OPENFAIL;
    }
	/* bugfix for sending break when startup under 485-2W */
    inter = Scf_getIfType(port);
    sio_setiftype(port, inter);
#ifndef CROSS
    uart_buffer_size = 2048;
#else
    ioctl(ptr->fd, MOXA_GET_WRITEROOM, &uart_buffer_size);
#endif


    fcntl(ptr->fd, F_SETFL, FNDELAY);       /* Set nonblocking mode */

    /*
     * Get the current options for the port...
     */
    tcgetattr(ptr->fd, &options);

    options.c_lflag = 0;
    options.c_iflag = 0;
    options.c_oflag = 0;

    /*
     * Choosing Noncanonical Input
     */
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /*
     * Enable the receiver and set local mode...
     */
     options.c_cflag |= (CLOCAL | CREAD);

#ifdef SUPPORT_CONNECT_GOESDOWN
    /*
     * Disable the flag to support Realcom's "Connection goes down"
     */     
     options.c_cflag &= ~HUPCL;
#endif
     
    /*
     * Setting software flow control characters
     */
    options.c_cc[VSTART]    = 0x11;     /* DC1 */
    options.c_cc[VSTOP]     = 0x13;     /* DC3 */


    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 0;

    /*
     * Set the new options for the port...
     */
    tcsetattr(ptr->fd, TCSANOW, &options);

#if 0
    // keep old setting
    /* change default serial parameter */
    {
        int baud, mode, flowctrl;

        Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
        sio_ioctl(port, baud, mode );

        sio_flowctrl(port, _sio_mapFlowCtrl(flowctrl));
    }
#endif
    //sio_reset_count(port);
    return ptr->fd;
}
#ifdef SUPPORT_SERCMD
int	check_trigger = 0;
#endif
int sio_close(int port)
{
    PortInfo_t ptr;
    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;

    if( ptr->fd < 0 )
        return SIO_BADPORT;

    if( close(ptr->fd) == -1 )
        return SIO_BADPORT;
    ptr->fd = -1;
#ifdef SUPPORT_SERCMD
    check_trigger = 0;
#endif
    return SIO_OK;
}

int sio_ioctl(int port, int baud, int mode)
{
    PortInfo_t ptr;
    struct termios options;
    long baudrate;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
//    if( baud < BAUD_50 || baud > BAUD_921600 )
//        return SIO_BADPARM;
    if( mode < 0 || mode > (BIT_8 | STOP_2 | P_SPC) )
        return SIO_BADPARM;

    /*
     * Get the current options for the port...
     */
    tcgetattr(ptr->fd, &options);
    printf("+sio_ioctl(), c_cflag=0x%x\n", options.c_cflag);
    /*
     * Set the baud rates
     */

    switch( baud )
    {
        case BAUD_50:       baudrate = B50;     break;
        case BAUD_75:       baudrate = B75;     break;
        case BAUD_110:      baudrate = B110;    break;
        case BAUD_134:      baudrate = B134;    break;
        case BAUD_150:      baudrate = B150;    break;
        case BAUD_300:      baudrate = B300;    break;
        case BAUD_600:      baudrate = B600;    break;
        case BAUD_1200:     baudrate = B1200;   break;
        case BAUD_1800:     baudrate = B1800;   break;
        case BAUD_2400:     baudrate = B2400;   break;
        case BAUD_4800:     baudrate = B4800;   break;
//        case BAUD_7200:     baudrate = B7200;   break;
        case BAUD_9600:     baudrate = B9600;   break;
        case BAUD_19200:    baudrate = B19200;  break;
        case BAUD_38400:    baudrate = B38400;  break;
        case BAUD_57600:    baudrate = B57600;  break;
        case BAUD_115200:   baudrate = B115200; break;
        case BAUD_230400:   baudrate = B230400; break;
        case BAUD_460800:   baudrate = B460800; break;
        case BAUD_921600:   baudrate = B921600; break;
		case BAUD_7200: 	baud     = 7200;		    
		/* termios.h doesn't have B7200. So we set baudrate 7200 by ioctl("MOXA_SET_SPECIAL_BAUD_RATE")*/
        default:            baudrate = -1;  	break;	/* any baudrate */
    }
	if (baudrate > 0)
	{
	    cfsetispeed(&options, baudrate);
	    cfsetospeed(&options, baudrate);
    }

    /*
     * Set the data bits
     */
    options.c_cflag &= ~CSIZE;
    switch( mode & 0x03 )
    {
        case BIT_5: options.c_cflag |= CS5;     break;
        case BIT_6: options.c_cflag |= CS6;     break;
        case BIT_7: options.c_cflag |= CS7;     break;
        case BIT_8: options.c_cflag |= CS8;     break;
        default:    options.c_cflag |= CS8;     break;
    }

    /*
     * Set the stop bits
     */
    switch( mode & 0x04 )
    {
        case STOP_1:    options.c_cflag &= ~CSTOPB; break;
        case STOP_2:    options.c_cflag |= CSTOPB;  break;
        default:        options.c_cflag |= CSTOPB;  break;
    }

    /*
     * Set the parity
     */
#ifdef __CYGWIN__
    #define CMSPAR 0
#endif // __CYGWIN__
    options.c_cflag &= ~(PARENB | PARODD | CMSPAR); //| INPCK | ISTRIP);
    switch( mode & 0x38 )
    {
        case P_EVEN:
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            break;
        case P_ODD:
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            break;
        case P_SPC:
            options.c_cflag |= PARENB;
            options.c_cflag &= ~PARODD;
            options.c_cflag |= CMSPAR;
            break;
        case P_MRK:
            options.c_cflag |= PARENB;
            options.c_cflag |= PARODD;
            options.c_cflag |= CMSPAR;
            break;
        case P_NONE:
        default:
            options.c_cflag &= ~PARENB;
            break;
    }

    /*
     * Set the new options for the port...
     */
    printf("-sio_ioctl(), c_cflag=0x%x\n", options.c_cflag);
    tcsetattr(ptr->fd, TCSANOW, &options);

	if (baudrate == -1)	/* set any baudrate */
		sio_baud(port, baud);
    
    return SIO_OK;
}

/* remap flow control value from config to sio */
int _sio_mapFlowCtrl(int flowctrl)
{
    int flowmode;

    switch (flowctrl)
    {
        case 1:        /* RTS/CTS */
            flowmode = F_HW;
            break;
        case 2:        /* XON/XOFF */
            flowmode = F_SW;
            break;
        case 0:        /* None */
        default:
            flowmode = 0x00;
            break;
     }

    return flowmode;
}

int sio_flowctrl(int port, int mode)
{
    PortInfo_t ptr;
    struct termios options;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;

    if( ptr->fd < 0 )
        return SIO_BADPORT;

    if( mode < 0 || mode > 0x0F )
        return SIO_BADPARM;
    /*
     * Get the current options for the port...
     */
    tcgetattr(ptr->fd, &options);

printf("CRTSCTS = [0x%x], c_cflag=0x%x\n", CRTSCTS, options.c_cflag);
    /*
     * Disable hardware/software flow control
     */
    options.c_cflag &= ~CRTSCTS;
    options.c_iflag &= ~(IXON | IXOFF | IXANY);

    if( mode & F_HW )
        options.c_cflag |= CRTSCTS;

    if( mode & F_SW )
        options.c_iflag |= (IXON | IXOFF);

    /*
     * Set the new options for the port...
     */
    printf("sio_flowctrl(), c_cflag=0x%x\n", options.c_cflag);
    tcsetattr(ptr->fd, TCSANOW, &options);
    return SIO_OK;
}


int sio_flush(int port, int func)
{
    PortInfo_t ptr;
    int queue_selector = 0;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( func < 0 || func > 0x02 )
        return SIO_BADPARM;
    switch( func )
    {
        case FLUSH_RX:  queue_selector = TCIFLUSH;  break;
        case FLUSH_TX:  queue_selector = TCOFLUSH;  break;
        case FLUSH_ALL: queue_selector = TCIOFLUSH; break;
    }

    tcflush(ptr->fd, queue_selector);
    return SIO_OK;
}

int sio_DTR(int port, int mode)
{
    PortInfo_t ptr;
    int status;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( mode < 0 || mode > (C_DTR | C_RTS) )
        return SIO_BADPARM;

    ioctl(ptr->fd, TIOCMGET, &status);

    status &= ~TIOCM_DTR;
    if( mode )
        status |= TIOCM_DTR;
    else
        status &= ~(TIOCM_DTR);

    ioctl(ptr->fd, TIOCMSET, &status);

    return SIO_OK;
}

int sio_getDTR(int port)
{
    PortInfo_t ptr;
    int status;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, TIOCMGET, &status);

    return !!(status&TIOCM_DTR);
}

int sio_RTS(int port, int mode)
{
    PortInfo_t ptr;
    int status;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( mode < 0 || mode > (C_DTR | C_RTS) )
        return SIO_BADPARM;

    ioctl(ptr->fd, TIOCMGET, &status);

    status &= ~TIOCM_RTS;
    if( mode )
        status |= TIOCM_RTS;
    else
        status &= ~(TIOCM_RTS);

    ioctl(ptr->fd, TIOCMSET, &status);

    return SIO_OK;
}

int sio_getRTS(int port)
{
    PortInfo_t ptr;
    int status;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, TIOCMGET, &status);

    return !!(status&TIOCM_RTS);
}


int sio_lctrl(int port, int mode)
{
    PortInfo_t ptr;
    int status;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( mode < 0 || mode > (C_DTR | C_RTS) )
        return SIO_BADPARM;

    ioctl(ptr->fd, TIOCMGET, &status);

    status &= ~TIOCM_DTR;
    status &= ~TIOCM_RTS;
    if( mode & C_DTR )
        status |= TIOCM_DTR;
    if( mode & C_RTS )
        status |= TIOCM_RTS;

    ioctl(ptr->fd, TIOCMSET, &status);

    return SIO_OK;
}
/* 2012/08/10 Develop for RealCOM Mode Win32API ActXon/ActXoff */
int sio_setxon(int port) /* actxon for win32 API */
{
	int ret;
	PortInfo_t ptr;
	
    	if ((ptr = getPort(port)) == NULL)
        	return SIO_BADPORT;
    	if (ptr->fd < 0)
        	return SIO_BADPORT;

    	ret = ioctl(ptr->fd, MOXA_ACT_XON_MU150);

	return SIO_OK;
}

int sio_setxoff(int port) /* actxoff for win32 API */
{
	int ret;
	PortInfo_t ptr;

    	if ((ptr = getPort(port)) == NULL)
        	return SIO_BADPORT;
    	if (ptr->fd < 0)
        	return SIO_BADPORT;

    	ret = ioctl(ptr->fd, MOXA_ACT_XOFF_MU150);

	return SIO_OK;
}

int sio_setxonxoff(int port, unsigned char xon, unsigned char xoff)
{
    PortInfo_t ptr;
    struct termios options;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    /*
     * Get the current options for the port...
     */
    tcgetattr(ptr->fd, &options);

    /*
     * Setting software flow control characters
     */
    options.c_cc[VSTART]    = xon;
    options.c_cc[VSTOP]     = xoff;

    /*
     * Set the new options for the port...
     */
    tcsetattr(ptr->fd, TCSANOW, &options);

    return SIO_OK;
}


int sio_baud(int port, long speed)
{
    PortInfo_t ptr;
    speed_t    baudrate;
    struct termios options;

//printf("sio_baud(), speed = %ld\n", speed);
    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    switch( speed )
    {
        case 50L:      baudrate = B50;     break;
        case 75:       baudrate = B75;     break;
        case 110:      baudrate = B110;    break;
        case 134:      baudrate = B134;    break;
        case 150:      baudrate = B150;    break;
        case 300:      baudrate = B300;    break;
        case 600:      baudrate = B600;    break;
        case 1200:     baudrate = B1200;   break;
        case 1800:     baudrate = B1800;   break;
        case 2400:     baudrate = B2400;   break;
        case 4800:     baudrate = B4800;   break;
        //case 7200:     baudrate = B7200;   break;
        case 9600:     baudrate = B9600;   break;
        case 19200:    baudrate = B19200;  break;
        case 38400:    baudrate = B38400;  break;
        case 57600:    baudrate = B57600;  break;
        case 115200:   baudrate = B115200; break;
        case 230400:   baudrate = B230400; break;
        case 460800:   baudrate = B460800; break;
        case 921600:   baudrate = B921600; break;
        default:       baudrate = -1;      break; /* any baudrate */
    }

	if (baudrate == -1)
	{
		if (50 <= speed && speed <= 921600)
			ioctl(ptr->fd, MOXA_SET_SPECIAL_BAUD_RATE, &speed);
	}
	else
	{
	    tcgetattr(ptr->fd, &options);

	    /*
	     * Set the baud rates
	     */
	    cfsetispeed(&options, baudrate);
	    cfsetospeed(&options, baudrate);

	    tcsetattr(ptr->fd, TCSANOW, &options);
    }

    return SIO_OK;
}


int sio_getch(int port)
{
    char ch;
    sio_read(port, &ch, 1);
    return (int) ch;
}

#ifdef SUPPORT_SERCMD
int Gsio_alarm = 0;
void sio_alarm_handler (int a)
{
	Gsio_alarm = 1;
	setitimer(ITIMER_REAL, NULL, NULL);
}
int sio_read(int port, char *buf, int len)
{
	PortInfo_t ptr;
    	int n, i;
	static u_char ch[3];
	static int trigger;
	static int cnt = 0;
//	static int check_trigger = 0;
	long ret;
	int k;
	struct itimerval t;

	t.it_interval.tv_usec = 100000;
	t.it_interval.tv_sec = 0;
	t.it_value.tv_usec = 100000;
	t.it_value.tv_sec = 0;
	ret = 0;

//	signal(SIGALRM, sio_alarm_handler );
//	setitimer(ITIMER_REAL, &t, NULL);

	if (!check_trigger) {
		trigger = Scf_getScmTrigger();
				Scf_getScmChar(&ch[0], &ch[1], &ch[2]);
		check_trigger = 1;
	}
	if (trigger == 2) { /* SW_TRIGGER */
//		Scf_getScmChar(&ch[0], &ch[1], &ch[2]);
		if( (ptr = getPort(port)) == NULL )
        		return SIO_BADPORT;
		if( ptr->fd < 0 )
        		return SIO_BADPORT;
	       if( buf == NULL || len < 0 )
        		return SIO_BADPARM;

		signal(SIGALRM, sio_alarm_handler );
		setitimer(ITIMER_REAL, &t, NULL);

	       n = read(ptr->fd, buf, len);
		k = n;
		if (Gsio_alarm == 1) {
			cnt = 0;
			Gsio_alarm = 0;
		}
    		if( n < 0 )
    		{
		       if( errno == EAGAIN )
            		return 0;
		       sio_errno = errno;
		       return SIO_WRITETIMEOUT;
    		}
		for (i = 0; i < n; i ++) {
			if (buf[i] == ch[0] && cnt == 0)  {
				cnt ++;
			} else if (buf[i] == ch[1] && cnt == 1) {
				cnt ++;
			} else if (buf[i] == ch[2] && cnt == 2) {
				cnt ++;

				kill(sys_get_pid(port, DSPORTD_PID_FILE), SIGUSR1);
				return 0;
			}
			else {
				cnt = 0;
			}
		}
#if 0
		if (cnt) {
			ret = sio_iqueue(port);
			if (cnt == 1 && ret >= 2) {
				n += read(ptr->fd, &buf[n], 2);
				if (buf[k + 1] == ch[1] && buf[k + 2] == ch[2]) {
				kill(sys_get_pid(port, DSPORTD_PID_FILE), SIGUSR1);
					return 0;
				}
			} else if (cnt == 2 && ret >= 1) {
				n += read(ptr->fd, &buf[n], 1);
				if (buf[k + 1] == ch[2]) {
				kill(sys_get_pid(port, DSPORTD_PID_FILE), SIGUSR1);
					return 0;
				}
			}
		}
#endif
	}
	else { /* sio_read original */
	    if( (ptr = getPort(port)) == NULL )
	        return SIO_BADPORT;
	    if( ptr->fd < 0 )
	        return SIO_BADPORT;
	    if( buf == NULL || len < 0 )
	        return SIO_BADPARM;
	    n = read(ptr->fd, buf, len);
	    if( n < 0 )
	    {
	        if( errno == EAGAIN )
	            return 0;
	        sio_errno = errno;
	        return SIO_WRITETIMEOUT;
	    }
	    return n;
	}

       return n;
}

int sio_read_ex(int port, char *buf, int len)
{
    PortInfo_t ptr;
    int n;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( buf == NULL || len < 0 )
        return SIO_BADPARM;
    n = read(ptr->fd, buf, len);
    if( n < 0 )
    {
        if( errno == EAGAIN )
            return 0;
        sio_errno = errno;
        return SIO_WRITETIMEOUT;
    }

    return n;
}

#else
//static int Gcnt = 0;
//#define __SCHE_BURNIN_DEBUG 
#ifdef __SCHE_BURNIN_DEBUG
static int Gptrb = 0;
static int burn1_ptr = 0;
static int burn2_ptr = 0;
static char burn1[64] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABC"};
static char burn2[64] = {"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabc"};
#endif
int sio_read(int port, char *buf, int len)
{
    PortInfo_t ptr;
    int n;
#ifdef __SCHE_BURNIN_DEBUG
    int i = 0;
#endif
    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( buf == NULL || len < 0 )
        return SIO_BADPARM;
    n = read(ptr->fd, buf, len);
    if( n < 0 )
    {
        if( errno == EAGAIN )
            return SIO_EAGAIN;
        if( errno == EWOULDBLOCK )
            return SIO_EWOULDBLOCK;
        sio_errno = errno;
        errno = 0;
        return SIO_WRITETIMEOUT;
    } else {
#ifdef __SCHE_BURNIN_DEBUG    
    	if (port == 2) {
		for (i = 0; i < n; i++) {
			if (Gptrb == 0) {
				if (burn1_ptr < 55) {
					if (buf[i] != burn1[burn1_ptr]) {
						printf("Err1 %c,%c, ptr=%d\r\n", buf[i], burn1[burn1_ptr],burn1_ptr);
					}
				}
				burn1_ptr++;
				if (burn1_ptr == 64) {
					burn1_ptr = 0;
					Gptrb = 1;
				}

			} else {
				if (burn2_ptr < 55) {
					if (buf[i] != burn2[burn2_ptr]) {
						printf("Err2 %c,%c, ptr=%d\r\n", buf[i], burn2[burn2_ptr],burn2_ptr);
					}
				}
				burn2_ptr++;
				if (burn2_ptr == 64) {
					burn2_ptr = 0;		
					Gptrb = 0;
				}
			}
		}
    	}
#endif		
    }
    return n;
}
#endif

int sio_read_timeout(int port, char* buf, int len, int timeout_ms)
{
#if 1
    static int count=0;
    static unsigned long last_ms=0;
    static int sleep_flag;

    if((sys_clock_ms() - last_ms) > 1000)
    {
        if(count > 10)
            sleep_flag = 2;
        else if(sleep_flag > 0)
            sleep_flag--;
        count = 0;
        last_ms = sys_clock_ms();
    }
    count++;
    if((sleep_flag > 0)
        && (sio_iqueue(port) < 1000) )
        usleep(timeout_ms*1000);
    return sio_read(port, buf, len);
#else
    unsigned long t1;
    int recvd;

    if(timeout_ms <= 0)
        return sio_read(port, buf, len);

    t1 = sys_clock_ms() + timeout_ms;
    recvd = 0;
    do {
        recvd += sio_read(port, &buf[recvd], len-recvd);
        if(recvd == len)
            break;
        usleep(100);
    }while((sys_clock_ms() <= t1));

    return recvd;
#endif
}

int sio_putch(int port, int term)
{
    return sio_write(port, (char *) &term, 1);
}

int sio_write(int port, char *buf, int len)
{
    PortInfo_t ptr;
    int n;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;
    if( buf == NULL || len < 0 )
        return SIO_BADPARM;
    n = write(ptr->fd, buf, len);

    if( n < 0 )
    {
        if( errno == EAGAIN )
            return SIO_EAGAIN;
        if( errno == EWOULDBLOCK )
            return SIO_EWOULDBLOCK;
        sio_errno = errno;
        errno = 0;
        return SIO_WRITETIMEOUT;
    }
    return n;
}


int sio_lstatus(int port)
{
    PortInfo_t ptr;
    int status;
    int result;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, TIOCMGET, &status);
    result = 0;
    if( status & TIOCM_DSR )    /* DSR */
        result |= S_DSR;
    if( status & TIOCM_CTS )    /* CTS */
        result |= S_CTS;
    if( status & TIOCM_RNG )    /* RI */
        result |= S_RI;
    if( status & TIOCM_CD )     /* DCD */
        result |= S_CD;

    return result;
}

long sio_iqueue(int port)
{
    PortInfo_t ptr;
    int bytes;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, FIONREAD, &bytes);

    return (long) bytes;
}

long sio_oqueue(int port)
{
#ifndef CROSS
    return 1024;
#else
    PortInfo_t ptr;
    int bytes;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd <= 0 ) /* bugfix: the initial fd is zero, but it's error */
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_OQUEUE, &bytes);
//printf("sio_oqueue(), get %d bytes\n", bytes);
    return (long)bytes;
#endif // CROSS
}


long sio_getbaud(int port)
{
#if 1
    SIODATA data;
    sio_getsiodata(port, &data);
    return data.baudrate;

#else
    PortInfo_t ptr;
    long result;
    speed_t speed;
    struct termios options;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    /*
     * Get the current options for the port...
     */
    tcgetattr(ptr->fd, &options);

    /*
     * Get the baud rates
     */
    speed = cfgetispeed(&options);
    speed = cfgetospeed(&options);
    switch( speed )
    {
        case B0:        result = 0;         break;
        case B50:       result = 50;        break;
        case B75:       result = 75;        break;
        case B110:      result = 110;       break;
        case B134:      result = 134;       break;
        case B150:      result = 150;       break;
        case B300:      result = 300;       break;
        case B600:      result = 600;       break;
        case B1200:     result = 1200;      break;
        case B1800:     result = 1800;      break;
        case B2400:     result = 2400;      break;
        case B4800:     result = 4800;      break;
        case B9600:     result = 9600;      break;
        case B19200:    result = 19200;     break;
        case B38400:    result = 38400;     break;
        case B57600:    result = 57600;     break;
        case B115200:   result = 115200;    break;
        case B230400:   result = 230400;    break;
        case B460800:   result = 460800;    break;
        case B921600:   result = 921600;    break;
        default:        result = 0;         break;
    }
    return result;
#endif
}


unsigned int sio_getmode(int port)
{
    SIODATA data;
    sio_getsiodata(port, &data);
    return data.charmode;
}

unsigned int sio_getflow(int port)
{
    SIODATA data;
    sio_getsiodata(port, &data);
    return data.flowctrl;
}

#ifdef SUPPORT_EXTERNAL_UART
struct __mxser_mon {
    unsigned long txcnt;
    unsigned long rxcnt;
    unsigned long up_rxcnt;
    unsigned long up_txcnt;
    int modem_status;
    unsigned char hold_reason;
};
typedef struct __mxser_mon _mxser_mon;

static int __sio_external_notify(int port, SIODATA* siodata)
{
    /*
     *      mon_data.rx_cnt         rx count
     *      mon_data.tx_cnt         tx count
     *      mon_data.modem_status   UART Modem Status Register
     *      mon_data.hold_reason    0x01: bit 0 on - Tx hold by CTS low
     *                              0x02: bit 1 on - Tx hold by DSR low
     *                              0x08: bit 3 on - Tx hold by Xoff received
     *                              0x10: bit 4 on - Xoff Sent
     */
    _mxser_mon mon_data;
    PortInfo_t ptr;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_MON, &mon_data);

    memset(siodata, 0, sizeof(SIODATA));

    siodata->tx_cnt = mon_data.txcnt;
    siodata->rx_cnt = mon_data.rxcnt;
    siodata->up_txcnt = mon_data.up_txcnt;
    siodata->up_rxcnt = mon_data.up_rxcnt;
    siodata->lstatus = (mon_data.modem_status>>4)&0x0F;
    return (int)mon_data.hold_reason;
}
#endif //SUPPORT_EXTERNAL_UART

#ifdef SUPPORT_INTERNAL_UART
static int __sio_internal_notify(int port, SIODATA *siodata)
{
    char hold_reason=0;
    int fd;
    PortInfo_t ptr;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;

    fd = ptr->fd;

    if(fd >= 0)
        ioctl(fd, MOXA_MON, &hold_reason);

    sio_getsiodata(port, siodata);

    return hold_reason;
}
#endif // SUPPORT_INTERNAL_UART

static int _sio_notify(int port, SIODATA *siodata)
{
    _port_map_t map;
    map = _getPortMap(port);

    switch(map->flag)
    {
#ifdef SUPPORT_INTERNAL_UART
    case SERIAL_PORT_INTERNAL_FLAG:
        return __sio_internal_notify(port, siodata);
#endif // SUPPORT_INTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
    case SERIAL_PORT_EXTERNAL_FLAG:
        return __sio_external_notify(port, siodata);
#endif //SUPPORT_EXTERNAL_UART
    }
    return -1;
}

int sio_notify_status(int port, int *msr, int *hold)
{
    SIODATA data;
    int m = 0;

    *hold = _sio_notify(port, &data);
    if(*hold < 0)
        return SIO_BADPORT;

    //m = 0x01;
    if (data.lstatus & S_CTS)
        m |= 0x10;
    if (data.lstatus & S_DSR)
        m |= 0x20;
    if (data.lstatus & S_RI)
        m |= 0x40;
    if (data.lstatus & S_CD)
        m |= 0x80;

    *msr = m;
    return SIO_OK;
}



int sio_notify_error(int port)
{
    /*
     * Return   = 0         no error happened
     *          > 0         0x01: bit 0 on - parity error
     *                      0x02: bit 1 on - framing error
     *                      0x04: bit 2 on - overrun error (hardware)
     *                      0x08: bit 3 on - overflow error (software)
     *                      0x10: bit 4 on - break signal
     */

    PortInfo_t ptr;
    unsigned char err_shadow=0;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_LSTATUS, &err_shadow);
    return (int) err_shadow;
}


/*
 * mode = 1 : On
 *      = 0 : Off
 */
int sio_break(int port, int mode)
{
    PortInfo_t ptr;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    if( mode )
        ioctl(ptr->fd, TIOCSBRK, NULL);
    else
        ioctl(ptr->fd, TIOCCBRK, NULL);
    return SIO_OK;
}

/*
 * mode = 1     : Enable FIFO
 *      = 0     : Disable FIFO
 */
int sio_fifo(int port, int mode)
{
#ifdef __CYGWIN__
    printf("Cygwin not support setting fifo: mode = %d\n", mode);
    return SIO_OK;
#else
#ifdef SUPPORT_INTERNAL_UART
    PortInfo_t ptr;
    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_SET_FIFO, &mode);

    return SIO_OK;
#endif
#ifdef SUPPORT_EXTERNAL_UART
    PortInfo_t ptr;
    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_SET_FIFO_MU150, &mode);

    return SIO_OK;

#endif
#endif // __CYGWIN__
}

int sio_xon(int port)
{
    return SIO_OK;
}

int sio_xoff(int port)
{
    return SIO_OK;
}


int sio_getiftype(int port)
{
    PortInfo_t ptr;
    int iftype;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_GET_OP_MODE, &iftype);
    return iftype;
}

int sio_setiftype(int port, int iftype)
{
    PortInfo_t ptr;

    if( (ptr = getPort(port)) == NULL )
        return SIO_BADPORT;
    if( ptr->fd < 0 )
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_SET_OP_MODE, &iftype);
    return SIO_OK;
}

#ifdef SUPPORT_INTERNAL_UART
static int _sio_getInternalSioData(int portno, SIODATA* siodata)
{
    FILE *fp;
    char line[256];
    char *token;
    unsigned int c_cflag=0, c_iflag=0;
    unsigned int charmode=0;
    unsigned int flowctrl=0;
    unsigned int lstatus=0;

    fp = fopen(SERIAL_PROC_FILE, "r");
    if (fp == NULL)
        return CFG_FILE_NOT_FOUND;


    while(fgets(line, sizeof(line), fp))
    {
        if ((int)line[0]<0x30 || (int)line[0]>0x39)
        {
            continue;
        }

        //if (port != (atoi(line)+1))
        //    continue;
        if(portno != atoi(line))
            continue;

        // parse modem status before calling strtok.
        // strtok will change the line content.
        if (strstr(line, "CTS"))
            lstatus |= S_CTS;

        if (strstr(line, "DSR"))
            lstatus |= S_DSR;

        if (strstr(line, "RI"))
            lstatus |= S_RI;

        if (strstr(line, "CD"))
            lstatus |= S_CD;

        siodata->lstatus = lstatus;

        token = strtok(line, " ");
        while (token)
        {
            if (!strncmp("utx:", token, 4))
                siodata->up_txcnt = atoi(token+4);

            else if (!strncmp("urx:", token, 4))
                siodata->up_rxcnt = atoi(token+4);

            else if (!strncmp("tx:", token, 3))
                siodata->tx_cnt = atoi(token+3);

            else if (!strncmp("rx:", token, 3))
                siodata->rx_cnt = atoi(token+3);

            else if (!strncmp("cf:", token, 3))
                c_cflag = atoi(token+3);

            else if (!strncmp("if:", token, 3))
                c_iflag = atoi(token+3);

            else if (!strncmp("ospeed:", token, 7))
            {
                siodata->baudrate = atol(token+7);
	        }

            token = strtok(NULL, " ");
        }
        break;
    }

    if (c_cflag & CRTSCTS)
        flowctrl |= F_HW;
    if (c_iflag & (IXON | IXOFF))
        flowctrl |= F_SW;

    switch (c_cflag & CSIZE)
    {
        case CS7:
            charmode = BIT_7;
            break;
        case CS8:
        default:
            charmode = BIT_8;
            break;
    }

    if (c_cflag & CSTOPB)
        charmode |= STOP_2;


    if (c_cflag & PARENB)
    {
        if (c_cflag & PARODD)
            charmode |= P_ODD;
        else
            charmode |= P_EVEN;
    }

    siodata->flowctrl = flowctrl;
    siodata->charmode = charmode;
    fclose(fp);
    return SIO_OK;
}
#endif // SUPPORT_INTERNAL_UART

#ifdef SUPPORT_EXTERNAL_UART
struct mxser_mon_ext
{
    unsigned long rx_cnt[32];
    unsigned long tx_cnt[32];
    unsigned long up_rxcnt[32];
    unsigned long up_txcnt[32];
    int modem_status[32];

    long baudrate[32];
    int databits[32];
    int stopbits[32];
    int parity[32];
    int flowctrl[32];
    int fifo[32];
    int iftype[32];
};
static int _sio_getExternalSioData(int port, SIODATA *siodata)
{
    _port_map_t p;
    int fd;
    struct mxser_mon_ext mon_ext_data;
    int charmode=0;
    return SIO_OK;

    p = _getPortMap(port);

    //fd = open(__get_port_node_name(port), O_RDWR);
    fd = open(D_MONITOR_INFO_PORT, O_RDWR);
    if(fd < 0)
    {
        printf("%s open fail \r\n", D_MONITOR_INFO_PORT);
        return SIO_BADPORT;
    }

    ioctl(fd, MOXA_MON_EXT, &mon_ext_data);
    siodata->iftype = mon_ext_data.iftype[p->portno];
    //siodata->charmode = (mon_ext_data.databits[p->portno] | mon_ext_data.stopbits[p->portno] | mon_ext_data.parity[p->portno]);
    switch(mon_ext_data.databits[p->portno])
    {
    case CS5:
        charmode = BIT_5;
        break;
    case CS6:
        charmode = BIT_6;
        break;
    case CS7:
        charmode = BIT_7;
        break;
    case CS8:
        charmode = BIT_8;
        break;
    }
    if(mon_ext_data.stopbits[p->portno])
        charmode |= STOP_2;

    if (mon_ext_data.parity[p->portno] & PARENB)
    {
        if(mon_ext_data.parity[p->portno] & CMSPAR)
        {
            if(mon_ext_data.parity[p->portno] & PARODD)
                charmode |= P_SPC;
            else
                charmode |= P_MRK;
        }
        else
        {
            if (mon_ext_data.parity[p->portno] & PARODD)
                charmode |= P_ODD;
            else
                charmode |= P_EVEN;
        }
    }
    siodata->charmode = charmode;

    siodata->flowctrl = mon_ext_data.flowctrl[p->portno];
    siodata->lstatus = (mon_ext_data.modem_status[p->portno]>>4)&0x0F;
    siodata->baudrate = mon_ext_data.baudrate[p->portno];
    siodata->tx_cnt = mon_ext_data.tx_cnt[p->portno];
    siodata->rx_cnt = mon_ext_data.rx_cnt[p->portno];
    siodata->up_txcnt = mon_ext_data.up_txcnt[p->portno];
    siodata->up_rxcnt = mon_ext_data.up_rxcnt[p->portno];
    siodata->fifo = mon_ext_data.fifo[p->portno];

    close(fd);
    return SIO_OK;
}
#endif // SUPPORT_EXTERNAL_UART

int sio_getsiodata(int port, SIODATA *siodata)
{
    _port_map_t p;

    p = _getPortMap(port);
    switch(p->flag)
    {
#ifdef SUPPORT_INTERNAL_UART
    case SERIAL_PORT_INTERNAL_FLAG:
        return _sio_getInternalSioData(p->portno, siodata);
#endif // SUPPORT_INTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
    case SERIAL_PORT_EXTERNAL_FLAG:
        return _sio_getExternalSioData(port, siodata);
#endif // SUPPORT_EXTERNAL_UART
    }
    return SIO_BADPORT;
}

/* Some empty functions */
long sio_ofree(int port)
{
    long len;
    len = (long)uart_buffer_size - sio_oqueue(port);
    return len;
}

//int sio_breakctl(int port, int flag) {   return 0; }


#ifdef SUPPORT_EXTERNAL_UART
static int _sio_getExtPortStatus(int port, _port_status_ext_t ext_status)
{
    int fd;
    return 0;


    fd = open(D_MONITOR_INFO_PORT, O_RDWR);
    if(fd < 0)
        return -1;

    ext_status->_config.portno = port -1;

    ioctl(fd, MOXA_MON_EXT_2, ext_status);

    close(fd);

    return 0;
}
#endif // SUPPORT_EXTERNAL_UART

int sio_getStatus(int port, _port_status_ext_t ext_status)
{
    _port_map_t p;

    p = _getPortMap(port);
    switch(p->flag)
    {
#ifdef SUPPORT_INTERNAL_UART
    case SERIAL_PORT_INTERNAL_FLAG:
        return 0;
#endif // SUPPORT_INTERNAL_UART
#ifdef SUPPORT_EXTERNAL_UART
    case SERIAL_PORT_EXTERNAL_FLAG:
        return _sio_getExtPortStatus(port, ext_status);
#endif // SUPPORT_EXTERNAL_UART
    }

    return SIO_BADPORT;
}

