#ifndef _SYSFAKECONF_H_
#define _SYSFAKECONF_H_
#include <header.h>

int Scf_getMaxConns(int port);
int Scf_getMaxPorts();
int Scf_getPortAliveCheck(int port);
int Scf_getAsyncIoctl(int port, int *baud, int *mode, int *flow);
int Scf_getAsyncFifo(int port);
int Scf_getIfType(int port);
int Scf_getOpMode(int port);
int Scf_getMaxConns(int port);
int Scf_getSkipJamIP(int port);
int Scf_getAllowDrvCtrl(int port);
int Scf_getRtsDtrAction(int port);
int Scf_getInactivityTime(int port);
int Scf_getTcpServer(int port, unsigned short *dataport, unsigned short *cmdport);
int Scf_getPortBuffering(int port);
int Scf_getSerialDataLog(int port);
int Scf_setSerialDataLog(int port, int yes);
int Scf_getDataPacking(int port, int *flag, INT8U *ch1, INT8U *ch2, INT16U *tout, int *mode, int *packlen);
int Scf_getInactivityTimeMin(int port);
int Scf_setInactivityTimeMin(int port, int tout);
#endif
