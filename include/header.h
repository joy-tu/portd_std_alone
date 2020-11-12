#ifndef _HEADER_H_
#define _HEADER_H_

/*****************************************************************************/
/*                              DATA TYPES                                   */
/*****************************************************************************/

#include <stdlib.h>
#include <errno.h>
#include <posix/netinet/tcp.h>
#include <posix/sys/ioctl.h>

#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#endif

#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#endif

#ifndef _INT8U_
    #define _INT8U_
    typedef unsigned char  INT8U;   /* Unsigned  8 bit quantity     	*/
#endif

#ifndef _INT8S_
    #define _INT8S_
    typedef signed   char  INT8S;   /* Signed 8 bit quantity			*/
#endif

#ifndef _INT16U_
    #define _INT16U_
    typedef unsigned short INT16U;  /* Unsigned 16 bit quantity			*/
#endif

#ifndef _INT16S_
    #define _INT16S_
    typedef signed   short INT16S;  /* Signed 16 bit quantity			*/
#endif

#ifndef _INT32U_
    #define _INT32U_
    typedef unsigned int   INT32U;  /* Unsigned 32 bit quantity			*/
#endif

#ifndef _INT32S_
    #define _INT32S_
    typedef signed   int   INT32S;  /* Signed 32 bit quantity			*/
#endif

#ifndef _FP32_
    #define _FP32_
    typedef float          FP32;    /* Single precision floating point	*/
#endif

#ifndef _FP64_
    #define _FP64_
    typedef double         FP64;    /* Double precision floating point	*/
#endif

#endif

