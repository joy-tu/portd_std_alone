/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    datalog.h

    data log processing

    2012-01-03	Albert Yu
		add license declaration
*/

#ifndef _DATALOG_H
#define _DATALOG_H

/*
 * APIs called by portd
 */
void log_init(int port);
int log_port (int port, char direction, char *msg, int length);
int log_sio_write(int port, char *buf, int len);
int log_sio_read(int port, char *buf, int len);

/*
 * APIs called by httpd or conosle
 */
int log_read_data(int port, char *buf, int size, int hex_mode);
int log_clear_data(int port);

#endif // _DATA_LOG_H
