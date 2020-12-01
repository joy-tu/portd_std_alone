/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    common.c
    common function for portd .
*/
#include <sys/times.h>
#include <stdio.h>
#include <errno.h>
//#include <sys/ioctl.h>
#include <platform.h>
#ifdef LINUX
#include <net/if.h>
#elif defined(ZEPHYR)
#include <posix/net/if.h>
#endif
#include <common.h>

//#include "../config.h"

#define SYS_HZ      100
#define SYS_UNIT    (1000/SYS_HZ)


static char macbuf[18];
#ifdef LINUX
unsigned long sys_clock_ms(void)
{
    struct tms ts;
    return ((unsigned long)times(&ts))*SYS_UNIT;
}
#elif defined(ZEPHYR)
int64_t sys_clock_ms(void)
{
	return k_uptime_get();
}
#endif
