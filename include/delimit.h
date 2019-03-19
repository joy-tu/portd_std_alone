/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

#ifndef _DELIMIT_H_
#define _DELIMIT_H_

#define DK_SHM_KEY			6310

#define DK_BUFFER_SIZE_E2S  1024

#define DK_DO_FORCE_TX	0x0001
#define DK_DO_DELI_CH1	0x0002
#define DK_DO_DELI_CH2	0x0004
#define DK_MODE_RAW_TCP 0x0008
#define DK_LINK_SIO		0x0010
#define DK_RUN			0x0020
#define DK_DO_BUFFERING 0x0040
//#define DK_DATA_IN_SD   0x0080
//#define DK_SD_DATA_END  0x0100
#define DK_RESET_BUFFER 0x0200
#define DK_CAN_BUFFER 	0x0400
#define DK_IGNORE_JAM_IP	0x0800
#define DK_DO_DELIMITER (DK_DO_FORCE_TX | DK_DO_DELI_CH1 | DK_DO_DELI_CH2)

struct data_keep_param
{
	int		port;

	int		max_conns;	/* Maximum connections */
	int		*fd_net;
	int		*data_sent;	/* see comment in port_buffering_tcp_is_clear() */
	int		fd_port;
	int 	iftype;

	unsigned int	flag;
	unsigned int	deli_process;
	unsigned char	deli_char1;
	unsigned char	deli_char2;
	unsigned int	force_tx_waitms;

	int		(*sendfunc)(int port, int fd_net, char *buf, int len);
	int		(*recvfunc)(int port, int fd_net, char *buf, int len);

	int		(*sio_read)(int port, char *buf, int len);
	int		(*sio_write)(int port, char *buf, int len);

	int		s2e_cflag;  	/* there is data to be checked */
	int		s2e_ccnt;   	/* checked data count, reset when delimiter found or every DK_FORCE_TX_SIZE */
	int		s2e_cndx;		/* serial to ethernet => data checking index	*/
	int		s2e_rndx;		/* serial to ethernet => data reading index 	*/
	int		s2e_wndx;		/* serial to ethernet => data writing index 	*/
	int		s2e_size;		/* serial buffer size */
	int		s2e_len;
	int		s2e_sentlen;	/* when packlen or delimiter+1/2, set this to inform sending out on next read */
	int		packlen;
	unsigned char deli_stat;	/* delimiter state */
	int		sent_to_tcp_len;	/* data sent to TCP buffer */
	unsigned char *s2e_buf;

	/*int 	e2s_cndx;	*/	/* ethernet to serial => data checking index 	*/	/* unused	*/
	int		e2s_rndx;		/* serial to ethernet => data reading index 	*/
	int		e2s_wndx;		/* serial to ethernet => data writing index 	*/
	int 	e2s_len;
	unsigned char e2s_buf[DK_BUFFER_SIZE_E2S];

	unsigned long 	lasttime;

	int		(*del_read)(int port, int send_buffered_data);
	int		(*del_write)(int port);

	int ipcfd[2];
};
typedef struct data_keep_param		dkparam;
typedef struct data_keep_param *	fdkparam_t;

#endif	/* _DELIMIT_H_ */
