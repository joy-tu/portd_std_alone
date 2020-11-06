
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include <netinet/in.h>

u_int __sys_get_pid(char *filename);
int __sys_save_pid(char* filename);
unsigned int sys_get_pid(int port, const char* pidfile);
int sys_save_pid(int port, const char* pidfile);
void sys_rm_pid(int port, const char* pidfile);
int sys_daemon_init(void);
#endif /* _SYSAPI_H_ */
