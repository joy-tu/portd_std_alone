/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#include <zephyr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/select.h>
#if defined(CONFIG_NET_SOCKETS_POSIX_NAMES)
#include <net/socket.h>
#else
#include <posix/sys/socket.h>
#endif
#include <net/socket_uart.h>
#include <drivers/uart.h>
#include <sio/mx_sio.h>

typedef struct _sio_data sio_data_t;

struct _sio_data
{
    uint32_t iftype;
    uint32_t charmode;
    uint32_t flowctrl;
    uint32_t lstatus;
    uint32_t baudrate;
    uint32_t tx_cnt;
    uint32_t rx_cnt;
    uint32_t up_txcnt;
    uint32_t up_rxcnt;
    uint32_t fifo;
};

#ifdef CONFIG_SIO_INTERNAL_UART
#define SERIAL_PROC_FILE        "/proc/tty/driver/ttymxc"
#endif

#ifdef CONFIG_SIO_EXTERNAL_UART
#define D_MONITOR_INFO_PORT     "/dev/ttyM32"
#endif

#define SERIAL_PORT_INTERNAL_FLAG 1
#define SERIAL_PORT_EXTERNAL_FLAG 2

struct __port_map
{
    int flag;
    int portno;
};
typedef struct __port_map _port_map;
typedef struct __port_map* _port_map_t;
_port_map port_map[] =
{
#ifdef CONFIG_SIO_INTERNAL_UART
    { SERIAL_PORT_INTERNAL_FLAG, 0 }, // port = 1
#endif
#ifdef CONFIG_SIO_EXTERNAL_UART
    { SERIAL_PORT_EXTERNAL_FLAG, 0 }, // port = 2
    { SERIAL_PORT_EXTERNAL_FLAG, 1 }, // port = 3
    { SERIAL_PORT_EXTERNAL_FLAG, 2 }, // port = 4
    { SERIAL_PORT_EXTERNAL_FLAG, 3 }, // port = 5
#endif
};
#define NUMBER_OF_PORTS() (sizeof(port_map)/sizeof(_port_map))

struct _portinfo
{
    const struct device *dev;
    int fd;
    _port_map_t map;
};
typedef struct _portinfo PortInfo;

int sio_errno;

static PortInfo GPortInfo[NUMBER_OF_PORTS()];

static char *__get_port_device_name(int port)
{
    _port_map_t p;

    static char port_buf[32];

    port--;

    if ((port < 0) || (port >= NUMBER_OF_PORTS()))
        return NULL;

    p = &port_map[port];

    switch (p->flag)
    {
#ifdef CONFIG_SIO_INTERNAL_UART

        case SERIAL_PORT_INTERNAL_FLAG:
            sprintf(port_buf, "UART_%d", p->portno + 1);
            break;
#endif
#ifdef CONFIG_SIO_EXTERNAL_UART

        case SERIAL_PORT_EXTERNAL_FLAG:
            sprintf(port_buf, "/dev/ttyM%d", p->portno);
            break;
#endif

        default:
            return NULL;
    }

    return port_buf;
}

//
// Static functions.
//
static PortInfo *getPort(int port)
{
    port--;

    if ((port < 0) || (port >= NUMBER_OF_PORTS()))
        return NULL;

    return &GPortInfo[port];
}

#if 0
static _port_map_t _getPortMap(int port)
{
    port--;

    if ((port < 0) || (port >= NUMBER_OF_PORTS()))
        return NULL;

    return &port_map[port];
}
#endif

static inline uint32_t _sio_baud_to_bps(int baud)
{
    struct baud_map {
    	int	baud_idx;
    	int	baud_bps;
    };
    static struct baud_map baudmap[] = {
    	{ BAUD_50, 50 },
    	{ BAUD_75, 75 },
    	{ BAUD_110, 110 },
    	{ BAUD_134, 134 },
    	{ BAUD_150, 150 },
    	{ BAUD_300, 300 },
    	{ BAUD_600, 600 },
    	{ BAUD_1200, 1200 },
    	{ BAUD_1800, 1800 },
    	{ BAUD_2400, 2400 },
    	{ BAUD_4800, 4800 },
    	{ BAUD_9600, 9600 },
    	{ BAUD_19200, 19200 },
    	{ BAUD_38400, 38400 },
    	{ BAUD_57600, 57600 },
    	{ BAUD_115200, 115200 },
    	{ BAUD_230400, 230400 },
    	{ BAUD_460800, 460800 },
    	{ BAUD_921600, 921600 }
    };
    int index;
    uint32_t baud_bps = 0;

	for (index=0; index < ((sizeof(baudmap)/sizeof(baudmap[0]))); index++) {
		if (baud == baudmap[index].baud_idx) {
            baud_bps = baudmap[index].baud_bps;
		}
	}

    if ((baud_bps == 0) && /* not mapped to standard baudrate */
        (50 <= baud && baud <= 921600)) {
        baud_bps = baud; /* set any baudrate */
    }

    return baud_bps;
}

static sio_ret_t _sio_get_config(int port, struct uart_config *cfg)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL)
    {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0)
    {
        return SIO_BADPORT;
    }

    if (uart_config_get(pinfo->dev, cfg) != 0)
    {
        return SIO_BADPORT;
    }

    return SIO_OK;
}

int sio_open(int port)
{
#if 0
    	PortInfo *pinfo;
    char *devname;
	struct net_if *iface;
	struct sockaddr_uart uart_addr;
	int iftype, fd, ret;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    devname = __get_port_device_name(port);

    if (devname == NULL) {
        return SIO_BADPORT;
    }

    pinfo->dev = device_get_binding(devname);

    if (pinfo->dev == NULL) {
        return SIO_OPENFAIL;
    }

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(UART));
	if (!iface) {
		printk("No UART network interface found!");
		return SIO_OPENFAIL;
	}

	fd = socket(AF_UART, SOCK_RAW, UART_RAW);
	if (fd <= 0) {
		printk("Cannot create UART socket (errno=%d)", errno);
		return SIO_OPENFAIL;
	}

	uart_addr.uart_ifindex = net_if_get_by_iface(iface);
	uart_addr.uart_family = PF_UART;

	ret = bind(fd, (struct sockaddr *)&uart_addr, sizeof(uart_addr));
	if (ret < 0) {
		printk("Cannot bind UART socket (%d)", -errno);
		zsock_close(fd); /* close() not defined, so use native zsock_close() */
		return SIO_OPENFAIL;
	}

    pinfo->fd = fd;

    //Scf_getSerialIfType(port, &iftype);
    iftype = RS232_MODE;
    sio_set_iftype(port, iftype);

    fcntl(fd, F_SETFL, O_NONBLOCK);

    return pinfo->fd;
#endif
}

sio_ret_t sio_close(int port)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

    pinfo->fd = 0;

    return SIO_OK;
}

sio_ret_t sio_ioctl(int port, sio_baud_t baud, sio_mode_t mode)
{
    PortInfo *pinfo;
    struct uart_config uart_cfg;
    int ret;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }
printf("Joy %s-%d\r\n", __func__, __LINE__);
    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if (mode < 0 || mode > (BIT_8 | STOP_2 | P_SPC)) {
        return SIO_BADPARM;
    }
printf("Joy %s-%d\r\n", __func__, __LINE__);
    /*
     * Get the current options for the port...
     */
    ret = uart_config_get(pinfo->dev, &uart_cfg);
    if (ret != 0) {
        return SIO_BADPORT;
    }

    /*
     * Set the baud rates
     */

    uart_cfg.baudrate = _sio_baud_to_bps(baud);
printf("Joy %s-%d, baud=%d,%d\r\n", __func__, __LINE__, 
	baud, uart_cfg.baudrate);
    if (uart_cfg.baudrate == 0) {
         return SIO_BADPARM;
    }

    /*
     * Set the data bits
     */
printf("Joy %s-%d, mode=%x\r\n", __func__, __LINE__, mode & 0x03);
     
    switch (mode & 0x03)
    {
        case BIT_5:
            uart_cfg.data_bits = UART_CFG_DATA_BITS_5;
            break;
        case BIT_6:
            uart_cfg.data_bits = UART_CFG_DATA_BITS_6;
            break;
        case BIT_7:
            uart_cfg.data_bits = UART_CFG_DATA_BITS_7;
            break;
        case BIT_8:
        default:
            uart_cfg.data_bits = UART_CFG_DATA_BITS_8;
            break;
    }

    /*
     * Set the stop bits
     */
printf("Joy %s-%d, mode=%x\r\n", __func__, __LINE__, mode & 0x04);     
    switch (mode & 0x04)
    {
        case STOP_1:
            uart_cfg.stop_bits = UART_CFG_STOP_BITS_1;
            break;
        case STOP_2:
        default:
            uart_cfg.stop_bits = UART_CFG_STOP_BITS_2;
            break;
    }
printf("Joy %s-%d, mode=%x\r\n", __func__, __LINE__, mode & 0x38);     
    /*
     * Set the parity
     */

    switch (mode & 0x38)
    {
        case P_EVEN:
            uart_cfg.parity = UART_CFG_PARITY_EVEN;
            break;
        case P_ODD:
            uart_cfg.parity = UART_CFG_PARITY_ODD;
            break;
        case P_SPC:
            uart_cfg.parity = UART_CFG_PARITY_SPACE;
            break;
        case P_MRK:
            uart_cfg.parity = UART_CFG_PARITY_MARK;
            break;
        case P_NONE:
        default:
            uart_cfg.parity = UART_CFG_PARITY_NONE;
            break;
    }

    /*
     * Set the new options for the port...
     */
    ret = uart_configure(pinfo->dev, &uart_cfg);
    if (ret != 0) {
printf("Joy %s-%d, mode=%x\r\n", __func__, __LINE__, mode & 0x38);       
        return SIO_OUTCONTROL;
    }
printf("Joy %s-%d, mode=%x\r\n", __func__, __LINE__, mode & 0x38);       
    return SIO_OK;
}

sio_ret_t sio_flowctrl(int port, sio_flowctrl_t ctrl)
{
    PortInfo *pinfo;
    struct uart_config uart_cfg;
    int ret;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if (ctrl < 0 || ctrl > 0x0F) {
        return SIO_BADPARM;
    }

    /*
     * Get the current options for the port...
     */
    ret = uart_config_get(pinfo->dev, &uart_cfg);
    if (ret != 0) {
        return SIO_BADPORT;
    }

    /*
     * Configure hardware/software flow control
     */
     ctrl = 0;
printf("Joy %s-%d, flow=%x\r\n", __func__, __LINE__, ctrl);            
    if (ctrl & F_HW) {
        uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
    } else if (ctrl & F_SW) {
        uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_XON_XOFF;
    } else {
        uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    }
printf("Joy %s-%d, flow=%x\r\n", __func__, __LINE__, ctrl);            
    /*
     * Set the new options for the port...
     */
    ret = uart_configure(pinfo->dev, &uart_cfg);
    if (ret != 0) {
        return SIO_OUTCONTROL;
    }
printf("Joy %s-%d, flow=%x\r\n", __func__, __LINE__, ctrl);            

    return SIO_OK;
}

sio_ret_t sio_flush(int port, sio_flush_t flags)
{
    PortInfo *pinfo;
//    int queue_selector = 0;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

/*
    if (flags & FLUSH_RX)
    {
    }

    switch (FLUSH_RX)
    {
        case FLUSH_RX:
            queue_selector = TCIFLUSH;
            break;

        case FLUSH_TX:
            queue_selector = TCOFLUSH;
            break;

        case FLUSH_ALL:
            queue_selector = TCIOFLUSH;
            break;
    }
  
    n = recv(pinfo->fd, buf, len, 0);

    tcflush(ptr->fd, queue_selector);
*/
    return SIO_OK;
}

sio_ret_t sio_set_dtr(int port, sio_onoff_t on_off)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if (!(on_off == SIO_ON || on_off == SIO_OFF)) {
        return SIO_BADPARM;
    }

    if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_DTR, &on_off, sizeof(on_off)) != 0)
        return SIO_OUTCONTROL;

    return SIO_OK;
}

sio_onoff_t sio_get_dtr(int port)
{
    PortInfo *pinfo;
    int on_off;
    socklen_t optlen = sizeof(on_off);

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_ONOFF_ERR;
    }

    if (pinfo->fd <= 0) {
        return SIO_ONOFF_ERR;
    }

    if (getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_DTR, &on_off, &optlen) != 0)
        return SIO_ONOFF_ERR;

    return on_off;
}

sio_ret_t sio_set_rts(int port, sio_onoff_t on_off)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if (!(on_off == SIO_ON || on_off == SIO_OFF)) {
        return SIO_BADPARM;
    }

    if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_RTS, &on_off, sizeof(on_off)) != 0)
        return SIO_OUTCONTROL;

    return SIO_OK;
}

sio_onoff_t sio_get_rts(int port)
{
    PortInfo *pinfo;
    int on_off;
    socklen_t optlen = sizeof(on_off);

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_ONOFF_ERR;
    }

    if (pinfo->fd <= 0) {
        return SIO_ONOFF_ERR;
    }

    if (getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_RTS, &on_off, &optlen) != 0)
        return SIO_ONOFF_ERR;

    return on_off;
}

sio_ret_t sio_lctrl(int port, sio_lctrl_t ctrl)
{
    PortInfo *pinfo;
    int on_off;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

    if (ctrl < 0 || ctrl > (C_DTR | C_RTS))
        return SIO_BADPARM;

    on_off = (ctrl & C_DTR) ? 1 : 0;
    setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_DTR, &on_off, sizeof(on_off));

    on_off = (ctrl & C_RTS) ? 1 : 0;
    setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_RTS, &on_off, sizeof(on_off));

    return SIO_OK;
}

sio_ret_t sio_act_xon(int port)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd < 0) {
        return SIO_BADPORT;
    }

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_ACT_XON, NULL, 0) != 0)
	    return SIO_ONOFF_ERR;

    return SIO_OK;
}

sio_ret_t sio_act_xoff(int port)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd < 0) {
        return SIO_BADPORT;
    }

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_ACT_XOFF, NULL, 0) != 0)
	    return SIO_ONOFF_ERR;

    return SIO_OK;
}

sio_ret_t sio_set_xonxoff(int port, uint8_t xon, uint8_t xoff)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd < 0) {
        return SIO_BADPORT;
    }

	setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_XON_CHAR, &xon, sizeof(xon));
	setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_XOFF_CHAR, &xoff, sizeof(xoff));

    return SIO_OK;
}

sio_ret_t sio_set_baud(int port, uint32_t baud)
{
    PortInfo *pinfo;
    struct uart_config uart_cfg;
    int ret;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if (baud == 0) {
        return SIO_BADPARM;
    }

    ret = uart_config_get(pinfo->dev, &uart_cfg);
    if (ret != 0) {
        return SIO_BADPORT;
    }

    /*
     * Set the baud rates
     */
    uart_cfg.baudrate = baud;

    ret = uart_configure(pinfo->dev, &uart_cfg);
    if (ret != 0) {
        return SIO_OUTCONTROL;
    }

    return SIO_OK;
}

int sio_getch(int port)
{
    ssize_t ret;
    char ch;

    ret = sio_read(port, &ch, 1);
    if (ret <= 0) {
        return (ret == 0) ? SIO_NODATA : ret;
    }

    return (int)ch;
}

ssize_t sio_read(int port, char *buf, size_t len)
{
    PortInfo *pinfo;
    int n;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if ((buf == NULL) || ((int)len < 0)) {
        return SIO_BADPARM;
    }

    n = recv(pinfo->fd, buf, len, 0);

    if (n < 0)
    {
        if (errno == EAGAIN)
            return 0;

        return SIO_NODATA;
    }

    return n;
}

ssize_t sio_read_timeout(int port, char *buf, size_t len, uint32_t timeout)
{
    PortInfo *pinfo;
	struct timeval tv;
	fd_set rset;
    int n;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

    if ((buf == NULL) || ((int)len < 0)) {
        return SIO_BADPARM;
    }

	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = timeout / 1000;
	tv.tv_usec = (timeout % 1000) * 1000;
	FD_ZERO(&rset);
    FD_SET(pinfo->fd, &rset);

	if (select(pinfo->fd + 1, &rset, NULL, NULL, &tv) > 0) {
    	if (FD_ISSET(pinfo->fd, &rset)) {
    	    n = recv(pinfo->fd, buf, len, 0);
            if ((n < 0) && (errno == EAGAIN)) {
                return 0;
            }
            return n;
        }
	}

	return 0;
}

sio_ret_t sio_putch(int port, char ch)
{
    return sio_write(port, &ch, 1);
}

ssize_t sio_write(int port, char *buf, size_t len)
{
    PortInfo *pinfo;
    int n;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd < 0)
        return SIO_BADPORT;

    if ((buf == NULL) || ((int)len < 0))
        return SIO_BADPARM;

    n = send(pinfo->fd, buf, len, 0);

    if (n < 0)
    {
        if (errno == EAGAIN)
            return SIO_EAGAIN;

        if (errno == EWOULDBLOCK)
            return SIO_EWOULDBLOCK;

        return SIO_WRITETIMEOUT;
    }

    return n;
}

sio_lstatus_t sio_lstatus(int port)
{
    PortInfo *pinfo;
    int status = 0;
    socklen_t optlen = sizeof(status);

    if ((pinfo = getPort(port)) == NULL)
        return SIO_LSTATUS_ERR;

    if (pinfo->fd <= 0)
        return SIO_LSTATUS_ERR;

    if (getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_LSTATUS, &status, &optlen) != 0)
        return SIO_LSTATUS_ERR;

    return (sio_lstatus_t)status;
}

ssize_t sio_iqueue(int port)
{
    PortInfo *pinfo;
    int bytes;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

//    ioctl(pinfo->fd, FIONREAD, &bytes);

    return (ssize_t)bytes;
}

ssize_t sio_oqueue(int port)
{
    PortInfo *pinfo;
    int bytes;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

//    ioctl(ptr->fd, MOXA_OQUEUE, &bytes);

    return (ssize_t)bytes;
}

int32_t sio_get_baud(int port)
{
    struct uart_config uart_cfg;
    sio_ret_t ret;

    ret = _sio_get_config(port, &uart_cfg);
    if (ret != SIO_OK)
    {
        return ret;
    }

    return (int32_t)uart_cfg.baudrate;
}

sio_mode_t sio_get_mode(int port)
{
    struct uart_config uart_cfg;
    sio_ret_t ret;
    sio_mode_t mode = 0;

    ret = _sio_get_config(port, &uart_cfg);
    if (ret != SIO_OK)
    {
        return SIO_MODE_ERR;
    }

    switch (uart_cfg.data_bits)
    {
        case UART_CFG_DATA_BITS_5:
            mode |= BIT_5;
            break;
        case UART_CFG_DATA_BITS_6:
            mode |= BIT_6;
            break;
        case UART_CFG_DATA_BITS_7:
            mode |= BIT_7;
            break;
        case UART_CFG_DATA_BITS_8:
            mode |= BIT_8;
            break;
    }

    switch (uart_cfg.stop_bits)
    {
        case UART_CFG_STOP_BITS_1:
            mode |= STOP_1;
            break;
        case UART_CFG_STOP_BITS_2:
            mode |= STOP_2;
            break;
    }

    switch (uart_cfg.parity)
    {
        case UART_CFG_PARITY_NONE:
            mode |= P_NONE;
            break;
        case UART_CFG_PARITY_ODD:
            mode |= P_ODD;
            break;
        case UART_CFG_PARITY_EVEN:
            mode |= P_EVEN;
            break;
        case UART_CFG_PARITY_MARK:
            mode |= P_MRK;
            break;
        case UART_CFG_PARITY_SPACE:
            mode |= P_SPC;
            break;
    }

    return mode;
}

sio_flowctrl_t sio_get_flow(int port)
{
    struct uart_config uart_cfg;
    sio_ret_t ret;
    sio_flowctrl_t flow;

    ret = _sio_get_config(port, &uart_cfg);
    if (ret != SIO_OK)
    {
        return SIO_FLOWCTRL_ERR;
    }

    switch (uart_cfg.flow_ctrl)
    {
        case UART_CFG_FLOW_CTRL_RTS_CTS:
            flow = F_HW;
            break;
        case UART_CFG_FLOW_CTRL_XON_XOFF:
            flow = F_SW;
            break;
        default:
            flow = F_NONE;
            break;
    }

    return flow;
}

#if 0
#ifdef CONFIG_SIO_EXTERNAL_UART
struct __mxser_mon
{
    uint32_t txcnt;
    uint32_t rxcnt;
    uint32_t up_rxcnt;
    uint32_t up_txcnt;
    uint32_t modem_status;
    sio_hold_t hold_reason;
};
typedef struct __mxser_mon _mxser_mon;

static sio_hold_t __sio_external_notify(int port, sio_data_t* siodata)
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
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

    ioctl(ptr->fd, MOXA_MON, &mon_data);

    memset(siodata, 0, sizeof(sio_data_t));

    siodata->tx_cnt = mon_data.txcnt;
    siodata->rx_cnt = mon_data.rxcnt;
    siodata->up_txcnt = mon_data.up_txcnt;
    siodata->up_rxcnt = mon_data.up_rxcnt;
    siodata->lstatus = (mon_data.modem_status >> 4) & 0x0F;

    return mon_data.hold_reason;
}
#endif //CONFIG_SIO_EXTERNAL_UART

#ifdef CONFIG_SIO_INTERNAL_UART
static sio_hold_t __sio_internal_notify(int port, sio_data_t *siodata)
{
    char hold_reason = 0;
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

//    ioctl(pinfo->fd, MOXA_MON, &hold_reason);

    sio_get_siodata(port, siodata);

    return hold_reason;
}
#endif // CONFIG_SIO_INTERNAL_UART

static sio_hold_t _sio_notify(int port, sio_data_t *siodata)
{
    _port_map_t map;
    map = _getPortMap(port);

    switch (map->flag)
    {
#ifdef CONFIG_SIO_INTERNAL_UART

        case SERIAL_PORT_INTERNAL_FLAG:
            return __sio_internal_notify(port, siodata);
#endif // CONFIG_SIO_INTERNAL_UART
#ifdef CONFIG_SIO_EXTERNAL_UART

        case SERIAL_PORT_EXTERNAL_FLAG:
            return __sio_external_notify(port, siodata);
#endif //CONFIG_SIO_EXTERNAL_UART
    }

    return -1;
}
#endif

sio_ret_t sio_notify_status(int port, sio_msr_t *msr, sio_hold_t *hold)
{
    PortInfo *pinfo;
    int optval;
    socklen_t optlen = sizeof(optval);
    sio_msr_t m = 0;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_LSTATUS_ERR;

    if (pinfo->fd <= 0)
        return SIO_LSTATUS_ERR;

    optval = 0;
    optlen = sizeof(optval);
    getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_HOLD_REASON, &optval, &optlen);
    *hold = (sio_hold_t)optval;

    optval = 0;
    optlen = sizeof(optval);
    getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_LSTATUS, &optval, &optlen);

    if (optval & S_CTS)
        m |= 0x10;

    if (optval & S_DSR)
        m |= 0x20;

    if (optval & S_RI)
        m |= 0x40;

    if (optval & S_CD)
        m |= 0x80;

    *msr = m;

    return SIO_OK;
}

sio_lerror_t sio_notify_error(int port)
{
    /*
     * Return   = 0         no error happened
     *          > 0         0x01: bit 0 on - parity error
     *                      0x02: bit 1 on - framing error
     *                      0x04: bit 2 on - overrun error (hardware)
     *                      0x08: bit 3 on - overflow error (software)
     *                      0x10: bit 4 on - break signal
     */

    PortInfo *pinfo;
    unsigned char err_shadow = 0;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_FLOWCTRL_ERR;

    if (pinfo->fd <= 0)
        return SIO_FLOWCTRL_ERR;

//    ioctl(pinfo->fd, MOXA_LSTATUS, &err_shadow);

    return (int)err_shadow;
}

sio_ret_t sio_break(int port, sio_onoff_t onoff)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
    {
        return SIO_BADPORT;
    }

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_BREAK, &onoff, sizeof(onoff)) != 0)
	    return SIO_ONOFF_ERR;

    return SIO_OK;
}

sio_ret_t sio_fifo(int port, bool enable)
{
    PortInfo *pinfo;
    int optval = enable;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
    {
        return SIO_BADPORT;
    }
    return SIO_OK;

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_FIFO, &optval, sizeof(optval)) != 0)
	    return SIO_OUTCONTROL;

    return SIO_OK;
}

sio_ret_t sio_rts_toggle(int port, bool enable, uint32_t rts_on_delay, uint32_t rts_off_delay)
{
    PortInfo *pinfo;
    int value;

    if ((pinfo = getPort(port)) == NULL)
        return SIO_BADPORT;

    if (pinfo->fd <= 0)
        return SIO_BADPORT;

    if ((rts_on_delay < RTS_ON_DELAY_MIN)  || (rts_on_delay > RTS_ON_DELAY_MAX) ||
            (rts_off_delay < RTS_OFF_DELAY_MIN) || (rts_off_delay > RTS_OFF_DELAY_MAX))
        return SIO_BADPARM;

    value = (rts_on_delay << 16) | rts_off_delay;

    if (enable)
    {
        value |= 0x80000000;
    }

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_RTS_TOGGLE, &value, sizeof(value)) != 0)
	    return SIO_OUTCONTROL;

    return SIO_OK;
}

sio_interface_t sio_get_iftype(int port)
{
    PortInfo *pinfo;
    int iftype;
    socklen_t optlen = sizeof(iftype);

    if ((pinfo = getPort(port)) == NULL)
        return SIO_INTERFACE_ERR;

    if (pinfo->fd < 0)
        return SIO_INTERFACE_ERR;

    if (getsockopt(pinfo->fd, SOL_UART_DRIVER, UART_GET_IF_TYPE, &iftype, &optlen) != 0)
        return SIO_ONOFF_ERR;

    return (sio_interface_t)iftype;
}

sio_ret_t sio_set_iftype(int port, sio_interface_t iftype)
{
    PortInfo *pinfo;

    if ((pinfo = getPort(port)) == NULL) {
        return SIO_BADPORT;
    }

    if (pinfo->fd <= 0) {
        return SIO_BADPORT;
    }

	if (setsockopt(pinfo->fd, SOL_UART_DRIVER, UART_SET_IF_TYPE, &iftype, sizeof(iftype)) != 0)
	    return SIO_OUTCONTROL;

    return SIO_OK;
}

#if 0
#ifdef CONFIG_SIO_INTERNAL_UART
static int _sio_getInternalSioData(int portno, SIODATA* siodata)
{
    FILE *fp;
    char line[256];
    char *token;
    unsigned int c_cflag = 0, c_iflag = 0;
    unsigned int charmode = 0;
    unsigned int flowctrl = 0;
    unsigned int lstatus = 0;

    fp = fopen(SERIAL_PROC_FILE, "r");

    if (fp == NULL)
        return CFG_FILE_NOT_FOUND;


    while (fgets(line, sizeof(line), fp))
    {
        if ((int)line[0] < 0x30 || (int)line[0] > 0x39)
        {
            continue;
        }

        //if (port != (atoi(line)+1))
        //    continue;
        if (portno != atoi(line))
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
                siodata->up_txcnt = atoi(token + 4);

            else if (!strncmp("urx:", token, 4))
                siodata->up_rxcnt = atoi(token + 4);

            else if (!strncmp("tx:", token, 3))
                siodata->tx_cnt = atoi(token + 3);

            else if (!strncmp("rx:", token, 3))
                siodata->rx_cnt = atoi(token + 3);

            else if (!strncmp("cf:", token, 3))
                c_cflag = atoi(token + 3);

            else if (!strncmp("if:", token, 3))
                c_iflag = atoi(token + 3);

            else if (!strncmp("ospeed:", token, 7))
            {
                siodata->baudrate = atol(token + 7);
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
#endif // CONFIG_SIO_INTERNAL_UART

#ifdef CONFIG_SIO_EXTERNAL_UART
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
    int charmode = 0;

    p = _getPortMap(port);

    //fd = open(__get_port_node_name(port), O_RDWR);
    fd = open(D_MONITOR_INFO_PORT, O_RDWR);

    if (fd < 0)
    {
        printf("%s open fail \r\n", D_MONITOR_INFO_PORT);
        return SIO_BADPORT;
    }

    ioctl(fd, MOXA_MON_EXT, &mon_ext_data);
    siodata->iftype = mon_ext_data.iftype[p->portno];

    //siodata->charmode = (mon_ext_data.databits[p->portno] | mon_ext_data.stopbits[p->portno] | mon_ext_data.parity[p->portno]);
    switch (mon_ext_data.databits[p->portno])
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

    if (mon_ext_data.stopbits[p->portno])
        charmode |= STOP_2;

    if (mon_ext_data.parity[p->portno] & PARENB)
    {
        if (mon_ext_data.parity[p->portno] & CMSPAR)
        {
            if (mon_ext_data.parity[p->portno] & PARODD)
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
    siodata->lstatus = (mon_ext_data.modem_status[p->portno] >> 4) & 0x0F;
    siodata->baudrate = mon_ext_data.baudrate[p->portno];
    siodata->tx_cnt = mon_ext_data.tx_cnt[p->portno];
    siodata->rx_cnt = mon_ext_data.rx_cnt[p->portno];
    siodata->up_txcnt = mon_ext_data.up_txcnt[p->portno];
    siodata->up_rxcnt = mon_ext_data.up_rxcnt[p->portno];
    siodata->fifo = mon_ext_data.fifo[p->portno];

    close(fd);
    return SIO_OK;
}
#endif // CONFIG_SIO_EXTERNAL_UART

static int32_t sio_get_siodata(int port, sio_data_t *sio_data)
{
    _port_map_t p;

    p = _getPortMap(port);

#if 0
    switch (p->flag)
    {
#ifdef CONFIG_SIO_INTERNAL_UART

        case SERIAL_PORT_INTERNAL_FLAG:
            return _sio_getInternalSioData(p->portno, siodata);
#endif // CONFIG_SIO_INTERNAL_UART
#ifdef CONFIG_SIO_EXTERNAL_UART

        case SERIAL_PORT_EXTERNAL_FLAG:
            return _sio_getExternalSioData(port, siodata);
#endif // CONFIG_SIO_EXTERNAL_UART
    }
#endif

    return SIO_BADPORT;
}
#endif

ssize_t sio_ofree(int port)
{
    ssize_t len;

    len = 0;
//    len = (long)uart_buffer_size - sio_oqueue(port);

    return len;
}
