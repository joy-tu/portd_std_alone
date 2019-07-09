#define MAX_PORT 4
#define TCP_ALIVE_MIN 7
#define BAUD_IDX 16 //115200
#define SERMODE   3 // 8N1
#define FLOW_CTRL 1 //RTS/CTS
#define INTERFACE 0 //RS-232
#if 0
#define OPMODE    256 //realcom
#else
#define OPMODE    512 //tcp server
#endif
#define MAX_CONN  1
#define SKIPJAMIP 0
#define ALLOWDRV 0
#define RTSDTRACT 3
#define INACTTIME 0
#define TCPSERV_BASE_PORT 4000
#define PORTBUFF 0
#define PACKLEN 0
#define DATAPAK_EN 0 
#define DELIM1 0
#define DELIM2 0
#define FORCE_TX 0
#define DELIMPROC 1 //Do nothing
#define SERDATALOG 0
