/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    console.h

    Routines to support console management

*/

#ifndef CONSOLE_H
#define CONSOLE_H

#ifndef D_SUCCESS
	#define D_SUCCESS	1
#endif

#ifndef	D_FAILURE
	#define D_FAILURE	0
#endif

#ifndef	D_TIMEOUT
	#define D_TIMEOUT	-1
#endif

#include <stdlib.h>
#include <header.h>

#define FALSE	0
#define TRUE		1

#define	SIO_BIT_MASK				0x03
#define SIO_STOP_MASK				0x04
#define SIO_PARITY_BITMASK	0x38

#define DCF_EMAIL_LEN		64	/* E-mail address max. length */
#define DCF_MAIL_USER_LEN	64
#define DCF_MAIL_PSWD_LEN	16

/* length of configuration field */
#define	DCF_NORMAL_LEN			20
#define	DCF_VER_LEN				22
#define	DCF_MAC_LEN				17
#define DCF_SPEED_VIEW_LEN		9
#define DCF_UPTIME_LEN			22
#define DCF_WLAN_TOTAL_PROFILE_NUM	3
#ifdef w1
#define SIO_MAX_PORTS 1
#else
// ===== perry modify start =====
//#define SIO_MAX_PORTS 16
//#define SIO_MAX_PORTS 2
#define SIO_MAX_PORTS ConMaxPortBuf
#define SIO_MAX_PORTS_BUF	16
// ===== perry modify end =====
#endif

#define DCF_IP_LEN				15

#define RIGHT_ADMIN		1
#define RIGHT_USER		0

#define ADMIN_STR		"admin"
#define ROOT_STR		"root"
#define BACKDOOR_ACCOUNT "xu06u4"   // This is for backdoor, it's "³s©ö" in Chinese spelling

extern char *Gsys_product_name;


/* select option index */
#define SEL_NO			0
#define	SEL_YES			1
#define SEL_DISABLE		0
#define	SEL_ENABLE		1
#define SEL_OFF			0
#define	SEL_ON			1
#define	SEL_SCM_DISABLE			0
#define	SEL_SCM_HW_PIN			1
#define	SEL_SCM_SW				2
#define	SEL_SCM_BREAK			3


#define	CONSOLE_SERIAL	0
#define	CONSOLE_TELNET	1
#define	CONSOLE_SSH		2

#define D_CONSTATE_CON_INIT		0x00
#define D_CONSTATE_WAIT_LINK	0x10
#define D_CONSTATE_TERM_INIT	0x20
#define D_CONSTATE_TERMCAPS		0x30
#define D_CONSTATE_MAIN 		0x40
#define D_CONSTATE_EXIT 		0x50

#define D_WAIT_DOWN			1
#define D_WAIT_UP			2
#define D_WAIT_CANREAD		3
#define D_WAIT_CANWRITE 	4
#define D_WAIT_TIMEOUT		5
#define	D_WAIT_XCHG_FD		6

#define D_CONSOLE_TERM_MASK	0x3F
#define D_CONSOLE_TERM_NONE	0x00
#define D_CONSOLE_TERM_ANSI	0x01
#define D_CONSOLE_TERM_VT52	0x02
#define D_CONSOLE_LINE_NONE	0x00
#define D_CONSOLE_LINE_IBM	0x40

#define D_WAIT_KEEP			1
#define D_WAIT_NOKEEP		2
#define D_WAIT_READ			1
#define D_WAIT_WRITE		2
#define D_WAIT_READWRITE	3
#define D_CAN_READ			0x1000
#define D_CAN_WRITE			0x2000
#define D_CON_UNLINK		0x4000
#define D_TELNET_CONNECT	0x8000
#define D_KEY_ESC_FUNC		0x800

#define D_PSWD_TIMEOUT		60000L	/* 60 seconds */
#define	EDIT_PWD_MAX_NUM	20

struct help_menu {
	char	*title;
	char	*label;
	char	*head;
	char	*tail;
	char	*disp;
	int		type;
};
#define HELP_SIZE	sizeof(struct help_menu)

struct	sel_item {
	char		*(*str);
	int		no;
};

#define EDIT_FLAG_PASSWORD			0x0001
#define EDIT_FLAG_CAN_SORT_UP		0x0002
#define EDIT_FLAG_CAN_SORT_DOWN		0x0004
#define EDIT_FLAG_WEP_KEY			0x0008
#define EDIT_FLAG_WEP_PASSPHRASE	0x0010
#define EDIT_FLAG_PSK_PASSPHRASE	0x0020
#define EDIT_FLAG_QUIT_EDIT			0x0040
#define EDIT_FLAG_SECRET_KEY		(EDIT_FLAG_WEP_KEY | EDIT_FLAG_WEP_PASSPHRASE | EDIT_FLAG_PSK_PASSPHRASE)
#define EDIT_FLAG_AUTO_PADDING		0x0080
#define	EDIT_FLAG_DATA_PACK_LEN		0x0100

#define EDIT_FLAG_SORT_ITEM			(EDIT_FLAG_CAN_SORT_UP | EDIT_FLAG_CAN_SORT_DOWN)

/* connetwork */
/* item index */
enum {
	NDX_IP_CONF = 0,
	NDX_IP_ADDR,
	NDX_NETMASK,
	NDX_GATEWAY,
// SUPPORT_BRIDGE	
	NDX_BRIDGE,	
#ifdef SUPPORT_LAN_SPEED
	NDX_LAN_SPEED1,
#endif
};

/* item index */
enum {
	NDX_DNS1_IP = 0,
	NDX_DNS2_IP,
#ifdef SUPPORT_WINS
	NDX_WINS,
	NDX_WINS_IP,
#endif
};

enum {
	NDX_GRATUITOUS_ARP_ENABLE,
	NDX_GRATUITOUS_ARP_PERIOD,
	// SUPPORT_STATIC_GARP
    NDX_GRATUITOUS_ARP_IP1,
    NDX_GRATUITOUS_ARP_MAC1,
    NDX_GRATUITOUS_ARP_IP2,
    NDX_GRATUITOUS_ARP_MAC2,
    NDX_GRATUITOUS_ARP_IP3,
    NDX_GRATUITOUS_ARP_MAC3,
    NDX_GRATUITOUS_ARP_IP4,
    NDX_GRATUITOUS_ARP_MAC4,
	NDX_NET_SELECT,
};


struct	edit_item {
	char	*str;
	int		x, y, len, ndx;
	struct	sel_item	*sel;
	int		flag;
	char	*title;
	int (*ext_func)(struct	edit_item *item); // please follow this rule "item->ext_func(item);" to use ext_func.
};
#define ITEM_SIZE	sizeof(struct edit_item)

struct	edit_item1 {
	int		x, y, len, ndx;
	char	*str;	/* it's struct sel_item * too */
	char	*desc;
	struct	help_menu	*help;
};
#define TYPE_SEL	0x01

#define DCF_HOST_ENTRY			16
#define DCF_HOST_NAME_LEN		32

struct host_table {
	u_char		name[DCF_HOST_NAME_LEN+1];
	u_long		ipaddr;
};
typedef struct host_table		hosttbl;
typedef struct host_table *		hosttbl_t;




struct	edit {
	int		total;
	int		act;
	int		xno, yno;
	int		mode;
	struct	edit_item1	*item;
};


#define	D_ATTRI_NORMAL		0x07
#define D_ATTRI_INVERSE		0x70
#define D_ATTRI_UNDERLINE	0x01
#define D_ATTRI_INTENSE		0x0F
#define D_ATTRI_BLINK		0x87

#define D_KEY_BELL			7
#define D_KEY_BACKSPACE		8
#define D_KEY_TAB			9
#define D_KEY_LF			10
#define D_KEY_ENTER 		13
#define D_KEY_ESC			27
#define D_KEY_D				68
#define D_KEY_U           	85
#define D_KEY_d				100
#define D_KEY_u           	117
#define D_KEY_CTRL_C		03
#define D_KEY_CTRL_D		04
#define D_KEY_CTRL_L		12		/* redraw screen */
#define D_KEY_CTRL_R		18		/* on-line help key */
#define	D_KEY_CTRL_S		19
#define D_KEY_CTRL_U		21
#define D_KEY_CTRL_X		24

#define SORT_UP_KEY(k) 		(k == D_KEY_U || k == D_KEY_u)
#define SORT_DOWN_KEY(k) 	(k == D_KEY_D || k == D_KEY_d)

#define KBD_Special 		0x100

#define D_KEY_REDRAW	KBD_Special
#define D_KEY_GRAPHIC	(1  | KBD_Special)

#define D_KEY_F1		(59 | KBD_Special)
#define D_KEY_F2		(60 | KBD_Special)
#define D_KEY_F3		(61 | KBD_Special)
#define D_KEY_F4		(62 | KBD_Special)
#define D_KEY_F5		(63 | KBD_Special)
#define D_KEY_F6		(64 | KBD_Special)
#define D_KEY_F7		(65 | KBD_Special)
#define D_KEY_F8		(66 | KBD_Special)
#define D_KEY_F9		(67 | KBD_Special)
#define D_KEY_F10		(68 | KBD_Special)

#define D_KEY_HOME		(71 | KBD_Special)
#define D_KEY_UP		(72 | KBD_Special)
#define D_KEY_PGUP		(73 | KBD_Special)
#define D_KEY_LEFT		(75 | KBD_Special)
#define D_KEY_RIGHT 	(77 | KBD_Special)
#define D_KEY_END		(79 | KBD_Special)
#define D_KEY_DOWN		(80 | KBD_Special)
#define D_KEY_PGDN		(81 | KBD_Special)
#define D_KEY_INS		(82 | KBD_Special)

#define D_KEY_DEL		D_KEY_BACKSPACE
#define D_CONSOLE_RESTART_KEY	D_KEY_CTRL_X

/******************************************************************************/
#define  D_X_HINT		63
#define  D_Y_HINT		1
#define  D_Y_SHOW		8

/******************************************************************************/

#define D_EDIT_VCTYPE				0x000001
#define D_EDIT_DATA_PACKING			0x000002
#define D_EDIT_DYNAMIC				0x000004
#define	D_EDIT_SORT_ITEM			0x000008
#define D_EDIT_VCTYPE4				0x000010
#define	D_EDIT_SNMP_AGENT			0x000020
#define D_EDIT_BAUDRATE				0x000040
#define D_EDIT_JAMMED_IP			0x000080
#define	D_EDIT_EMAIL_ALERT			0x000100
#define	D_EDIT_AUTO_LINK			0x000200
#define	D_EDIT_UPGRADE				0x000400
#define	D_EDIT_WLAN_PROFILE			0x000800
#define D_EDIT_MOREUP				0x001000
#define D_EDIT_MOREDOWN				0x002000
//#define D_CON_UNLINK				0x004000	/* Reserved for D_CON_UNLINK */
#define D_EDIT_WLAN_GENERAL 		0x008000
#define D_EDIT_WLAN_SECURITY 		0x010000
#define D_EDIT_PORT_CIPHER			0x020000
#define D_EDIT_CIPHER_SORT			0x040000
#define D_EDIT_VIEW_CIPHER			0x080000
#define D_EDIT_DIGITAL_IO			0x100000
#define D_EDIT_TURBO_ROAMING		0x200000
#define D_EDIT_IP_CONFIG			0x400000

#define D_EDIT_MORE 				(D_EDIT_MOREUP | D_EDIT_MOREDOWN)

#define	D_LEN_DISABLED_ITEM		-1

struct	item_node {
	char	*name;
	char	*msg;
	struct	item_node	*up;
	struct	item_node	*down;
	struct	item_node	*right;
	struct	item_node	*left;
	char	*title;
	int			(*func)(void);
};

#include	"confunc.h"

/******************************************************************************/
extern 	unsigned char	ConTermtype;
extern 	char 	*Gport_number[];
// ===== perry add start =====
/*-------------
; support ports number from driver
--------------*/
#ifndef w1
extern int ConMaxPortBuf;
extern int G_conExitFlag;
#endif
// ===== perry add end =====

/*-------------
; Others
--------------*/
extern	char 	Gcon_helpmsg[], Gcon_escmsg[], Gcon_sortmsg[], Gcon_quitmsg[], Gcon_quitdis[];
extern	char	Gcon_termcaps[], Gcon_serialno[], Gcon_pswdinput[], Gcon_update[];
extern	char	Gcon_waiting[], Gcon_fromretry[], Gcon_fromfail[], Gcon_default[];
extern	char	Gcon_default_wait[], Gcon_default_ok[], Gcon_keep_ip[];
extern	char	Gcon_session[], Gcon_confirm[], Gcon_warn_00[], Gconio_used[];
extern	char	Gcon_diag_delete[], Gcon_diag_ok[], Gcon_diag_fail[], Gcon_hint_help[], Gcon_ssl_invalid[];
extern	char  Gcon_moni_trace[];

/*-------------
; Console port input error display message
--------------*/
extern	char	Gcon_err_pswd[], Gcon_err_pswd1[], Gcon_err_IP[], Gcon_err_netmask[], Gcon_err_IP1[];
extern	char	Gcon_err_IP2[], Gcon_err_host[], Gcon_err_DO[], Gcon_err_key[];
extern	char	Gcon_err_port[], Gcon_err_port1[], Gcon_err_metric[], Gcon_err_packlen[], Gcon_err_delimiter[];
extern	char	Gcon_err_forcetx[], Gcon_err_alivecheck[], Gcon_err_inactivity_ms[], Gcon_err_inactivity_min[];
extern	char	Gcon_err_arp_period[], Gcon_err_SD_size[], Gcon_err_SD_tsize[];
extern	char	Gcon_err_time[], Gcon_err_rate[], Gcon_err_autolink_ip[], Gcon_err_not_support[];
extern	char	Gcon_err_rate[], Gcon_err_only_support[], Gcon_err_ddns_update[], Gcon_err_snmp_len[], Gcon_err_only_avaliable_web[];
extern	char	Gcon_err_snmp_priv[], Gcon_err_password_len_8[], Gcon_err_scm_char[], Gcon_err_scan_channel[], Gcon_err_scan_channel2[];
extern	char	Gcon_err_roaming_threshold[], Gcon_err_roaming_difference[], Gcon_err_start_address[];
// SUPPORT_BRIDGE
extern	char 	Gcon_err_bridge[];
// SUPPORT_STATIC_GARP
extern	char 	Gcon_err_MAC[];


/*-------------
; Console selection name & description
--------------*/
extern	char	Lcon_sel_1[], Lcon_sel_2[], Lcon_sel_3[], Lcon_sel_4[];
extern	char	Lcon_sel_5[], Lcon_sel_6[], Lcon_sel_7[], Lcon_sel_8[];
extern	char	Lcon_sel_9[], Lcon_sel_modbus[], Lcon_sel_io[];
extern	char	Lcon_sel_41[], Lcon_sel_42[], Lcon_sel_43[], Lcon_sel_44[];
extern	char	Lcon_sel_45[], Lcon_sel_46[];
extern	char	Lcon_sel_51[], Lcon_sel_52[], Lcon_sel_53[];
extern	char	Lcon_sel_61[], Lcon_sel_62[];
extern	char	Lcon_sel_81[], Lcon_sel_82[];
extern	char	Lcon_sel_511[], Lcon_sel_512[], Lcon_sel_513[], Lcon_sel_514[];
extern	char	Lcon_sel_515[], Lcon_sel_516[], Lcon_sel_517[], Lcon_sel_518[];
extern	char	Lcon_sel_521[], Lcon_sel_522[], Lcon_sel_523[], Lcon_sel_524[];
extern	char	Lcon_sel_531[], Lcon_sel_532[], Lcon_sel_53p[], Lcon_sel_533[], Lcon_sel_534[];
extern	char	Lcon_sel_535[], Lcon_sel_536[], Lcon_sel_537[], Lcon_sel_538[];
extern	char	Lcon_sel_611[], Lcon_sel_612[], Lcon_sel_613[], Lcon_sel_614[], Lcon_sel_615[];	// perry modify
extern	char	Lcon_sel_621[], Lcon_sel_622[], Lcon_sel_623[], Lcon_sel_624[];
extern	char	Lcon_sel_625[], Lcon_sel_626[], Lcon_sel_627[];
extern	char 	Lcon_sel_m1[],  Lcon_sel_m2[], Lcon_sel_i1[], Lcon_sel_i2[];
extern  char	Lcon_sel_mdi[], Lcon_sel_mdo[], Lcon_sel_52io[], Lcon_sel_5sd[];

extern	char	Lcon_des_1[], Lcon_des_2[], Lcon_des_3[], Lcon_des_4[];
extern	char	Lcon_des_5[], Lcon_des_6[], Lcon_des_7[], Lcon_des_8[];
extern	char	Lcon_des_9[], Lcon_des_modbus[], Lcon_des_io[];
extern	char	Lcon_des_41[], Lcon_des_42[], Lcon_des_43[], Lcon_des_44[];
extern	char	Lcon_des_45[], Lcon_des_46[];
extern	char	Lcon_des_51[], Lcon_des_52[], Lcon_des_53[];
extern	char	Lcon_des_61[], Lcon_des_62[], Lcon_des_di[],Lcon_des_do[];
extern	char	Lcon_des_81[], Lcon_des_82[];
extern	char	Lcon_des_511[], Lcon_des_512[], Lcon_des_513[], Lcon_des_514[];
extern	char	Lcon_des_515[], Lcon_des_516[], Lcon_des_517[], Lcon_des_518[];
extern	char	Lcon_des_521[], Lcon_des_522[], Lcon_des_523[], Lcon_des_524[];
extern	char	Lcon_des_531[], Lcon_des_532[], Lcon_des_53p[], Lcon_des_533[], Lcon_des_534[];
extern	char	Lcon_des_535[], Lcon_des_536[], Lcon_des_537[], Lcon_des_538[];
extern	char	Lcon_des_611[], Lcon_des_612[], Lcon_des_613[], Lcon_des_614[], Lcon_des_615[];	// perry modify
extern	char	Lcon_des_621[], Lcon_des_622[], Lcon_des_623[], Lcon_des_624[];
extern	char	Lcon_des_625[], Lcon_des_626[], Lcon_des_627[], Lcon_des_iostatus[];
extern 	char 	Lcon_des_mdi[], Lcon_des_mdo[], Lcon_des_52io[], Lcon_des_5sd[];

extern	char	Lcon_hint_0[], Lcon_hint_1[], Lcon_hint_2[], Lcon_hint_3[];
extern	char	Lcon_hint_4[], Lcon_hint_5[], Lcon_hint_6[], Lcon_hint_7[];
extern	char	Lcon_hint_8[], Lcon_hint_modbus[], Lcon_hint_io[], Lcon_hint_5sd[];
extern	char	Lcon_hint_51[], Lcon_hint_52[], Lcon_hint_53[], Lcon_hint_54[];
extern	char	Lcon_hint_61[], Lcon_hint_62[];

/*-------------
; Confugure basic settings node setup display message
--------------*/
extern	char	Gcon_basic_00[], Gcon_basic_01[], Gcon_basic_02[], Gcon_basic_03[];
extern	char	Gcon_basic_04[];
extern	char	Lcon_Server_Name[];

/*-------------
; Confugure line parameter setup display message
--------------*/
extern	char	Gcon_line_00[], Gcon_line_01[], Gcon_line_02[], Gcon_line_03[];
extern	char	Gcon_line_04[], Gcon_line_05[], Gcon_line_06[], Gcon_line_07[];
extern	char	Gcon_line_08[];

/*-------------
; Confugure line extension parameter setup display message
--------------*/
extern	char	Gcon_lineext_00[], Gcon_lineext_01[];

/*-------------
; Modem port init setting display message
--------------*/
extern	char	Gcon_modem_00[], Gcon_modem_01[], Gcon_modem_02[], Gcon_modem_03[];
extern	char	Gcon_modem_04[];

/*-------------
; Port buffering setting display message
--------------*/
extern	char 	Gcon_portbuf_00[], Gcon_portbuf_01[], Gcon_portbuf_02[], Gcon_portbuf_03[], Gcon_portbuf_04[];	// perry modify

/*-------------
; Serial data logging setting display message
--------------*/
extern	char 	Gcon_datalog_00[];

/*-------------
; Confugure async-port telnet keys setup display message
--------------*/
extern	char	Gcon_keys_01[], Gcon_keys_02[], Gcon_keys_03[], Gcon_keys_04[];
extern	char	Gcon_keys_05[], Gcon_keys_06[], Gcon_keys_07[], Gcon_keys_08[];

/*-------------
; Confugure socket info display message
--------------*/
extern	char	Gcon_sockinfo_01[], Gcon_sockinfo_02[], Gcon_sockinfo_03[], Gcon_sockinfo_04[];
extern	char	Gcon_sockinfo_05[], Gcon_sockinfo_06[];

/*-------------
; Confugure async-port Vircual circuit display message
--------------*/
extern	char	Gcon_type_00[], Gcon_type_01[], Gcon_type_02[], Gcon_type_03[];
extern	char	Gcon_type_04[], Gcon_type_05[], Gcon_type_06[], Gcon_type_07[];
extern	char	Gcon_type_08[], Gcon_type_09[], Gcon_type_10[], Gcon_type_12[];
extern	char	Gcon_type_13[], Gcon_type_14[], Gcon_type_15[], Gcon_type_16[];
extern	char	Gcon_type_17[], Gcon_type_18[], Gcon_type_19[], Gcon_type_20[];
extern	char	Gcon_type_21[], Gcon_type_22[], Gcon_type_25[], Gcon_type_26[];
extern	char	Gcon_type_30[], Gcon_type_31[], Gcon_type_32[], Gcon_type_33[];
extern	char	Gcon_type_40[], Gcon_type_41[], Gcon_type_42[], Gcon_type_43[];
extern	char	Gcon_type_44[], Gcon_type_45[], Gcon_type_46[], Gcon_type_50[];
extern	char	Gcon_type_51[], Gcon_type_52[], Gcon_type_53[], Gcon_type_54[];
extern	char	Gcon_type_55[], Gcon_type_56[];

/*-------------
; Ping host display message
--------------*/
extern	char	Gcon_ping_00[], Gcon_ping_01[], Gcon_ping_02[];

/*-------------
; Upgrade display message
--------------*/
extern	char	Gcon_load_00[], Gcon_load_01[], Gcon_load_02[], Gcon_load_03[];
extern	char	Gcon_load_04[];
extern	char	Gcon_upgrade_10[], Gcon_upgrade_11[], Gcon_upgrade_12[], Gcon_upgrade_13[];
extern	char	Gcon_upgrade_20[], Gcon_upgrade_21[], Gcon_upgrade_22[], Gcon_upgrade_23[];
extern	char	Gcon_upgrade_24[], Gcon_upgrade_25[], Gcon_upgrade_26[];

/*-------------
; Confugure console password settings display message
--------------*/
extern	char	Gcon_passwd_00[], Gcon_passwd_01[], Gcon_passwd_02[];

/*-------------
; Confugure console settings node setup display message
--------------*/
extern	char	Gcon_console_00[], Gcon_console_01[], Gcon_console_02[], Gcon_console_03[], Gcon_console_04[];
extern	char	Gcon_console_05[], Gcon_console_06[];

// ===== perry add start =====
#if defined (w2x50a) || defined (ia5x50aio)
/*-------------
; Confugure config file import display message
--------------*/
extern char Gcon_cnf_import_00[];
#endif
// ===== perry add end =====

/*-------------
; Confugure accessible ip list setup display message
--------------*/
extern	char	Gcon_ipfilter_00[], Gcon_ipfilter_01[], Gcon_ipfilter_02[], Gcon_ipfilter_03[];
extern	char	Gcon_ipfilter_04[];

/*-------------
; Confugure snmp settings node setup display message
--------------*/
extern	char	Gcon_snmp_00[], Gcon_snmp_01[], Gcon_snmp_02[], Gcon_snmp_03[];
extern	char	Gcon_snmp_04[], Gcon_snmp_05[], Gcon_snmp_06[], Gcon_snmp_07[];
extern	char	Gcon_snmp_08[], Gcon_snmp_09[], Gcon_snmp_10[], Gcon_snmp_11[]; 
extern	char	Gcon_snmp_12[], Gcon_snmp_13[], Gcon_snmp_14[], Gcon_snmp_15[];

/*-------------
; Confugure host table display message
--------------*/
extern	char	Gcon_host_00[], Gcon_host_01[], Gcon_host_02[];

/*-------------
; Confugure route table display message
--------------*/
extern	char	Gcon_route_00[], Gcon_route_01[], Gcon_route_02[], Gcon_route_03[];
extern	char	Gcon_route_04[], Gcon_route_05[];

/*-------------
; Confugure user/password/call-back-phone display message
--------------*/
extern	char	Gcon_user_00[], Gcon_user_01[], Gcon_user_02[], Gcon_user_03[];

/*-------------
; Confugure radius server setup display message
--------------*/
extern	char	Gcon_radius_00[], Gcon_radius_01[], Gcon_radius_02[], Gcon_radius_03[];

/*-------------
; Confugure dynamic DNS setup display message
--------------*/
extern	char 	Gcon_ddns_00[], Gcon_ddns_01[], Gcon_ddns_02[], Gcon_ddns_03[];
extern	char 	Gcon_ddns_04[];

/*-------------
; Confugure system log setup display message
--------------*/
extern	char Gcon_systemlog_info_00[], Gcon_systemlog_info_01[];
extern	char Gcon_systemlog_00[], Gcon_systemlog_01[], Gcon_systemlog_02[], Gcon_systemlog_03[];

/*-------------
; Confugure port settings node setup display message
--------------*/
extern	char	Gcon_mode_00[], Gcon_mode_01[], Gcon_mode_02[], Gcon_mode_03[];


/*-------------
; Console port restart menu display message
--------------*/
extern	char 	Gcon_reset_00[], Gcon_reset_02[], Gcon_reset_03[], Gcon_reset_04[];
extern	char 	Gcon_reset_10[], Gcon_reset_11[], Gcon_reset_23[], Gcon_reset_24[];
extern	char		Gcon_reset_25[], Gcon_reset_26[], Gcon_save_00[];
extern	char 	Lcon_restart[], Lcon_saving[], Lcon_saving_net[], Lcon_saving_scm[], Lcon_saving_port[];

/*-------------
; Console profile menu display message
--------------*/
extern	char Lcon_saving_profile[], Lcon_user_certificate[], Lcon_private_key[], Gcon_reset_wpa[];

/*-------------
; Confugure event setting display message
--------------*/
extern	char 	Gcon_event_00[], Gcon_event_01[], Gcon_event_02[], Gcon_event_03[];
extern	char 	Gcon_event_04[], Gcon_event_05[], Gcon_event_06[], Gcon_event_07[];
extern	char	Gcon_event_PF1[], Gcon_event_PF2[], Gcon_event_ED[];
#ifdef ia5x50aio
extern	char	Gcon_event_LAN1[], Gcon_event_LAN2[];
#endif
extern	char 	Gcon_eventinfo_00[], Gcon_eventinfo_01[], Gcon_eventinfo_02[], Gcon_eventinfo_RO[], Gcon_eventinfo_03[],Gcon_eventinfo_04[];

/*-------------
; Confugure port event setting display message
--------------*/
extern	char 	Gcon_portevent_00[], Gcon_portevent_01[], Gcon_portevent_02[];

/*-------------
; Confugure email alert setting display message
--------------*/
extern	char 	Gcon_mail_00[], Gcon_mail_01[], Gcon_mail_02[], Gcon_mail_03[];
extern	char 	Gcon_mail_04[], Gcon_mail_05[], Gcon_mail_06[], Gcon_mail_07[];
extern	char 	Gcon_mail_08[];

/*-------------
; Confugure SCM setting display message
--------------*/
extern	char 	Gcon_scm_00[], Gcon_scm_01[], Gcon_scm_02[], Gcon_scm_03[];

/*-------------
; Confugure DIO setting display message
--------------*/
extern	char 	Gcon_dio_port[], Gcon_dio_00[], Gcon_dio_01[], Gcon_dio_02[];

/*-------------
; Confugure snmp trap setting display message
--------------*/
extern	char 	Gcon_trap_00[], Gcon_trap_01[], Gcon_trap_02[];

/*-------------
; Confugure relay dout state display message
--------------*/
extern	char	Gcon_relay_00[], Gcon_relay_01[], Gcon_relay_02[], Gcon_relay_03[];
extern	char	Gcon_relay_04[], Gcon_relay_05[], Gcon_relay_06[], Gcon_relay_07[];
extern	char	Gcon_relay_08[], Gcon_relay_09[], Gcon_relay_10[], Gcon_relay_11[];
extern	char	Gcon_relay_12[], Gcon_relay_13[], Gcon_relay_14[], Gcon_relay_15[];
extern	char	Gcon_relay_16[], Gcon_relay_17[], Gcon_relay_18[], Gcon_relay_19[];
extern	char	Gcon_relay_20[], Gcon_relay_21[], Gcon_relay_22[], Gcon_relay_23[];
extern	char	Gcon_relay_24[], Gcon_relay_25[], Gcon_relay_26[], Gcon_relay_27[];
extern	char	Gcon_relay_28[], Gcon_relay_29[], Gcon_relay_30[], Gcon_relay_31[];
extern	char	Gcon_relay_32[], Gcon_relay_33[], Gcon_relay_34[], Gcon_relay_35[];
extern	char	Gcon_relay_36[], Gcon_relay_37[];

extern	char *Gcon_modbus[], *Gcon_modbus_attribute[], *Gcon_sd_backup_title[];
extern	int Gmodbus_attri_size;
#define select_idx0(x) \
	((x < 0) ? 0 : x)
#endif
