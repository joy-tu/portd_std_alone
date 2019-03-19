#include "net_config.h"

#ifndef __SYSMAPS_H__
#define __SYSMAPS_H__

struct __config_map {
    const int index;
    const char* name;
};
typedef struct __config_map _config_map;
typedef struct __config_map * _config_map_t;

/* ethernet speed */
#define DNET_SPEED_AUTO		111  /* Ethernet link Auto */
#define DNET_SPEED_UNLINK		0	/* Ethernet un-link */
#define DNET_SPEED_10_HALF	10	/* Ethernet link 10M, Half Duplex */
#define DNET_SPEED_10_FULL		11	/* Ethernet link 10M, Full Duplex */
#define DNET_SPEED_100_HALF	100	/* Ethernet link 100M, Half Duplex */
#define DNET_SPEED_100_FULL	101	/* Ethernet link 100M, Full Duplex */

/* mode */
enum wlan_mode{
    DCF_WLAN_MODE_INFRA=0,   /* infrastructure */
    DCF_WLAN_MODE_IBSS,     /* IBSS (ad-hoc) */
};

/* proto */
enum wlan_proto {
    DCF_WLAN_PROTO_WPA=1,
    DCF_WLAN_PROTO_RSN,
};

/* key mgmt */
enum key_mgmt{
    DCF_WLAN_KEYMGMT_WPA_PSK=1,
    DCF_WLAN_KEYMGMT_WPA_EAP,
    DCF_WLAN_KEYMGMT_IEEE8021X,
    DCF_WLAN_KEYMGMT_NONE,
    DCF_WLAN_KEYMGMT_WPA_PSK_SHA256,
    DCF_WLAN_KEYMGMT_WPA_EAP_SHA256,
};

/* auth_alg */
enum wlan_auth_alg {
    DCF_WPA_AUTH_ALG_OPEN=1,
    DCF_WPA_AUTH_ALG_SHARED,
    DCF_WPA_AUTH_ALG_LEAP,
};

/* pair wise */
enum wlan_pairwise {
    DCF_WLAN_PAIRWISE_CCMP=1,
    DCF_WLAN_PAIRWISE_TKIP,
};

/* group */
enum wlan_group {
    DCF_WLAN_GROUP_CCMP=1,
    DCF_WLAN_GROUP_TKIP,
    DCF_WLAN_GROUP_WEP104,
    DCF_WLAN_GROUP_WPE40,
    DCF_WLAN_GROUP_CCMP_TKIP,
};

/* eap */
enum wlan_eap {
    DCF_WLAN_EAP_MD5=1,
    DCF_WLAN_EAP_MSCHAPV2,
    DCF_WLAN_EAP_OPT,
    DCF_WLAN_EAP_GTC,
    DCF_WLAN_EAP_TLS,
    DCF_WLAN_EAP_PEAP,
    DCF_WLAN_EAP_TTLS,
    DCF_WLAN_EAP_LEAP,
};

/* phase1 */
enum wlan_phase1 {
    DCF_WLAN_PAHSE1_PEAPV0=1,
    DCF_WLAN_PAHSE1_PEAPV1,
    DCF_WLAN_PAHSE1_PEAPLABLE,
};

/* phase2 */
enum wlan_phase2 {
    DCF_WLAN_PAHSE2_GTC=1,
    DCF_WLAN_PAHSE2_MD5,
    DCF_WLAN_PAHSE2_MSCHAPV2,
    DCF_WLAN_PAHSE2_TTLS_PAP,
    DCF_WLAN_PAHSE2_TTLS_CHAP,
    DCF_WLAN_PAHSE2_TTLS_MSCHAP,
    DCF_WLAN_PAHSE2_TTLS_MSCHAPV2,
    DCF_WLAN_PAHSE2_EAP_GTC,
    DCF_WLAN_PAHSE2_EAP_MD5,
    DCF_WLAN_PAHSE2_EAP_MSCHAPV2,
};

/* reconnection rule */
enum _reconnRule {
    D_SIGNAL_STRENGTH_AP=0,
    D_PRIORITY_SEQUENTIAL,
    D_FIXED_FIRST,
};

enum _databit_string {
    D_DATA_BITS_5=1,
    D_DATA_BITS_6,
    D_DATA_BITS_7,
    D_DATA_BITS_8
};

enum _parity_string {
    D_PARITY_NONE=1,
    D_PARITY_ODD,
    D_PARITY_EVEN,
    D_PARITY_MARK,
    D_PARITY_SPACE
};

/* TCP Client mode */
#define DCF_CLI_MODE_ON_STARTUP     0x0001
#define DCF_CLI_MODE_ON_ANYCHAR     0x0002
#define DCF_CLI_MODE_ON_DSRON       0x0004
#define DCF_CLI_MODE_ON_DCDON       0x0008
#define DCF_CLI_MODE_ON_MASK        0x00FF

#define DCF_CLI_MODE_OFF_NONE       0x0100
#define DCF_CLI_MODE_OFF_INACT      0x0200
#define DCF_CLI_MODE_OFF_DSROFF     0x0400
#define DCF_CLI_MODE_OFF_DCDOFF     0x0800
#define DCF_CLI_MODE_OFF_MASK       0xFF00

/* SNMP */
#define DCF_SNMP_VERSION_V1     0x01
#define DCF_SNMP_VERSION_V2C    0x02
#define DCF_SNMP_VERSION_V3     0x04

#define DCF_SNMP_AUTH_DISABLE   0
#define DCF_SNMP_AUTH_MD5       1
#define DCF_SNMP_AUTH_SHA       2

#define DCF_SNMP_PRIV_DISABLE   0
#define DCF_SNMP_PRIV_DES       1
#define DCF_SNMP_PRIV_AES       2

#define DCF_SCMDFLAG_DISABLE	0
#define DCF_SCMDFLAG_HW_TRIG	1
#define DCF_SCMDFLAG_SW_TRIG	2
#define DCF_SCMDFLAG_BREAK		3

/* ssl cert file index */
enum _cert_fname_index {
    D_SSL_ETH_CERT=1,
    D_SSL_WLAN_CERT,
    D_SSL_WPA_SERVER_CERT,
    D_SSL_WPA_USER_CERT,
    D_SSL_WPA_USER_KEY,
};

#if 0
enum
{
    TCP_ESTABLISHED = 1,
    TCP_SYN_SENT,
    TCP_SYN_RECV,
    TCP_FIN_WAIT1,
    TCP_FIN_WAIT2,
    TCP_TIME_WAIT,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
    TCP_LISTEN,
    TCP_CLOSING,         /* now a valid state */
};
#endif 

enum {
    SYS_NETSTAT_TCP=0,
    SYS_NETSTAT_UDP,
    SYS_NETSTAT_RAW,
    SYS_NETSTAT_UNIX,
};

char* search_map_by_index(int idx, _config_map_t map);
int search_map_by_name(char* name, _config_map_t map);
int configmap_size(_config_map_t map);
int configmap_index(_config_map_t map, int id, char *str);
_config_map_t sys_get_adhoc_map(int opmode);

extern _config_map map_wlan_mode[];
extern _config_map map_wlan_a_channel_adhoc_us[];
extern _config_map map_wlan_a_channel_adhoc_eu[];
extern _config_map map_wlan_a_channel_adhoc_jp[];
extern _config_map map_wlan_a_channel_adhoc_cn[];
extern _config_map map_wlan_bg_channel_adhoc_us[];
extern _config_map map_wlan_bg_channel_adhoc_eu[];
extern _config_map map_wlan_bg_channel_adhoc_jp[];
extern _config_map map_wlan_bg_channel_adhoc_cn[];
extern _config_map map_wlan_a_channel_adhoc_all[];
extern _config_map map_wlan_bg_channel_adhoc_all[];
extern _config_map map_wlan_a_channel_us[];
extern _config_map map_wlan_a_channel_eu[];
extern _config_map map_wlan_a_channel_jp[];
extern _config_map map_wlan_a_channel_cn[]; 
extern _config_map map_wlan_bg_channel_us[];
extern _config_map map_wlan_bg_channel_eu[];
extern _config_map map_wlan_bg_channel_jp[];
extern _config_map map_wlan_bg_channel_cn[]; 
extern _config_map map_wlan_mix_channel_us[];
extern _config_map map_wlan_mix_channel_eu[];
extern _config_map map_wlan_mix_channel_jp[];
extern _config_map map_wlan_mix_channel_cn[];
extern _config_map map_wlan_a_channel_all[];
extern _config_map map_wlan_bg_channel_all[];
extern _config_map map_wlan_abg_channel_all[];
extern _config_map map_wlan_abg_channel_to_freq[];

extern _config_map map_ipconfig[];
extern _config_map map_eth_speed[];
extern _config_map map_wlan_proto[];
extern _config_map map_wlan_auth_alg[];
extern _config_map map_wlan_key_mgmt[];
extern _config_map map_wlan_pairwise[];
extern _config_map map_wlan_group[];
extern _config_map map_wlan_eap[];
extern _config_map map_wlan_phase1[];
extern _config_map map_wlan_phase2[];
extern _config_map map_ReconnectRule[];
extern _config_map map_SwitchThreshold[];
extern _config_map map_TurboRoaming[];
extern _config_map map_wlan_opmode[];
extern _config_map map_adhoc_opmode[];

extern _config_map map_wlan_channel_BG[];
extern _config_map map_web_authentication[];
extern _config_map map_web_Encryption[];
extern _config_map map_web_wepkeylen[];
extern _config_map map_web_wepKeyFormat[];
extern _config_map map_web_eapMehtod[];
extern _config_map map_web_tunnAuth[];

extern _config_map map_opMode[];
extern _config_map map_Baudrate[];
extern _config_map map_DataBits[];
extern _config_map map_StopBits[];
extern _config_map map_Parity[];
extern _config_map map_FlowCtrl[];
extern _config_map map_FIFO[];
extern _config_map map_Interface[];

extern _config_map map_Delimiter[];
extern _config_map map_ConnCtrl[];

extern _config_map mapSerialCommandMode[];

/* SNMP */
extern _config_map map_SNMP_Version[];
extern _config_map map_SNMP_Auth[];
extern _config_map map_SNMP_Priv[];

extern _config_map map_sslInfp[];
extern _config_map map_certFile[];

extern _config_map map_tcp_state[];
extern _config_map map_protocol[];

/* Wlan */
extern _config_map mapWlanRegion[];

extern _config_map mapActiveInterface[];

extern _config_map mapLANSpeed[];

extern _config_map map_DisableEnable[]; //added by vince
extern _config_map map_ipconf[]; //added by vince

extern _config_map map_CountryCode[];

// SUPPORT_RTERMINAL_MODE
extern _config_map mapAuthType[];
extern _config_map mapMapKeys[];
extern _config_map mapAuthServerPort[];

// SUPPORT_NPWIO_MODBUS_FUNCTION_CODE
extern _config_map mapModbusFunctionCodeFull[];
extern _config_map mapModbusFunctionCode0304[];
extern _config_map mapModbusFunctionCode0204[];
extern _config_map mapModbusFunctionCode04[];

// =====

extern _config_map mapDIChannelMode[];
extern _config_map mapDOChannelMode[];
extern _config_map mapDICntTrigger[];
extern _config_map mapDIOStatus[];
extern _config_map mapDOSafeStatusSetting[];

#endif // __SYSMAPS_H__
