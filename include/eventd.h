
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _EVENTD_H
#define _EVENTD_H


struct msgbuf
{
    long mtype;
    int event_id;
    int context;
    int opmode;
    int ip;
    int port;
};

#define EVENT_PORT_MASK		0x00f0
#define EVENT_PORTSIG_MASK	0x000f

#define EVENT_MSGKEY    	0x12345678
#define EVENT_LOG_PATH  	"/var/log/locallog"
#define EVENT_PID_FILE   	"/var/run/devsvr/eventd.pid"
#define MAIL_RESULT_PATH	"/var/sendmail_result"

#define EVENT_TYPE_MAIL             1001
#define EVENT_TYPE_TRAP             1002
#define EVENT_TYPE_LOG              1003

#define EVENT_ID_SERIALDATA         0       // Serial data log
#define EVENT_ID_COLDSTART          0x100   // System Cold Start
#define EVENT_ID_WARMSTART          0x101   // System Warm Start
#define EVENT_ID_LINKDOWN           0x102   // link down
#define EVENT_ID_LINKUP             0x103   // link up
#define EVENT_ID_POWER1_FAIL        0x104   // Power 1 fail
#define EVENT_ID_POWER2_FAIL        0x105   // Power 2 fail
#define EVENT_ID_LINK2DOWN           0x106   // link down
#define EVENT_ID_LINK2UP             0x107   // link up

#define EVENT_ID_IPRENEW            0x200   // DHCP/BOOTP/PPPoE Get IP/Renew
#define EVENT_ID_NTP                0x201   // NTP
#define EVENT_ID_MAILFAIL           0x202   // Mail Fail
#define EVENT_ID_NTPCONNFA          0x203   // NTP Connect Fail
#define EVENT_ID_DHCPFAIL           0x204   // PPPoE Get IP Fail
#define EVENT_ID_IPCONFLICT         0x205   // IP Conflict
#define EVENT_ID_NETLINKDOWN        0x206   // Ethernet Link Down
#define EVENT_ID_WLANLINKDOWN       0x207   // WLAN Link Down
#define EVENT_ID_WLANLOW            0x208   // Wireless Signal Below Threshold
#define EVENT_ID_WLANPROFILE        0x209   // Wireless Active Profile Changed
#define EVENT_ID_MODBUS_DISCONNECT	0x20A // Modbus/TCP disconnect
#define EVENT_ID_ENTER_DO_SAFEMODE	0x20B // Enter DO safe mode

#define EVENT_ID_LOGINFAIL          0x300   // Login Fail
#define EVENT_ID_IPCHANGED          0x301   // IP Changed
#define EVENT_ID_PWDCHANGED         0x302   // Password Changed
#define EVENT_ID_CONFIGCHANGED      0x303   // Config Changed
#define EVENT_ID_FWUPGRADE          0x304   // Firmware Upgrade
#define EVENT_ID_SSLIMPORT          0x305   // SSL Certificate Import
#define EVENT_ID_CONFIGIMPORT       0x306   // Config Import
#define EVENT_ID_CONFIGEXPORT       0x307   // Config Export
#define EVENT_ID_WLANCERTIMPORT     0x308   // Wireless Certificate Import
#define EVENT_ID_SERIALLOGEXPORT    0x309   // Serial Data Log Export

#define EVENT_ID_OPMODE_CONNECT     0x400   // Connect
#define EVENT_ID_OPMODE_DISCONNECT  0x401   // Disconnect
#define EVENT_ID_OPMODE_AUTHFAIL    0x402   // Authentication Fail
#define EVENT_ID_OPMODE_RESTART     0x403   // Restart

#define EVENT_ID_DCDCHANGE          0x500   // serial DCD change
#define EVENT_ID_DCDCHANGE_MAIL     0x501   // serial DCD change (mail)
#define EVENT_ID_DCDCHANGE_TRAP     0x502   // serial DCD change (trap)
#define EVENT_ID_DSRCHANGE          0x510   // serial DSR change
#define EVENT_ID_DSRCHANGE_MAIL     0x511   // serial DSR change (mail)
#define EVENT_ID_DSRCHANGE_TRAP     0x512   // serial DSR change (trap)
#define EVENT_ID_DCDCHANGE_LOCAL    0x513   // serial DCD change (local)
#define EVENT_ID_DSRCHANGE_LOCAL    0x514   // serial DSR change (local)
#if 1
#define EVENT_ID_POWER_FAIL_MAIL		0x600	// Power Failure (mail)
#define EVENT_ID_POWER_FAIL_RELAY	0x601	// Power Failure (relay)
#define EVENT_ID_LINKDOWN_MAIL		0x602	// Link Down (mail)didn't support
#define EVENT_ID_LINKDOWN_TRAP		0x603	// Link Down (trap)didn't support
#define EVENT_ID_LINKDOWN_RELAY		0x604	// Link Down (relay)
#endif
#define EVENT_ID_DI1_CHANGED_TRAP	0x701	//DI1 Changed Trap
#define EVENT_ID_DI2_CHANGED_TRAP	0x702	//DI1 Changed Trap
#define EVENT_ID_DI3_CHANGED_TRAP	0x703	//DI1 Changed Trap
#define EVENT_ID_DI4_CHANGED_TRAP	0x704	//DI1 Changed Trap
#define EVENT_ID_DI5_CHANGED_TRAP	0x705	//DI1 Changed Trap
#define EVENT_ID_DI6_CHANGED_TRAP	0x706	//DI1 Changed Trap
#define EVENT_ID_DI7_CHANGED_TRAP	0x707	//DI1 Changed Trap
#define EVENT_ID_DI8_CHANGED_TRAP	0x708	//DI1 Changed Trap

#define EVENT_ID_DO1_CHANGED_TRAP	0x801	//DO1 Changed Trap
#define EVENT_ID_DO2_CHANGED_TRAP	0x802	//DO1 Changed Trap
#define EVENT_ID_DO3_CHANGED_TRAP	0x803	//DO1 Changed Trap
#define EVENT_ID_DO4_CHANGED_TRAP	0x804	//DO1 Changed Trap

void eventd_reload();

#endif
