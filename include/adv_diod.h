/*  Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 */

#ifndef __ADV_DIOD_H__
#define __ADV_DIOD_H__
#define MTC_MSGKEY    	0x4d54434f
#define MTC_TAG_STR_LEN 32
struct mtc_msg_stru
{
	long mtype;
	struct timeval tm;
	int		trigger_cond;
	char		tag_str[MTC_TAG_STR_LEN];
	char		active_str[MTC_TAG_STR_LEN];
	char		inactive_str[MTC_TAG_STR_LEN];
	int		tbl_idx;
};
/* General Definition for DIO */
//#define MAX_DI 8
//#define MAX_DO 4
/* IOCTL Command for IO Driver */
#define PNDRV_MAGIC 'M'
#define IOCTL_IO_DI_INIT             		_IOW(PNDRV_MAGIC, 1, int)
#define IOCTL_IO_DI_MONITOR             _IOW(PNDRV_MAGIC, 2, int)
#define IOCTL_IO_DI_WRITE             	_IOW(PNDRV_MAGIC, 3, int)
#define IOCTL_IO_DI_SLEEP           		_IOW(PNDRV_MAGIC, 4, int)
#define IOCTL_IO_DI_WAKEUP          	_IOW(PNDRV_MAGIC, 5, int)
#define IOCTL_IO_MAPPING			_IOW(PNDRV_MAGIC, 6, int)
#define IOCTL_IO_DI_TEST            		_IOW(PNDRV_MAGIC, 99, int)
#define IOCTL_IO_DO_INIT             		_IOW(PNDRV_MAGIC, 101, int)
#define IOCTL_IO_DO_WRITE             		_IOW(PNDRV_MAGIC, 102, int)
#define IOCTL_IO_DO_MONITOR			_IOW(PNDRV_MAGIC, 103, int)
/* DIO Structure Definition */
/* DI Mode Definition */
#define IO_DI_MODE_DI 			0
#define IO_DI_MODE_CNT 		1

/* DI CNT Trigger Definition */
#define IO_DI_CNT_TRIG_LH		0
#define IO_DI_CNT_TRIG_HL		1
#define IO_DI_CNT_TRIG_BOTH	2

/* DI Status */
#define IO_DI_STAT_OFF		0
#define IO_DI_STAT_ON		1

/* DO Mode  */
#define IO_DO_MODE_DO			0
#define IO_DO_MODE_PULSE_CNT	1

/* DO Status */
#define IO_DO_STAT_OFF		0
#define IO_DO_STAT_ON		1

/* DO Safe Status Setting */
#define IO_DO_SAVE_STAT_OFF	0
#define IO_DO_SAVE_STAT_ON	1
#define IO_DO_SAVE_STAT_LAST	2

/* DO Safe Status Setting */
#define IO_DO_PULSE_SAVE_STAT_DIS	0
#define IO_DO_PULSE_SAVE_STAT_EN	1

#define ADV_DIOD_SIGNAL SIGRTMIN + 2	/* 34 + 2 = 36, reliable signal is number greater than SIGRTMIN */
#define ADV_DIOD_SIGNAL_DO SIGRTMIN + 3
#define ADV_DIOD_SIGNAL_SAFE SIGRTMIN + 4
#define ADV_DIOD_SIGNAL_MTC SIGRTMIN + 5
#define ADV_DIOD_SIGNAL_MTC_PULSE_DEF SIGRTMIN + 6
#define ADV_DIOD_SIGNAL_NOTIFY_DO_COMPLETE SIGRTMIN + 10
#define ADV_DIOD_PID_FILE   	"/var/run/devsvr/adv_diod.pid"

#endif // __DIOD_H__

