/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    systime.c
    System time apis.

    2011-01-05  James Wang
        First release.
*/


#include <stdio.h>
#include <stdlib.h>
#include <header.h>
#include <time.h>
#include <sys/timeb.h>
#include <errno.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#ifndef __CYGWIN__
#include <linux/unistd.h>
#include <linux/kernel.h>
#endif // __CYGWIN__

#include <debug.h>
#include <unistd.h>
#include <sys/times.h>

#if 0
int sys_clock_get(long *second, long *millisecond)
{
    FILE *fp;
    char buffer[80];
    int num = 0;
    long s = 0, ms = 0;

    if( (fp = fopen("/proc/uptime", "r")) == NULL )
        return 0;

    if( fgets(buffer, sizeof(buffer), fp) != NULL )
        num = sscanf(buffer, "%ld.%ld %*d.%*d\n", &s, &ms);

    *second = s;
    *millisecond = s * 1000L + ms*10;
    fclose(fp);

    return num;
}

unsigned long sys_clock_s(void)
{
    long second, millisecond;
    sys_clock_get(&second, &millisecond);

    return (unsigned long) second;
}

unsigned long sys_clock_ms(void)
{
    long second, millisecond;
    sys_clock_get(&second, &millisecond);
    return (unsigned long) millisecond;
}

#else

#define SYS_HZ      100
#define SYS_UNIT    (1000/SYS_HZ)
unsigned long sys_clock_s(void)
{
    struct sysinfo info;

    sysinfo(&info);
    return info.uptime;
}

unsigned long sys_clock_ms(void)
{
    struct tms ts;
    return ((unsigned long)times(&ts))*SYS_UNIT;
}

#endif

void sys_time_get(int *year, int *month, int *day, int *hour, int *minute, int *second)
{
    FILE *fp;
    char buffer[80];

    system("date +%Y:%m:%d:%T > /var/date");

    if( (fp = fopen("/var/date", "r")) == NULL )
        return;

    if( fgets(buffer, sizeof(buffer), fp) != NULL )
        sscanf(buffer, "%d:%d:%d:%d:%d:%d\n", year, month, day, hour, minute, second);

    fclose(fp);
}

void sys_time_set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm tm;
    struct timeval  tv;
    struct timezone tz;
    time_t now_sec;
    char ori_tz[255] = {0};

    tm.tm_year = year - 1900;
    tm.tm_mon = month -1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;

    strncpy(ori_tz, getenv("TZ"), sizeof(ori_tz)-1);
    //Because TZ env value is a link file, so we need to update the env value to let tzset to take effect.
    setenv("TZ", "NULL", 1);
    tzset();
    setenv("TZ", ori_tz, 1);
    tzset();
    gettimeofday(&tv, &tz);
    now_sec = mktime(&tm);
    tv.tv_sec = now_sec;
    tv.tv_usec = 0;
    settimeofday(&tv,&tz);
    system("hwclock -w -u");

    memset(&tm,0,sizeof(struct tm));
}

long get_uptime()
{
#ifdef __CYGWIN__
    return 0;
#else
    struct sysinfo s_info;
    int error;

    error = sysinfo(&s_info);
    if(error)
        printf("%s: error code %d\n", __func__, error);

    return s_info.uptime;
#endif // __CYGWIN__
}

unsigned long sys_get_jiffies(void)
{
    struct tms ts;
    return (unsigned long)times(&ts);
}

