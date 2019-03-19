/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    modbus_slaved.h

    Description: modbus slave TCP service declare

    2016-04-12  Vince Wu
        Created
*/
#ifndef __MODBUS_SLAVE_H__
#define __MODBUS_SLAVE_H__

#include "../modbus_slaved/rtu_common.h"

#define MODBUS_SLAVED_PID_FILE    "/var/run/devsvr/modbus_slaved.pid"
#define MODBUS_SLAVED_SERV_PORT    502
#define MODBUS_SLAVED_MAX_CONNECTION    10
#define MODBUS_SLAVED_MIN_IDLE_TIMEOUT    1
#define MODBUS_SLAVED_MAX_ITEM    8

struct MAP_DESC_T
{
    UINT8 nAddrGroup;
    UINT16 nAddrStart;
    UINT16 nAddrCount;
    UINT16 nStartReg;
    int (*pfnModRead)(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
    int (*pfnModWrite)(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
}  __attribute__((packed));
typedef struct MAP_DESC_T MAP_DESC;

typedef struct MODBUS_SLAVE_INFO_T
{
    int quantity; // total quantity
    int handle; // tcp only
    int tcpPort;
    int unitID;
    int maxCon;
    int userTimeout;
    MAP_DESC *pModbusMap;
} MODBUS_SLAVE_INFO;

int mod_do_get_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_set_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_get_value_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_set_value_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_get_pwm_start(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_set_pwm_start(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_get_pwm_start_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBith);
int mod_do_set_pwm_start_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_get_value_all(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_do_set_value_all(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);

int mod_di_get_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_value_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_value_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_counter_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_counter_value(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_value_all(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_value_all(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_counter_start(int nStartReg, int nRegCount, UINT8 *pData, int nOffBith);
int mod_di_set_counter_start(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_counter_start_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_counter_start_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_counter_clear(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_counter_clear(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_get_counter_clear_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
int mod_di_set_counter_clear_word(int nStartReg, int nRegCount, UINT8 *pData, int nOffBit);
#endif

