/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _COMMON_H
#define _COMMON_H
#include "platform.h"
#ifdef LINUX
#include <netinet/in.h>
#elif defined(ZEPHYR)
#include <posix/netinet/in.h>
#endif
/* For Event Notification */
#define EVENT_ID_OPMODE_CONNECT     0x400   // Connect
#define EVENT_ID_OPMODE_DISCONNECT  0x401   // Disconnect

/* Time Calculation */
#ifdef LINUX
unsigned long sys_clock_ms(void);
#elif defined(ZEPHYR)
int64_t sys_clock_ms(void);
#endif

#endif /* _COMMON_H */

