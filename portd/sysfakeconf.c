#include <header.h>
#include <platform.h>
#ifdef LINUX
#include "sio.h"
#elif defined(ZEPHYR)
#include <sio/mx_sio.h>
#endif
#include "../config.h"

int Scf_getMaxPorts(){
	return MAX_PORT;
}

//----------------------------------------------------------------
// Function Name: Scf_getPortAliveCheck
// Function Desc:
// Return Value:
//      TCP alive check time;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getPortAliveCheck(int port)
{
	return TCP_ALIVE_MIN;
}
//
// Communication Parameters.
//

//----------------------------------------------------------------
// Function Name: Scf_getAsyncIoctl
// Function Desc:
//      Get IO control of a port, including baudrate, mode and flow control.
// Return Value:
//      0   -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAsyncIoctl(int port, int *baud, int *mode, int *flow)
{
	/* 
	 * termios.h doesn't have B7200 and
	 * we also don't support user-defined baud rate.
	 */
	*baud = BAUD_IDX;
	if (*baud > 10)
		++(*baud);

	*mode = 0;
	switch (DATABITS)
	{
		case 5: *mode |= BIT_5;  break;
		case 6: *mode |= BIT_6;  break;
		case 7: *mode |= BIT_7;  break;
		case 8:
		default: *mode |= BIT_8; break;
	}


	switch (STOPBITS)
	{
		case 2: *mode |= STOP_2;  break;
		case 1:
		default: *mode |= STOP_1; break;
	}

	switch (PARITY)
	{
		case 1: *mode |= P_ODD;   break;
		case 2: *mode |= P_EVEN;  break;
		case 3: *mode |= P_MRK;   break;
		case 4: *mode |= P_SPC;   break;
		case 0:
		default: *mode |= P_NONE; break;
	}

	*flow = FLOW_CTRL;

	return 0;
}
//----------------------------------------------------------------
// Function Name: Scf_getAsyncFifo
// Function Desc:
//      Get the fifo of a port.
// Return Value:
//      0 -> fifo is disabled;
//      1 -> fifo is enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAsyncFifo(int port){
	return FIFO;
}

//----------------------------------------------------------------
// Function Name: Scf_getIfType
// Function Desc:
//      Get the interface type of a port.
// Return Value:
//      0 -> RS-232;
//      1 -> RS-422;
//      2 -> RS-485 2-wire;
//      3 -> RS-485 4-wire;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getIfType(int port)
{
	return INTERFACE;
}

//
// Operation Mode.
//

//----------------------------------------------------------------
// Function Name: Scf_getOpMode
// Function Desc:
//      Get the operation mode.
// Return Value:
//      high byte, 0=disable
//      high byte, 1=Device control;	low byte, 0=RealCom, 1=RFC2217
//      high byte, 2=Socket;	        low byte, 0=Server,  1=Client, 2=UDP
//      high byte, 3=Pair Connection;   low byte, 0=Master,  1=Slave
//      high byte, 4=Ethernet Modem;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getOpMode(int port)
{
	return OPMODE;
}
//----------------------------------------------------------------
// Function Name: Scf_getMaxConns
// Function Desc:
//      Get the max TCP connections.
// Return Value:
//      Max connection of the port.
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getMaxConns(int port)
{
	return MAX_CONN;
}

//----------------------------------------------------------------
// Function Name: Scf_getSkipJamIP
// Function Desc:
// Return Value:
//      0 -> disabled;
//      1 -> enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSkipJamIP(int port)
{
	return SKIPJAMIP;
}
//----------------------------------------------------------------
// Function Name: Scf_getAllowDrvCtrl
// Function Desc:
// Return Value:
//      0 -> disabled;
//      1 -> enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAllowDrvCtrl(int port)
{
	return ALLOWDRV;
}


//----------------------------------------------------------------
// Function Name: Scf_getRtsDtrAction
// Function Desc:
// Return Value:
//      bit 1 (RTS):  0=RTS low,  1=RTS high,
//      bit 0 (DTR):  0=DTR low,  1=DTR high,
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getRtsDtrAction(int port)
{
	return RTSDTRACT;
}

//----------------------------------------------------------------
// Function Name: Scf_getInactivityTime
// Function Desc:
// Return Value:
//      Inactivity Time (millisecond)
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getInactivityTime(int port)
{
	return INACTTIME;
}

//----------------------------------------------------------------
// Function Name: Scf_getTcpServer
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTcpServer(int port, unsigned short *dataport, unsigned short *cmdport)
{
#if 1
	*dataport = TCPSERV_BASE_PORT;
	*cmdport  = TCPSERV_CMDB_PORT;
	return 0;
#else
    *dataport = TCPSERV_BASE_PORT+port;
	*cmdport  = 965 + port;
	return TCPSERV_BASE_PORT+port;
#endif
}


//
// Port buffering
//
int Scf_getPortBuffering(int port)
{
	return PORTBUFF;
}


//
// Serial data log
//
int Scf_getSerialDataLog(int port)
{
	return SERDATALOG;
}
int Scf_setSerialDataLog(int port, int yes);

//
// Data Packing.
//

//----------------------------------------------------------------
// Function Name: Scf_getDataPacking
// Function Desc:
//      port: 1~Scf_getMaxPorts()
//      flag: bit0: enable/disable ch1, bit1: enable/disable ch2
//      ch1: delimiter 1
//      ch2: delimiter 2
//      tout: force transmit
//      mode: 1=Do nothing,
//            2=Delimiter+1,
//            4=Delimiter+2,
//            8=Strip Delimiter
//      packlen: packing length
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getDataPacking(int port, int *flag, INT8U *ch1, INT8U *ch2, INT16U *tout, int *mode, int *packlen)
{

	*flag = DATAPAK_EN;
	*ch1  = DELIM1; 
	*ch2  = DELIM2;
	*tout = FORCE_TX;
	*mode = (1 << DELIMPROC);
	*packlen = PACKLEN;
}
int Scf_getInactivityTimeMin(int port);
int Scf_setInactivityTimeMin(int port, int tout);
