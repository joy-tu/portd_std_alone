/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <header.h>
#include <aptypes.h>

#define EXPORT_SIZE ((int)(16384*4))

#define DCF_EMODEM_REGS 	20	/* Max. E-Modem Registers := 20 */
#define DCF_SERIAL_ALIAS_MAX_LENGTH 16
#define DCF_MAX_IPRULES             16
#define DCF_SNMP_COMMUNITY          32
#define DCF_SNMPTRAP_SERVER_LENGTH  40

#define DCF_LOCAL_TIME_LEN			20
#define DCF_TIME_SERVER_LEN			40
#define	DCF_SERVER_LOCATION_LEN		40
#define DCF_SYS_NAME_LEN			40
#define DCF_MBUS_LEN				5
#define DCF_WATCHDOG_LEN			5

#define __MTC

/* Extra System Configuration */
#define SYS_ETH_IP_CFG 0
#define SYS_ETH_IP 1
#define SYS_ETH_MASK 2
#define SYS_ETH_GWAY 3
#define SYS_WLAN_IP_CFG 4
#define SYS_WLAN_IP 5
#define SYS_WLAN_MASK 6
#define SYS_WLAN_GWAY 7
#define SYS_DNS_1 8
#define SYS_DNS_2 9

/* Wireless Configuration */
#define WLAN_NETWORK_TYPE 0
#define WLAN_PROFILE_PRI 1
#define WLAN_CONNECT_RULE 2
#define WLAN_THRESHOLD 3

/* Wireless Profile Configuration */
#define WLAN_PROFILE_NAME 0
#define WLAN_PROFILE_ENABLE 1
#define WLAN_PROFILE_OPMODE 2
#define WLAN_PROFILE_SSID 3
#define WLAN_PROFILE_CHANNEL 4

/* Wireless Profile Security Configuration */
#define WLAN_SEC_AUTH 0
#define WLAN_SEC_ENCRY 1
#define WLAN_SEC_KEY_LEN 2
#define WLAN_SEC_KEY_IDX 3
#define WLAN_SEC_KEY_FMT 4
#define WLAN_SEC_KEY1 5
#define WLAN_SEC_KEY2 6
#define WLAN_SEC_KEY3 7
#define WLAN_SEC_KEY4 8
#define WLAN_SEC_EAP_METHOD 9
#define WLAN_SEC_TUNN_AUTH 10
#define WLAN_SEC_USR 11
#define WLAN_SEC_PASS 12
#define WLAN_SEC_ANONY_USR 13
#define WLAN_SEC_VERI_SVR_CERT 14
#define WLAN_SEC_PSK 15

/* Accessible IP List Settings */
#define ACC_IP_ENABLE 0
#define ACC_IP_ENTRY 1

/* Host Table Settings */
#define HOST_TBL_ENTRY 0
#define HOST_ALL_SET 0xffff

/*
 * bit 0-3 : Authentication
 * nit 4 : psk
 * bit 5 : TKIP/CCMP
 */
#define D_FLAG_AUTH_DISABLE     0x0000
#define D_FLAG_AUTH_WEP         0x0001
#define D_FLAG_AUTH_WPA         0x0002
#define D_FLAG_AUTH_WPA2        0x0004
#define D_FLAG_AUTH_PSK         0x0008
#define D_FLAG_AUTH_MASK        0x000F

/* IO Modbus Definition */
#define IO_MODBUS_DATA_TYPE_1BIT		0
#define IO_MODBUS_DATA_TYPE_1WORD	1
#define IO_MODBUS_DATA_TYPE_2WORD	2

#define IO_MODBUS_FUNC_CODE_COIL_STATUS	0
#define IO_MODBUS_FUNC_CODE_INPUT_STATUS	1
#define IO_MODBUS_FUNC_CODE_HOLD_REG		2
#define IO_MODBUS_FUNC_CODE_INPUT_REG		3

#define IO_MODBUS_R_MODE				0
#define IO_MODBUS_RW_MODE			1

#define IO_MODBUS_COIL_STATUS_START_REF_ADDR	0
#define IO_MODBUS_INPUT_STATUS_START_REF_ADDR	10000
#define IO_MODBUS_HOLD_REG_START_REF_ADDR		40000
#define IO_MODBUS_INPUT_REG_START_REF_ADDR		30000
/* Serial Driver Status */
/* Error Code */
#define CFG_OK  0
#define CFG_ITEM_NOT_FOUND  -1
#define CFG_FILE_NOT_FOUND  -2
#define CFG_BUFFER_IS_NULL  -3
#define CFG_BUFFER_TOO_SMALL    -4
#define CFG_INVALID_PARAMETER   -5
#define CFG_SECTION_NOT_FOUND	-6
#define CFG_UNKNOW_INTERFACE    -10


#define VERIFY_RANGE(v, l, u)    if (!(v>=l&&v<=u)) {return(CFG_INVALID_PARAMETER);}

//----------------------------------------------------------------
// Function Name: Scf_getServerName
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getServerName(char *buffer, int bufsize);

//----------------------------------------------------------------
// Function Name: Scf_getServerLocation
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getServerLocation(char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_getTimeZoneIndex
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTimeZoneIndex();

//----------------------------------------------------------------
// Function Name: Scf_getTimeServer
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTimeServer(char *buffer, int bufsize);

//----------------------------------------------------------------
// Function Name: Scf_getProductName
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getProductName(char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_getModelName
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getModelName(char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_getSerialNumber
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSerialNumber(int* serialno);

//----------------------------------------------------------------
// Function Name: Scf_getFullSerialNumber
// Function Desc: Get 12-character serial number
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getFullSerialNumber(char *buffer, int bufSize);

//----------------------------------------------------------------
// Function Name: Scf_getMaxPorts
// Function Desc:
// Return Value:
//      return the number of serial ports.
//----------------------------------------------------------------
int Scf_getMaxPorts();


//----------------------------------------------------------------
// Function Name: Scf_getTcpAliveCheck
// Function Desc:
// Return Value:
//      TCP alive check time;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTcpAliveCheck();

//----------------------------------------------------------------
// Function Name: Scf_getPortAliveCheck
// Function Desc:
// Return Value:
//      TCP alive check time;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getPortAliveCheck(int port);

//----------------------------------------------------------------
// Function Name: Scf_getSystemLog
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSystemLog(int *sys, int *net, int *cfg, int *op);

//----------------------------------------------------------------
// Function Name: Scf_getBridgeMode
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
// SUPPORT_BRIDGE
int Scf_getBridgeMode(int *bridge);

//----------------------------------------------------------------
// Function Name: Scf_getConsoleSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getConsoleSetting(int *http, int *https, int *telnet, int *ssh);

//----------------------------------------------------------------
// Function Name: Scf_getSerialConsoleSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSerialConsoleSetting(int *serial);

//----------------------------------------------------------------
// Function Name: Scf_getResetButtonSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getResetButtonSetting(int *reset);

//----------------------------------------------------------------
// Function Name: Scf_getPassword
// Function Desc:
//      "account" equals 0 stands for "admin".
//      "account" equals 1 stands for "user".
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getPassword(int account, char *buffer, int bufsize);

//----------------------------------------------------------------
// Function Name: Scf_getEventMail
// Function Desc:
// Return Value:
//      1 -> on;
//      0 -> off;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getEventMail(int event_id);

//----------------------------------------------------------------
// Function Name: Scf_getEventTrap
// Function Desc:
// Return Value:
//      1 -> on;
//      0 -> off;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getEventTrap(int event_id);

//----------------------------------------------------------------
// Function Name: Scf_getSerialEvent
// Function Desc:
// Return Value:
//      1 -> on;
//      0 -> off;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSerialEvent(int port, int event_id);


//----------------------------------------------------------------
// Function Name: Scf_setServerName
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setServerName(char *buffer, int bufsize);

//----------------------------------------------------------------
// Function Name: Scf_setServerLocation
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setServerLocation(char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_setTimeZoneIndex
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setTimeZoneIndex(int index);

//----------------------------------------------------------------
// Function Name: Scf_setTimeServer
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setTimeServer(char *buffer, int bufsize);




//----------------------------------------------------------------
// Function Name: Scf_setTcpAliveCheck
// Function Desc:
// Return Value:
//      0   -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setTcpAliveCheck(int checktime);

//----------------------------------------------------------------
// Function Name: Scf_setPortAliveCheck
// Function Desc:
// Return Value:
//      0   -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setPortAliveCheck(int port, int checktime);

//----------------------------------------------------------------
// Function Name: Scf_setSystemLog
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setSystemLog(int sys, int net, int cfg, int op);

//----------------------------------------------------------------
// Function Name: Scf_setBridgeMode
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
// SUPPORT_BRIDGE
int Scf_setBridgeMode(int bridge);

//----------------------------------------------------------------
// Function Name: Scf_setConsoleSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setConsoleSetting(int http, int https, int telnet, int ssh);

//----------------------------------------------------------------
// Function Name: Scf_setSerialConsoleSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setSerialConsoleSetting(int serial);

//----------------------------------------------------------------
// Function Name: Scf_setResetButtonSetting
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setResetButtonSetting(int  reset);

//----------------------------------------------------------------
// Function Name: Scf_setPassword
// Function Desc:
//      "account" equals 0 stands for "admin".
//      "account" equals 1 stands for "user".
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setPassword(int account, char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_setEventMail
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setEventMail(int event_id, int onoff);

//----------------------------------------------------------------
// Function Name: Scf_setEventTrap
// Function Desc:
// Return Value:
//      0  -> success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setEventTrap(int event_id, int onoff);

//----------------------------------------------------------------
// Function Name: Scf_setSerialEvent
// Function Desc:
// Return Value:
//      1 -> on;
//      0 -> off;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setSerialEvent(int port, int event_id, int onoff);


//
// Communication Parameters.
//

//----------------------------------------------------------------
// Function Name: Scf_getPortAlias
// Function Desc:
//      Get the alias name of a port, and stor the value in the
//      buffer pointer, bufsize is the size of the buffer pointed
//      by *buffer.
// Return Value:
//      0   -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getPortAlias(int port, char *buffer, int bufsize);

//----------------------------------------------------------------
// Function Name: Scf_getAsyncIoctl
// Function Desc:
//      Get IO control of a port, including baudrate, mode and flow control.
// Return Value:
//      0   -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAsyncIoctl(int port, int *baud, int *mode, int *flow);


//----------------------------------------------------------------
// Function Name: Scf_getAsyncFifo
// Function Desc:
//      Get the fifo of a port.
// Return Value:
//      0 -> fifo is disabled;
//      1 -> fifo is enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAsyncFifo(int port);


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
int Scf_getIfType(int port);


//----------------------------------------------------------------
// Function Name: Scf_setPortAlias
// Function Desc:
//      Set the alias name of a port.
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setPortAlias(int port, char *buffer, int bufsize);


//----------------------------------------------------------------
// Function Name: Scf_setAsyncIoctl
// Function Desc:
//      Set the IO control name of a port, including baudrate,
//      serial mode and flow control.
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setAsyncIoctl(int port, int baud, int mode, int flow);


//----------------------------------------------------------------
// Function Name: Scf_setAsyncFifo
// Function Desc:
//      Set fifo enable/disable of a port.
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setAsyncFifo(int port, int fifo);


//----------------------------------------------------------------
// Function Name: Scf_setIfType
// Function Desc:
//      Set interface type of a port.
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setIfType(int port, int iftype);



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
int Scf_getOpMode(int port);

//----------------------------------------------------------------
// Function Name: Scf_getMaxConns
// Function Desc:
//      Get the max TCP connections.
// Return Value:
//      Max connection of the port.
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getMaxConns(int port);


//----------------------------------------------------------------
// Function Name: Scf_getSkipJamIP
// Function Desc:
// Return Value:
//      0 -> disabled;
//      1 -> enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getSkipJamIP(int port);


//----------------------------------------------------------------
// Function Name: Scf_getAllowDrvCtrl
// Function Desc:
// Return Value:
//      0 -> disabled;
//      1 -> enabled;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getAllowDrvCtrl(int port);


//----------------------------------------------------------------
// Function Name: Scf_getRtsDtrAction
// Function Desc:
// Return Value:
//      bit 1 (RTS):  0=RTS low,  1=RTS high,
//      bit 0 (DTR):  0=DTR low,  1=DTR high,
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getRtsDtrAction(int port);


//----------------------------------------------------------------
// Function Name: Scf_getInactivityTime
// Function Desc:
// Return Value:
//      Inactivity Time (millisecond)
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getInactivityTime(int port);



//----------------------------------------------------------------
// Function Name: Scf_getTcpServer
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTcpServer(int port, u_short *dataport, u_short *cmdport);



//----------------------------------------------------------------
// Function Name: Scf_getTcpClientHost
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getTcpClientHost(int port, int ndx, char *dhost, int dlen, INT16U *dport, INT16U *lport);


//----------------------------------------------------------------
// Function Name: Scf_getTcpClientMode
// Function Desc:
// Return Value:
//      TCP client mode, connection control mode;
//      high byte, 0=Startup
//      high byte, 1=Any character
//      high byte, 2=DSR On
//      high byte, 3=DCD On
//      low byte, 0=None
//      low byte, 1=Inactivity time
//      low byte, 2=DSR Off
//      low byte, 3=DCD Off
//      <0 -> fail;
//----------------------------------------------------------------
INT16U Scf_getTcpClientMode(int port);


//----------------------------------------------------------------
// Function Name: Scfw_getUdpS2E
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getUdpS2E(int port, int idx, INT32U *bip, INT32U *eip, int *pno);


//----------------------------------------------------------------
// Function Name: Scf_getUdpPort
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
INT16U Scf_getUdpPort(int port);


//----------------------------------------------------------------
// Function Name: Scf_getEmodemRegs
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getEmodemRegs(int port, int* reg, int regs);


//----------------------------------------------------------------
// Function Name: Scf_setOpMode
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int	Scf_setOpMode(int port, int opmode);


//----------------------------------------------------------------
// Function Name: Scf_setMaxConns
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setMaxConns(int port, int max);


//----------------------------------------------------------------
// Function Name: Scf_setSkipJamIP
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setSkipJamIP(int port, int yes);


//----------------------------------------------------------------
// Function Name: Scf_setAllowDrvCtrl
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setAllowDrvCtrl(int port, int yes);


//----------------------------------------------------------------
// Function Name: Scf_setRtsDtrAction
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setRtsDtrAction(int port, int rts, int dtr);

//----------------------------------------------------------------
// Function Name: Scf_setInactivityTime
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setInactivityTime(int port, int tout);



//----------------------------------------------------------------
// Function Name: Scf_setTcpServer
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setTcpServer(int port, INT16U dataport, INT16U cmdport);


//----------------------------------------------------------------
// Function Name: Scf_setTcpClientHost
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int	Scf_setTcpClientHost(int port, int ndx, char *dhost, int dlen, INT16U dport, INT16U lport);


//----------------------------------------------------------------
// Function Name: Scf_setTcpClientMode
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setTcpClientMode(int port, int mode);


//----------------------------------------------------------------
// Function Name: Scf_setTcpClientHost
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setUdpS2E(int port, int idx, INT32U bip, INT32U eip, INT16U pno);

//----------------------------------------------------------------
// Function Name: Scf_setUdpPort
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setUdpPort(int port, INT16U udp_port);


//----------------------------------------------------------------
// Function Name: Scf_setEmodemRegs
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setEmodemRegs(int port, int* reg, int regs);


//----------------------------------------------------------------
// Function Name: Scf_setEmodemDefault
// Function Desc:
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setEmodemDefault(int port);

//
// Port buffering
//
int Scf_getPortBuffering(int port);
int Scf_setPortBuffering(int port, int yes);

//
// Serial data log
//
int Scf_getSerialDataLog(int port);
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
//      packlen: packet length
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_getDataPacking(int port, int *flag, INT8U *ch1, INT8U *ch2, INT16U *tout, int *mode, int *packlen);


//----------------------------------------------------------------
// Function Name: Scf_setDataPacking
// Function Desc:
//      port: 1~Scf_getMaxPorts()
//      flag: bit0: enable/disable ch1, bit1: enable/disable ch2
//      ch1: delimiter 1
//      ch2: delimiter 2
//      tout: force transmit
//      mode: 0=Do nothing,
//            1=Delimiter+1,
//            2=Delimiter+2,
//            3=Strip Delimiter
//      packlen: packet length
// Return Value:
//      0 -> Success;
//      <0 -> fail;
//----------------------------------------------------------------
int Scf_setDataPacking(int port, int flag, INT8U ch1, INT8U ch2, INT16U tout, int mode, int packlen);


int Scf_getAccessibleIP(void);
int Scf_setAccessibleIP(int enable);

void Scf_setEmodemDefaultRegs(int *regs);

int Scf_getSNMPEnable(void);
int Scf_setSNMPEnable(int enable);

int Scf_getSNMPContact(char *buffer, int bufsize);
int Scf_setSNMPContact(char *buffer, int bufsize);

int Scf_getSNMPReadComm(char *buffer, int bufsize);
int Scf_setSNMPReadComm(char *buffer, int bufsize);

int Scf_getSNMPWriteComm(char *buffer, int bufsize);
int Scf_setSNMPWriteComm(char *buffer, int bufsize);

int Scf_getSNMPROUser(char *buffer, int bufsize);
int Scf_setSNMPROUser(char *buffer, int bufsize);

int Scf_getSNMPROPasswd(char *buffer, int bufsize);
int Scf_setSNMPROPasswd(char *buffer, int bufsize);

int Scf_getSNMPROPriv(char *buffer, int bufsize);
int Scf_setSNMPROPriv(char *buffer, int bufsize);

int Scf_getSNMPRWUser(char *buffer, int bufsize);
int Scf_setSNMPRWUser(char *buffer, int bufsize);

int Scf_getSNMPRWPasswd(char *buffer, int bufsize);
int Scf_setSNMPRWPasswd(char *buffer, int bufsize);

int Scf_getSNMPRWPriv(char *buffer, int bufsize);
int Scf_setSNMPRWPriv(char *buffer, int bufsize);

int Scf_getSNMPVersion(void);
int Scf_setSNMPVersion(int version);

int Scf_getSNMPROAuth(void);
int Scf_setSNMPROAuth(int auth);

int Scf_getSNMPROPrivMode(void);
int Scf_setSNMPROPrivMode(int priv);

int Scf_getSNMPRWAuth(void);
int Scf_setSNMPRWAuth(int auth);

int Scf_getSNMPRWPrivMode(void);
int Scf_setSNMPRWPrivMode(int priv);


/* snmp trap */
int Scf_getSNMPTrap(char* buffer, int bufsize);
int Scf_setSNMPTrap(char* buffer, int bufsize);

int Scf_getSNMPTrapVersion();
int Scf_setSNMPTrapVersion(int version);

int Scf_getSNMPTrapCommunity(char* buffer, int bufsize);
int Scf_setSNMPTrapCommunity(char* buffer, int bufsize);


/* mail settings */
int Scf_getMailServer(char *buffer, int bufsize);
int Scf_getSMTP_Auth();
int Scf_getSMTP_AuthName(char *buffer, int bufsize);
int Scf_getSMTP_AuthPass(char *buffer, int bufsize);
int Scf_getSMTP_FromAddr(char *buffer, int bufsize);
int Scf_getSMTP_ToAddr(int index, char *buffer, int bufsize);


int Scf_setMailServer(char *buffer, int bufsize);
int Scf_setSMTP_Auth(int onoff);
int Scf_setSMTP_AuthName(char *buffer, int bufsize);
int Scf_setSMTP_AuthPass(char *buffer, int bufsize);
int Scf_setSMTP_FromAddr(char *buffer, int bufsize);
int Scf_setSMTP_ToAddr(int index, char *buffer, int bufsize);

int Scf_ChangePasswd(char * username, char * old_passwd, char * new_passwd);

/* Load Factory Default */
int Scf_LoadDefault(int keep_ip);

/* Modbus Load default */
int Scf_Modbus_LoadDefault(void);

/* SCM settings */
int Scf_getScmTrigger(void);
int Scf_setScmTrigger(int trigger);
int Scf_getScmChar(INT8U *ch1, INT8U *ch2, INT8U *ch3);
int Scf_setScmChar(INT8U ch1, INT8U ch2, INT8U ch3);

/* DIO settings */
int Scf_getWlanStrengthLedFunction(void);
int Scf_setWlanStrengthLedFunction(int enabled);
int	Scf_getSDioMode(int io);
int	Scf_setSDioMode(int io, int mode);
int	Scf_getSDioState(int io);
int	Scf_setSDioState(int io, int state);
int Scf_getDioTcpPort(void);
int	Scf_setDioTcpPort(int port);


int Scf_getModelRegion(void);
int Scf_getActiveInterface(void);
int Scf_setActiveInterface(int type);

int Scf_getLanSpeed(int interface);
int Scf_setLanSpeed(int interface, int speed);

#define D_LAN_SPEED_AUTO	0
#define D_LAN_SPEED_10M_HALF	1
#define D_LAN_SPEED_10M_FULL	2
#define D_LAN_SPEED_100M_HALF	3
#define D_LAN_SPEED_100M_FULL	4

#define DLANTYPE_AUTO	0	// Auto select
#define DLANTYPE_DIX	1	// Select by DI
#define DLANTYPE_ETHER	2	// Fixed to LAN
#define DLANTYPE_WLAN	3	// Fixed to WLAN

// SUPPORT_RTERMINAL_MODE
#define DCF_AUTH_SERVER_LEN 40
#define DCF_AUTH_KEY_LEN 16
int Scf_getInactivityTimeMin(int port);
int Scf_setInactivityTimeMin(int port, int tout);
int Scf_getRTerminal(int port, int *tcpport, int *mapkeys);
int Scf_setRTerminal(int port, int tcpport, int mapkeys);
int	Scf_getAuthType(int port);
int Scf_setAuthType(int port, int type);
int	Scf_getRadius(char *server, char *key, int *port, int *accounting);
int	Scf_setRadius(char *server, char *key, int port, int accounting);
// =====

int Scf_gethwextid(void);

int Scf_getWlanLog(void);
int Scf_setWlanLog(int enable);
/* NPort IO New Configuration API */
int Scf_get_modbus_timeout_en(void);
int Scf_set_modbus_timeout_en(int val);

int Scf_get_modbus_timeout_val(void);
int Scf_set_modbus_timeout_val(int val);

int Scf_get_io_watchdog_en(void);
int Scf_set_io_watchdog_en(int val);

int Scf_get_io_watchdog_val(void);
int Scf_set_io_watchdog_val(int val);

int Scf_get_user_def_modbus_addr_en(void);
int Scf_set_user_def_modbus_addr_en(int val);

int Scf_get_default_modbus_info(char *modbus_item);

int Scf_get_io_di_mode(int di_port);
int Scf_set_io_di_mode(int di_port, int val);

int Scf_get_io_di_filter(int di_port);
int Scf_set_io_di_filter(int di_port, int val);

int Scf_get_io_di_counter_trigger(int di_port);
int Scf_set_io_di_counter_trigger(int di_port, int val);

int Scf_get_io_di_power_on_setting(int di_port);
int Scf_set_io_di_power_on_setting(int di_port, int val);

int Scf_get_io_di_save_cnt_on_failure(int di_port);
int Scf_set_io_di_save_cnt_on_failure(int di_port, int val);

int Scf_get_io_di_cnt_on_failure(int di_port);
int Scf_set_io_di_cnt_on_failure(int di_port, int val);

int Scf_get_io_di_channel_alias(int di_port, char *buffer, int bufsize);
int Scf_set_io_di_channel_alias(int di_port, char *buffer, int bufsize);

int Scf_get_io_di_off_alias(int di_port, char *buffer, int bufsize);
int Scf_set_io_di_off_alias(int di_port, char *buffer, int bufsize);

int Scf_get_io_di_on_alias(int di_port, char *buffer, int bufsize);
int Scf_set_io_di_on_alias(int di_port, char *buffer, int bufsize);

int Scf_get_io_do_mode(int do_port);
int Scf_set_io_do_mode(int do_port, int val);

#if 0
int Scf_get_io_do_status(int do_port);
int Scf_set_io_do_status(int do_port, int val);
#endif

int Scf_get_io_do_power_on_setting(int do_port);
int Scf_set_io_do_power_on_setting(int do_port, int val);

int Scf_get_io_do_safe_status(int do_port);
int Scf_set_io_do_safe_status(int do_port, int val);

int Scf_get_io_do_pulse_off_width(int do_port);
int Scf_set_io_do_pulse_off_width(int do_port, int val);

int Scf_get_io_do_pulse_on_width(int do_port);
int Scf_set_io_do_pulse_on_width(int do_port, int val);

int Scf_get_io_do_pulse_cnt(int do_port);
int Scf_set_io_do_pulse_cnt(int do_port, int val);

int Scf_get_io_do_pulse_power_on_setting(int do_port);
int Scf_set_io_do_pulse_power_on_setting(int do_port, int val);

int Scf_get_io_do_pulse_safe_status_setting(int do_port);
int Scf_set_io_do_pulse_safe_status_setting(int do_port, int val);

int Scf_get_io_do_pulse_on_delay(int do_port);
int Scf_set_io_do_pulse_on_delay(int do_port, int val);

int Scf_get_io_do_channel_alias(int do_port, char *buffer, int bufsize);
int Scf_set_io_do_channel_alias(int do_port, char *buffer, int bufsize);

int Scf_get_io_do_off_alias(int do_port, char *buffer, int bufsize);
int Scf_set_io_do_off_alias(int do_port, char *buffer, int bufsize);

int Scf_get_io_do_on_alias(int do_port, char *buffer, int bufsize);
int Scf_set_io_do_on_alias(int do_port, char *buffer, int bufsize);

int Scf_get_default_modbus_content(char *modbus_item[2], int *rw_mode, unsigned long *ref_addr, 
										int *data_type);

int Scf_get_modbus_do_value_content(int *rw_mode, unsigned long *ref_addr, 
					int *total_ch, int *data_type);

int Scf_get_modbus_do_pulse_status_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_do_value_all_ch_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_di_value_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_di_cnt_value_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_di_val_all_ch_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_di_cnt_start_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_di_clear_cnt_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_do_value_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_do_pulse_status_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_do_value_all_ch_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_di_value_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_di_cnt_value_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_di_val_all_ch_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_di_cnt_start_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_def_modbus_di_clear_cnt_content(int *rw_mode, 
		unsigned long *ref_addr, int *total_ch, int *data_type);

int Scf_get_modbus_do_value_start_addr(void);
int Scf_set_modbus_do_value_start_addr(int val);

int Scf_get_modbus_do_value_func_code(void);
int Scf_set_modbus_do_value_func_code(int val);

int Scf_get_modbus_do_pulse_status_start_addr(void);
int Scf_set_modbus_do_pulse_status_start_addr(int val);

int Scf_get_modbus_do_pulse_status_func_code(void);
int Scf_set_modbus_do_pulse_status_func_code(int val);

int Scf_get_modbus_do_value_all_ch_start_addr(void);
int Scf_set_modbus_do_value_all_ch_start_addr(int val);

int Scf_get_modbus_do_value_all_ch_func_code(void);
int Scf_set_modbus_do_value_all_ch_func_code(int val);

int Scf_get_modbus_di_value_start_addr(void);
int Scf_set_modbus_di_value_start_addr(int val);

int Scf_get_modbus_di_value_func_code(void);
int Scf_set_modbus_di_value_func_code(int val);

int Scf_get_modbus_di_cnt_value_start_addr(void);
int Scf_set_modbus_di_cnt_value_start_addr(int val);

int Scf_get_modbus_di_cnt_value_func_code(void);
int Scf_set_modbus_di_cnt_value_func_code(int val);

int Scf_get_modbus_di_value_all_ch_start_addr(void);
int Scf_set_modbus_di_value_all_ch_start_addr(int val);

int Scf_get_modbus_di_value_all_ch_func_code(void);
int Scf_set_modbus_di_value_all_ch_func_code(int val);

int Scf_get_modbus_di_cnt_start_start_addr(void);
int Scf_set_modbus_di_cnt_start_start_addr(int val);

int Scf_get_modbus_di_cnt_start_func_code(void);
int Scf_set_modbus_di_cnt_start_func_code(int val);

int Scf_get_modbus_di_clear_cnt_start_addr(void);
int Scf_set_modbus_di_clear_cnt_start_addr(int val);

int Scf_get_modbus_di_clear_cnt_func_code(void);
int Scf_set_modbus_di_clear_cnt_func_code(int val);


int Scf_get_def_modbus_do_value_start_addr(void);

int Scf_get_def_modbus_do_value_func_code(void);

int Scf_get_def_modbus_do_pulse_status_start_addr(void);

int Scf_get_def_modbus_do_pulse_status_func_code(void);

int Scf_get_def_modbus_do_value_all_ch_start_addr(void);

int Scf_get_def_modbus_do_value_all_ch_func_code(void);

int Scf_get_def_modbus_di_value_start_addr(void);

int Scf_get_def_modbus_di_value_func_code(void);

int Scf_get_def_modbus_di_cnt_value_start_addr(void);

int Scf_get_def_modbus_di_cnt_value_func_code(void);

int Scf_get_def_modbus_di_value_all_ch_start_addr(void);

int Scf_get_def_modbus_di_value_all_ch_func_code(void);

int Scf_get_def_modbus_di_cnt_start_start_addr(void);

int Scf_get_def_modbus_di_cnt_start_func_code(void);

int Scf_get_def_modbus_di_clear_cnt_start_addr(void);

int Scf_get_def_modbus_di_clear_cnt_func_code(void);

int Scf_get_io_di_snmp_trap_en(int di_port);
int Scf_set_io_di_snmp_trap_en(int do_port, int val);

int Scf_get_io_do_snmp_trap_en(int di_port);
int Scf_set_io_do_snmp_trap_en(int do_port, int val);

int Scf_get_event_relay(int event_id);
int Scf_set_event_relay(int event_id, int val);

int Scf_get_max_do();
int Scf_get_max_di();

int Scf_get_sd_cfg_auto_load_en(void);
int Scf_set_sd_cfg_auto_load_en(int val);
int Scf_getPresharedKey(char *buffer, int bufsize);
int Scf_setPresharedKey(char *buffer, int bufsize);
/* MTConnect API() */
int Scf_get_mtc_func_enabled(void);
int Scf_set_mtc_func_enabled(int val, int sync_flag);
int Scf_get_mtc_localtime_enabled(void);
int Scf_set_mtc_localtime_enabled(int val, int sync_flag);
int Scf_get_mtc_inter_act(int idx);
int Scf_set_mtc_inter_act(int idx, int act_flag, int sync_flag);
int Scf_get_mtc_inter_input(int idx);
int Scf_set_mtc_inter_input(int idx, int di_port, int sync_flag);
int Scf_get_mtc_inter_mode(int idx);
int Scf_set_mtc_inter_mode(int idx, int di_mode, int sync_flag);
int Scf_get_mtc_inter_period(int idx);
int Scf_set_mtc_inter_period(int idx, int di_period, int sync_flag);
int Scf_get_mtc_cond_input(int idx);
int Scf_set_mtc_cond_input(int idx, int di_port, int sync_flag);
int Scf_get_mtc_cond_mode(int idx);
int Scf_set_mtc_cond_mode(int idx, int di_mode, int sync_flag);
int Scf_get_mtc_cond_period(int idx);
int Scf_set_mtc_cond_period(int idx, int di_period, int sync_flag);
int Scf_get_mtc_mtc_tag_str(int idx, char *buffer, int bufsize);
int Scf_set_mtc_mtc_tag_str(int idx, char *buffer, int bufsize, int sync_flag);
int Scf_get_mtc_mtc_act_str(int idx, char *buffer, int bufsize);
int Scf_set_mtc_mtc_act_str(int idx, char *buffer, int bufsize, int sync_flag);
int Scf_get_mtc_mtc_inact_str(int idx, char *buffer, int bufsize);
int Scf_set_mtc_mtc_inact_str(int idx, char *buffer, int bufsize, int sync_flag);
int Scf_get_mtc_pulse_on_act_cnt(void);
int Scf_set_mtc_pulse_on_act_cnt(int pulse_cnt);
int Scf_get_mtc_pulse_off_act_cnt(void);
int Scf_set_mtc_pulse_off_act_cnt(int pulse_cnt);
int Scf_get_mtc_pulse_on_inact_mode(void);
int Scf_set_mtc_pulse_on_inact_mode(int pulse_mode);
int Scf_get_mtc_pulse_off_inact_mode(void);
int Scf_set_mtc_pulse_off_inact_mode(int pulse_mode);
int Scf_get_mtc_pulse_on_inact_period(void);
int Scf_set_mtc_pulse_on_inact_period(int pulse_period);
int Scf_get_mtc_pulse_off_inact_period(void);
int Scf_set_mtc_pulse_off_inact_period(int pulse_period);
int Scf_get_mtc_def_tag_str(int idx, char *buffer, int bufsize);
int Scf_set_mtc_def_tag_str(int idx, char *buffer, int bufsize);
int Scf_get_mtc_def_tag_status(int idx, char *buffer, int bufsize);
int Scf_set_mtc_def_tag_status(int idx, char *buffer, int bufsize);
#if 1 /* MTC_VR */
int Scf_get_mtc_device_id(int idx, char *buffer, int bufsize);
int Scf_set_mtc_device_id(int idx, char *buffer, int bufsize);
int Scf_get_mtc_device_name(int idx, char *buffer, int bufsize);
int Scf_set_mtc_device_name(int idx, char *buffer, int bufsize);
int Scf_get_mtc_item_id(int idx, char *buffer, int bufsize);
int Scf_set_mtc_item_id(int idx, char *buffer, int bufsize, int sync_flag);
int Scf_get_mtc_item_name(int idx, char *buffer, int bufsize);
int Scf_set_mtc_item_name(int idx, char *buffer, int bufsize, int sync_flag);
int	Scf_get_mtc_item_device(int idx);
int	Scf_set_mtc_item_device(int idx, int device_idx, int sync_flag);
int	Scf_get_mtc_item_cate(int idx);
int	Scf_set_mtc_item_cate(int idx, int cate, int sync_flag);
int Scf_get_mtc_item_type(int idx, char *buffer, int bufsize);
int Scf_set_mtc_item_type(int idx, char *buffer, int bufsize, int sync_flag);
int Scf_get_mtc_item_subtype(int idx, char *buffer, int bufsize);
int Scf_set_mtc_item_subtype(int idx, char *buffer, int bufsize, int sync_flag);

#endif
struct __config_split
{
    char *section_name;
    char *split_file;
};
u_long cal_chksum(char *Buf, long BufLen);
int Scf_splitConfigFile(char *filename);
int Scf_importConfigFiles(int caller_flag, int include_ip);
#define SECTION_SYSTEM					"[system]"
#endif	/* _CONFIG_H_ */

