#ifndef _WLAN_LOGD_H
#define _WLAN_LOGD_H

#define WLOG_PID_FILE			"/var/run/devsvr/wlan_logd.pid"
#define WLOG_MSGKEY			0x12345675
#define WLOG_BYTES_PER_ROW	80
#define WLOG_ROWS_PER_FILE	1024
#define WLOG_SIZE				WLOG_BYTES_PER_ROW*WLOG_ROWS_PER_FILE
#define WLOG_TIMESTAMP_LEN	20     
#define WLOG_INDEX				"/mnt/wlan_log_index"  
#define WLOG_FILE				"/mnt/wlan_log"

struct wlogdbuf
{
    long mtype;
    char mtext[256];
};

#endif
