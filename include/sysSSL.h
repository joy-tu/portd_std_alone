#ifndef __SYSSSL_H__
#define __SYSSSL_H__

#define D_SSL_CHECK_CERT    0x00000001L
#define D_SSL_CHECK_KEY     0x00000002L

int sslX509_info(int type, char *infile, char *info, int info_len);
int checkAndSetCertFile(int index, char* file, int len, char *errStr, int errlen);

#endif // __SYSSSL_H__

