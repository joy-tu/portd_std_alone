/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    cfg_api.h

    Config data API for MiiNEPort W1.

    2010-12-31  James Wang
                developing...
*/

#ifndef _CFG_API_H
#define _CFG_API_H

typedef enum _DATATYPE
{
    DATATYPE_INT8U = 1,
    DATATYPE_INT16U,
    DATATYPE_INT32U,
    DATATYPE_INT8S,
    DATATYPE_INT16S,
    DATATYPE_INT32S,
    DATATYPE_STRING
} DATATYPE;
/*
#define CONFIG_FILEPATH         "/etc/devsvr"
#define CONFIG_FILE_SYSTEM      "cf_sys.conf"
#define CONFIG_FILE_PORT        "cf_port.conf"
#define CONFIG_FILE_MISC        "cf_misc.conf"
#define CONFIG_FILE_WARN        "cf_warn.conf"
#define CONFIG_FILE_WIRELESS    "cf_wireless.conf"
#define CONFIG_FILE_IO			"cf_io.conf"
#define CONFIG_DEFAULTFILEPATH  "/etc/default"
*/
#define IPC_CONFIG_FILEPATH	"/var/run"
#define IPC_CONFIG_FILE_IO		"cf_ipc.conf"
#define CONFIG_IPC_PATH        "/var/tmp"

union semun
{
    int val;                /* value for SETVAL */
    struct semid_ds *buf;   /* buffer for IPC_STAT & IPC_SET */
    unsigned short *array;          /* array for GETALL & SETALL */
    struct seminfo *__buf;  /* buffer for IPC_INFO */
    void *__pad;
};


// FUNCTION HEADER ----------------------------------------------------------
//
// FUNCTION NAME: config_get
// FUNCTION DESCRIPTION
//   Parameters:
//   Return valie:
//       CFG_OK  0
//       CFG_ITEM_NOT_FOUND      -1
//       CFG_FILE_NOT_FOUND      -2
//       CFG_BUFFER_IS_NULL      -3
//       CFG_BUFFER_TOO_SMALL    -4
//       CFG_INVALID_PARAMETER   -5
// --------------------------------------------------------------------------
int ipc_config_get(char* item, int data_type, void* buf, int size);


// FUNCTION HEADER ----------------------------------------------------------
//
// FUNCTION NAME: config_set
// FUNCTION DESCRIPTION
//   Parameters:
//   Return valie:
//       CFG_OK  0
//       CFG_ITEM_NOT_FOUND      -1
//       CFG_FILE_NOT_FOUND      -2
//       CFG_BUFFER_IS_NULL      -3
//       CFG_BUFFER_TOO_SMALL    -4
//       CFG_INVALID_PARAMETER   -5
// --------------------------------------------------------------------------
int ipc_config_set(char* item, int data_type, void* buf, int len);


//
// Semaphore functions for protecting config files.
//
int ipc_create_sem(int *sid, key_t key, int members);
int ipc_open_sem(int *sid, key_t key);
int ipc_sem_up(int semid, int member);
int ipc_sem_down(int semid, int member);
int ipc_create_file(char *path);

#endif
