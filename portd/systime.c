/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    systime.c
    System time apis.
*/
#include <sys/times.h>

#define SYS_HZ      100
#define SYS_UNIT    (1000/SYS_HZ)

unsigned long sys_clock_ms(void)
{
    struct tms ts;
    return ((unsigned long)times(&ts))*SYS_UNIT;
}
