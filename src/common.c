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
#include <posix/net/if.h>
#include <common.h>

#define SYS_HZ      100
#define SYS_UNIT    (1000/SYS_HZ)


static char macbuf[18];

int64_t sys_clock_ms(void)
{
//    struct tms ts;
//    return ((unsigned long)times(&ts))*SYS_UNIT;
	return k_uptime_get();
}
