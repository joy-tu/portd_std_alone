

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <arpa/inet.h>



#include <config.h>
#include <portd.h>
#include <sio.h>
#include <sysmaps.h>
#include <sysapi.h>
#include <systime.h>
#include <datalog.h>
#include <eventd.h>

#ifdef __CYGWIN__

typedef unsigned short INT16U;
#endif // __CUGWIN__

//#define _DEBUG
#ifdef _DEBUG
#define DBG  printf
#else
#define DBG(...)
#endif // _DEBUG

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned long u_long;

#define TMP_LEN     256

extern int portd_terminate;

#define D_STAT_INIT             0   /*! Initialize */
#define D_STAT_LISTEN           1   /*! On listan */
#define D_STAT_DIALOUT          2   /*! Do dialout */
#define D_STAT_DIALOUT_WAIT     3
#define D_STAT_RING             4   /*! Accept, show ring. */
#define D_STAT_WAIT_CONNECT     5   /*! Connecting */
#define D_STAT_CONNECTED        6   /*! Connect completed */
#define D_STAT_WAIT_CLOSE       7   /*! Do close */


#define D_EFLAG_CHANGED         0x0001
#define D_EFLAG_SAVE            0x0002
#define D_EFLAG_ASYNC_UP        0x0004
#define D_EFLAG_OFF_HOOK        0x0008
#define D_EFLAG_LOCAL_ECHO      0x0010
#define D_EFLAG_QUIET_CMD       0x0020
#define D_EFLAG_RSP_NUMBER      0x0040
#define D_EFLAG_DTE_DCD         0x0080
#define D_EFLAG_DTE_DTR         0x0100

#define D_RSP_OK            0x00
#define D_RSP_Connect       0x01
#define D_RSP_Ring          0x02
#define D_RSP_NoCarrier     0x03
#define D_RSP_Error         0x04
#define D_RSP_Nothing       0x10

#define D_EMODEM_REG_S0         0
#define D_EMODEM_REG_S1         1
#define D_EMODEM_REG_S2         2
#define D_EMODEM_REG_S3         3
#define D_EMODEM_REG_S4         4
#define D_EMODEM_REG_S5         5
#define D_EMODEM_REG_S6         6
#define D_EMODEM_REG_S7         7
#define D_EMODEM_REG_S8         8
#define D_EMODEM_REG_S9         9
#define D_EMODEM_REG_S10        10
#define D_EMODEM_REG_S11        11
#define D_EMODEM_REG_S12        12
#define D_EMODEM_REG_ECHO       13
#define D_EMODEM_REG_VERBOSE    14
#define D_EMODEM_REG_QUIET      15
#define D_EMODEM_REG_DTR        16
#define D_EMODEM_REG_DCD        17
#define D_EMODEM_REGS           18


static int emodem_stat;         /*! Store now socket status. */
static u_long emodem_lasttime;


static int emodem_sreg[DCF_EMODEM_REGS];
static int emodem_flag;
static int emodem_sreg_flash[DCF_EMODEM_REGS];
static int emodem_flag_flash;
static char atcmd_buf[60];
static int escape_len, atcmd_len;

static  u_long  emodem_dial_ip;
static  u_short emodem_dial_port;

static int data_mode;


void *emodem_start(void* portno);


#define D_ASYNC_ADTYPE_NONE     0	/* Async auto-disconnect : none */
#define D_ASYNC_ADTYPE_DCDOFF	2	/* Async auto-disconnect : DCD off */
#define D_ASYNC_ADTYPE_DSROFF	4	/* Async auto-disconnect : DSR off */

static int adtype=0;
static int Scfw_setAdtype(int port, int mode)
{
    adtype = mode;
    return 1;
}

static int sio_isup(int port)
{
    int lstatus;
    int mask;

    if(adtype == D_ASYNC_ADTYPE_DCDOFF)
    {
        mask = S_CD;
    }
    else if(adtype == D_ASYNC_ADTYPE_DSROFF)
    {
        mask = S_DSR;
    }
    else
        return 1;

	if( Scf_getIfType(port)!= 0x00 )
	{// isn't RS232 mode
		return 1;
	}

    lstatus = sio_lstatus(port);

    if(lstatus & mask)
        return 1;

    return 0;
}

static  int write_port(int fd, char *buf, int len)
{
    int n, cnt;
    u_long  t;

    t = sys_clock_ms();
    cnt = len;
    n = 0;
    while(cnt)
    {
        //n = send(fd, buf, cnt, 0);
        n = write(fd, buf, cnt);
        if(n > 0)
            cnt -= n;
        else if((sys_clock_ms() - t) > 2000)
            return(-1);
        else
            usleep(10000);
    }
    return(len);
}

static int emodem_open_serial(int port)
{
    int fd_port;
    int baud, mode, flowctrl;

    while((fd_port = sio_open(port)) < 0)
        sleep(1);

    sio_DTR(port, 0);             /* DTR off at init state */
    sio_RTS(port, 1);             /* RTS off at init state */
    sio_flush(port, FLUSH_ALL);

    sio_fifo(port, Scf_getAsyncFifo(port));		/* FIFO */
    Scf_getAsyncIoctl(port, &baud, &mode, &flowctrl);
    sio_ioctl(port, baud, mode);      /* Set baud rate, data bits, stop bits and parity */

    switch (flowctrl)
    {
    case 0:     /* None */
        flowctrl = F_NONE;
        break;
    case 1:     /* RTS/CTS */
        flowctrl = F_HW;
        break;
    case 2:     /* XON/XOFF */
        flowctrl = F_SW;
        break;
//		case 3:     /* DTR/DSR */
//			flowctrl = 0x60;
//			break;
    default:
        flowctrl = F_NONE;
        break;
    }
	if( Scf_getIfType(port)!= 0x00 )
	{// not 232 mode
		if( flowctrl != F_SW )
			flowctrl = F_NONE;
	}
    sio_flowctrl(port, flowctrl); /* Set flow control */


    return fd_port;
}

static int emodem_init_regs(int port)
{
    Scf_getEmodemRegs(port, emodem_sreg_flash, DCF_EMODEM_REGS);
    memcpy(emodem_sreg, emodem_sreg_flash, sizeof(emodem_sreg_flash));
    return 0;
}

/**
 * \brief Get modem regs.
 * \param port
 * \param reg
 * \param regs
 * \param mode 0 for flash setting, 1 for running setting.
 * \return
 */
static int emodem_get_regs(int port, int* reg, int regs, int mode)
{
    int *reg_t;
    int i;

    if(mode == 0)
        reg_t = emodem_sreg_flash;
    else
        reg_t = emodem_sreg;

    for(i=0; i<regs; i++, reg, reg_t)
        reg[i] = reg_t[i];

    return 0;
}

/**
 * \brief
 * \param port
 * \param reg
 * \param regs
 * \param mode 0 for flash setting, 1 for running setting.
 * \return
 */
static int emodem_set_regs(int port, int* reg, int regs, int mode)
{
    int *reg_t;
    int i;

    if(mode == 0)
        reg_t = emodem_sreg_flash;
    else
        reg_t = emodem_sreg;

    for(i=0; i<regs; i++, reg, reg_t)
        reg_t[i] = reg[i];

    return 0;
}

static int emodem_set_default(int port)
{
    Scf_setEmodemDefault(port);
    emodem_init_regs(port);
    return 0;
}

#define EMODEM_SOCKET_MODE_LISTAN   0
#define EMODEM_SOCKET_MODE_CONNECT  1
/**
 * @brief Create socket.
 * @param [in] port Serial port index.
 * @param [inout] fd_net Socket fd.
 * @param [in] mode Listen or connect.
 * @retval 1 Keep old socket.
 * @retval 2 Create a new socket.
 */
static int emodem_open_socket(int port, int *fd_net, int mode)
{
    struct sockaddr_in sin;
    INT16U dataport, cmdport;
    int arg;

    if(*fd_net >= 0)
    {
        if(mode == EMODEM_SOCKET_MODE_LISTAN)
        {
            if(tcp_state(*fd_net) != TCP_LISTEN)
            {
                close(*fd_net);
                *fd_net = -1;
            }
        }
        else if(mode == EMODEM_SOCKET_MODE_CONNECT)
        {
            if(tcp_state(*fd_net) == TCP_LISTEN)
            {
                close(*fd_net);
                *fd_net = -1;
            }
        }
    }

    if(*fd_net >= 0)
    {
        return 1;
    }

    while((*fd_net = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        sleep(1);

    /* non-block */
    arg = 1;
    ioctl(*fd_net, FIONBIO, &arg);

    while (setsockopt(*fd_net, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(int)) == -1)
        sleep(1);
#ifdef __CYGWIN__
    dataport = 4001;
    cmdport = 966;
#else
    Scf_getTcpServer(port, &dataport, &cmdport);
#endif // __CYGWIN__

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(dataport);
    bind(*fd_net, (const struct sockaddr *)&sin, sizeof(sin));

    emodem_stat = D_STAT_INIT;

    return 2;
}

static void emodem_set_listen(int port, int *fd_net)
{
    int ret;

    while( (ret = emodem_open_socket(port, fd_net, EMODEM_SOCKET_MODE_LISTAN)) < 0)
        sleep(1);

    if(ret == 2)
    {
        listen(*fd_net, 1);
        emodem_stat = D_STAT_LISTEN;
    }
}

static void emodem_set_connect(int port, int *fd_net)
{
    int ret;
    struct sockaddr_in sin;

    ret = emodem_open_socket(port, fd_net, EMODEM_SOCKET_MODE_CONNECT);

    if(ret == 1)
        return;

    if( (sys_clock_ms() - emodem_lasttime) > emodem_sreg[D_EMODEM_REG_S7]*1000)
    {
        /* connect timeout */
        emodem_stat = D_STAT_WAIT_CLOSE;
        return;
    }

    if(tcp_state(*fd_net) == TCP_SYN_SENT)
    {
        return;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = emodem_dial_ip;
    sin.sin_port = htons(emodem_dial_port);

    ret = connect(*fd_net, (struct sockaddr *)&sin, sizeof(sin));

    if(ret == 0)
    {
        /* connect OK */
        emodem_stat = D_STAT_CONNECTED;
        emodem_flag |= D_EFLAG_OFF_HOOK;
        emodem_lasttime = sys_clock_ms();
    }

    if(ret == -1)
    {
        if(errno == EINPROGRESS)
        {
            emodem_stat = D_STAT_DIALOUT_WAIT;
            emodem_flag |= D_EFLAG_OFF_HOOK;
            //emodem_lasttime = sys_clock_ms();
        }
        else
            emodem_stat = D_STAT_INIT;
    }
}

static int send_response(int port, int fd_port, int type)
{
    int len;
    char    *p;
    char    buf[32];

    if(emodem_flag & D_EFLAG_QUIET_CMD)
        return(0);
    p = buf;
    p[0] = emodem_sreg[D_EMODEM_REG_S3];
    if((emodem_flag & D_EFLAG_RSP_NUMBER) == 0)
    {
        p[1] = emodem_sreg[D_EMODEM_REG_S4];
        len = 2;
    }
    else
        len = 1;

    switch(type)
    {
    case D_RSP_OK:
        if(emodem_flag & D_EFLAG_RSP_NUMBER)
        {
            p[len++] = '0';
        }
        else
        {
            strncpy(&p[len], "OK", 2);
            len += 2;
        }
        break;

    case D_RSP_Connect:
        if(emodem_flag & D_EFLAG_RSP_NUMBER)
        {
            p[len++] = '1';
        }
        else
        {
            buf[len] = 0;
            len += snprintf((char *)&buf[len], sizeof(buf)-len, "CONNECT %ld", sio_getbaud(port));
            //p = stpcpy((char *)&buf[len], "CONNECT ");
            //itoa(p, sio_getbaud(port));
            //p = p + strlen(p);
            //len = (int)(p - (char *)buf);
            p = buf;
        }
        break;

    case D_RSP_Ring:
        if(emodem_flag & D_EFLAG_RSP_NUMBER)
        {
            p[len++] = '2';
        }
        else
        {
            strncpy(&p[len], "RING", 4);
            len += 4;
        }
        break;

    case D_RSP_NoCarrier:
        if(emodem_flag & D_EFLAG_RSP_NUMBER)
        {
            p[len++] = '3';
        }
        else
        {
            strncpy(&p[len], "NO CARRIER", 10);
            len += 10;
        }
        break;

    case D_RSP_Error:
        if(emodem_flag & D_EFLAG_RSP_NUMBER)
        {
            p[len++] = '4';
        }
        else
        {
            strncpy(&p[len], "ERROR", 5);
            len += 5;
        }
        break;

    default:
        len = 0;
    }
    if((len == 0) || (p == NULL))
        return(0);
    p[len++] = emodem_sreg[D_EMODEM_REG_S3];
    if((emodem_flag & D_EFLAG_RSP_NUMBER) == 0)
        p[len++] = emodem_sreg[D_EMODEM_REG_S4];
    usleep(300*1000);     /* delay 300 mini-seconds */
    return(write_port(fd_port, p, len));
}

static void emodem_accept(int port, int fd_port, int *fd_net)
{
    int fd_accept;
    struct sockaddr_in sin;
    socklen_t len;

    len = sizeof(sin);

    fd_accept = accept(*fd_net, (struct sockaddr *)&sin, &len);
    if(fd_accept < 0)
    {
        DBG("accept fail");
        return;
    }

    /* close listen fd. */
    close(*fd_net);
    *fd_net = fd_accept;

    if(emodem_sreg[D_EMODEM_REG_S0] == 0)
        emodem_stat = D_STAT_RING;
    else
        emodem_stat = D_STAT_WAIT_CONNECT;

}

static  void    emodem_save_sreg(int port)
{
    int reg[D_EMODEM_REGS];

    if((emodem_flag & D_EFLAG_CHANGED) == 0)
        return;
    emodem_get_regs(port, reg, D_EMODEM_REGS, 0);
    //Scfw_getEmodemRegs(port, reg, D_EMODEM_REGS);
    reg[D_EMODEM_REG_S0] = emodem_sreg[D_EMODEM_REG_S0];
    reg[D_EMODEM_REG_S2] = emodem_sreg[D_EMODEM_REG_S2];
    reg[D_EMODEM_REG_S3] = emodem_sreg[D_EMODEM_REG_S3];
    reg[D_EMODEM_REG_S4] = emodem_sreg[D_EMODEM_REG_S4];
    reg[D_EMODEM_REG_S5] = emodem_sreg[D_EMODEM_REG_S5];
    reg[D_EMODEM_REG_S6] = emodem_sreg[D_EMODEM_REG_S6];
    reg[D_EMODEM_REG_S7] = emodem_sreg[D_EMODEM_REG_S7];
    reg[D_EMODEM_REG_S12] = emodem_sreg[D_EMODEM_REG_S12];
    if(emodem_flag & D_EFLAG_LOCAL_ECHO)
        reg[D_EMODEM_REG_ECHO] = 1;
    else
        reg[D_EMODEM_REG_ECHO] = 0;
    if(emodem_flag & D_EFLAG_RSP_NUMBER)
        reg[D_EMODEM_REG_VERBOSE] = 0;
    else
        reg[D_EMODEM_REG_VERBOSE] = 1;
    if(emodem_flag & D_EFLAG_QUIET_CMD)
        reg[D_EMODEM_REG_QUIET] = 1;
    else
        reg[D_EMODEM_REG_QUIET] = 0;
    if(emodem_flag & D_EFLAG_DTE_DCD)
        reg[D_EMODEM_REG_DCD] = 1;
    else
        reg[D_EMODEM_REG_DCD] = 0;
    if(emodem_flag & D_EFLAG_DTE_DTR)
        reg[D_EMODEM_REG_DTR] = 1;
    else
        reg[D_EMODEM_REG_DTR] = 0;

    emodem_set_regs(port, reg, D_EMODEM_REGS, 0);
    //Scf_setEmodemRegs(port, reg, D_EMODEM_REGS);
    if(emodem_flag & D_EFLAG_SAVE)
        Scf_setEmodemRegs(port, reg, D_EMODEM_REGS);    // save to config file.
    emodem_flag &= ~(D_EFLAG_CHANGED | D_EFLAG_SAVE);
}

static int atcmd_get_ip_port(int portno, u_long *ip, u_short *port)
{
    int i, n, dot = 0;
    char    buf[40], c;

    n = 3;
    c = atcmd_buf[n];
    if(c == 'T' || c == 't' || c == 'P' || c == 'p')
        n++;
    while((n < atcmd_len) && (atcmd_buf[n] == ' '))
        n++;

    /* get IPv4 IP & port */
    for(i = 0; i < atcmd_len - n; i++)
    {
        if(atcmd_buf[n+i] == '.')
        {
            dot = 1;
            break;
        }
    }

    if(dot)
    {
        for(i = 0; i < 17; i++, n++)
        {
            if(n >= atcmd_len)
                return(0);
            if((c = atcmd_buf[n]) == ':')
                break;
            buf[i] = c;
        }
        if(i == 17)
            return(0);
        buf[i] = 0;

        if((*(u_long *)ip = inet_addr(buf)) == 0xFFFFFFFFL)
            return(0);
        n++;
    }
    else    /* supporting IP without dot */
    {
        /*
         * When you using numbers-and-dots notation for addresses,
         * be aware that each number will be interpreted as octal if preceded by a 0
         * and as hexadecimal if preceded by 0x.
         *
         * For example, inet_aton("226.000.000.037", &t) will interpret the address as 226.0.0.31 and not 226.0.0.37.
         */
        int rest = atcmd_len - n;
        int s=0;
        int flag;

        if(rest != 17)      /* IP&Port (12+5)*/
            return 0;

        flag = 0;

        for(i = 0; i < 15; i++)
        {
            if(i % 4 == 3)
            {
                buf[s++] = '.';
                flag = 1;
            }
            else
            {
                if( (flag == 1)
                    && (atcmd_buf[n] == '0'))
                {
                    n++;
                    continue;
                }
                buf[s++] = atcmd_buf[n++];
                flag = 0;
            }
        }
        buf[i] = 0;

        if((*(u_long *)ip = inet_addr(buf)) == 0xFFFFFFFFL)
            return(0);
    }

    for(i = 0; n < atcmd_len; i++, n++)
    {
        if((c = atcmd_buf[n]) < '0' || c > '9')
            return(0);
        buf[i] = c;
    }
    if(i == 0)
        return(0);
    buf[i] = 0;

    *port = (u_short)atoi(buf);

    return(1);
}

static int atcmd_get_int(int port, int ofs, int *value)
{
    int i;
    char    buf[20], c;

    for(i = 0; ofs < atcmd_len; i++, ofs++)
    {
        c = atcmd_buf[ofs];
        if(c < '0' || c > '9')
            break;
        buf[i] = c;
    }
    if(i == 0)
        return(-1);
    buf[i] = 0;
    *value = (int)atoi(buf);
    return(ofs);
}

static void show_setting(int port, int fd_port, int eflag, int *sregs, char *msg)
{
    int i, n;
    char    buf[128];
    char    *p;

    buf[0] = emodem_sreg[D_EMODEM_REG_S3];
    buf[1] = emodem_sreg[D_EMODEM_REG_S4];
    strcpy(&buf[2], msg);
    p = buf + strlen(buf);
    *p++ = emodem_sreg[D_EMODEM_REG_S3];
    *p++ = emodem_sreg[D_EMODEM_REG_S4];
    write_port(fd_port, buf, (int)(p - buf));

    p = buf;
    *p++ = 'E';
    if(eflag & D_EFLAG_LOCAL_ECHO)
        *p++ = '1';
    else
        *p++ = '0';
    *p++ = ' ';
    *p++ = 'Q';
    if(eflag & D_EFLAG_QUIET_CMD)
        *p++ = '1';
    else
        *p++ = '0';
    *p++ = ' ';
    *p++ = 'V';
    if(eflag & D_EFLAG_RSP_NUMBER)
        *p++ = '0';
    else
        *p++ = '1';
    *p++ = ' ';
    *p++ = '&';
    *p++ = 'C';
    if(eflag & D_EFLAG_DTE_DCD)
        *p++ = '1';
    else
        *p++ = '0';
    *p++ = ' ';
    *p++ = '&';
    *p++ = 'D';
    if(eflag & D_EFLAG_DTE_DTR)
        *p++ = '1';
    else
        *p++ = '0';
    *p++ = emodem_sreg[D_EMODEM_REG_S3];
    *p++ = emodem_sreg[D_EMODEM_REG_S4];
    write_port(fd_port, buf, (int)(p - buf));

    p = buf;
    for(i = 0; i <= 12; i++)
    {
        *p++ = 'S';
        *p++ = '0' + (i / 10);
        *p++ = '0' + (i % 10);
        *p++ = ':';
        n = (int)sregs[i];
        *p++ = '0' + (n / 100);
        n = n % 100;
        *p++ = '0' + (n / 10);
        *p++ = '0' + (n % 10);
        *p++ = ' ';
    }
    *p++ = emodem_sreg[D_EMODEM_REG_S3];
    *p++ = emodem_sreg[D_EMODEM_REG_S4];
    write_port(fd_port, buf, (int)(p - buf));
}


static int emodem_eflags(int *reg)
{
    int eflag=0;

    if(reg[D_EMODEM_REG_ECHO])
        eflag |= D_EFLAG_LOCAL_ECHO;
    else
        eflag &= ~D_EFLAG_LOCAL_ECHO;
    if(reg[D_EMODEM_REG_VERBOSE])
        eflag &= ~D_EFLAG_RSP_NUMBER;
    else
        eflag |= D_EFLAG_RSP_NUMBER;
    if(reg[D_EMODEM_REG_QUIET])
        eflag |= D_EFLAG_QUIET_CMD;
    else
        eflag &= ~D_EFLAG_QUIET_CMD;
    if(reg[D_EMODEM_REG_DCD])
        eflag |= D_EFLAG_DTE_DCD;
    else
        eflag &= ~D_EFLAG_DTE_DCD;
    if(reg[D_EMODEM_REG_DTR])
        eflag |= D_EFLAG_DTE_DTR;
    else
        eflag &= ~D_EFLAG_DTE_DTR;

    return eflag;
}

static  int emodem_read_sreg(int port, int eflag, int *sreg)
{
    int i, reg[D_EMODEM_REGS];

    //Scf_getEmodemRegs(port, reg, D_EMODEM_REGS);
    emodem_get_regs(port, reg, D_EMODEM_REGS, 0);

    for(i = 0; i < 13; i++)
        sreg[i] = reg[i];

    return emodem_eflags(reg);
}

static  void    show_flashrom_setting(int port, int fd_port)
{
    show_setting(port, fd_port, emodem_flag_flash, emodem_sreg_flash, "Flashrom Setting:");
}

static  void    show_factory_setting(int fd_port, int port)
{
    int eflag;
    int tmpregs[D_EMODEM_REGS];

    Scf_setEmodemDefaultRegs(tmpregs);
    eflag = emodem_eflags(tmpregs);
    show_setting(port, fd_port, eflag, tmpregs, "Factory Setting:");
}

static  int atcmd_process(int port, int fd_port, int fd_net)
{
    int ok, delchars, n;
    char    c, d = 0;

    if(atcmd_len == 2)
    {
        ok = D_RSP_OK;
        c = 0;
    }
    else
    {
        ok = D_RSP_Error;
        c = atcmd_buf[2];
    }

    do
    {
        delchars = 0;
        switch(c)
        {
        case 'A':   /* ATA */
        case 'L':   /* ATL */
        case 'M':   /* ATM */
        case 'O':   /* ATO */
        case 'P':   /* ATP */
        case 'T':   /* ATT */
        case 'Z':   /* ATZ */
        case 'X':   /* ATX */
            if(atcmd_len < 3)
                break;
            if(atcmd_len > 3)
                delchars = 1;
            ok = D_RSP_OK;
            if(c == 'L' || c == 'M' || c == 'P' || c == 'T' || c == 'X')
                break;
            if(c == 'A')
            {
                ok = D_RSP_Nothing;
                if(emodem_stat == D_STAT_RING)
                {
                    emodem_stat = D_STAT_WAIT_CONNECT;
#ifdef SHOW_CONNECT_MSG

                    {
                        char tmp[20];
                        snprintf(tmp, sizeof(tmp), "\r\nCONNECT %d", io_getbaud(port))
                        send(fd_net, tmp, strlen(tmp), 0);
                    }
#endif
                    send(fd_net, "\r\n", 2, 0);
                    //send_response(port, fd_port, D_RSP_Connect);

                }
                else {
                    send_response(port, fd_port, D_RSP_NoCarrier);
		}
            }
            else if(c == 'O')
            {
                if(emodem_stat == D_STAT_CONNECTED)
                {
                    ok = D_RSP_Connect;
                    data_mode = 1;
                }
                else
                {
                    ok = D_RSP_Nothing;
                    send_response(port, fd_port, D_RSP_NoCarrier);
                }
            }
            else if(c == 'Z')
            {
                int i;
                if(emodem_flag & D_EFLAG_OFF_HOOK)
                    emodem_stat = D_STAT_WAIT_CLOSE;

                emodem_flag = emodem_flag_flash;
                for(i = 0; i < 13; i++)
                    emodem_sreg[i] = emodem_sreg_flash[i];
            }
            break;

        case 'D':   /* ATD<IP>:<Port> */
        {
            if(emodem_stat != D_STAT_LISTEN)
                break;
            if(atcmd_get_ip_port(port, &emodem_dial_ip, (u_short *)&(emodem_dial_port)) == 1)
            {
                ok = D_RSP_Nothing;
                usleep(1);           /* task swap */
                emodem_stat = D_STAT_DIALOUT;
            }
        }
        break;

        case 'E':   /* ATE, ATE0, ATE1 */
        case 'H':   /* ATH, ATH0, ATH1 */
        case 'Q':   /* ATQ, ATQ0, ATO1 */
        case 'I':   /* *ATI, ATI0, ATI1, ATI2 */
        case 'V':   /* ATV, ATV0, ATV1 */
            if(atcmd_len >= 4)
            {
                d = atcmd_buf[3];
                if(d < '0' || d > '2')
                    break;
                if(d == '2' && c != 'I')
                    break;
                delchars = 2;
            }
            ok = D_RSP_OK;
            if(c == 'I')
            {
                char    p[40], tmp[4];
                int main_ver, sub_ver, ext_ver;

                usleep(300*1000);     /* delay 300 mini-seconds */
                tmp[0] = emodem_sreg[D_EMODEM_REG_S3];    /* Carrier Return */
                tmp[1] = emodem_sreg[D_EMODEM_REG_S4];    /* Line Free */

                sys_getVersionExt(&main_ver, &sub_ver, &ext_ver);

                if(ext_ver == 0)
                    snprintf(p, sizeof(p), "%d.%d", main_ver, sub_ver);
                else
                    snprintf(p, sizeof(p), "%d.%d.%d", main_ver, sub_ver, ext_ver);

                write_port(fd_port, tmp, 2);
                write_port(fd_port, p, strlen(p));
                write_port(fd_port, tmp, 2);
                break;
            }

            if(c == 'E')
            {
                if(atcmd_len == 4 && d == '1')
                    emodem_flag |= D_EFLAG_LOCAL_ECHO;
                else
                    emodem_flag &= ~D_EFLAG_LOCAL_ECHO;
                emodem_flag |= D_EFLAG_CHANGED;
            }
            else if(c == 'H')
            {
                if(atcmd_len == 4 && d == '1')
                {
                    if(((emodem_flag & D_EFLAG_OFF_HOOK) == 0) && emodem_stat == D_STAT_RING)
                        emodem_stat = D_STAT_WAIT_CONNECT;
                }
                else
                {
                    if(emodem_flag & D_EFLAG_OFF_HOOK)
                        emodem_stat = D_STAT_WAIT_CLOSE;
                }
            }
            else if(c == 'Q')
            {
                if(atcmd_len == 3 || d == '0')
                    emodem_flag &= ~D_EFLAG_QUIET_CMD;
                else
                    emodem_flag |= D_EFLAG_QUIET_CMD;
                emodem_flag |= D_EFLAG_CHANGED;
            }
            else if(c == 'V')
            {
                if(atcmd_len == 3 || d == '0')
                    emodem_flag |= D_EFLAG_RSP_NUMBER;
                else
                    emodem_flag &= ~D_EFLAG_RSP_NUMBER;
                emodem_flag |= D_EFLAG_CHANGED;
            }
            break;

        case 'S':   /* ATSr=n, ATSr? */
        {
            int regno, value, ofs;
            char    tmp[12];

            if((ofs = atcmd_get_int(port, 3, &regno)) < 0)
                break;
            if(ofs == atcmd_len)
                break;
            if(regno > 12)
                break;
            d = atcmd_buf[ofs++];
            if(d == '=')
            {
                if(atcmd_len == ofs)
                    break;
                ofs = atcmd_get_int(port, ofs, &value);
                if(ofs == -1)
                    break;
                if(ofs != atcmd_len)
                {
                    delchars = ofs - 2;
                }
                if(regno == 0 || (regno >= 2 && regno <= 7) || regno == 12)
                {
                    /* check the value range of S register */
                    if((regno == 0 || regno == 6 || regno == 7 || regno == 12) &&
                        (value < 0 || value > 255))
                        break;
                    if((regno >= 2 && regno <= 5) && (value < 0 || value > 127))
                        break;
                    emodem_sreg[regno] = value;
                    emodem_flag |= D_EFLAG_CHANGED;
                }
                ok = D_RSP_OK;
            }
            else if(d == '?' && atcmd_len == ofs)
            {
                ok = D_RSP_OK;
                tmp[0] = emodem_sreg[D_EMODEM_REG_S3];    /* Carrier Return */
                tmp[1] = emodem_sreg[D_EMODEM_REG_S4];    /* Line Free */
                snprintf(&tmp[2], sizeof(tmp)-2, "%ld", (u_long)emodem_sreg[regno]);
                ofs = strlen(tmp);
                tmp[ofs++] = emodem_sreg[D_EMODEM_REG_S3];    /* Carrier Return */
                tmp[ofs++] = emodem_sreg[D_EMODEM_REG_S4];    /* Line Free */
                usleep(300*1000);     /* delay 300 mini-seconds */
                write_port(fd_port, tmp, ofs);
            }
            break;
        }

        case '&':
        {
            if(atcmd_len < 4)
                break;
            c = atcmd_buf[3];
            switch(c)
            {
            case 'C':       /* AT&C, AT&C0, AT&C1 */
                if(atcmd_len >= 5)
                {
                    d = atcmd_buf[4];
                    if(d < '0' || d > '1')
                        break;
                    delchars = 3;
                }
                ok = D_RSP_OK;
                if(atcmd_len == 4 || d == '0')
                {
                    emodem_flag &= ~D_EFLAG_DTE_DCD;
                    sio_DTR(port, 1);
                }
                else
                {
                    emodem_flag |= D_EFLAG_DTE_DCD;
                    if(emodem_stat == D_STAT_CONNECTED)
                        sio_DTR(port, 1);
                    else
						if( Scf_getIfType(port)==0x00)
						{/* is rs232 mode */
	                        sio_DTR(port, 0);
						}
                }
                emodem_flag |= D_EFLAG_CHANGED;
                break;

            case 'D':       /* AT&D, AT&D0, AT&D1, AT&D2 */
                if(atcmd_len >= 5)
                {
                    d = atcmd_buf[4];
                    if(d < '0' || d > '2')
                        break;
                    delchars = 3;
                }
                ok = D_RSP_OK;
                if(atcmd_len == 4 || d == '0')
                {
                    emodem_flag &= ~D_EFLAG_DTE_DTR;
                    Scfw_setAdtype(port, D_ASYNC_ADTYPE_NONE);
                }
                else
                {
                    emodem_flag |= D_EFLAG_DTE_DTR;
                    Scfw_setAdtype(port, D_ASYNC_ADTYPE_DSROFF);
                }
                emodem_flag |= D_EFLAG_CHANGED;
                break;

            case 'F':       /* AT&F */
            case 'G':       /* *AT&G */
            case 'R':       /* *AT&R */
            case 'S':       /* *AT&S */
            case 'V':       /* AT&V */
            case 'W':       /* AT&W */
            {
                if(atcmd_len > 4)
                    delchars = 2;
                ok = D_RSP_OK;
                if(c == 'G' || c == 'R' || c == 'S')
                    break;
                if(c == 'F')
                {
                    emodem_set_default(port);
                    emodem_flag = emodem_read_sreg(port, emodem_flag, emodem_sreg);
                }
                else if(c == 'V')
                {
                    char    tmp[4];

                    ok = D_RSP_OK;
                    usleep(300*1000);     /* delay 300 mini-seconds */
                    tmp[0] = emodem_sreg[D_EMODEM_REG_S3];  /* Carrier Return */
                    tmp[1] = emodem_sreg[D_EMODEM_REG_S4];  /* Line Free */
                    write_port(fd_port, tmp, 2);
                    show_setting(port, fd_port, emodem_flag, emodem_sreg, "Current Setting:");
                    show_flashrom_setting(port, fd_port);
                    show_factory_setting(fd_port, port);
                }
                else if(c == 'W')
                {
                    emodem_flag |= (D_EFLAG_CHANGED | D_EFLAG_SAVE);
                    emodem_save_sreg(port);
                    emodem_flag_flash = emodem_flag;
                    {
                        int i;
                        for(i = 0; i < 13; i++)
                            emodem_sreg_flash[i] = emodem_sreg[i];
                    }
                }
                break;
            }

            default:
            {
                delchars = 0;
                if(emodem_sreg[D_EMODEM_REG_S6] == 0)
                    ok = D_RSP_Error;
                else if(emodem_sreg[D_EMODEM_REG_S6] == 1)
                    ok = D_RSP_OK;
            }
            }
            break;
        }

        default:
        {
            delchars = 0;
            if(emodem_sreg[D_EMODEM_REG_S6] == 0)
                ok = D_RSP_Error;
            else if(emodem_sreg[D_EMODEM_REG_S6] == 1)
                ok = D_RSP_OK;
        }
        }

        if(delchars > 0)
        {
            for(n = 0; n < delchars; n++)
            {
                atcmd_buf[2+n] = atcmd_buf[2+n+delchars];
                atcmd_len--;
            }
            if(atcmd_len > 2)
                c = atcmd_buf[2];
            else
                delchars = 0;
        }
    }
    while(delchars);

    if(ok == D_RSP_Nothing)
        return(0);
    return(send_response(port, fd_port, ok));
}

static  void    emodem_decode(int port, int fd_port, int fd_net, char *buf, int len)
{
    int i;
    char    c;

    i = 0;
    if(data_mode)
    {
        while((buf[i] == emodem_sreg[D_EMODEM_REG_S2]) && (i < len) && (escape_len < 3))
        {
            if((escape_len == 0)
                && ((sys_clock_ms() - emodem_lasttime) < (u_long)(emodem_sreg[D_EMODEM_REG_S12] * 20)))  // escape_guardtime
                break;
            escape_len++;

            i++;
            if((escape_len == 3) && (i < len))
            {
                escape_len = 0;
                break;
            }
        }
        if(i < len)
            escape_len = 0;
        send(fd_net, (char *)buf, len, 0);
        return;
    }

    /* Command mode: AT command process */
    if(emodem_flag & D_EFLAG_LOCAL_ECHO)
        write_port(fd_port, buf, len);
    if(atcmd_len == 0)
    {
        do
        {
            if(i >= len)
                return;
        }
        while(((c = buf[i++]) != 'a') && (c != 'A'));
        atcmd_buf[0] = 'A';
        atcmd_len = 1;
    }

    if(atcmd_len == 1)
    {
        do
        {
            if(i >= len)
                return;
        }
        while(((c = buf[i++]) != 't') && (c != 'T'));
        atcmd_buf[0] = 'T';
        atcmd_len = 2;
    }

    for(; i < len; i++)
    {
        c = buf[i];
        if(c == emodem_sreg[D_EMODEM_REG_S3])                 /* Carrier Return */
        {
            atcmd_process(port, fd_port, fd_net);
            atcmd_len = 0;
            break;
        }

        if(c == emodem_sreg[D_EMODEM_REG_S4])                 /* Line Feed */
            continue;

        if(c == emodem_sreg[D_EMODEM_REG_S5])                 /* backspace */
        {
            if(atcmd_len)
                atcmd_len--;
            continue;
        }

        if(atcmd_len < 60)
        {
            if(c >= 'a' && c <= 'z')
                c -= 0x20;
            atcmd_buf[atcmd_len++] = c;
        }
    }
}

/**
 * \brief
 * \param port
 * \param fd_net
 * \retval -1 sio_ofree() < TMP_LEN
 */
static int emodem_recv(int port, int fd_net, int fd_port, int do_log, char* buf)
{
    int i, n;

    if(sio_ofree(port) < TMP_LEN)
        return -1;

    i = recv(fd_net, buf, TMP_LEN, 0);
    if(i > 0)
    {
        n = write_port(fd_port, buf, i);
		if (n > 0 && do_log)
			log_port(port, 'T', buf, n);
    }
    else if(i == 0)
    {
        emodem_stat = D_STAT_WAIT_CLOSE;
    }
    return 0;

}

static void emodem_main(int port, int fd_port)
{
    int fd_net;
    fd_set rfds, wfds;
    struct timeval tv;
    int maxfd, do_log;
    int port_write_flag;
    int net_write_flag;
    char tmp[TMP_LEN];
    int ring_count;
    u_long ring_time;
  	int   (*emodem_sio_read)(int port, char *buf, int len);

	if (Scf_getSerialDataLog(port))
  	{
    	do_log = 1;
    	emodem_sio_read = log_sio_read;
  	}
  	else
  	{
    	do_log = 0;
    	emodem_sio_read = sio_read;
  	}

    fd_net = -1;
    port_write_flag = net_write_flag = 0;

    ring_count = 0;
    ring_time = 0;

    emodem_init_regs(port);

    emodem_flag = emodem_read_sreg(port, 0, emodem_sreg);
    emodem_flag_flash = emodem_read_sreg(port, 0, emodem_sreg_flash);
    if(emodem_flag & D_EFLAG_DTE_DTR)
        Scfw_setAdtype(port, D_ASYNC_ADTYPE_DSROFF);
    if(emodem_flag & D_EFLAG_DTE_DCD)
		if( Scf_getIfType(port)==0x00)
		{/* is rs232 mode */
	        sio_DTR(port, 0);   /* DTR:->DCD off at init state */
		}
		else
		{
	        sio_DTR(port, 1);   /* DTR:->DCD off at init state */
		}
    else
        sio_DTR(port, 1);   /* DTR:->DCD on all the time */
    emodem_lasttime = sys_clock_ms();
    escape_len = atcmd_len = 0;
    emodem_stat = D_STAT_INIT;
    data_mode = 0;

    /* main loop */
    while(portd_terminate == 0)
    {
        switch(emodem_stat)
        {
        case D_STAT_INIT:
            port_write_flag = net_write_flag = 0;
            emodem_flag &= ~D_EFLAG_OFF_HOOK;
            emodem_set_listen(port, &fd_net);
            break;
        case D_STAT_WAIT_CLOSE:
            send_response(port, fd_port, D_RSP_NoCarrier);
            sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);
            if(fd_net >= 0)
            {
                shutdown(fd_net, 2);
                close(fd_net);
            }
            fd_net = -1;
            emodem_stat = D_STAT_INIT;
            data_mode = atcmd_len = 0;
            if(emodem_flag & D_EFLAG_DTE_DCD)
				if( Scf_getIfType(port)==0x00)
				{/* is rs232 mode */
	                sio_DTR(port, 0);   /* ->DCD off */
				}
            break;
        case D_STAT_RING:
            if((ring_count == 0) || ((sys_clock_ms() - ring_time) >= 2500))
            {
                if(ring_count < 3)
                {
                    ring_count++;
                    ring_time = sys_clock_ms();
                    send_response(port, fd_port, D_RSP_Ring);
                }
                else
                {
                    shutdown(fd_net, 2);
                    close(fd_net);
                    emodem_stat = D_STAT_INIT;
                    if(emodem_flag & D_EFLAG_DTE_DCD)
						if( Scf_getIfType(port)==0x00)
						{/* is rs232 mode */
	                        sio_DTR(port, 0);
						}
                }
            }
            break;
        case D_STAT_WAIT_CONNECT:
        case D_STAT_DIALOUT_WAIT:
            if(fd_net >= 0)
            {
                int state = tcp_state(fd_net);
                if(state == TCP_ESTABLISHED)
                {
                    emodem_stat = D_STAT_CONNECTED;
                    send_response(port, fd_port, D_RSP_Connect);
                    data_mode = 1;
                    emodem_flag |= D_EFLAG_OFF_HOOK;
                    if(emodem_flag & D_EFLAG_DTE_DCD)
                        sio_DTR(port, 1);
                    sys_send_events(EVENT_ID_OPMODE_CONNECT, port << 4);
// ===== perry add start =====
#ifdef SUPPORT_TCP_KEEPALIVE
						tcp_setAliveTime(port, fd_net);
#endif // SUPPORT_TCP_KEEPALIVE
// ===== perry add end =====
                }

                if(emodem_stat == D_STAT_DIALOUT_WAIT)
                {
                    if(state == 0)
                    {
                        close(fd_net);
                        fd_net = -1;
                        emodem_stat = D_STAT_DIALOUT;
                    }

                    if( (sys_clock_ms() - emodem_lasttime) > (emodem_sreg[D_EMODEM_REG_S7]*1000) )
                    {
                        DBG("connect timeout\n");
                        emodem_stat = D_STAT_WAIT_CLOSE;
                    }
                }
            }
            break;
        }

        /* Setup fd. */
        FD_ZERO(&rfds);
        FD_ZERO(&wfds);

        maxfd = 0;
#define CHK_SET_FD(f, s) \
	    do { \
	        if(f >= 0) \
	        { \
	            FD_SET(f, &s); \
	            maxfd = MAX(maxfd, f); \
	        } \
	    }while(0)

        if(emodem_stat == D_STAT_LISTEN)
            CHK_SET_FD(fd_net, rfds);

        if(port_write_flag)
            CHK_SET_FD(fd_port, wfds);
        else if(emodem_stat != D_STAT_LISTEN)
            CHK_SET_FD(fd_net, rfds);

        if( (net_write_flag)
            && (emodem_stat != D_STAT_LISTEN))
            CHK_SET_FD(fd_net, wfds);
        else
            CHK_SET_FD(fd_port, rfds);

#undef CHK_SET_FD
        /* End setup fd. */
        tv.tv_sec = 1;
        tv.tv_usec = 0L;

        if(select(maxfd+1, &rfds, &wfds, NULL, &tv) <= 0)
        {
            if(sio_isup(port))
                emodem_flag |= D_EFLAG_ASYNC_UP;
            else
                emodem_flag &= ~D_EFLAG_ASYNC_UP;

            if(data_mode)
            {
                if((escape_len == 3)
                    && ((sys_clock_ms() - emodem_lasttime) >= (u_long)(emodem_sreg[D_EMODEM_REG_S12] * 20)))     // escape_guardtime
                {
                    /* receiver '+++', change to cmd mode. */
                    data_mode = atcmd_len = 0;
                    escape_len = 0;
                    send_response(port, fd_port, D_RSP_OK);
                }
                else if(escape_len && ((sys_clock_ms() - emodem_lasttime) >= 10000))
                    escape_len = 0;
            }

            /* select timeout. */
            switch(emodem_stat)
            {
            case D_STAT_DIALOUT:
                emodem_set_connect(port, &fd_net);
                break;
            }
        }

        if(FD_ISSET(fd_port, &rfds))
        {
            int i;
            i = emodem_sio_read(port, (char *)tmp, TMP_LEN);
            if(i > 0)
            {
                if(emodem_flag & D_EFLAG_ASYNC_UP)
                    emodem_decode(port, fd_port, fd_net, (char *)tmp, i);
                emodem_lasttime = sys_clock_ms();
            }
            else if(i == 0)
            {
                /* do close */
                emodem_stat = D_STAT_WAIT_CLOSE;
            }
        }

        if(FD_ISSET(fd_port, &wfds))
            port_write_flag = 0;

        if(fd_net < 0)
            continue;

        if(FD_ISSET(fd_net, &rfds))
        {
            switch(emodem_stat)
            {
            case D_STAT_LISTEN:
                emodem_accept(port, fd_port, &fd_net);
                ring_count = 0;
                break;
            default:
                if(emodem_recv(port, fd_net, fd_port, do_log, tmp) < 0)
                    port_write_flag = 1;
                break;
            }
        }

        if(FD_ISSET(fd_net, &wfds))
            net_write_flag = 0;
    }

    /* close socket */
    if(fd_net >= 0)
    {
        shutdown(fd_net, 2);
        close(fd_net);
    }
}

void *emodem_start(void* portno)
{
    int fd_port;
    int port;

    port = (int)portno;

    fd_port = emodem_open_serial(port);

    emodem_main(port, fd_port);
	if (emodem_stat == D_STAT_CONNECTED)
		sys_send_events(EVENT_ID_OPMODE_DISCONNECT, port << 4);
    sio_close(port);
    return NULL;
}
