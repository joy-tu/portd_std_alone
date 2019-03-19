#ifndef __REDBOOT_H__
#define __REDBOOT_H__

#include <header.h>
/*
 * Get Redboot config function
 */
int redboot_get_serialNo(void);
int redboot_get_fullSerialNo(char *buffer, int bufSize);
int redboot_get_MAC(char* _mac, int _buf_size);
int redboot_get_flag(void);
int redboot_get_hwextid(void);

/*
 * veriosn example:
 * version 1.0      ==> return 0x01000000
 * version 1.4      ==> return 0x01040000
 * version 1.4.9    ==> return 0x01040900
 */
unsigned int redboot_get_bios_version(void);

/*
 * Set Redboot config function
 */
int redboot_set_flag(int flag);

/*
 * Flag value
 */
#define REDBOOT_FLAG_MP         0x01
#define REDBOOT_FLAG_SERMAC     0x02
#define REDBOOT_FLAG_BR         0x08
#define REDBOOT_FLAG_EOT        0x04
#define REDBOOT_FLAG_HWEXTID    0x10
#define REDBOOT_FLAG_WBR        0x20

/*
 * HW external ID
 */
#ifdef w1
#define REDBOOT_EXTID_W1_US     1
#define REDBOOT_EXTID_W1_EU     2
#define REDBOOT_EXTID_W1_JP     3
#elif defined (w2x50a)
#if 0
#define REDBOOT_EXTID_W2150A_US 1
#define REDBOOT_EXTID_W2150A_EU 2
#define REDBOOT_EXTID_W2150A_JP 3
#define REDBOOT_EXTID_W2250A_US 4
#define REDBOOT_EXTID_W2250A_EU 5
#define REDBOOT_EXTID_W2250A_JP 6
#define REDBOOT_EXTID_W2150A_CN 9
#define REDBOOT_EXTID_W2250A_CN 10
#define REDBOOT_EXTID_W2150A    0x08
#define REDBOOT_EXTID_W2250A    0x0C
#endif
#define REDBOOT_EXTID_W5150A_6IO_US 		0x01
#define REDBOOT_EXTID_W5150A_6IO_EU 		0x02
#define REDBOOT_EXTID_W5150A_6IO_JP 		0x03
#define REDBOOT_EXTID_W5150A_6IO_CN 		0x04
#define REDBOOT_EXTID_W5250A_6IO_US 		0x05
#define REDBOOT_EXTID_W5250A_6IO_EU 		0x06
#define REDBOOT_EXTID_W5250A_6IO_JP 		0x07
#define REDBOOT_EXTID_W5250A_6IO_CN 		0x08
#define REDBOOT_EXTID_W5150A_12IO_US 	0x09
#define REDBOOT_EXTID_W5150A_12IO_EU 	0x0a
#define REDBOOT_EXTID_W5150A_12IO_JP 	0x0b
#define REDBOOT_EXTID_W5150A_12IO_CN 	0x0c
#define REDBOOT_EXTID_W5250A_12IO_US 	0x0d
#define REDBOOT_EXTID_W5250A_12IO_EU 	0x0e
#define REDBOOT_EXTID_W5250A_12IO_JP 	0x0f
#define REDBOOT_EXTID_W5250A_12IO_CN 	0x10
#elif defined(ia5x50aio)
#if 1
//Joy
#define REDBOOT_EXTID_5150A_6IO 	0x21
#define REDBOOT_EXTID_5250A_6IO 	0x22
#define REDBOOT_EXTID_5150A_12IO 	0x23
#define REDBOOT_EXTID_5250A_12IO 	0x24
#else
#define REDBOOT_EXTID_5150A_6IO 	0x01
#define REDBOOT_EXTID_5250A_6IO 	0x05
#define REDBOOT_EXTID_5150A_12IO 	0x09
#define REDBOOT_EXTID_5250A_12IO 	0x0d
#endif
#else
#error Unknow module
#endif // w1 || w2x50a

#endif // __REDBOOT_H__
