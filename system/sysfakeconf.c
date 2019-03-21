#include "../config.h"
#include <header.h>

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
	*baud = BAUD_IDX;
	*mode = SERMODE;
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
	return FLOW_CTRL;
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
int Scf_getTcpServer(int port, u_short *dataport, u_short *cmdport)
{
	return TCPSERV_BASE_PORT+port;
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
	*mode = DELIMPROC;
	*packlen = PACKLEN;
}
int Scf_getInactivityTimeMin(int port);
int Scf_setInactivityTimeMin(int port, int tout);
