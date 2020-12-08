#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#define ZEPHYR
#ifdef ZEPHYR
#include <posix/sys/time.h>
#include <posix/pthread.h>
#include <sio/mx_sio.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include "sio.h"
#endif

#define USE_SIO

#define COM1 1
#define BUF_SIZE 4096
#define TX_SIZE 4096
#define PORT_START_TURE 1
#define PORT_START PORT_START_TURE - 1
#define PORT_NUM 1
#define PORT_BASE 0
#define PATTERN_SIZE 256
struct uart_local
{
	char m_rx_buf[BUF_SIZE];
	char m_tx_buf[BUF_SIZE];
	int fd;
	int m_rx_index;
	int m_tx_index;
	int m_port_index;
	double m_timer_sum;
	int m_count;
	int data_err_flag;
	unsigned long m_total_rx;
	time_t m_t0,m_t1;
} Guart[PORT_NUM];

int tran_baud(char *str, int baud)
{
#ifdef USE_SIO
	switch (baud) {
		case 50:
			memcpy(str, "50", sizeof("50"));
			return BAUD_50;
		case 300:
			memcpy(str, "300", sizeof("300"));
			return BAUD_300;
		case 600:
			memcpy(str, "600", sizeof("600"));
			return BAUD_600;
		case 1200:
			memcpy(str, "1200", sizeof("1200"));
			return BAUD_1200;
		case 2400:
			memcpy(str, "2400", sizeof("2400"));
			return BAUD_2400;
		case 4800:
			memcpy(str, "4800", sizeof("4800"));
			return BAUD_4800;
		case 9600:
			memcpy(str, "9600", sizeof("9600"));
			return BAUD_9600;
		case 19200:
			memcpy(str, "19200", sizeof("19200"));
			return BAUD_19200;
		case 38400:
			memcpy(str, "38400", sizeof("38400"));
			return BAUD_38400;
		case 57600:
			memcpy(str, "57600", sizeof("57600"));
			return BAUD_57600;
		case 115200:
			memcpy(str, "115200", sizeof("115200"));
			return BAUD_115200;
		case 230400:
			memcpy(str, "230400", sizeof("230400"));
			return BAUD_230400;
		case 460800:
			memcpy(str, "460800", sizeof("460800"));
			return BAUD_460800;
		case 921600:
			memcpy(str, "921600", sizeof("921600"));
			return BAUD_921600;
		default:
			memcpy(str, "115200", sizeof("115200"));
			return BAUD_115200;
	}	
#else
	switch (baud) {
		case 50:
			memcpy(str, "50", 10);
			return B50;
		case 300:
			memcpy(str, "300", 10);
			return B300;
		case 600:
			memcpy(str, "600", 10);
			return B600;
		case 1200:
			memcpy(str, "1200", 10);
			return B1200;
		case 2400:
			memcpy(str, "2400", 10);
			return B2400;
		case 4800:
			memcpy(str, "4800", 10);
			return B4800;
		case 9600:
			memcpy(str, "9600", 10);
			return B9600;
		case 19200:
			memcpy(str, "19200", 10);
			return B19200;
		case 38400:
			memcpy(str, "38400", 10);
			return B38400;
		case 57600:
			memcpy(str, "57600", 10);
			return B57600;
		case 115200:
			memcpy(str, "115200", 10);
			return B115200;
		case 230400:
			memcpy(str, "230400", 10);
			return B230400;
		case 460800:
			memcpy(str, "460800", 10);
			return B460800;
		case 921600:
			memcpy(str, "921600", 10);
			printf("Here are baudrate = %d\r\n", B921600);
			return B921600;
		default:
			return B115200;
	}	
#endif	
}

void uart_init(int baud)
{
	char baudstr[10];
	int ret, i,  port_index, baudrate, port;
#ifdef USE_SIO
#else	
	char devname[256];
	int fd;
	struct termios t;
#endif
	time_t t0;
	time(&t0);

	baudrate = tran_baud(baudstr, baud);
	for(port_index = PORT_START; port_index < PORT_NUM; port_index++){
#ifdef USE_SIO
		port = port_index + 1;
#endif
		Guart[port_index].m_timer_sum = 0;
		Guart[port_index].m_port_index = port_index + PORT_BASE;
		Guart[port_index].m_count = 0;
		Guart[port_index].m_rx_index = 0;
		Guart[port_index].m_tx_index = 0;
		Guart[port_index].m_total_rx = 0;
		Guart[port_index].m_t0 = t0;
		Guart[port_index].data_err_flag = 0;
		//sio_open(Guart[port_index].m_port_index);
#ifdef USE_SIO
		ret = sio_open(port); /* base 1 = port 1 */
		printf("sio_open%d = %d\r\n",port, ret);
#ifdef ZEPHYR		
		sio_set_iftype(port, SIO_LOOPBACK_MODE);
#else
		sio_set_iftype(port, RS232_MODE);
#endif
#ifdef ZEPHYR
		ret = sio_set_dtr(port, 1);
		printf("sio_DTR%d = %d\r\n",port,ret);
		ret = sio_set_rts(port, 1);
		printf("sio_RTS%d = %d\r\n",port,ret);
#else
		ret = sio_DTR(port, 1);
		printf("sio_DTR%d = %d\r\n",port,ret);
		ret = sio_RTS(port, 1);
		printf("sio_RTS%d = %d\r\n",port,ret);
#endif		
		ret = sio_ioctl(port, baudrate, BIT_8 | STOP_1 | P_NONE);
		printf("sio_ioctl%d = %d\r\n",port,ret);
		//ret = sio_flowctrl(port, F_HW);
		printf("sio_flowctrl%d = %d\r\n",port,ret);
#ifdef ZEPHYR		
		ret = sio_flush(port, SIO_FLUSH_ALL);
		printf("sio_flush%d = %d\r\n",port,ret);
#else
		ret = sio_flush(port, FLUSH_ALL);
		printf("sio_flush%d = %d\r\n",port,ret);
#endif		
		Guart[port_index].fd = port;
#else /* ZEPHYR */
		sprintf(devname, "/dev/ttyMXUSB%d", Guart[port_index].m_port_index);
		fd = open(devname, O_RDWR);
		Guart[port_index].fd = fd;
		tcgetattr(fd , &t);
		t.c_lflag = 0;
		t.c_cc[VMIN] = 0;
		t.c_cc[VTIME] = 1;
//		t.c_cflag = B921600 | CRTSCTS | CS8 | CREAD | CLOCAL;
		t.c_cflag = baudrate | CRTSCTS | CS8 | CREAD | CLOCAL;
		t.c_iflag = 0;
		t.c_oflag = 0;
		tcsetattr(fd,TCSANOW,&t);

		tcflush(fd,TCIOFLUSH);
#endif		
		//		sio_ioctl(Guart[port_index].m_port_index, B921600, P_NONE|BIT_8|STOP_1);
//		sio_flowctrl(Guart[port_index].m_port_index, H_FLOW_CONTROL);
//		sio_interrupt_mode(Guart[port_index].m_port_index);

		for(i = 0; i < BUF_SIZE; i++){
			//if(i < PATTERN_SIZE)
				Guart[port_index].m_tx_buf[i] = 0x41 + i;
			//else
			//	Guart[port_index].m_tx_buf[i] = i - PATTERN_SIZE;
		}
	}
	printf("------------------------------------\r\n");
	printf("-UART Performance Evaluation Tool  -\r\n");
	printf("-UARTs are Burnining               -\r\n");
	printf("-Port%d~Port%d baudrate%s,N81    -\r\n", PORT_BASE+1, PORT_NUM, baudstr);
	printf("------------------------------------\r\n");
}

void uart_release(void)
{
	int index;
	for(index = 0 ; index < PORT_NUM ; index++)
#ifdef USE_SIO
		sio_close(index + 1);
#else
		close(Guart[index].fd);
#endif
}

void *write_thread(void* data)
{
	int i;
	struct uart_local *uart_p;
	i = PORT_START;
	
	for (;;) {
		while(i < PORT_NUM){
			uart_p = &Guart[i];
			i++;
#ifdef USE_SIO
//			wlen = sio_write(uart_p->fd, "Joy", sizeof("joy"));
//			rlen = sio_read(uart_p->fd, buf, 4);
//			printf("Wlen=%d,Rlen=%d,Rdata=%s\r\n", 
//				wlen, rlen, buf);
			uart_p->m_tx_index += sio_write(uart_p->fd, &uart_p->m_tx_buf[uart_p->m_tx_index], TX_SIZE - uart_p->m_tx_index);
//			printf("Wlen = %d\r\n", uart_p->m_tx_index);
#else
			uart_p->m_tx_index += write(uart_p->fd, &uart_p->m_tx_buf[uart_p->m_tx_index], TX_SIZE - uart_p->m_tx_index);
#endif
			if(uart_p->m_tx_index >= TX_SIZE)
				uart_p->m_tx_index = 0;

		}
		i = PORT_START;
	}
	pthread_exit(NULL);
}

void *read_thread(void* data)
{
	int i;
	struct uart_local *uart_p;
	int  compare_index;
	double/* timer,*/ speed;
	int timer;
	struct timeval tv;

	i = PORT_START;
	for (;;) {
		while(i < PORT_NUM){
			uart_p = &Guart[i];
			i++;
#ifdef USE_SIO
			uart_p->m_rx_index = sio_read(uart_p->fd, uart_p->m_rx_buf, BUF_SIZE);

#else
			uart_p->m_rx_index = read(uart_p->fd, uart_p->m_rx_buf, BUF_SIZE);
#endif
			uart_p->m_total_rx += uart_p->m_rx_index;

			for(compare_index = 0; compare_index < uart_p->m_rx_index; compare_index++){
				if(uart_p->data_err_flag == 1)
					break;

				if(uart_p->m_rx_buf[compare_index] != uart_p->m_tx_buf[uart_p->m_count]){
					printf("PORT[%d] DATA ERROR! %x->%x\n",i, uart_p->m_rx_buf[compare_index], uart_p->m_tx_buf[uart_p->m_count]);
					uart_p->data_err_flag = 1;

				}
				uart_p->m_count++;
				if(uart_p->m_count == BUF_SIZE)
					uart_p->m_count = 0;

			}

//			time(&uart_p->m_t1);
			//uart_p->m_t1 = time(0);
			//timer = difftime(uart_p->m_t1, uart_p->m_t0);
			//
			gettimeofday(&tv, NULL);
			uart_p->m_t1 = tv.tv_sec;
			timer = uart_p->m_t1 - uart_p->m_t0;
			if(timer >= 10){
			//if (uart_p->m_t1 - uart_p->m_t0 >= 10) {
				uart_p->m_timer_sum += timer;
				speed = uart_p->m_total_rx / uart_p->m_timer_sum;
				if(!(uart_p->data_err_flag))
					printf("port[%d] time: %f (sec), total_rx: %ld, throughput %f bytes/sec\n",i, uart_p->m_timer_sum, uart_p->m_total_rx, speed);
				else
					printf("port[%d] DATA ERROR\n",i);
				uart_p->m_t0 = uart_p->m_t1;
			}
		
		}
		i = PORT_START;
	}
	pthread_exit(NULL);
}

//#define WRITE
//#define READ
#ifdef ZEPHYR
#define BURN_WSTACK_SIZE 4096 
K_THREAD_STACK_DEFINE(burn_wstack, BURN_WSTACK_SIZE);
#define BURN_RSTACK_SIZE 4096 
K_THREAD_STACK_DEFINE(burn_rstack, BURN_RSTACK_SIZE);

void do_burnin(void)
#else
void main(void)
#endif
{
	int i;
	pthread_t t1, t2;

	i = PORT_START;

	uart_init(115200);
#ifdef ZEPHYR
	pthread_attr_t _attr;
#if 1
	 (void)pthread_attr_init(&_attr);
	 (void)pthread_attr_setstack(&_attr, &burn_wstack,
			    BURN_WSTACK_SIZE);
	  pthread_create(&t1, &_attr, &write_thread, (void *)0);
#endif
#if 1
	 (void)pthread_attr_init(&_attr);
	 (void)pthread_attr_setstack(&_attr, &burn_rstack,
			    BURN_RSTACK_SIZE);
	  pthread_create(&t2, &_attr, &read_thread, (void *)0);
#endif	  
#else
	pthread_create(&t1, NULL, write_thread, "write_thread");
	pthread_create(&t2, NULL, read_thread, "read_thread");
#endif
	for(;;){
		sleep(5);
	}
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	uart_release();
}

