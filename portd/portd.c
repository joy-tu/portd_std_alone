/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    portd.c

    Daemon for operation mode.

    2011-01-06  James Wang
        First version.
*/

#ifndef _PORTD_C
#define _PORTD_C
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <header.h>
#include <debug.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <portd.h>
#include <sio.h>
#include <config.h>
#include <sysapi.h>
#include <rfc2217.h>
#include <pair.h>
#include "aspp.h"
#include "udp/raw_udp.h"
#include <support.h>
#include <datalog.h>
#include <eventd.h>
#include "debug.h"
#include "../message.h"
#include <sys/types.h>          /* See NOTES */
#include <sys/stat.h>          /* See NOTES */


#ifdef SUPPORT_SERCMD
#include <scm.h>
#endif


//#define _DEBUG

#ifdef _DEBUG
#define PORTD_DBG printf
#else
#define PORTD_DBG(...)
#endif // _DEBUG

#ifdef SUPPORT_SERCMD
extern int Gscm_active;
extern int Gscm_online;
#endif

struct port_data Gport;

static volatile sig_atomic_t portd_received_sighup = 0;
volatile int portd_terminate = 0;
#ifdef SUPPORT_RTERMINAL_MODE
u_long PortdLastTime = 0;
char PortdUsername[PORTD_USER_LEN+1];
char PortdPassword[PORTD_PSWD_LEN+1]; 
#endif


static int portd_init(int port_idx);
static void portd(int port_idx);
static void portd_exit(int port_idx);

extern int delimiter_init(int port, int has_delimiter, int has_buffering);
extern void delimiter_exit(int port);

#ifdef SUPPORT_SERCMD
void sigusr1_handler(int sig)
{
	Gscm_active = 1;
}
#endif

/*
 * Signal handler for SIGHUP.  Sshd execs itself when it receives SIGHUP;
 * the effect is to reread the configuration file (and to regenerate
 * the server key).
 */
void sighup_handler(int sig)
{
    int save_errno = errno;

    portd_received_sighup = 1;
    signal(SIGHUP, sighup_handler);
    signal(SIGQUIT, sighup_handler);
    signal(SIGINT, sighup_handler);
    errno = save_errno;

}

static void sigpip_handler(int sig)
{
    signal(SIGPIPE, sigpip_handler);
}


/*
 * Called from the main program after receiving SIGHUP.
 * Restarts the server.
 */
static void sighup_restart(int port_idx)
{
    sys_rm_pid(port_idx, DSPORTD_PID_FILE);
    portd_received_sighup = 0;

    portd_setexitflag(port_idx, 1);
//	sys_send_events(EVENT_ID_OPMODE_RESTART, port_idx << 4);
  	port_buffering_reset(port_idx);

}


static void usage(char *progname)
{
    fprintf(stdout, "Usage: %s [options]\n", progname);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "Start TCP server mode for the specific port: -p \"port\"\n");
    fprintf(stdout, "Input config file:                           -f \"config file\"\n");
    fprintf(stdout, "Show software version:                       -v\n");
    fprintf(stdout, "How to use this tool:                        -h\n");
}

int main(int argc, char *argv[])
{
    extern char *optarg;
    int daemon=1;
    int port_idx=0;
    char buf[30];
    int ret;
    u_int pid;

    if( argc < 2 ) // at least include program name and option 
    {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }
    else
    {
        int opt;
        while( (opt = getopt(argc, argv, "f:p:dvh") ) != -1 )
        {
            switch (opt)
            {
                case 'f':
                    if (config_parser(optarg) < 0)
                        exit(EXIT_FAILURE);

                    break;
                case 'p':
                    if (argc < 3)
                    {
                        usage(argv[0]);
                        exit(EXIT_FAILURE);
                    }

                    port_idx = atoi(optarg);
                    if (get_ttyname(port_idx) < 0)
                        exit(EXIT_FAILURE);

                    break;
                case 'v':
                    sys_getVersionString(buf, sizeof(buf));
                    printf("%s\n", buf);

                    exit(EXIT_SUCCESS);
                case 'h':
                    usage(argv[0]);
                    exit(EXIT_SUCCESS);
                case 'd':
                    daemon = 0;
                    break;
            }
        }
    }

    load_runtime_conf(port_idx);

    pid = sys_get_pid(port_idx, DSPORTD_PID_FILE);
    if (pid)
    {
        PORTD_DEBUG("get pid: %d\n", pid);
        ret = kill(pid, 0);
        /* The process exists, but permission is not enough. */
        if (ret < 0 && errno == EPERM)
        {
            SHOW_LOG(stderr, port_idx, MSG_ERR,
                        "Insufficient permission to restart portd, please use sudo or run as root to execute the program.\n");
            exit(EXIT_FAILURE);
        }
        /* The process exists. */
        if (ret == 0)
        {
            int retry_cnt = 50;
            while (retry_cnt-- && kill(pid, SIGTERM) < 0)
            {
                usleep(100 * 1000L);
            }

            if (retry_cnt <= 0) // force to kill the process
            {
                PORTD_DEBUG("kill SIGKILL pid: %d\n", pid);

                kill(pid, SIGKILL);
            }
            else
            {
                PORTD_DEBUG("kill SIGTERM pid: %d\n", pid);
            }

            PORTD_DEBUG("process exist: pid: %d\n", pid);
        }
        else
        {
            PORTD_DEBUG("process not exist: pid: %d\n", pid);
        }
    }

	if (port_idx <= 0)
    {
        SHOW_LOG(stderr, -1, MSG_ERR, "Invalid port specified!\n");
        exit(EXIT_FAILURE);
    }

    if( daemon )
        sys_daemon_init();

	log_init(port_idx);

    while( 1 )
    {
        if (sys_save_pid(port_idx, DSPORTD_PID_FILE) < 0)
            exit(EXIT_FAILURE);

        /*
         * Arrange to restart on SIGHUP.  The handler needs
         * listen_sock.
         */
        signal(SIGHUP, sighup_handler);
        signal(SIGQUIT, sighup_handler);
        signal(SIGINT, sighup_handler);
        signal(SIGPIPE, sigpip_handler);
#ifdef SUPPORT_SERCMD
        signal(SIGUSR1, sigusr1_handler);
#endif

        if (!portd_init(port_idx))
        {
			sleep(1);    /* sleep 1 second */
            continue;
        }
        portd_terminate = 0;
        portd_received_sighup = 0;
		portd(port_idx);

        portd_exit(port_idx);
    }

    exit(EXIT_SUCCESS);
}



static int portd_init(int port_idx)
{
    struct port_data *ptr;
    int val;
    int ret;

    ptr = &Gport;
    ptr->port_idx = port_idx;

    val = (int) Scf_getOpMode(ptr->port_idx);

    ptr->opmode = val & 0xff00;
    ptr->application = val & 0xff;

    ptr->detail = NULL;

    if ((ret = mx_uart_init()) < 0) {
        SHOW_LOG(stderr, port_idx, MSG_ERR, "Initialize Moxa uart control library failed(ErrCode: %d).\n", ret);
        exit(EXIT_FAILURE);
    }


    // A quick open port to make /proc/tty/driver/ttymxc return
    // correct information after boot up the first time.
    //sio_open(port_idx);
    //sio_close(port_idx);
    
    switch (ptr->opmode)
    {
        case CFG_APPLICATION_DEVICE_CONTROL:
        {
            if (ptr->application == CFG_OPMODE_REALCOM) // RealCOM
            {
                PORTD_DBG("%s(), RealCOM\n", __FUNCTION__);

				delimiter_init(port_idx, 1, 1);
                ptr->detail = (struct aspp_serial*) malloc(sizeof(struct aspp_serial));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(struct aspp_serial));
            }
#if 0
            else if (ptr->application == CFG_OPMODE_RFC2217) // RFC2217
            {
                PORTD_DBG("%s(), RFC2217\n", __FUNCTION__);
                delimiter_init(port_idx, 1, 0);
            }
#endif
            break;
        }
        case CFG_APPLICATION_SOCKET:
        {
            if (ptr->application == CFG_OPMODE_TCPSERVER) // TCP Server
            {
                PORTD_DBG("%s(), TCP server\n", __FUNCTION__);
                delimiter_init(port_idx, 1, 1);
                ptr->detail = (struct aspp_serial*) malloc(sizeof(struct aspp_serial));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(struct aspp_serial));
            }
#if 0
            else if (ptr->application == CFG_OPMODE_TCPCLIENT) // TCP Client
            {
                PORTD_DBG("%s(), TCP client\n", __FUNCTION__);
                delimiter_init(port_idx, 1, 1);
                ptr->detail = (RAW_TCP_SERIAL*) malloc(sizeof(RAW_TCP_SERIAL));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(RAW_TCP_SERIAL));

            }
            else if (ptr->application == CFG_OPMODE_UDP) // UDP
            {
                PORTD_DBG("%s(), UDP\n", __FUNCTION__);
                delimiter_init(port_idx, 1, 0);
                ptr->detail = (PRAW_UDP_SERIAL) malloc(sizeof(RAW_UDP_SERIAL));
                if(ptr->detail == NULL)
                    return 0;
                memset(ptr->detail, 0, sizeof(RAW_UDP_SERIAL));
            }
#endif
            break;
        }
#if 0
        case CFG_APPLICATION_PAIR_CONNECTION:
        {
        	delimiter_init(port_idx, 0, 1);
            if (ptr->application == CFG_OPMODE_PAIR_MASTER) // Pair master
            {
                PORTD_DBG("%s(), pair master\n", __FUNCTION__);
            }
            else if (ptr->application == CFG_OPMODE_PAIR_SLAVE) // Pair slave
            {
                PORTD_DBG("%s(), pair slave\n", __FUNCTION__);
            }
            break;
        }
        case CFG_APPLICATION_ETH_MODEM:     // Ethernet Modem
        {
            PORTD_DBG("%s(), ethernet modem\n", __FUNCTION__);
            break;
        }
        case CFG_APPLICATION_DISABLED:      // Disabled
        {
			PORTD_DBG("%s(), disabled\n", __FUNCTION__);
            break;
        }
#ifdef SUPPORT_RTERMINAL_MODE
		case CFG_APPLICATION_RTERMINAL: 	// Reverse Terminal
		{
			PORTD_DBG("%s(), reverse terminal\n", __FUNCTION__);
            break;
		}
#endif			
#endif
        default:
            return 0;
            break;
    }

    return 1;
}

static void portd(int port_idx)
{
    struct port_data *ptr;

    ptr = &Gport;
#if defined(w2x50a) || defined (ia5x50aio)
    if (port_idx == 1)
    {
        struct stat st;
        while (!stat("/var/._serial_console_port1", &st))
        {
            PORTD_DBG("waiting for serial console timeout\n");
            usleep(100*1000);
        }
    }
#endif
#ifdef SUPPORT_SERCMD
    int ret;
    ret = pthread_create(&ptr->thread_scm_id,
                         NULL,
                         (void *)scmd,
                         (void *)port_idx);
    //pthread_detach(ptr->thread_scm_id);
#endif

    ptr->thread_id = (pthread_t)NULL;

    switch (ptr->opmode)
    {
        case CFG_APPLICATION_DEVICE_CONTROL:
        {
            if (ptr->application == CFG_OPMODE_REALCOM) // RealCOM
            {
                printf("Start port %d as RealCOM mode\n", port_idx);
                PORTD_DBG("%s(), RealCOM\n", __FUNCTION__);
                PORTD_DBG("pthread_create = %d \n", pthread_create(&ptr->thread_id, NULL, &aspp_start, (void *)port_idx));
            }
#if 0            
            else if (ptr->application == CFG_OPMODE_RFC2217) // RFC2217
            {
                PORTD_DBG("%s(), RFC2217\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &rfc2217_start, (void *)port_idx);
            }
#endif
            break;
        }
        case CFG_APPLICATION_SOCKET:
        {
            if (ptr->application == CFG_OPMODE_TCPSERVER) // TCP Server
            {
                printf("Start port %d as TCP server mode\n", port_idx);
                PORTD_DBG("%s(), TCP server\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &aspp_start, (void *)(port_idx|0x8000));
            }
#if 0
            else if (ptr->application == CFG_OPMODE_TCPCLIENT) // TCP Client
            {
                PORTD_DBG("%s(), TCP client\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &raw_tcp_start, (void *)port_idx);
            }
            else if (ptr->application == CFG_OPMODE_UDP) // UDP
            {
                PORTD_DBG("%s(), UDP\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &raw_udp_start, (void *)port_idx);
            }
            break;
        }

        case CFG_APPLICATION_PAIR_CONNECTION:
        {
            if (ptr->application == CFG_OPMODE_PAIR_MASTER) // Pair master
            {
                PORTD_DBG("%s(), pair master\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &pair_master_start, (void *)port_idx);
            }
            else if (ptr->application == CFG_OPMODE_PAIR_SLAVE) // Pair slave
            {
                PORTD_DBG("%s(), pair slave\n", __FUNCTION__);
                pthread_create(&ptr->thread_id, NULL, &pair_slave_start, (void *)port_idx);
            }
            break;
        }
        case CFG_APPLICATION_ETH_MODEM:     // Ethernet Modem
        {
            extern void *emodem_start(void* portno);
            PORTD_DBG("%s(), ethernet modem\n", __FUNCTION__);
            pthread_create(&ptr->thread_id, NULL, &emodem_start, (void *)port_idx);
            break;
        }
        case CFG_APPLICATION_DISABLED:      // Disabled
        {
            PORTD_DBG("%s(), disabled\n", __FUNCTION__);
#endif
            break;
        }
#ifdef SUPPORT_RTERMINAL_MODE
		case CFG_APPLICATION_RTERMINAL:     // Reverse Terminal
        {
			extern void *rtelnet_start(void* arg);
			extern void *radius(void* arg);
			PORTD_DBG("%s(), reverse terminal\n", __FUNCTION__);
			pthread_create(&ptr->thread_id, NULL, &rtelnet_start, (void *)port_idx);
			pthread_create(&ptr->thread_radius_id, NULL, &radius, (void *)port_idx);
			pthread_detach(ptr->thread_radius_id); 
            break;
        }
#endif
        default:
            PORTD_DBG("%s(), unknown opmode\n", __FUNCTION__);
            return;
            break;
    }

    while (1)
    {
        if (portd_received_sighup)
        {
            portd_terminate = 1;
            if(ptr->thread_id)
                pthread_join(ptr->thread_id, NULL);

#ifdef SUPPORT_SERCMD
            Gscm_active = 0;
            Gscm_online = 0;
            pthread_join(ptr->thread_scm_id, NULL);
#endif
            sighup_restart(port_idx);
            return;
        }
        sleep(1);                    /* always sleep */
    }
}


static void portd_exit(int port_idx)
{
    struct port_data *ptr;

    ptr = &Gport;
    if(ptr->detail)
    {
        free(ptr->detail);
        ptr->detail = NULL;
    }

	delimiter_exit(port_idx);

    PORTD_DBG("portd_exit(): port = %d\n", port_idx);
}

static int __tcp_info(int fd, int mode);
#define TCP_INFO_MODE_GET_STATE     0
#define TCP_INFO_MODE_GET_OQUEUE    1

int tcp_oqueue(int fd)
{
    return __tcp_info(fd, TCP_INFO_MODE_GET_OQUEUE);
}

int tcp_ofree(int fd)
{
#if 1
    int txq;
    txq = __tcp_info(fd, TCP_INFO_MODE_GET_OQUEUE);
    return DCF_SOCK_BUF - txq;
#else
    return 4096;
#endif
}

/**
 * \brief
 * \param fd
 * \retval -1 Open proc file fail.
 * \retval -2 fd < 0.
 */
int tcp_state(int fd)
{
    int retry = 3;
    int ret = 0;

    while(retry)
    {
        if((ret = __tcp_info(fd, TCP_INFO_MODE_GET_STATE)) != 0)
        {
            break;
        }
        
        usleep(1000); /* Connection missing, wait 1ms  */
        retry --;
    }
    

    return ret;
}

static int __tcp_info(int fd, int mode)
{
    FILE *fp;
    char name[64];
    char buf[1024];
    int count, line;
    int ino;

    if(fd < 0)
    {
        return -2;
    }

    sprintf(name, "/proc/%d/fd/%d", getpid(), fd);

    count = readlink(name, buf, sizeof(buf));
    buf[count] = 0;

    sscanf(buf, "socket:[%d]", &ino);

    fp = fopen("/proc/net/tcp", "r");
    if(fp == NULL)
        return -1;

    line = 0;
    while(fgets(buf, sizeof(buf), fp))
    {
        //int d;
        //char local_addr[64], rem_addr[64];
        //int local_port, rem_port;
        int state;
        unsigned long rxq, txq;
        //int timer_run;
        //unsigned long  time_len, retr;
        //int uid;
        //int timeout;
        unsigned long inode;
        //char more[512];
        int num;

        if(line)
        {
            //num = sscanf(buf,
            //    "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
            //    &d, local_addr, &local_port, rem_addr, &rem_port, &state,
            //    &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);

            num = sscanf(buf,
                "%*d: %*64[0-9A-Fa-f]:%*X "
                "%*64[0-9A-Fa-f]:%*X %X "
                "%lX:%lX %*X:%*X "
                "%*X %*d %*d %ld\n",
                &state, &txq, &rxq, &inode);
            if(ino == inode)
            {
                fclose(fp);
                switch(mode)
                {
                case TCP_INFO_MODE_GET_STATE:
                    return state;
                case TCP_INFO_MODE_GET_OQUEUE:
                    return txq;
                }
                return 0;
            }
        }
        ++line;
    }

    fclose(fp);
    return 0;
}

void portd_wait_empty(int port, int fd_port, unsigned long timeout)
{
    int     n, i;
    unsigned long   t;

	PORTD_DBG("portd_wait_empty(): port = %d\n", port);
    if ( (n = (int)sio_oqueue(port)) <= 0 ) /* bugfix: < 0 means error port */
        return;

    t = sys_clock_ms();
    while ( 1 )
    {
        if ( (i = (int)sio_oqueue(port)) <= 0 ) /* bugfix: < 0 means error port */
            return;
        if ( n == i )
        {
            if ( (sys_clock_ms() - t) >= timeout )
            {
                sio_flush(port, 2);
                return;
            }
        }
        else
        {
            n = i;
            t = sys_clock_ms();
        }
        usleep(200000);
    }
}

unsigned long portd_getlocal_ip(void)
{
    char ipaddr[20];

    sys_getActiveIP(ipaddr);

    return inet_addr(ipaddr);
}

unsigned long portd_getbcast_ip(void)
{
    char ipaddr[20];

    sys_getActiveBcase(ipaddr);

    return htonl(inet_addr(ipaddr));
}

int portd_getexitflag(int port)
{
    return !!portd_terminate;
}

void portd_setexitflag(int port, int flag)
{
    portd_terminate = flag;
}

#define PATCH_TCP_USER_TIMEOUT
#ifdef SUPPORT_TCP_KEEPALIVE
void tcp_setAliveTime(int port, int fd)
{
    int on;
    int alive;

    alive = Scf_getPortAliveCheck(port) * 60; // s

    //printf("alive = %d\r\n", alive);

    if(alive > 0)
        on = 1;
    else
        on = 0;
    /* enable/disable TCP alive check time */
    if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (void *)&on, sizeof(on)) < 0)
    {
        SHOW_LOG(stderr, port, MSG_ERR, "Socket error, fail to set alive time.\n");
        exit(EXIT_FAILURE);
    }

    if(on == 1)
    {
#ifdef PATCH_TCP_USER_TIMEOUT
#define TCP_USER_TIMEOUT    18  /* How long for loss retry before timeout */
        int usertimeout = alive * 1000;
        if(alive < (16*60)) {
            if(setsockopt(fd, SOL_TCP, TCP_USER_TIMEOUT , &usertimeout, sizeof(int)) < 0)
            {
                SHOW_LOG(stderr, port, MSG_ERR, "Socket error, fail to set alive time.\n");
                exit(EXIT_FAILURE);
            }
        }
#endif

        /* set avlie check time parameter */
        int cnt, idle, intvl;
#if 1
        //cnt = 8;
        //idle = alive / 4;
        //intvl = alive * 3 / 4 / 8;

		//For wireless environment, only send 8 keep-alive pkts in a period seems not enough.
		//So we change to send keep-alive packet every 5 secs.
        //intvl = 5;
        intvl = 3;
        //idle = alive / 4;
        //RealCom 966 send every 6 secs, so can't use too small period
        idle = 7;

        cnt = (alive-idle)/intvl;
        if(((alive-idle)%intvl) > 0)
        	cnt++;

        //#define MAX_TCP_KEEPIDLE        32767
        //#define MAX_TCP_KEEPINTVL       32767
        //#define MAX_TCP_KEEPCNT         127

        if(cnt > 127)
        {
        	cnt = 127;	
        	intvl = (alive-idle)/cnt;
        	if(((alive-idle)%cnt) > 0)
        		intvl++;
        }
        
#else /* modify for precision */
	 cnt = alive / 2;
	 idle = 5;
	 intvl = 2;
#endif

		// number of keep alive probe
        if(setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &cnt, sizeof(int)) < 0)
        {
            SHOW_LOG(stderr, port, MSG_ERR, "Socket error, fail to set alive time.\n");
            exit(EXIT_FAILURE);
        }

		// the keep alive time
        if(setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) < 0)
        {
            SHOW_LOG(stderr, port, MSG_ERR, "Socket error, fail to set alive time.\n");
            exit(EXIT_FAILURE);
        }

		// the interval between keep alive probes
        if(setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &intvl, sizeof(int)) < 0)
        {
            SHOW_LOG(stderr, port, MSG_ERR, "Socket error, fail to set alive time.\n");
            exit(EXIT_FAILURE);
        }

        //printf("TCP alive param(%d): cnt = %d, idle = %d, invtl = %d\r\n",
        //    alive, cnt, idle, intvl);
    }

}
#endif // SUPPORT_TCP_KEEPALIVE

