/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    sysapi.c
    System generic API.

    2011-01-05  James Wang
        First release.
*/


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/reboot.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <header.h>
#include <netdb.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net_config.h>
#include <sysmaps.h>
#include <dirent.h>
#include <signal.h>
#include <linux/reboot.h>
#include <linux/ethtool.h>
#include <linux/sockios.h>
#if 0
#include <openssl/ts.h>
#include <openssl/x509.h>
#include <openssl/objects.h>
#include <openssl/asn1.h>
#include <openssl/ossl_typ.h>
#endif
#include <aptypes.h>
#include <config.h>
#include <sysapi.h>
#include <eventd.h>
#include <miscd.h>
#include <portd.h>
#include <debug.h>
#include <iwlib.h>
#include <firmware_handle.h>
#include <support.h>
#include <wlan_logd.h>
#include <adv_diod.h>
#include "ipc_cfg_api.h"
#include "../message.h"
#include "../config.h"


#define ITEM_LEN 64
#define AR6000_XIOCTL_WMI_GET_RD                        54 //ioctrl get country code

#define SNMPD_RESTART_SIGNAL SIGRTMIN + 11	/* 34 + 11 = 45, reliable signal is number greater than SIGRTMIN */

//#define _DEBUG_API

#ifdef _DEBUG_API
#define DBG_API(...) \
    do { \
        printf("%s(%d): ", __func__, __LINE__); \
        printf(__VA_ARGS__); \
    }while(0)
#else
#define DBG_API(...)
#endif // _DEBUG_API


static char macbuf[18];

static void _remove_comment(char* str);
static void _trim_left_space(char* line);

extern struct runtime_config Grun_conf;

//
// System API protection
//
char* _strcpy(char *dest, const char *src)
{
    if ((dest==NULL) || (src==NULL))
    {
        DBG_API("\r\nNULL pointer passed in %s()\r\n", __FUNCTION__);
        return NULL;
    }
    else
        return strcpy(dest, src);
}

size_t _strlen(const char *str)
{
    if (str == NULL)
    {
        DBG_API("\r\nNULL pointer passed in %s()\r\n", __FUNCTION__);
        return 0;
    }
    else
        return strlen(str);
}

int _atoi(const char *str)
{
    if (str == NULL)
    {
        DBG_API("\r\nNULL pointer passed in %s()\r\n", __FUNCTION__);
        return 0;
    }
    else
        return atoi(str);
}

int _strcmp(const char* s1, const char* s2)
{
    if ((s1==NULL) || (s2==NULL))
    {
        DBG_API("\r\nNULL pointer passed in %s()\r\n", __FUNCTION__);
        return 1;
    }
    else
        return strcmp(s1, s2);
}
////


static void _remove_comment(char* str)
{
    int len, i;
    len = _strlen(str);
    i = 0;

    while (len && str[i])
    {
        if (str[i] == '#')
        {
            str[i] = 0;
            break;
        }
        i++;
    }
    return;
}

static void _trim_left_space(char* line)
{
    int i, j, len;
    len = _strlen(line);

    if (!len)
        return;

    i = 0;
    while (line[i] == ' ')
        i++;
    if (i)
    {
        for (j = 0; j < len; j++, i++)
            line[j] = line[i];

        line[j] = 0;
    }
    return;
}

u_int __sys_get_pid(char *filename)
{
    FILE *f;
    u_int pid=0;

	
	if ( (f = fopen(filename, "rb")) == NULL )
    {
        /* This file might not exist at first, don't show error message. */
        //fprintf(stderr, "Couldn't create pid file \"%s\": %s", filename, strerror(errno));
    }
    else
    {
        fscanf(f, "%u\n", &pid);
        fclose(f);
    }

    return pid;
}

u_int sys_get_pid(int port, const char* pidfile)
{
    u_int pid;
    char filename[256];

    sprintf(filename, pidfile, port);
    pid = __sys_get_pid(filename);
    return pid;
}

int __sys_save_pid(char* filename)
{
    FILE *f;
    if ((f = fopen(filename, "wb")) == NULL)
    {
        SHOW_LOG(stderr, Grun_conf.port, MSG_ERR, "Couldn't create pid file \"%s\": %s.\n", filename, strerror(errno));
        return -1;
    }
    else
    {
        fprintf(f, "%u\n", (u_int)getpid());
        fclose(f);
    }
    return 0;
}

/*
 * Record our pid in /var/run/moxa/portXX.pid (XX=01~16) to make it
 * easier to kill the correct portd.  We don't want to
 * do this before the bind above because the bind will
 * fail if there already is a daemon, and this will
 * overwrite any old pid in the file.
 */
int sys_save_pid(int port, const char* pidfile)
{
    char filename[256];

    sprintf(filename, pidfile, port);
    return __sys_save_pid(filename);
}


void sys_rm_pid(int port, const char* pidfile)
{
    char filename[256];

    sprintf(filename, pidfile, port);
    unlink(filename);
    return;
}

int sys_daemon_init(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        return -1;
    else if (pid != 0)
    {
        exit(0);                /* parent goes bye-bye */
    }

    /* child continues */
    setsid();                   /* become session leader */
    chdir("/");                 /* change working directory */
    umask(0);                   /* clear our file mode creation mask */

    return(0);
}


/**
 * \brief Get IP address by host name.
 * \param [in] host Host name.
 * \param [out] inaddr Host's IP address.
 * \param [on] timeout Wait time. (Unuse)
 * \retval 0 Success.
 * \retval -1 Fail.
 */
int Ssys_getServerIp(char* host, unsigned long *inaddr, int timeout)
{
    struct hostent* he;
    struct in_addr **addr_list;

    if ((he = gethostbyname(host)) == NULL)
    {
#ifdef DEBUG_W1
        /* get the host info */
        herror("gethostbyname");
#endif
        return -1;
    }

    addr_list = (struct in_addr **)he->h_addr_list;
    *inaddr = (unsigned long) addr_list[0]->s_addr;

    return 0;
}


char* _chk_interface(char *interface)
{
#ifdef WIRELESS_WLAN
    static char *wirelessName="wlan0";

    if (_strcmp(interface, "eth1") == 0)
        return wirelessName;
#endif // WIRELESS_WLAN
    return interface;
}

/**
 * \brief Get network information.
 * \param [in] interface Network interface name.
 * \param [in] request Requset function.
 * \return Request's value.
 */
char *sys_getnetworkinfo(const char *interface, int request)
{
    int sockfd;
    struct ifconf ifc;
    struct ifreq *ifr, ifrcopy;
    char *buf, *ptr;
    int lastlen, len;
    int flags;
    struct sockaddr_in sin;
    u_char hwaddr[32];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    lastlen = 0;
    len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
    /* A fundamental problem with the SIOCGIFCONF request is that some
     * implementations do not return an error if the buffer is not large
     * enough to hold the result. Instead, the result is truncated and
     * success is returned (a return value of 0 from ioctl). This means
     * the only way we know that our buffer is large enough is to issue
     * the request, save the return length, issue the request again with
     * a larger buffer, and compare the length with the saved value.
     * Only if the two lengths are the same is our buffer large enough.
     */

    while (1)
    {
        buf = (char *) malloc(len);
        ifc.ifc_len = len;
        ifc.ifc_buf = buf;
        if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0)
        {
            if (errno != EINVAL || lastlen != 0)
            {
                dbg_printf("call ioctl error\n");
                close(sockfd);
                free(buf);
                return "";
            }
        }
        else
        {
            if (ifc.ifc_len == lastlen)
                break;			/* success, len has not changed */
            lastlen = ifc.ifc_len;
        }
        len += 10 * sizeof(struct ifreq);	/* increment */
        free(buf);
    }

    len = sizeof(struct sockaddr);

    for (ptr = buf; ptr < buf + ifc.ifc_len;)
    {
        ifr = (struct ifreq *) ptr;
        ptr += sizeof(struct ifreq);	/* for next one in buffer */

        if (if_nametoindex(ifr->ifr_name) != if_nametoindex(_chk_interface((char*)interface)))
            continue;

        ifrcopy = *ifr;

        ioctl(sockfd, SIOCGIFFLAGS, &ifrcopy);
        flags = ifrcopy.ifr_flags;
        if ((flags & IFF_UP) == 0)
            continue;			/* ignore if interface not up */

        switch (request)
        {
        case SIOCGIFADDR:
            switch (ifr->ifr_addr.sa_family)
            {
            case AF_INET:
            default:
                memcpy(&sin, &ifr->ifr_addr, sizeof(struct sockaddr_in));
                break;
            }
            close(sockfd);
            free(buf);
            return inet_ntoa(sin.sin_addr);

        case SIOCGIFHWADDR:		/* HW addr */
            if (!(flags & IFF_LOOPBACK))
            {
                ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy);
                memcpy(hwaddr, &ifrcopy.ifr_hwaddr.sa_data, 8);
                memset(macbuf, 0, sizeof(macbuf));
                sprintf(macbuf, "%02X:%02X:%02X:%02X:%02X:%02X",
                        hwaddr[0], hwaddr[1], hwaddr[2],
                        hwaddr[3], hwaddr[4], hwaddr[5]);
                close(sockfd);
                free(buf);
                return macbuf;
            }
            break;

        case SIOCGIFNETMASK:	/* Netmask */
            ioctl(sockfd, SIOCGIFNETMASK, &ifrcopy);
            memcpy(&sin, &ifrcopy.ifr_netmask, sizeof(struct sockaddr_in));
            close(sockfd);
            free(buf);
            return inet_ntoa(sin.sin_addr);

        case SIOCGIFBRDADDR:
            if (flags & IFF_BROADCAST)
            {
                ioctl(sockfd, SIOCGIFBRDADDR, &ifrcopy);
                memcpy(&sin, &ifrcopy.ifr_broadaddr, sizeof(struct sockaddr_in));
                close(sockfd);
                free(buf);
                return inet_ntoa(sin.sin_addr);
            }
            break;

        case SIOCGIFDSTADDR:
            if (flags & IFF_POINTOPOINT)
            {
                ioctl(sockfd, SIOCGIFDSTADDR, &ifrcopy);
                memcpy(&sin, &ifrcopy.ifr_dstaddr, sizeof(struct sockaddr_in));
                close(sockfd);
                free(buf);
                return inet_ntoa(sin.sin_addr);
            }
            break;

        default:
            break;
        }
    }
    close(sockfd);
    free(buf);

    return "";
}

/**
 * \brief Get interface's MAC.
 * \param [in] interface Need get MAC's interface name, ex: eth0.
 * \param [out] addr
 * \return MAC string
 */
int sys_getmacaddr(const char *interface, char *addr)
{
    u_char hwaddr[6];
    struct ifreq ifr;
    int fd;

    /* retrieve ethernet MAC address */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    strcpy(ifr.ifr_name, _chk_interface((char*)interface));
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0)
    {
        dbg_printf("wlan_if_config > get ETH MAC address failed!\r\n");
        return D_FAILURE;
    }
    close(fd);

    hwaddr[0] = ifr.ifr_hwaddr.sa_data[0];
    hwaddr[1] = ifr.ifr_hwaddr.sa_data[1];
    hwaddr[2] = ifr.ifr_hwaddr.sa_data[2];
    hwaddr[3] = ifr.ifr_hwaddr.sa_data[3];
    hwaddr[4] = ifr.ifr_hwaddr.sa_data[4];
    hwaddr[5] = ifr.ifr_hwaddr.sa_data[5];
    sprintf(addr, "%02X:%02X:%02X:%02X:%02X:%02X",
            hwaddr[0], hwaddr[1], hwaddr[2],
            hwaddr[3], hwaddr[4], hwaddr[5]);

    return 1;
}

int sys_getmyip(const char *interface, char *addr)
{
#ifdef SUPPORT_BRIDGE
	int bridge;    
	Scf_getBridgeMode(&bridge);    
	if(bridge == 1)	
		strcpy(addr, sys_getnetworkinfo(IF_NAME_DEV_BR0, SIOCGIFADDR));    
	else	
#endif		
	strcpy(addr, sys_getnetworkinfo(interface, SIOCGIFADDR));
	return 1;
}


int sys_getmynetmask(const char *interface, char *addr)
{
#ifdef SUPPORT_BRIDGE
	int bridge; 
	Scf_getBridgeMode(&bridge); 
	if (bridge == 1) 
	{		
		strcpy(addr, sys_getnetworkinfo(IF_NAME_DEV_BR0, SIOCGIFNETMASK));
	} 
	else		
#endif
	strcpy(addr, sys_getnetworkinfo(interface, SIOCGIFNETMASK));
    return 1;
}

int sys_getmybroadcast(const char *interface, char *addr)
{
    strcpy(addr, sys_getnetworkinfo(interface, SIOCGIFBRDADDR));
    return 1;
}


void sys_setEventFlag(unsigned long old_ip)
{
    char cmd[64];
    struct in_addr in;

    in.s_addr = old_ip;
    sprintf(cmd, "echo -n %s > %s", inet_ntoa(in), SYS_IPCHANGE_FLAG);
    system(cmd);
}

/**
 * \brief The system event API. This is the main function to issue an events.
 * \brief The eventd will do the log\mail\trap according to the settings automatically.
 * \param [in] event_id An id to specify the event content.
 * \param [in] buf Strings containing event information.
 * \param [in] len The length of parameter 'buf'.
 * \retval 0 Success.
 * \retval -1 Fail.
 */
#if 0 
int sys_send_events(int event_id, int context)
{
    int qid, msgsz;
    struct msgbuf data;

    qid = msgget(EVENT_MSGKEY, 0);
    if (qid==-1)
    {
        dbg_printf("Message queue is missing! Check if eventd is running or not.\n");
        return -1;
    }

    memset(&data, 0, sizeof(data));
    data.mtype = 1;
    data.event_id = event_id;
    data.context = context;

    msgsz = sizeof(data)-sizeof(long);
    msgsnd(qid, &data, msgsz, 0);

    return 0;
}
#endif
int sys_send_events_ex(int event_id, int context, unsigned long opmode, int ip, int port)
{
    int qid, msgsz;
    struct msgbuf data;

    qid = msgget(EVENT_MSGKEY, 0);
    if (qid==-1)
    {
        dbg_printf("Message queue is missing! Check if eventd is running or not.\n");
        return -1;
    }

    memset(&data, 0, sizeof(data));
    data.mtype = 1;
    data.event_id = event_id;
    data.context = context;
    data.opmode = opmode;
    data.ip = ip;
    data.port = port;

    msgsz = sizeof(data)-sizeof(long);
    msgsnd(qid, &data, msgsz, 0);

    return 0;
}


void sys_reload_eventd()
{
    kill(sys_get_pid(0, EVENT_PID_FILE), SIGHUP);
    return;
}

/**
 * \brief The miscd API. This is the main function to issue a miscd job.
 * \param [in] event_id An id to specify the miscd job.
 * \param [in] buf Strings containing job information.
 * \param [in] len The length of parameter 'buf'.
 * \retval 0 Success.
 * \retval -1 Fail.
 */
int sys_miscd_send(int event_id, int value)
{
    int qid, msgsz;
    struct miscdbuf data;
	int retry_cnt = 0;

	while(1)
	{
	    qid = msgget(MISCD_MSGKEY, 0);
	    if (qid==-1)
	    {
			retry_cnt++;
	        dbg_printf("Message queue is missing! Check if miscd is running or not.\n");
	
			if(retry_cnt >= 5)
	        {
	        	return -1;
	        }
	        sleep(1);
	    }
	    else
	    {
	    	break;	
	    }
	}

    memset(&data, 0, sizeof(data));
    data.mtype = 1;
    data.event_id = event_id;
    data.value = value;

    msgsz = sizeof(data)-sizeof(long);
    msgsnd(qid, &data, msgsz, 0);

    return 0;
}

void sys_reload_miscd()
{
    kill(sys_get_pid(0, MISCD_PID_FILE), SIGHUP);
    return;
}

int sys_gethostTableEntry(int index, char *host, int host_size, char *ip, int ip_size)
{
    FILE *fp;
    char *token;
    char *delim = " \t\n\r";
    char line[180];
    int j = 0;

    fp = fopen("/etc/hosts", "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp))
    {
        _remove_comment(line);
        _trim_left_space(line);

        token = strtok(line, delim);
        if (token)
        {
            j++;
            if (j == index)
            {
                if (_strlen(token) < ip_size)
                    sprintf(ip, "%s", token);
                token = strtok(NULL, delim);
                if (token && _strlen(token) < host_size)
                    sprintf(host, "%s", token);
            }
        }
    }
    fclose(fp);
    return 0;
}

int sys_sethostTableEntry(char host[16][40], char ip[16][32])
{
    FILE *fp;
    char line[180];
    int i = 0;

    fp = fopen("/etc/hosts", "w");
    if (fp == NULL)
        return -1;

    for (i = 0; i < 16; i++)
    {
        if ((_strlen(ip[i]) > 0) && (_strlen(host[i]) > 0))
        {
            memset(line, 0, sizeof(line));
            sprintf(line, "%s %s\n", ip[i], host[i]);
            fputs(line, fp);
        }
    }
    fclose(fp);

    return 0;
}

void sys_reboot(int mode)
{
#ifdef CROSS
#define RB_AUTOBOOT 0x01234567  // This define is learned from busybox.
    char cmd[64];
    int pid;

    if(mode == 0)
    {
        //original
        sprintf(cmd, "touch %s", SYS_WARMSTART_FLAG);
        system(cmd);
        sleep(2);
        reboot(RB_AUTOBOOT);
    }
    else if(mode == 1)
    {
        if((pid = fork()) < 0)
        {
            //error
            return;
        }
        else if (pid == 0) //reboot process
        {
            setsid(); /* become session leader */
            chdir("/"); /* change working directory */
            umask(0); /* clear file mode creation mask */
            close(0); /* close stdin */
            close(1); /* close stdout */
            close(2); /* close stderr */

            sprintf(cmd, "touch %s", SYS_WARMSTART_FLAG);
            system(cmd);
            sleep(2);
            reboot(RB_AUTOBOOT);
        }
    }
#endif
    return;
}

/*
 * Get the active network port
 * 0: ethernet, 1: wlan
 */
int sys_getActiveNetworkPort(void)
{
    FILE *f;
    int ifnum=-1;

    if ((f = fopen(IF_ACTIVE_FILE, "rb")) == NULL)
        fprintf(stderr, "Couldn't open file \"%s\": %s\r\n", IF_ACTIVE_FILE, strerror(errno));
    else
    {
        fscanf(f, "%d\r\n", &ifnum);
        fclose(f);
    }

    return ifnum;
}

void sys_setActiveNetworkPort(int ifnum)
{
    FILE *f;

    if ((f = fopen(IF_ACTIVE_FILE, "wb")) == NULL)
        fprintf(stderr, "Couldn't open file \"%s\": %s\r\n", IF_ACTIVE_FILE, strerror(errno));
    else
    {
        fprintf(f, "%d\r\n", ifnum);
        fclose(f);
    }
    return;
}

int sys_checkEthLink(void)
{
    int fd;
    struct ifreq ifr;
    struct ethtool_value edata;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        dbg_printf("sys_getActiveNetworkPort > socket open failed\r\n");
        return D_FAILURE;
    }

    /* check ethernet link status */
    memset(&ifr, 0 , sizeof(ifr));
    edata.cmd = ETHTOOL_GLINK;
    strncpy(ifr.ifr_name, IF_NAME_DEV_ETH0, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_data = (char *) &edata;

    if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)
    {
        dbg_printf("sys_getActiveNetworkPort > ETHTOOL_GLINK failed: %s\r\n", strerror(errno));
        close(fd);
        return D_FAILURE;
    }
    close(fd);

    return edata.data;
}

int sys_getActiveNetworkName(char* buf)
{
    int ifnum;

    if(buf == NULL)
        return -2;

    ifnum = sys_getActiveNetworkPort();
    if (ifnum == IF_NUM_DEV_ETH0)
        strcpy(buf, IF_NAME_DEV_ETH0);
    else if (ifnum == IF_NUM_DEV_ETH1)
        strcpy(buf, IF_NAME_DEV_ETH1);
    else
    {
        dbg_printf("sys_getActiveIP > unknown interface\r\n");
        return -1;
    }

    return 0;
}

int sys_getActiveIP(char *addr)
{
    char buf[10];

    if(sys_getActiveNetworkName(buf) != 0)
        return -1;

    return sys_getmyip(buf, addr);
}

int sys_getActiveNetmask(char *addr)
{
    char buf[10];

    if(sys_getActiveNetworkName(buf) != 0)
        return -1;

    return sys_getmynetmask(buf, addr);
}

int sys_getActiveBcase(char *addr)
{
    char buf[10];

    if(sys_getActiveNetworkName(buf) != 0)
        return -1;

    return sys_getmybroadcast(buf, addr);
}

/*
 * get netstat
 */

static int sys_netstat_tcp(int flags, _NETSTAT_TABLE nt[], int nt_size);
static int sys_netstat_udp(int flags, _NETSTAT_TABLE nt[], int nt_size);

int sys_getnetstat(int flags, _NETSTAT_TABLE nt[], int nt_size)
{
    int tcp, udp, raw;

    tcp = udp = raw = 0;
    if (flags & NETSTAT_TCP)
        tcp = sys_netstat_tcp(flags, &nt[0], nt_size);
    if (flags & NETSTAT_UDP)
        udp = sys_netstat_udp(flags, &nt[tcp], nt_size-tcp);
    return tcp + udp;
}

#define _NETSTAT_PASTER_FLAG_SKIP   0
#define _NETSTAT_PASTER_FLAG_NORMAL 1

static int _netstat_paster(FILE *fp, int flag,  _NETSTAT_TABLE* nt)
{
    char buffer[8192], local_addr[64], rem_addr[64];
    int local_port, rem_port, state, timer_run, num;
    u_long rxq, txq, time_len;

    if(fgets(buffer, sizeof(buffer), fp) == 0)
        return -1;

    if(flag == _NETSTAT_PASTER_FLAG_SKIP)
        return 0;

    num = sscanf(buffer,
        "%*d: %64[0-9A-Fa-f]:%X "
        "%64[0-9A-Fa-f]:%X %X "
        "%lX:%lX %X:%lX "
        "%*X %*d %*d %*d\n",
        local_addr, &local_port,
        rem_addr, &rem_port, &state,
        &txq, &rxq, &timer_run, &time_len);

    if(num < 9)
        return -2;


    memset(&(nt->local_addr), '\0', sizeof(struct sockaddr_in));
    memset(&(nt->rem_addr), '\0', sizeof(struct sockaddr_in));

    /* Address family */
    nt->local_addr.sin_family = AF_INET;
    nt->rem_addr.sin_family = AF_INET;

    /* Port Number */
    nt->local_addr.sin_port = local_port;
    nt->rem_addr.sin_port = rem_port;

    /* Internet address */
    sscanf(local_addr, "%X", &((struct sockaddr_in *) &nt->local_addr)->sin_addr.s_addr);
    sscanf(rem_addr, "%X", &((struct sockaddr_in *) &nt->rem_addr)->sin_addr.s_addr);

    nt->state = state;
    nt->txq = txq;
    nt->rxq = rxq;
    nt->timer_run = timer_run;
    nt->time_len = time_len;

    return 1;
}

static int sys_netstat_tcp(int flags, _NETSTAT_TABLE nt[], int nt_size)
{
    FILE *fp;
    int nl;

    nl = 0;
    if ((fp = fopen(_PATH_PROCNET_TCP, "r")) == NULL)
        return 0;

    if(_netstat_paster(fp, _NETSTAT_PASTER_FLAG_SKIP, &nt[nl-1]) < 0)
        goto end_netstat_tcp;

    while(nl < nt_size)
    {
        if(_netstat_paster(fp, _NETSTAT_PASTER_FLAG_NORMAL, &nt[nl]) < 0)
            break;

        nt[nl].protocol = SYS_NETSTAT_TCP;

        nl++;
    }

end_netstat_tcp:
    fclose(fp);
    return nl;
}


static int sys_netstat_udp(int flags, _NETSTAT_TABLE nt[], int nt_size)
{
    int nl = 0;
    FILE *fp;

    if ((fp = fopen(_PATH_PROCNET_UDP, "r")) == NULL)
        return 0;

    nl = 0;
    if(_netstat_paster(fp, _NETSTAT_PASTER_FLAG_SKIP, &nt[nl-1]) < 0)
        goto end_netstat_udp;

    while(nl < nt_size)
    {
        if(_netstat_paster(fp, _NETSTAT_PASTER_FLAG_NORMAL, &nt[nl]) < 0)
            break;

        nt[nl].protocol = SYS_NETSTAT_UDP;

        if(nt[nl].state == TCP_ESTABLISHED)
            nt[nl].state = 1;
        else
            nt[nl].state = 0;

        nl++;
    }

end_netstat_udp:
    fclose(fp);
    return nl;
}

extern const char *SxNetDefaultGateway(char *ifname);
char *sys_getGateway(char *ifname)
{
#ifdef SUPPORT_BRIDGE
	int bridge;	
	Scf_getBridgeMode(&bridge);	
	if (bridge == 1) 
	{		
		return (char*)SxNetDefaultGateway("br0");	
	} 
	else
#endif
	{
		ifname = _chk_interface(ifname);
		return (char*)SxNetDefaultGateway(ifname);
	}	
}


#define OP_REALCOM CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_REALCOM
#define OP_RFC2217 CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_RFC2217
#define OP_TCPSERVER CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPSERVER
#define OP_TCPCLIENT CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPCLIENT
#define OP_UDP CFG_APPLICATION_SOCKET | CFG_OPMODE_UDP
#define OP_PAIRMASTER CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_MASTER
#define OP_PAIRSLAVE CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_SLAVE
#define OP_EMODEM CFG_APPLICATION_ETH_MODEM
#ifdef SUPPORT_RTERMINAL_MODE
#define OP_RTERMINAL CFG_APPLICATION_RTERMINAL
#endif

#define LINK_NUM DCF_MAX_SERIAL_PORT * 8 +32


int sys_getS2NStatus(int port, char ip[8][S2N_IP_LEN])
{
    int op, compare = 0, pid, tmp, count, nl;
    u_short data_port, cmd_port;
    char buf[8192], name[256];
    FILE *pPidfile;
    DIR *dp;
    struct dirent *ep;
    struct in_addr in;
    _NETSTAT_TABLE net_tbl[LINK_NUM];
    FILE *fp;
    off_t offset;

    if (ip == NULL)
        return 0;

    memset(buf,0,sizeof(buf));
    memset(net_tbl,0,sizeof(net_tbl));

    sprintf(buf,DSPORTD_PID_FILE, port+1);
    pPidfile = fopen(buf,"r");

    if (pPidfile == NULL)
        return -1;

    memset(buf,0,sizeof(buf));
    fgets(buf, sizeof(buf) , pPidfile);
    pid = _atoi(buf);

    op = Scf_getOpMode(port+1);

    switch (op)
    {
    case OP_REALCOM:
        data_port = 950 + port;
        compare = 0;
        break;

    case OP_RFC2217:
    case OP_TCPSERVER:
    case OP_EMODEM:
    case OP_PAIRSLAVE:
        Scf_getTcpServer(port+1, &data_port, &cmd_port);
        compare = 0;
        break;

    case OP_TCPCLIENT:
    case OP_PAIRMASTER:
        compare = 1;
        break;

    case OP_UDP:
        fclose(pPidfile);
        return 0; // dont have to show anything

#ifdef SUPPORT_RTERMINAL_MODE
	case OP_RTERMINAL:
	{
		int tcpport, mapkeys;
        Scf_getRTerminal(port+1, &tcpport, &mapkeys);
		data_port = (u_short)tcpport;
		printf("sys_getS2NStatus: %d", data_port);
        compare = 0;
	}
		break;
#endif
    default:  //disable
        fclose(pPidfile);
        return 0;
    }

    sprintf(buf, "/proc/%d/fd", pid);
    dp = opendir (buf);

    if (dp == NULL)
    {
        fclose(pPidfile);
        return -1;
    }

    offset = telldir(dp);
    tmp = 0;
    nl = 0;

    if ((fp = fopen(_PATH_PROCNET_TCP, "r")) == NULL)
    {
        closedir(dp);
        fclose(pPidfile);
        return 0;
    }

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        int d, local_port, rem_port, state;
        int timer_run, uid, timeout, num;
        char local_addr[64], rem_addr[64], more[512];
        u_long rxq, txq, time_len, retr, inode;

        if (nl)
        {
            more[0] = '\0';
            num = sscanf(buf,
                         "%d: %64[0-9A-Fa-f]:%X %64[0-9A-Fa-f]:%X %X %lX:%lX %X:%lX %lX %d %d %ld %512s\n",
                         &d, local_addr, &local_port, rem_addr, &rem_port, &state,
                         &txq, &rxq, &timer_run, &time_len, &retr, &uid, &timeout, &inode, more);

            if (inode == 0)
                continue;

            while ((ep = readdir(dp)) != NULL)
            {
                int ino;

                sprintf(name, "/proc/%d/fd/%s", pid,ep->d_name);
                count = readlink(name, buf, sizeof(buf));
                buf[count] = 0;
                ino = 0;
                sscanf(buf, "socket:[%d]", &ino);

                if (compare)
                {
                    if ((inode == ino) && (state == TCP_ESTABLISHED))
                    {
                        sscanf(rem_addr, "%X", &in.s_addr);
                        strcpy(ip[tmp],inet_ntoa(in));
                        tmp++;
                        break;
                    }
                }
                else
                {
                    if ((inode == ino) && (data_port == local_port) &&
                            (state == TCP_ESTABLISHED) )
                    {
                        sscanf(rem_addr, "%X", &in.s_addr);
                        strcpy(ip[tmp],inet_ntoa(in));
                        tmp++;
                        break;
                    }
                }
            }

        }
        seekdir(dp, offset);
        nl++;

        if (tmp == 8)
        {
            fclose(fp);
            closedir(dp);
            fclose(pPidfile);
            return 0;
        }
    }
    fclose(fp);
    closedir(dp);
    fclose(pPidfile);

    return 0;

}

#ifdef SUPPORT_EXTERNAL_UART
#include <sio.h>
#include <linux/serial.h>
static int __get_externalPortStatus(int port, PORT_STATUS *pstatus)
{
    SIODATA siodata;
    _port_status_ext ext_status;

    if ( sio_getsiodata(port+1, &siodata) < 0)
        return -1;

    if (sio_getStatus(port+1, &ext_status) < 0)
        return -2;

    pstatus->port = port;
    pstatus->total_tx = siodata.tx_cnt;
    pstatus->total_rx = siodata.rx_cnt;
    pstatus->tx = siodata.up_txcnt;
    pstatus->rx = siodata.up_rxcnt;
    pstatus->ospeed = siodata.baudrate;
    pstatus->ispeed = siodata.baudrate;
    pstatus->fifo = siodata.fifo;
    pstatus->DSR = ((siodata.lstatus & S_DSR) ? 1: 0);
    pstatus->CTS = ((siodata.lstatus & S_CTS) ? 1: 0);
    pstatus->DCD = ((siodata.lstatus & S_CD) ? 1: 0);
    pstatus->mmio = ext_status.mmio;
    pstatus->irq = ext_status._config.irq;
    pstatus->fr = ext_status.frame_error_cnt; // frame error
    pstatus->pr = ext_status.parity_error_cnt; // parity error
    pstatus->_or = ext_status.overrun_error_cnt; // overrun error
    pstatus->br = ext_status.break_cnt; // break
    pstatus->c_flag = ext_status.c_flag;
    pstatus->i_flag = ext_status.i_flag;
    pstatus->DTR = ext_status.dtr;
    pstatus->RTS = ext_status.rts;

    return 0;
}
#endif // SUPPORT_EXTERNAL_UART

#ifdef SUPPORT_INTERNAL_UART
static int __get_internalPortStatus(int port, PORT_STATUS *p_status)
{
    FILE *fp;
    char buf[512], signal_str[20];
    int num, p, irq, fifo;
    u_int c_flag, i_flag, mmio, total_tx, total_rx, tx, rx;
    u_int fr, pr, or, br, ospeed, ispeed;

    fp = fopen(PROC_DRIVER,"r");
    if (fp == NULL)
        return -1;

    p = -1;
    memset(buf,0,sizeof(buf));

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
        memset(signal_str,0,sizeof(signal_str));
        num = sscanf(buf,
                     "%d: mmio:0x%X irq:%d utx:%u urx:%u tx:%u rx:%u fr:%u pr:%u or:%u br:%u ospeed:%u ispeed:%u cf:%u if:%u fifo:%d %20s",
                     &p, &mmio, &irq, &total_tx, &total_rx, &tx, &rx, &fr, &pr, &or, &br, &ospeed, &ispeed, &c_flag, &i_flag, &fifo,signal_str);

        if (p == port)
        {
            p_status->port = p;
            p_status->mmio = mmio;
            p_status->irq = irq;
            p_status->total_tx = total_tx;
            p_status->total_rx = total_rx;
            p_status->tx = tx;
            p_status->rx = rx;
            p_status->fr = fr;
            p_status->pr = pr;
            p_status->_or = or;
            p_status->br = br;
            p_status->ospeed = ospeed;
            p_status->ispeed = ispeed;
            p_status->c_flag = c_flag;
            p_status->i_flag = i_flag;
            p_status->fifo = fifo;
            p_status->DSR = (strstr(signal_str,"DSR") ? 1: 0);
            p_status->DTR = (strstr(signal_str,"DTR") ? 1: 0);
            p_status->RTS = (strstr(signal_str,"RTS") ? 1: 0);
            p_status->CTS = (strstr(signal_str,"CTS") ? 1: 0);
            p_status->DCD = (strstr(signal_str,"CD") ? 1: 0);
            break;
        }
    }

    fclose(fp);

    return 0;
}
#endif // SUPPORT_INTERNAL_UART

#include <sio.h>
int sys_getPortStatus(int port, PORT_STATUS *p_status)
{
    int mode;

    mode = _sio_getPortType(port+1);

    switch (mode)
    {
#ifdef SUPPORT_EXTERNAL_UART
    case SERIAL_PORT_EXTERNAL_FLAG:
        return __get_externalPortStatus(port, p_status);
#endif // SUPPORT_EXTERNAL_UART
#ifdef SUPPORT_INTERNAL_UART
    case SERIAL_PORT_INTERNAL_FLAG:
        return __get_internalPortStatus(port, p_status);
#endif // SUPPORT_INTERNAL_UART
    }

    return -2;
}

#ifdef SUPPORT_DIO
/*
 *	get the single IO mode.
 */
int	DIO_GetSingleIO(int io, int *mode)
{
    int ret;

    if (io < 0 || io > MAX_DIO)
        return D_FAILURE;
    ret = sys_get_dio_mode(io);
    if (ret < 0)
        return ret;
    else
        *mode = ret;

    return D_SUCCESS;
}

/*
 *	get the single IO now status
 */
int	DIO_GetSingleIOStatus(int io, int *highlow)
{
    int ret;

    if (io < 0 || io > MAX_DIO)
        return D_FAILURE;
    ret = sys_get_dio(io);
    if (ret < 0)
        return ret;
    else
        *highlow = ret;

    return D_SUCCESS;
}

int sys_reload_diod(int sig)
{
    u_int pid;
    char cmd[64];

    if ((sig != DIOD_RELOAD_FUNC)
            && (sig != DIOD_RELOAD_ALL)
            && (sig != DIOD_RESTART) )
        return D_FAILURE;

    pid = __sys_get_pid("/var/run/moxa/diod.pid");
    if (pid == 0)
        return D_FAILURE;

    if (sig == DIOD_RESTART)
    {
        sprintf(cmd, "kill -9 %u && diod &", pid);
        system(cmd);
    }
    else
    {
        sprintf(cmd, "kill -USR%d %u", sig, pid);
        system(cmd);
    }

    return D_SUCCESS;
}
#endif // SUPPORT_DIO


int sys_getSCMStatus()
{
    FILE *f;
    int flag;

    if ((f = fopen(SYS_SCMSTATUS_FLAG, "r")) == NULL)
    {
        return SCM_DATA_MODE;
    }
    else
    {
        fscanf(f, "%d\r\n", &flag);
        fclose(f);
    }
    return flag;
}

int sys_setSCMStatus(int flag)
{
    FILE *f;

    VERIFY_RANGE(flag, SCM_DATA_MODE, SCM_CMD_MODE);
    if ((f = fopen(SYS_SCMSTATUS_FLAG, "w")) == NULL)
        fprintf(stderr, "Couldn't open file \"%s\": %s\r\n", IF_ACTIVE_FILE, strerror(errno));
    else
    {
        fprintf(f, "%d\r\n", flag);
        fclose(f);
    }
    return 0;
}

/*
 * Get site survey flag
 * 0: scan is completed, 1: scan is in progress
 */
int sys_getSiteSurveyFlag(void)
{
    FILE *f;
    int flag = -1;

    if ((f = fopen(SITE_SURVEY_FLAG, "rb")) == NULL)
        fprintf(stderr, "Couldn't open file \"%s\": %s\r\n", SITE_SURVEY_FLAG, strerror(errno));
    else
    {
        fscanf(f, "%d\r\n", &flag);
        fclose(f);
    }

    return flag;
}

void sys_setSiteSurveyFlag(int flag)
{
    FILE *f;

    if ((f = fopen(SITE_SURVEY_FLAG, "wb")) == NULL)
        fprintf(stderr, "Couldn't open file \"%s\": %s\r\n", SITE_SURVEY_FLAG, strerror(errno));
    else
    {
        fprintf(f, "%d\r\n", flag);
        fclose(f);
    }
}

int sys_log_clear(void)
{
	FILE *fplog_index;
	char file_path[64], tmp[8];

	system("cat /dev/null >/mnt/log ");

	sprintf(file_path, "/mnt/log_index");

	fplog_index = fopen(file_path, "r+");

	if (fplog_index == NULL) {
		return -1;
	}

	sprintf(tmp, "%d;%d", 0, 0);

	rewind(fplog_index);

	fputs(tmp, fplog_index);

	fclose(fplog_index);

	return 0;
}


#ifdef SUPPORT_STATIC_GARP
int check_ip(char *buf, int size)
{
	u_char	ip[4];
	int	i, j, v;
	
	for(i = 0; i < 4; i++)
	{
		v = 0;
		for(j = 0; j < 3 && j < size; j++)
		{
			if(buf[j] == '.')
				break;
			if(buf[j] < '0' || buf[j] > '9')
				return 0;
			v = v * 10 + (buf[j] - '0');
			if(v > 255)
				return 0;
		}
		if(j == 0)
			return 0;
		
		if(i != 3)
		{
			if(j >= size)
				return 0;
			if(j == 3) {
				if(buf[j] != '.') {
					return 0;
				}
			}
		}
		else
		{
			if(j < size)
				return 0;
		}
		ip[i] = v;
		buf += j + 1;
		size -= j + 1;
	}
	if(ip[0]+ip[1]+ip[2]+ip[3] == 0)
	{
		return 0;
	}	
	if(ip[0]+ip[1]+ip[2]+ip[3] == 255*4)
	{
		return 0;
	}	
	return 1;
}

int check_mac(char *buf, int size)
{
	int	i, j, v;
	int tmp;
	
	for(i = 0; i < 6; i++)
	{
		v = 0;
		for(j = 0; j < 2 && j < size; j++)
		{
			if(buf[j] == ':')
				break;
			if(buf[j] >= '0' && buf[j] <= '9') {
				tmp = buf[j] - '0';
			}
			else if(buf[j] >= 'A' && buf[j] <= 'F') {
				tmp = buf[j] - 'A' + 10;
			}
			else if(buf[j] >= 'a' && buf[j] <= 'f') {
				tmp = buf[j] - 'a' + 10;
			}
			else {
				return 0;
			}
			v = v * 16 + tmp;
			if(v > 255)
				return 0;
		}
		if(j == 0)
			return 0;
		if(i != 5)
		{
			if(j >= size)
				return 0;
			if(j == 2) {
				if(buf[j] != ':') {
					return 0;
				}
			}
		}
		else
		{
			if(j < size)
				return 0;
		}
		buf += j + 1;
		size -= j + 1;
	}
	return 1;
}
#endif

int cmpFile(char *filename1, char *filename2)
{
    int fd[2];
    char buf[2][4096];
    ssize_t len[2];
    int i, ret, j = 0;

    fd[0] = open(filename1, O_RDONLY);
    if(fd[0] == -1)
        return 1;	/* filename1 not exist */
 
    fd[1] = open(filename2, O_RDONLY);
    if(fd[1] == -1)
    {
        close(fd[0]);
        return 2;	/* filename2 not exist */
    }

    ret = 0;

    while(1)
    {
        for(i = 0; i < 2; i++)
        {
            len[i] = read(fd[i], buf[i], sizeof(buf[0]));
            if(len[i] < 0)
            {
                ret = -1;
                goto done;
            }
        }
        if(len[0] != len[1])
        {
            ret = -1;
            break;
        }
        if(len[0] == 0)  /* EOF */
            break;
        if(memcmp(buf[0], buf[1], len[0]))
        {
            ret = -1;
            break;
        }
        j += len[0];
    }

done:
    for(i = 0; i < 2; i++)
        close(fd[i]);

    return ret;
}

int cmpDir(char *dirname1, char *dirname2)
{
#define TMP_DIFF_RESULT "/var/tmp/diff.result"
    char cmd[512];
    struct stat sbuf;
    int ret = -1;

    sprintf(cmd, "diff %s %s 1>"TMP_DIFF_RESULT" 2>&1", dirname1, dirname2);
    system(cmd);

    if(stat(TMP_DIFF_RESULT, &sbuf) == 0)
    {
        if(sbuf.st_size == 0)	/* not output for 'diff' command means no difference between two directories */
            ret = 0;
    }

    system("rm -f "TMP_DIFF_RESULT);

    return ret;
}

int dirExist(char *dirname)
{
    struct stat sbuf;
    if((stat(dirname, &sbuf) == 0) && S_ISDIR(sbuf.st_mode))
        return 1;   // Exist
    return 0;   // NOT exist
}

int dns_lookup(char *hostname, char *actual_ip, unsigned int ip_len)
{
	struct addrinfo hints, *res = NULL, *ptr = NULL;
	int ret = -1;
	int i;

	/* Check ip len is enough */
	if(ip_len < INET6_ADDRSTRLEN)
		goto end;

	/* Check if there is empty in domain name */
	for(i = 0; i < ip_len; i++)
	{
		if(hostname[i] == ' ')
			goto end;
	}

	/* init hints */
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // only get IPv4
	hints.ai_socktype = SOCK_STREAM;
	/* init real_ip */
	memset(actual_ip, 0, ip_len);
	
	/* Get IP address */
	if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
		// Unable to get IP address for hostname
		goto end;
  	}
	for(ptr = res; ptr != NULL; ptr = ptr->ai_next) {
		if (ptr->ai_family == AF_INET) { // IPv4
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)ptr->ai_addr;
			inet_ntop(ptr->ai_family, &(ipv4->sin_addr), actual_ip, ip_len);
			ret = 0;
			break;
		}
	}

end:
	/* Release resource */
	if(res != NULL)
		freeaddrinfo(res);
	return ret;
}

int is_valid_domain_name(char *hostname, int name_len)
{
	int i;

	/* According to RFC 1123, domain name should not be larger than 255 bytes. */
	if(name_len > 255)
		return 0;
	
	/* Check the hostname length and it should not over name_len*/
	if(strlen(hostname) > name_len)
		return 0;

	/* Check the char in domain name */
	for(i = 0; i < name_len; i++)
	{
		/* range: 0-9 */
		if(hostname[i] >= 0x30 && hostname[i] <= 0x39)
			continue;
		/* range: A-Z */
		else if(hostname[i] >= 0x41 && hostname[i] <= 0x5A)
			continue;
		/* range: a-z */
		else if(hostname[i] >= 0x61 && hostname[i] <= 0x7A)
			continue;
		else if(hostname[i] == '.' || hostname[i] == '-')  // The special chars are available in domain name
			continue;
		else if(hostname[i] == '\0') // The end of domain name
			break;
		else
			return 0;
	}
	return 1;
}

/* 
 * The function is to escape " and \
 * that is to change " to \" and change \ to \\ 
 */
int escape_quote_in_string(char *str, char *new_buf, int new_buf_len)
{
	int i, j;
	int str_len = strlen(str);
	int quote_num = 0;

	/*Check the number of "*/
	for(i = 0; i < str_len; i++)
	{
		if(str[i] == '"')
			quote_num++;
	}
	/*buffer is not enough to put escape string*/
	if(new_buf_len <= quote_num+str_len)
		return -1;

	/*Start to escape buffer*/
	memset(new_buf, 0, new_buf_len);
	for(i = 0, j = 0; i < str_len; i++, j++)
	{
		if(str[i] == '"' || str[i] == '\\')
		{
			new_buf[j++] = '\\';  // escape character
			new_buf[j] = str[i];
		}else{
			new_buf[j] = str[i];
		}
	}
	return 0;
}

/*
 * Return:
 *  -1: system error.
 *  -2: MAC address does not belong to MOXA.
 */
int lock_program_with_mac(void)
{
    struct ifreq ifr;
    struct ifreq *it, *end;
    struct ifconf ifc;
    int sock;
    char buf[1024];
    unsigned char mac_address[6 + 1];
    const char max_prefix[3] = {0x00, 0x90, 0xe8};
    int ret = 0;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock == -1)
    {
        return -1;
    }

    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = buf;
    if (ioctl(sock, SIOCGIFCONF, &ifc) < 0)
    {
        ret = -1;
        goto EXIT;
    }

    it = ifc.ifc_req;
    end = it + (ifc.ifc_len / sizeof(struct ifreq));
    for (; it != end; ++it)
    {
        strcpy(ifr.ifr_name, it->ifr_name);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (!(ifr.ifr_flags & IFF_LOOPBACK))    // don't count loopback
            {
                if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
                {
                    memset(mac_address, 0, sizeof(mac_address));
                    memcpy(mac_address, ifr.ifr_hwaddr.sa_data, 6);

                    /*
                    printf("[%s %d] interface: %s, mac: %02x:%02x:%02x:%02x:%02x:%02x\n",
                            __func__, __LINE__, ifr.ifr_name,
                            mac_address[0], mac_address[1], mac_address[2],
                            mac_address[3], mac_address[4], mac_address[5]);
                    */

                    if (memcmp(mac_address, max_prefix, sizeof(max_prefix)) != 0)
                    {
                        ret = -2;
                        goto EXIT;
                    }
                }
                else
                {
                    ret = -1;
                    goto EXIT;
                }
            }
        }
        else
        {
            ret = -1;
            goto EXIT;
        }
    }

EXIT:
    close(sock);

    return ret;
}