
/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _MISCD_H
#define _MISCD_H

#define MISCD_MSGKEY	0x12345671
#define	MISCD_PID_FILE	"/var/run/devsvr/miscd.pid"
#define POWER_STATUS_TMP_PATH   		"/var/tmp/_power_status"
#define ETHERNET_STATUS_TMP_PATH   		"/var/tmp/_ethernet_status"
#define ETHERNET_POWER_STATUS_TMP_PATH  "/var/tmp/_ethernet_and_power_status"

#define ETH_LINK_STATUS_PATH_FORMAT    "/sys/class/net/%s/carrier"
#define ETH_INTERFACE_NAME             IF_NAME_DEV_ETH0

#define RELAY_OUTPUT_ON  0x00
#define RELAY_OUTPUT_OFF 0x01

//ntp status
#define NTP_STATUS_INIT     0
#define NTP_STATUS_FAIL     1
#define NTP_STATUS_OK       2

/*
 *    Alarm Type
 */
enum
{
    eALARM_MIN = 0,
    eALARM_POWER_1 = eALARM_MIN,
    eALARM_POWER_2,
    eALARM_ETHERNET_1,
#if defined (ia5x50aio)    
    eALARM_ETHERNET_2,
#endif    
    eALARM_MAX
};

/*
 *    Alarm Entry
 *
 *    one entry represents a single alarm.
 */
typedef struct _ALARM_ENTRY
{
    int enable;         //0: enable alarm, 1: disable alarm, read from config file
    int status;         //0: no alarm, 1: alarm occur
    int acked;          //0: nothing, 1: user temporarily excludes alarm
    int acked_mask;     //bitmask for extracting acked bit
} ALARM_ENTRY;

struct miscdbuf
{
    long mtype;
    int event_id;
    int value;
};

#if defined(w2x50a) || defined (ia5x50aio)
#define SUPPORT_BUZZER
#define SUPPORT_DEBUGLED
#define SUPPORT_RELAYOUTPUT
#elif defined(w1)
#define SUPPORT_DIO
#endif // w2x50a


#define MISCD_EVENT_NTPSERVER_CHANGED   0x100   //ntp server change
#define MISCD_EVENT_GRATUITOUS_CHANGED  0x101   //gratuitous setting change
#define MISCD_EVENT_WLANLED_CHANGED     0x102   //WLAN Led setting change
#define MISCD_EVENT_SAFESTATUS_CHANGED  0x103 //safe status setting change

#define MISCD_EVENT_WLAN_STRENGTH       0x200   // wlan strength led
#define MISCD_EVENT_WLAN_LINK           0x201   // wlan link led
#define MISCD_EVENT_READY_LED           0x202   // ready LED
#define MISCD_EVENT_FAULT_LED           0x203   // fault LED
#define MISCD_EVNET_WLAN_READY        0x204  // wlan link ready
#define MISCD_EVENT_READY_LED_RESTORE    0x205  // check ready LED to prevent from both fault(IP conflict) and ready(NPort Locate) to light on at the same time
#define MISCD_EVENT_READY_LED_TEMP    0x206  // temporary change LED status

#ifdef SUPPORT_DIO
#define MISCD_EVENT_DIO_MODE            0x300   // set DIO mode
#define MISCD_EVENT_DIO_GET             0x301   // get DIO value
#define MISCD_EVENT_DIO_SET             0x302   // set DIO value
#endif // SUPPORT_DIO

#ifdef SUPPORT_BUZZER
#define MISCD_EVENT_BUZZER              0x400   // set buzzer
#endif // SUPPORT_BUZZER

#ifdef SUPPORT_DEBUGLED
#define MISCD_EVENT_DEBUGLED            0x500   // debug led
#endif // SUPPORT_DEBUGLED

#ifdef SUPPORT_RELAYOUTPUT
/* To short the relay, call sys_miscd_send(MISCD_RELAY_OUTPUT, 1) */
/* To open the relay,  call sys_miscd_send(MISCD_RELAY_OUTPUT, 0) */
#define MISCD_RELAY_OUTPUT              0x900
#endif
/*
 *   bit definitions for MISCD_ALARM_ACK
 *
 *   bit0 = 1: ack power 1
 *   bit1 = 1: ack power 2
 *   bit2 = 1: ack ethernet
 */
#define MISCD_ALARM_ACK            0x901

#define BIT_ALARM_ACK_POWER_1      0x01
#define BIT_ALARM_ACK_POWER_2      0x02
#define BIT_ALARM_ACK_ETHERNET_1   0x04
#define BIT_ALARM_ACK_ETHERNET_2   0x08

/*
 *    bit definition for MISCD_POWER_STATUS
 *
 *    bit0 = 1: power 1 failure
 *    bit1 = 1: power 2 failure
 *    bit2 ~ 3: reserved
 *    bit4 = 1: power 1 is acked
 *    bit5 = 1: power 2 is acked
 *
 */
#define MISCD_POWER_STATUS  0xa00

#define BIT_POWER_1_STATUS  0x01
#define BIT_POWER_2_STATUS  0x02
#define BIT_POWER_1_ACKED   0x10
#define BIT_POWER_2_ACKED   0x20

/*
 *    bit definition for MISCD_GET_ETHERNET_STATUS
 *
 *    bit0 = 1: ethernet 1 link down
 *    bit1 = 1: ethernet 2 link down
 *    bit2 ~ 3: reserved
 *    bit4 = 1: ethernet 1 is acked
 *    bit5 = 1: ethernet 2 is acked
 */
#define MISCD_GET_ETHERNET_STATUS       0xa01

#define BIT_ETHERNET_1_STATUS  0x01
#define BIT_ETHERNET_2_STATUS  0x02
#define BIT_ETHERNET_1_ACKED   0x10
#define BIT_ETHERNET_2_ACKED   0x20

/*
 *    bit definition for MISCD_GET_ETHERNET_AND_POWER_STATUS
 *
 *    bit0 = 1: power 1 failure
 *    bit1 = 1: power 2 failure
 *    bit2 = 1: ethernet 1 link down
 *    bit3 = 1: ethernet 2 link down
 *    bit4 = 1: power 1 is acked
 *    bit5 = 1: power 2 is acked
 *    bit6 = 1: ethernet 1 is acked
 *    bit7 = 1: ethernet 2 is acked
 */
#define MISCD_GET_ETHERNET_AND_POWER_STATUS       0xa02

#define BIT_COMMON_POWER_1_STATUS    0x01
#define BIT_COMMON_POWER_2_STATUS    0x02
#define BIT_COMMON_ETHERNET_1_STATUS 0x04
#define BIT_COMMON_ETHERNET_2_STATUS 0x08
#define BIT_COMMON_POWER_1_ACKED     0x10
#define BIT_COMMON_POWER_2_ACKED     0x20
#define BIT_COMMON_ETHERNET_1_ACKED  0x40
#define BIT_COMMON_ETHERNET_2_ACKED  0x80

// WLAN LED strength
#define WLAN_STRENGTH_LEVEL_5   5
#define WLAN_STRENGTH_LEVEL_4   4
#define WLAN_STRENGTH_LEVEL_3   3
#define WLAN_STRENGTH_LEVEL_2   2
#define WLAN_STRENGTH_LEVEL_1   1
#define WLAN_STRENGTH_LEVEL_0   0

// ready/fault LED status
#define LED_OFF                 0
#define LED_ON                  1
#define LED_SHINE(_time)        (_time) // MUST > 1, UNIT: ms

// SNMP restart function
#define MISCD_EVENT_SNMP_RESET  0x600

// For roam
//#define MISCD_EVENT_ROAMING_RESET 0x700
//#define MISCD_EVENT_ROAMING_CHECK 0x701

// For WLAN HTC TX Jam Reset
#define MISCD_HTC_TX_JAM_RESET  0x800

#define MISCD_EVNET_MODBUS_CONNECTION 0x1000  // modbus connection
#endif
