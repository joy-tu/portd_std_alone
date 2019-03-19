#ifndef _SERCMD_H
#define _SERCMD_H

int sercmd_open(int port);
int sercmd_close(int port);
int sercmd_read(int port, char *buf, int len);
int sercmd_write(int port, char *buf, int len);
int sercmd_ioctl(int port, int baud, int mode);
int sercmd_notify_error(int port);

#endif /* _SERCMD_H */

