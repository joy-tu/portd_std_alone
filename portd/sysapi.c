/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    sysapi.c
    System generic API.
*/
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sysapi.h>
#include "../message.h"
#include "../config.h"

static char macbuf[18];

extern struct runtime_config Grun_conf;

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