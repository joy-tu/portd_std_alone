/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    aspp.h

    NPort S8000 RealCOM(ASPP) mode header file.

    2008-07-15	James Wang
		new release

*/

#ifndef _ASPP_H_
#define _ASPP_H_
#include <posix/sys/select.h>
#ifdef	_ASPP_C
#define	ASPP_EXTERN
#else
#define	ASPP_EXTERN extern
#endif
//#define UART_BURN //undef this will be tcp echo server (for TCP burn only)//
void *aspp_start(void* arg);


#define ASPP_DATA_BASE_PORT1	950		/* Port 1~16 Data Channel 				*/
#define ASPP_CMD_BASE_PORT1		966		/* Port 1~16 Cmd Channel				*/
#define ASPP_DATA_BASE_PORT2	982		/* Port 17~32 Data Channel				*/
#define ASPP_CMD_BASE_PORT2		998		/* Port 17~32 Cmd Channel				*/

#define FLAG_DATA_UP			0x01	/* Data port is connected				*/
#define FLAG_CMD_UP				0x02	/* Cmd port is connected				*/
#define FLAG_SET_NOTIFY			0x04	/* set notify echo						*/
#define FLAG_WAIT_EMPTY			0x08	/* need to wait tx queue empty			*/
#define FLAG_LINKBY_IPADDR		0x10	/* connect to some IP only 				*/
#define FLAG_LINKBY_PASSWD		0x20	/* connect need password				*/
#define FLAG_IS_LISTENTO		0x40	/* socket is listen to someone			*/
#define FLAG_LISTEN_SPECIAL		0x80	/* listen to special one IP address 	*/
#define FLAG_PROMPT_USRNAME		0x100	/* prompt for user name					*/
#define FLAG_WAIT_USRNAME		0x200	/* waiting for user name				*/
#define FLAG_PROMPT_PASSWD		0x400	/* prompt for password					*/
#define FLAG_WAIT_PASSWD		0x800	/* waiting for password					*/
#define FLAG_PROMPT_LOGIN_FAIL	0x1000	/* prompt for login fail 				*/
#define	FLAG_FLUSH_TXDATA		0x4000	/* Flush transmit data of this line		*/
#define	FLAG_FLUSH_RXDATA		0x8000	/* Flush received data of this line		*/

#define	FLAG_FLUSH_DATA			(FLAG_FLUSH_TXDATA|FLAG_FLUSH_RXDATA)

#define FLAG_SOMEONE_ONLY		(FLAG_LINKBY_IPADDR | FLAG_LINKBY_PASSWD)

#define D_ASPP_CMD_IOCTL			0x10
#define D_ASPP_CMD_FLOWCTL			0x11
#define D_ASPP_CMD_LCTRL			0x12
#define D_ASPP_CMD_LSTATUS			0x13
#define D_ASPP_CMD_FLUSH			0x14
#define D_ASPP_CMD_IQUEUE			0x15
#define D_ASPP_CMD_OQUEUE			0x16
#define D_ASPP_CMD_BAUDRATE			0x17
#define D_ASPP_CMD_XONXOFF			0x18	/* Set XON, XOFF characters         */
#define D_ASPP_CMD_RESET			0x20
#define D_ASPP_CMD_BREAKON			0x21	/* Send break signal ON 	        */
#define D_ASPP_CMD_BREAKOFF			0x22	/* Send break singal OFF	        */
#define D_ASPP_CMD_BREAKSEND		0x23	/* Send break some mini-seconds     */
#define D_ASPP_CMD_NOTIFYON			0x24	/* Start singals notify 	        */
#define D_ASPP_CMD_NOTIFYOFF		0x25	/* Stop singals notify		        */
#define D_ASPP_RSP_NOTIFY			0x26	/* CN2000 to PC only		        */
#define D_ASPP_CMD_ALIVE			0x27	/* CN2000 to PC ask alive cmd.	    */
#define D_ASPP_RSP_ALIVE			0x28	/* PC to CN2000 alive response	    */
#define D_ASPP_CMD_GETCHID			0x29	/* Get challenge ID number	        */
#define D_ASPP_CMD_PSWDCHK			0x2A	/* Connection password check	    */
#define D_ASPP_CMD_LINKNAME			0x2B	/* Set connection remote name	    */
#define D_ASPP_CMD_SETPORT			0x2C	/* Set port's parameters            */
#define D_ASPP_RSP_DATAOUT			0x2D	/* CN2000 to PC: oqueue free	    */
#define D_ASPP_CMD_REXMITTIME		0x2E	/* Set TCP rexmit time		        */
#define D_ASPP_CMD_NOT_OFREE		0x2F	/* Set OutQueue free notify	        */
#define D_ASPP_CMD_SET_TXFIFO		0x30	/* Enable/Disable TX FIFO	        */
#define D_ASPP_CMD_SET_COMM			0x31	/* Set T/R buffer size & H/L water  */
#define	D_ASPP_CMD_DSR_SEN			0x32	/* Set DSR sensitivity		        */
#define	D_ASPP_CMD_SETXON			0x33	/* Set recv XON state		        */
#define	D_ASPP_CMD_SETXOFF			0x34	/* Set recv XOFF state		        */
#define	D_ASPP_CMD_FLUSH_START		0x35	/* Start flush transmit data	    */
#define	D_ASPP_CMD_FLUSH_STOP		0x36	/* Stop flush transmit data	        */
#define	D_ASPP_CMD_BREAK_COUNT		0x37	/* Get Break Count	                */
#define	D_ASPP_CMD_DATA_STATUS		0x38	/* Get Data Status	                */

#define D_ASPP_EV_PARITY			0x01	/* parity error 		            */
#define D_ASPP_EV_FRAMING			0x02	/* framing error		            */
#define D_ASPP_EV_HW_OVERRUN		0x04	/* H/W overrun error		        */
#define D_ASPP_EV_SW_OVERRUN		0x08	/* S/W overrun error		        */
#define D_ASPP_EV_BREAK				0x10	/* receive BREAK singal 	        */
#define D_ASPP_EV_MSR_CHG			0x20	/* modem status change		        */
#define D_ASPP_EV_LSR_DATA			0x40	/* line status change with data     */
#define D_ASPP_EV_LSR_NODATA		0x80	/* line status change		        */

#define D_ASPP_EV2_CTSHOLD			0x01	/* TX hold by CTS	                */
#define D_ASPP_EV2_DSRHOLD			0x02	/* TX hold by DSR	                */
#define D_ASPP_EV2_RLSDHOLD			0x04	/* TX hold by DCD	                */
#define D_ASPP_EV2_XOFFHOLD			0x08	/* TX hold by XOFF	                */
#define D_ASPP_EV2_XOFFSENT			0x10	/* Remote TX hold by sending XOFF   */



/* Parameters for D_ASPP_CMD_IOCTL ioctl command */
#define D_ASPP_IOCTL_B300		0	/* IOCTL : baud rate = 300 bps 		*/
#define D_ASPP_IOCTL_B600		1	/* IOCTL : baud rate = 600 bps 		*/
#define D_ASPP_IOCTL_B1200		2	/* IOCTL : baud rate = 1200 bps 	*/
#define D_ASPP_IOCTL_B2400		3	/* IOCTL : baud rate = 2400 bps 	*/
#define D_ASPP_IOCTL_B4800		4	/* IOCTL : baud rate = 4800 bps 	*/
#define D_ASPP_IOCTL_B7200		5	/* IOCTL : baud rate = 7200 bps 	*/
#define D_ASPP_IOCTL_B9600		6	/* IOCTL : baud rate = 9600 bps 	*/
#define D_ASPP_IOCTL_B19200		7	/* IOCTL : baud rate = 19200 bps 	*/
#define D_ASPP_IOCTL_B38400		8	/* IOCTL : baud rate = 38400 bps 	*/
#define D_ASPP_IOCTL_B57600		9	/* IOCTL : baud rate = 57600 bps 	*/
#define D_ASPP_IOCTL_B115200	10	/* IOCTL : baud rate = 115200 bps 	*/
#define D_ASPP_IOCTL_B230400	11	/* IOCTL : baud rate = 230400 bps 	*/
#define D_ASPP_IOCTL_B460800	12	/* IOCTL : baud rate = 230400 bps 	*/
#define D_ASPP_IOCTL_B921600	13	/* IOCTL : baud rate = 921600 bps 	*/
#define D_ASPP_IOCTL_B150		14	/* IOCTL : baud rate = 150 bps 		*/
#define D_ASPP_IOCTL_B134		15	/* IOCTL : baud rate = 134 bps 		*/
#define D_ASPP_IOCTL_B110		16	/* IOCTL : baud rate = 110 bps 		*/
#define D_ASPP_IOCTL_B75		17	/* IOCTL : baud rate = 75 bps 		*/
#define D_ASPP_IOCTL_B50		18	/* IOCTL : baud rate = 50 bps 		*/

#define D_ASPP_IOCTL_NONE		0	/* IOCTL : none parity				*/
#define D_ASPP_IOCTL_EVEN		8	/* IOCTL : even parity 				*/
#define D_ASPP_IOCTL_ODD		16	/* IOCTL : odd parity 				*/
#define D_ASPP_IOCTL_MARK		24	/* IOCTL : mark parity				*/
#define D_ASPP_IOCTL_SPACE		32	/* IOCTL : space parity				*/

#define CMD_LEN 1024

// for DSCI command 0x14 (dsc_GetNetstat)
typedef struct _aspp_socket_stat
{
	unsigned long remote_ip;
	unsigned long local_ip;
	unsigned short remote_port;
	unsigned short local_port;
	unsigned char socket_type;
	unsigned char tcp_state;
	unsigned char serial_port;
	unsigned char reserved;
} aspp_socket_stat;


void aspp_open_data_listener(ASPP_SERIAL *detail);
void aspp_open_cmd_listener(ASPP_SERIAL *detail);
void aspp_close_data_listener(ASPP_SERIAL *detail);
void aspp_close_cmd_listener(ASPP_SERIAL *detail);


void aspp_ssl_init(ASPP_SERIAL *detail);

void aspp_setup_fd(int port, struct timeval *tv, fd_set *rfds, fd_set *wfds, int *maxfd, int do_port_buffering, int serial_buffered);
void aspp_main(int port, int is_driver);

void aspp_main(int port, int is_driver);
int aspp_check_inactivity(int port, int n, unsigned long idletime);

int	aspp_sendfunc(int port, int fd_net, char* buf, int len);
int aspp_recvfunc(int port, int fd_net, char *buf, int len);

int aspp_command(int port, int conn, char *buf, int len);
int aspp_convert_baud(int baud);
int aspp_convert_parity(int mode);
int aspp_convert_flow(int cts, int rts, int stx, int srx);

int	aspp_flush_data(int port, int fd_port, int fd_data, int mode);
int	aspp_flush_data(int port, int fd_port, int fd_data, int mode);
int	aspp_flush_reply(int realtty, char * buf, int fd_data);

int aspp_accept_data(int port);
int aspp_accept_cmd(int port);

void aspp_close_data(int port, int index);
void aspp_close_cmd(int port, int index);
int aspp_open_serial(int port);
void aspp_close_serial(int port);

void aspp_update_lasttime(int port);
int aspp_tcp_iqueue(int fd);
void aspp_tcp_flush_iqueue(int port, int fd);
#endif
