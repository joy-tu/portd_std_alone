
#ifndef _DEBUG_H
#define _DEBUG_H


#define __CONFIG_DEBUG    0
#define __SIO_DEBUG       0
#define __PORTD_DEBUG     0



#if __CONFIG_DEBUG
#	define CONFIG_DEBUG    printf
#else
#	define CONFIG_DEBUG(...)
#endif

#if __SIO_DEBUG
#	define SIO_DEBUG       printf
#else
#	define SIO_DEBUG(...)
#endif

#if __PORTD_DEBUG
#	define PORTD_DEBUG       printf
#else
#	define PORTD_DEBUG(...)
#endif

#define DEBUG_W1    0
//#define DEBUG_GET_CONFIG	1

#ifdef DEBUG_W1
#include <stdio.h>
#define dbg_printf(...) \
	do { \
		char tmpbuf[128]; \
		int ii; \
		ii = sprintf(tmpbuf, "printf \""); \
		ii += sprintf(&tmpbuf[ii], __VA_ARGS__); \
		sprintf(&tmpbuf[ii], "\" >> /var/log/debug"); \
		system(tmpbuf); \
	} while(0)
#define nc_printf(ip,port,...) \
	do { \
		char tmpbuf[128]; \
		int ii; \
		ii = sprintf(tmpbuf, "printf \""); \
		ii += sprintf(&tmpbuf[ii], __VA_ARGS__); \
		sprintf(&tmpbuf[ii], "\" | nc %s %d",ip,port); \
		system(tmpbuf); \
	} while(0)
#else
#define dbg_printf(...)
#define nc_printf(ip,port,...)
#endif
#endif
