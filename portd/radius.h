/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    radius.h

    Routines to support Authentication (RADIUS) declare

    2008-05-27	Kevin Chu
		new release
*/

#ifndef RADIUS_H
#define RADIUS_H



#define RADIUS_VECTOR_LEN	16

#define AUTH_ID_LEN		64
#define AUTH_STRING_LEN 	128	/* maximum of 253 */

#define FILTER_LEN		16

typedef struct	radius_hdr
{
	u_char		code;
	u_char		id;
	u_short		length;
	u_char		vector[RADIUS_VECTOR_LEN];
} radius_hdr;

typedef struct	radius_hdr *	radius_hdr_t;

#define RADIUS_HDR_LEN			sizeof(radius_hdr)

#define MAX_SECRET_LENGTH		16
#define CHAP_VALUE_LENGTH		16

/*
#define PW_AUTH_UDP_PORT		1645
#define PW_ACCT_UDP_PORT		1646
*/

/* because RFC2138/2139 change the UDP port
   we need let user config it */
/* default radius UDP port number   */
#define RADIUS_UDP_PORT 		1645
#define RADIUS_ACC_UDP_PORT		1646

#define RADIUS_NEW_UDP_PORT		1812
#define RADIUS_NEW_ACC_UDP_PORT 	1813

#define PW_TYPE_STRING			0
#define PW_TYPE_INTEGER 		1
#define PW_TYPE_IPADDR			2
#define PW_TYPE_DATE			3

/* standard RADIUS codes */

#define PW_ACCESS_REQUEST		1
#define PW_ACCESS_ACCEPT		2
#define PW_ACCESS_REJECT		3
#define PW_ACCOUNTING_REQUEST		4
#define PW_ACCOUNTING_RESPONSE		5
#define PW_ACCOUNTING_STATUS		6
#define PW_PASSWORD_REQUEST		7
#define PW_PASSWORD_ACK 		8
#define PW_PASSWORD_REJECT		9
#define PW_ACCOUNTING_MESSAGE		10
#define PW_ACCESS_CHALLENGE		11
#define PW_STATUS_SERVER		12
#define PW_STATUS_CLIENT		13


/* standard RADIUS attribute-value pairs */

#define PW_USER_NAME			1	/* string */
#define PW_USER_PASSWORD		2	/* string */
#define PW_CHAP_PASSWORD		3	/* string */
#define PW_NAS_IP_ADDRESS		4	/* ipaddr */
#define PW_NAS_PORT			5	/* integer */
#define PW_SERVICE_TYPE 		6	/* integer */
#define PW_FRAMED_PROTOCOL		7	/* integer */
#define PW_FRAMED_IP_ADDRESS		8	/* ipaddr */
#define PW_FRAMED_IP_NETMASK		9	/* ipaddr */
#define PW_FRAMED_ROUTING		10	/* integer */
#define PW_FILTER_ID			11	/* string */
#define PW_FRAMED_MTU			12	/* integer */
#define PW_FRAMED_COMPRESSION		13	/* integer */
#define PW_LOGIN_IP_HOST		14	/* ipaddr */
#define PW_LOGIN_SERVICE		15	/* integer */
#define PW_LOGIN_PORT			16	/* integer */
#define PW_OLD_PASSWORD 		17	/* string */ /* deprecated */
#define PW_REPLY_MESSAGE		18	/* string */
#define PW_LOGIN_CALLBACK_NUMBER	19	/* string */
#define PW_FRAMED_CALLBACK_ID		20	/* string */
#define PW_EXPIRATION			21	/* date */ /* deprecated */
#define PW_FRAMED_ROUTE 		22	/* string */
#define PW_FRAMED_IPX_NETWORK		23	/* integer */
#define PW_STATE			24	/* string */
#define PW_CLASS			25	/* string */
#define PW_VENDOR_SPECIFIC		26	/* string */
#define PW_SESSION_TIMEOUT		27	/* integer */
#define PW_IDLE_TIMEOUT 		28	/* integer */
#define PW_TERMINATION_ACTION		29	/* integer */
#define PW_CALLED_STATION_ID		30	/* string */
#define PW_CALLING_STATION_ID		31	/* string */
#define PW_NAS_IDENTIFIER		32	/* string */
#define PW_PROXY_STATE			33	/* string */
#define PW_LOGIN_LAT_SERVICE		34	/* string */
#define PW_LOGIN_LAT_NODE		35	/* string */
#define PW_LOGIN_LAT_GROUP		36	/* string */
#define PW_FRAMED_APPLETALK_LINK	37	/* integer */
#define PW_FRAMED_APPLETALK_NETWORK	38	/* integer */
#define PW_FRAMED_APPLETALK_ZONE	39	/* string */
#define PW_CHAP_CHALLENGE		60	/* string */
#define PW_NAS_PORT_TYPE		61	/* integer */
#define PW_PORT_LIMIT			62	/* integer */

/*	Accounting */

#define PW_ACCT_STATUS_TYPE		40	/* integer */
#define PW_ACCT_DELAY_TIME		41	/* integer */
#define PW_ACCT_INPUT_OCTETS		42	/* integer */ /* not impl. */
#define PW_ACCT_OUTPUT_OCTETS		43	/* integer */ /* not impl. */
#define PW_ACCT_SESSION_ID		44	/* string */
#define PW_ACCT_AUTHENTIC		45	/* integer */
#define PW_ACCT_SESSION_TIME		46	/* integer */
#define PW_ACCT_INPUT_PACKETS		47	/* integer */ /* not impl. */
#define PW_ACCT_OUTPUT_PACKETS		48	/* integer */ /* not impl. */
#define PW_ACCT_TERMINATE_CAUSE 	49	/* integer */

/*	Merit Experimental Extensions */

#define PW_AVAILABLE_TIME		209	/* integer */
#define PW_INFO_PORT			210	/* integer */
#define PW_PROXY_ACTION 		211	/* string */
#define PW_SIGNATURE			212	/* string */
#define PW_TOKEN			213	/* string */
#define PW_ACCT_RATE			214	/* string */
#define PW_ACCT_CHARGE			215	/* string */
#define PW_ACCT_TRANSACTION_ID		216	/* string */
#define PW_ACCT_CHARGE_ALLOWED		217	/* string */
#define PW_MAXIMUM_TIME 		218	/* integer */
/* #define unavailable due to collision 219	*/ /* ??????? */
#define PW_TIME_USED			220	/* integer */
#define PW_HUNTGROUP_NAME		221	/* string */
#define PW_USER_ID			222	/* string */
#define PW_USER_REALM			223	/* string */

/*	SERVICE TYPES	*/

#define PW_LOGIN			1
#define PW_FRAMED			2
#define PW_CALLBACK_LOGIN		3
#define PW_CALLBACK_FRAMED		4
#define PW_OUTBOUND_USER		5
#define PW_ADMINISTRATIVE_USER		6
#define PW_SHELL_USER			7
#define PW_AUTHENTICATE_ONLY		8
#define PW_CALLBACK_ADMIN_USER		9

/*	FRAMED PROTOCOLS	*/

#define PW_PPP				1
#define PW_SLIP 			2

/*	FRAMED ROUTING VALUES	*/

#define PW_NONE 			0
#define PW_BROADCAST			1
#define PW_LISTEN			2
#define PW_BROADCAST_LISTEN		3

/*	FRAMED COMPRESSION TYPES	*/

#define PW_VAN_JACOBSON_TCP_IP		1
#define PW_IPX_HEADER_COMPRESSION	2

/*	LOGIN SERVICES	*/

#define PW_TELNET			0
#define PW_RLOGIN			1
#define PW_TCP_CLEAR			2
#define PW_PORTMASTER			3
#define PW_LAT				4

/*	TERMINATION ACTIONS	*/

#define PW_DEFAULT			0
#define PW_RADIUS_REQUEST		1

/*	ACCOUNTING STATUS TYPES    */

#define PW_STATUS_START 	1
#define PW_STATUS_STOP		2
#define PW_STATUS_ALIVE 	3
#define PW_STATUS_MODEM_START	4
#define PW_STATUS_MODEM_STOP	5
#define PW_STATUS_CANCEL	6
#define PW_ACCOUNTING_ON	7
#define PW_ACCOUNTING_OFF	8

/*	ACCOUNTING TERMINATION CAUSES	 */

#define PW_USER_REQUEST 	1
#define PW_LOST_CARRIER 	2
#define PW_LOST_SERVICE 	3
#define PW_ACCT_IDLE_TIMEOUT	4
#define PW_ACCT_SESSION_TIMEOUT 5
#define PW_ADMIN_RESET		6
#define PW_ADMIN_REBOOT 	7
#define PW_PORT_ERROR		8
#define PW_NAS_ERROR		9
#define PW_NAS_REQUEST		10
#define PW_NAS_REBOOT		11
#define PW_PORT_UNNEEDED	12
#define PW_PORT_PREEMPTED	13
#define PW_PORT_SUSPENDED	14
#define PW_SERVICE_UNAVAILABLE	15
#define PW_CALLBACK		16

/*	NAS PORT TYPES	  */

#define PW_ASYNC		0
#define PW_SYNC 		1
#define PW_ISDN_SYNC		2
#define PW_ISDN_SYNC_V120	3
#define PW_ISDN_SYNC_V110	4

void		radius(u_long arg);
void		radius_clear(int port);
void		radius_login(int port, int (*func)(int, int), int mode);
void		radius_logout(int port, int (*func)(int, int));
void		radius_up(void);
void		radius_down(void);
int			radius_down_ok(void);
void		radius_terminate_cause(int port, int cause);

#endif
