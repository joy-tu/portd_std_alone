#include <stdio.h>
#include <string.h>
#include <portd.h>
#include <sio.h>
#include <sysmaps.h>
#include <eventd.h>
#include <config.h>
#include <sysapi.h>

_config_map map_ipconfig[] = { // for writing to config file
    {0, "static"},
    {1, "dhcp"},
    {2, "bootp"},
    {0, NULL},
};

_config_map map_ipconf[] = {  // for console
    {0, "Static"},
    {1, "DHCP"},
    {2, "BOOTP"},
    {0, NULL},
};

_config_map map_eth_speed[] = {
    {DNET_SPEED_AUTO, "Auto"},
    {DNET_SPEED_10_HALF, "10Mbps Half"},
    {DNET_SPEED_10_FULL, "10Mbps Full"},
    {DNET_SPEED_100_HALF, "100Mbps Half"},
    {DNET_SPEED_100_FULL, "100Mbps Full"},
    {0, NULL},
};

_config_map map_wlan_mode[] = {
    {DCF_WLAN_MODE_INFRA, "Infrastructure Mode"},
    {DCF_WLAN_MODE_IBSS, "Ad-hoc Mode"},
    {0, NULL},
};

_config_map map_wlan_a_channel_adhoc_us[] = {
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
//    {5745, "149"},
//    {5765, "153"},
//    {5785, "157"},
//    {5805, "161"},
//    {5825, "165"},    
    {0, NULL},
};

_config_map map_wlan_a_channel_adhoc_eu[] = {
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
    {0, NULL},
};

_config_map map_wlan_a_channel_adhoc_jp[] = {
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
    {0, NULL},
};

_config_map map_wlan_a_channel_adhoc_cn[] = {
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
//    {5745, "149"},
//    {5765, "153"},
//    {5785, "157"},
//    {5805, "161"},
//    {5825, "165"},    
    {0, NULL},
};

_config_map map_wlan_bg_channel_adhoc_us[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_adhoc_eu[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_adhoc_jp[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    //{2484, "14"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_adhoc_cn[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    {0, NULL},
};

_config_map map_wlan_a_channel_adhoc_all[] = {
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
//    {5745, "149"},
//    {5765, "153"},
//    {5785, "157"},
//    {5805, "161"},
//    {5825, "165"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_adhoc_all[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    {0, NULL},
};

_config_map map_wlan_a_channel_us[] = {
    {0, "N/A"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},
    {0, NULL},
};

_config_map map_wlan_a_channel_eu[] = {
    {0, "N/A"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {0, NULL},
};

_config_map map_wlan_a_channel_jp[] = {
    {0, "N/A"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {0, NULL},
};

_config_map map_wlan_a_channel_cn[] = {
    {0, "N/A"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_us[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_eu[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_jp[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    // {14, "14"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_cn[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {0, NULL},
};

_config_map map_wlan_mix_channel_us[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},	
    {0, NULL},
};

_config_map map_wlan_mix_channel_eu[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {0, NULL},
};

_config_map map_wlan_mix_channel_jp[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {0, NULL},
};

_config_map map_wlan_mix_channel_cn[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},
    {0, NULL},
};

_config_map map_wlan_a_channel_all[] = {
    {0, "N/A"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},
    {0, NULL},
};

_config_map map_wlan_bg_channel_all[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},
    {14, "14"},   
    {0, NULL},
};

_config_map map_wlan_abg_channel_all[] = {
    {0, "N/A"},
    {1, "1"},
    {2, "2"},
    {3, "3"},
    {4, "4"},
    {5, "5"},
    {6, "6"},
    {7, "7"},
    {8, "8"},
    {9, "9"},
    {10, "10"},
    {11, "11"},
    {12, "12"},
    {13, "13"},    
    {14, "14"},
    {36, "36"},
    {40, "40"},
    {44, "44"},
    {48, "48"},
    {52, "52"},
    {56, "56"},
    {60, "60"},
    {64, "64"},
    {100, "100"},
    {104, "104"},
    {108, "108"},
    {112, "112"},
    {116, "116"},
    {120, "120"},
    {124, "124"},
    {128, "128"},
    {132, "132"},
    {136, "136"},
    {140, "140"},
    {149, "149"},
    {153, "153"},
    {157, "157"},
    {161, "161"},
    {165, "165"},
    {0, NULL},
};

_config_map map_wlan_abg_channel_to_freq[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    {2484, "14"},
    {5180, "36"},
    {5200, "40"},
    {5220, "44"},
    {5240, "48"},
    {5260, "52"},
    {5280, "56"},
    {5300, "60"},
    {5320, "64"},
    {5500, "100"},
    {5520, "104"},
    {5540, "108"},
    {5560, "112"},
    {5580, "116"},
    {5600, "120"},
    {5620, "124"},
    {5640, "128"},
    {5660, "132"},
    {5680, "136"},
    {5700, "140"},
    {5745, "149"},
    {5765, "153"},
    {5785, "157"},
    {5805, "161"},
    {5825, "165"},    
    {0, NULL},
};

_config_map map_wlan_proto[] = {
    {DCF_WLAN_PROTO_WPA, "WPA"},
    {DCF_WLAN_PROTO_RSN, "RSN"},
    {0, NULL},
};

_config_map map_wlan_auth_alg[] = {
    {DCF_WPA_AUTH_ALG_OPEN, "OPEN"},
    {DCF_WPA_AUTH_ALG_SHARED, "SHARED"},
    {DCF_WPA_AUTH_ALG_LEAP, "LEAP"},
    {0, NULL},
};

_config_map map_wlan_key_mgmt[] = {
    {DCF_WLAN_KEYMGMT_WPA_PSK, "WPA-PSK"},
    {DCF_WLAN_KEYMGMT_WPA_EAP, "WPA-EAP"},
    {DCF_WLAN_KEYMGMT_IEEE8021X, "IEEE8021X"},
    {DCF_WLAN_KEYMGMT_NONE, "NONE"},
    {0, NULL},
};

_config_map map_wlan_pairwise[] = {
    {DCF_WLAN_PAIRWISE_CCMP, "CCMP"},
    {DCF_WLAN_PAIRWISE_TKIP, "TKIP"},
    {0, NULL},
};

_config_map map_wlan_group[] = {
    {DCF_WLAN_GROUP_CCMP, "CCMP"},
    {DCF_WLAN_GROUP_TKIP, "TKIP"},
    {DCF_WLAN_GROUP_CCMP_TKIP, "CCMP TKIP"},
    {0, NULL},
};

_config_map map_wlan_eap[] = {
    {DCF_WLAN_EAP_MD5, "MD5"},
    {DCF_WLAN_EAP_MSCHAPV2, "MSCHAPV2"},
    {DCF_WLAN_EAP_OPT, "OPT"},
    {DCF_WLAN_EAP_GTC, "GTC"},
    {DCF_WLAN_EAP_TLS, "TLS"},
    {DCF_WLAN_EAP_PEAP, "PEAP"},
    {DCF_WLAN_EAP_TTLS, "TTLS"},
    {DCF_WLAN_EAP_LEAP, "LEAP"},
    {0, NULL},
};

_config_map map_wlan_phase1[] = {
    {DCF_WLAN_PAHSE1_PEAPV0, "peapver=0"},
    {DCF_WLAN_PAHSE1_PEAPV1, "peapver=1"},
    {DCF_WLAN_PAHSE1_PEAPLABLE, "peaplabel"},
    {0, NULL},
};

_config_map map_wlan_phase2[] = {
    {DCF_WLAN_PAHSE2_GTC, "auth=GTC"},
    {DCF_WLAN_PAHSE2_MD5, "auth=MD5"},
    {DCF_WLAN_PAHSE2_MSCHAPV2, "auth=MSCHAPV2"},
    {DCF_WLAN_PAHSE2_TTLS_PAP, "auth=PAP"},
    {DCF_WLAN_PAHSE2_TTLS_CHAP, "auth=CHAP"},
    {DCF_WLAN_PAHSE2_TTLS_MSCHAP, "auth=MSCHAP"},
    {DCF_WLAN_PAHSE2_TTLS_MSCHAPV2, "auth=MSCHAPV2"},
    {DCF_WLAN_PAHSE2_EAP_GTC, "autheap=EAP_GTC"},
    {DCF_WLAN_PAHSE2_EAP_MD5, "autheap=EAP_MD5"},
    {DCF_WLAN_PAHSE2_EAP_MSCHAPV2, "autheap=EAP-MSCHAPV2"},
    {0, NULL},
};

_config_map map_ReconnectRule[] = {
    {D_SIGNAL_STRENGTH_AP, "Signal strength of AP"},
    {D_PRIORITY_SEQUENTIAL, "Priority sequential"},
    {D_FIXED_FIRST, "Fixed on 1st priority"},
    {0, NULL}
};

_config_map map_SwitchThreshold[] = {
    {0, "None"},
    {1, "< 20%"},
    {2, "< 40%"},
    {3, "< 60%"},
    {4, "< 80%"},
    {0, NULL}
};

_config_map map_TurboRoaming[] = {
    {0, "Disable"},
    {1, "Enable"},
    {0, NULL},
};

_config_map map_wlan_opmode[] = {
    {0, "Auto"},
    {1, "802.11a"},
    {2, "802.11b/g"},
    {3, "802.11a/n"},
    {4, "802.11b/g/n"},
    {0, NULL},
};

_config_map map_adhoc_opmode[] = {
    //{1, "802.11a"},
    {2, "802.11b/g"},
    //{3, "802.11a/n"},
    //{4, "802.11b/g/n"},
    {0, NULL},
};

_config_map map_wlan_channel_BG[] = {
    {2412, "1"},
    {2417, "2"},
    {2422, "3"},
    {2427, "4"},
    {2432, "5"},
    {2437, "6"},
    {2442, "7"},
    {2447, "8"},
    {2452, "9"},
    {2457, "10"},
    {2462, "11"},
    {2467, "12"},
    {2472, "13"},
    {2484, "14"},
    {0, NULL},
};

_config_map map_web_authentication[] = {
    {1, "Open System"},
    {2, "Shared Key"},
    {3, "WPA"},
    {4, "WPA-PSK"},
    {5, "WPA2"},
    {6, "WPA2-PSK"},
    {0, NULL},
};

_config_map map_web_Encryption[] = {
    {1, "Disable"},
    {2, "WEP"},
    {3, "TKIP"},
    {4, "AES-CCMP"},
    {0, NULL},
};

_config_map map_web_wepkeylen[] = {
    {1, "64-bits"},
    {2, "128-bits"},
    {0, NULL},
};

_config_map map_web_wepKeyFormat[] = {
    {1, "ASCII"},
    {2, "HEX"},
    {0, NULL},
};

_config_map map_web_eapMehtod[] = {
    {1, "TLS"},
    {2, "PEAP"},
    {3, "TTLS"},
    {4, "LEAP"},
    {0, NULL},
};

_config_map map_web_tunnAuth[] = {
    {DCF_WLAN_PAHSE2_GTC, "GTC"},
    {DCF_WLAN_PAHSE2_MD5, "MD5"},
    {DCF_WLAN_PAHSE2_MSCHAPV2, "MSCHAPV2"},
    {DCF_WLAN_PAHSE2_TTLS_PAP, "PAP"},
    {DCF_WLAN_PAHSE2_TTLS_CHAP, "CHAP"},
    {DCF_WLAN_PAHSE2_TTLS_MSCHAP, "MSCHAP"},
    {DCF_WLAN_PAHSE2_TTLS_MSCHAPV2, "MSCHAPV2"},
    {DCF_WLAN_PAHSE2_EAP_GTC, "EAP-GTC"},
    {DCF_WLAN_PAHSE2_EAP_MD5, "EAP-MD5"},
    {DCF_WLAN_PAHSE2_EAP_MSCHAPV2, "EAP-MSCHAPV2"},
    {0, NULL},
};

_config_map map_opMode[] = {
    {CFG_APPLICATION_DISABLED, "Disable"},
    {CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_REALCOM, "Real COM"},
    {CFG_APPLICATION_DEVICE_CONTROL | CFG_OPMODE_RFC2217, "RFC2217"},
    {CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPSERVER, "TCP Server"},
    {CFG_APPLICATION_SOCKET | CFG_OPMODE_TCPCLIENT, "TCP Client"},
    {CFG_APPLICATION_SOCKET | CFG_OPMODE_UDP, "UDP"},

// ===== perry modify start =====
#ifdef SUPPORT_PAIRCONN_MODE
    {CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_MASTER, "Pair Connection Master"},
    {CFG_APPLICATION_PAIR_CONNECTION | CFG_OPMODE_PAIR_SLAVE, "Pair Connection Slave"},
#endif
#ifdef SUPPORT_EMODEM_MODE
    {CFG_APPLICATION_ETH_MODEM, "Ethernet Modem"},
#endif
// ===== perry modify end =====
#ifdef SUPPORT_RTERMINAL_MODE
    {CFG_APPLICATION_RTERMINAL, "Reverse Terminal"},
#endif
    {-1, NULL}
};

_config_map map_Baudrate[] = {
    {BAUD_50, "50"},
    {BAUD_75, "75"},
    {BAUD_110, "110"},
    {BAUD_134, "134"},
    {BAUD_150, "150"},
    {BAUD_300, "300"},
    {BAUD_600, "600"},
    {BAUD_1200, "1200"},
    {BAUD_1800, "1800"},
    {BAUD_2400, "2400"},
    {BAUD_4800, "4800"},
    //{BAUD_7200,     "7200"},
    {BAUD_9600, "9600"},
    {BAUD_19200, "19200"},
    {BAUD_38400, "38400"},
    {BAUD_57600, "57600"},
    {BAUD_115200, "115200"},
    {BAUD_230400, "230400"},
    {BAUD_460800, "460800"},
    {BAUD_921600, "921600"},
    {-1, "Other"},
    {0, NULL},
};

_config_map map_DataBits[] = {
#if defined(w2x50a) || defined (ia5x50aio)
    {BIT_5, "5"},
    {BIT_6, "6"},
#endif
    {BIT_7, "7"},
    {BIT_8, "8"},
    {0, NULL},
};

_config_map map_StopBits[] = { // These constants are not used by API. Only used in UI.
    {STOP_1, "1"},
#if defined(w2x50a) || defined (ia5x50aio)
    {STOP_2, "1.5"},
#endif   
    {STOP_2 + 1, "2"},
    {0, NULL},
};

_config_map map_Parity[] = {
    {P_NONE, "None"},
    {P_ODD, "Odd"},
    {P_EVEN, "Even"},
#if defined(w2x50a) || defined (ia5x50aio)
    {P_MRK, "Mark"},
    {P_SPC, "Space"},
#endif 
    {0, NULL}
};

_config_map map_FlowCtrl[] = {
    {0, "None"},
    {1, "RTS/CTS"},
//#if defined(w2x50a)
	//{2,     "DTR/DSR"},
//#endif
    {2, "XON/XOFF"},
    {0, NULL},
};

_config_map map_FIFO[] = {
    {0, "Disable"},
    {1, "Enable"},
    {0, NULL},
};

#ifdef w1
_config_map map_Interface[] = {
    {RS232_MODE, "RS-232"},
    {RS485_2WIRE_MODE, "RS-422/485"},
    {0, NULL},
};
#else
_config_map map_Interface[] = {
    {RS232_MODE, "RS-232"},
    {RS422_MODE, "RS-422"},
    {RS485_2WIRE_MODE, "RS-485 2wire"},
    {RS485_4WIRE_MODE, "RS-485 4wire"},
    {0, NULL},
};
#endif

_config_map map_Delimiter[] = {
    {DEL_NOTHING, "Do Nothing"},
    {DEL_PLUSONE, "Delimiter+1"},
    {DEL_PLUSTWO, "Delimiter+2"},
    {DEL_STRIP, "Strip Delimiter"},
    {0, NULL},
};

_config_map map_ConnCtrl[] = {
    {DCF_CLI_MODE_ON_STARTUP | DCF_CLI_MODE_OFF_NONE, "Startup/None"},
    {DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_NONE, "Any Character/None"},
    {DCF_CLI_MODE_ON_ANYCHAR | DCF_CLI_MODE_OFF_INACT, "Any Character/Inactivity Time"},
    {DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_DSROFF, "DSR ON/DSR OFF"},
    {DCF_CLI_MODE_ON_DSRON | DCF_CLI_MODE_OFF_NONE, "DSR ON/None"},
    {DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_DCDOFF, "DCD ON/DCD OFF"},
    {DCF_CLI_MODE_ON_DCDON | DCF_CLI_MODE_OFF_NONE, "DCD ON/None"},
    {0, NULL},
};

/* SNMP */
_config_map map_SNMP_Version[] = {
    {DCF_SNMP_VERSION_V1 | DCF_SNMP_VERSION_V2C | DCF_SNMP_VERSION_V3, "V1, V2c, V3"},
    {DCF_SNMP_VERSION_V1 | DCF_SNMP_VERSION_V2C, "V1, V2c"},
    {DCF_SNMP_VERSION_V3, "V3 only"},
    {0, NULL},
};

_config_map map_SNMP_Auth[] = {
    {DCF_SNMP_AUTH_DISABLE, "Disable"},
    {DCF_SNMP_AUTH_MD5, "MD5"},
    {DCF_SNMP_AUTH_SHA, "SHA"},
    {0, NULL},
};

_config_map map_SNMP_Priv[] = {
    {DCF_SNMP_PRIV_DISABLE, "Disable"},
    {DCF_SNMP_PRIV_DES, "DES"},
    {DCF_SNMP_PRIV_AES, "AES"},
    {0, NULL},
};

_config_map map_Events[] = {
    {EVENT_ID_SERIALDATA, "Serial data log"},
    {EVENT_ID_COLDSTART, "System Cold Start"},
    {EVENT_ID_WARMSTART, "System Warm Start"},

    {EVENT_ID_IPRENEW, "DHCP/BOOTP/PPPoE Get IP/Renew"},
    {EVENT_ID_NTP, "NTP"},
    {EVENT_ID_MAILFAIL, "Mail Fail"},
    {EVENT_ID_NTPCONNFA, "NTP Connect Fail"},
    {EVENT_ID_DHCPFAIL, "PPPoE Get IP Fail"},
    {EVENT_ID_IPCONFLICT, "IP Conflict"},
    {EVENT_ID_NETLINKDOWN, "Ethernet Link Down"},
    {EVENT_ID_WLANLINKDOWN, "WLAN Link Down"},
    {EVENT_ID_WLANLOW, "Wireless Signal Below Threshold"},
    {EVENT_ID_WLANPROFILE, "Wireless Active Profile Changed"},

    {EVENT_ID_LOGINFAIL, "Login Fail"},
    {EVENT_ID_IPCHANGED, "IP Changed"},
    {EVENT_ID_PWDCHANGED, "Password Changed"},
    {EVENT_ID_CONFIGCHANGED, "Config Changed"},
    {EVENT_ID_FWUPGRADE, "Firmware Upgrade"},
    {EVENT_ID_SSLIMPORT, "SSL Certificate Import"},
    {EVENT_ID_CONFIGIMPORT, "Config Import"},
    {EVENT_ID_CONFIGEXPORT, "Config Export"},
    {EVENT_ID_WLANCERTIMPORT, "Wireless Certificate Import"},
    {EVENT_ID_SERIALLOGEXPORT, "Serial Data Log Export"},

    {EVENT_ID_OPMODE_CONNECT, "Connect"},
    {EVENT_ID_OPMODE_DISCONNECT, "Disconnect"},
    {EVENT_ID_OPMODE_AUTHFAIL, "Authentication Fail"},
    {EVENT_ID_OPMODE_RESTART, "Restart"}
};

_config_map map_sslInfp[] = {
    {1, "ISSUEDTO"},
    {2, "ISSUEDBY"},
    {3, "VALIDFROM"},
    {4, "VALIDTO"},
    {0, NULL},
};
#if 0
_config_map map_certFile[] = {
    {D_SSL_ETH_CERT, SSL_CERT_ETH},
    {D_SSL_WLAN_CERT, SSL_CERT_WLAN},
    {D_SSL_WPA_SERVER_CERT, wlan_ca_cert},
    {D_SSL_WPA_USER_CERT, wlan_client_cert},
    {D_SSL_WPA_USER_KEY, wlan_private_key},
    {0, NULL},
};
#endif
_config_map map_tcp_state[] = {
    {TCP_ESTABLISHED, "ESTABLISHED"},
    {TCP_SYN_SENT, "SYN_SENT"},
    {TCP_SYN_RECV, "SYN_RECV"},
    {TCP_FIN_WAIT1, "FIN_WAIT1"},
    {TCP_FIN_WAIT2, "FIN_WAIT2"},
    {TCP_TIME_WAIT, "TIME_WAIT"},
    {TCP_CLOSE, "CLOSE"},
    {TCP_CLOSE_WAIT, "CLOSE_WAIT"},
    {TCP_LAST_ACK, "LAST_ACK"},
    {TCP_LISTEN, "LISTEN"},
    {TCP_CLOSING, "CLOSING"},
    {0, ""},
    {0, NULL},
};

_config_map map_protocol[] = {
    {SYS_NETSTAT_TCP, "TCP"},
    {SYS_NETSTAT_UDP, "UDP"},
    {SYS_NETSTAT_RAW, "RAW"},
    {SYS_NETSTAT_UNIX, "UNIX"},
    {0, NULL},
};

_config_map mapWlanRegion[] = {
    {0, "US"},
    {1, "EU"},
    {2, "JP"},
    {0, NULL}
};

_config_map mapLANSpeed[] = {
    {D_LAN_SPEED_AUTO, "Auto"},
    {D_LAN_SPEED_10M_HALF, "10Mbps Half"},
    {D_LAN_SPEED_10M_FULL, "10Mbps Full"},
    {D_LAN_SPEED_100M_HALF, "100Mbps Half"},
    {D_LAN_SPEED_100M_FULL, "100Mbps Full"},
    {0, NULL}
};

_config_map mapActiveInterface[] = {
    {DLANTYPE_AUTO, "Auto Detect"},
#ifdef SUPPORT_DIO		// perry add
    {DLANTYPE_DIX, "Select by DI6"},
#endif	
    {DLANTYPE_ETHER, "Force Wired Ethernet"},
    {DLANTYPE_WLAN, "Force Wireless LAN"},
    {0, NULL}
};

_config_map mapSerialCommandMode[] = {
    {DCF_SCMDFLAG_DISABLE, "Disable"},
    {DCF_SCMDFLAG_HW_TRIG, "H/W control pin DI7"},
    {DCF_SCMDFLAG_SW_TRIG, "Activate by characters"},
    {DCF_SCMDFLAG_BREAK, "Activate by break signal"},
    {0, NULL}
};

_config_map map_DisableEnable[] = {
    {0, "Disable"},
    {1, "Enable"},
    {0, NULL},
};

_config_map map_CountryCode[] = {
    {50, "DB"},
    {516, "NA"},
    {8, "AL"},
    {12, "DZ"},
    {32, "AR"},
    {51, "AM"},
    {36, "AU"},
    {40, "AT"},
    {31, "AZ"},
    {48, "BH"},
    {112, "BY"},
    {56, "BE"},
    {84, "BZ"},
    {68, "BO"},
    {76, "BR"},
    {96, "BN"},   
    {100, "BG"},
    {124, "CA"},
    {152, "CL"},
    {156, "CN"},
    {170, "CO"},
    {188, "CR"},
    {191, "HR"},
    {196, "CY"},
    {203, "CZ"},
    {208, "DK"},
    {214, "DO"},
    {218, "EC"},
    {818, "EG"},
    {222, "SV"},
    {233, "EE"},
    {246, "FI"},  
    {250, "EU"}, // For EU model
    /*{250,"FR"},*/
    {268, "GE"},
    {276, "DE"},
    {300, "GR"},
    {320, "GT"},
    {340, "HN"},
    {344, "HK"},
    {348, "HU"},
    {352, "IS"},
    {356, "IN"},
    {360, "ID"},
    {364, "IR"},
    {372, "IE"},
    {376, "IL"},
    {380, "IT"},
    {392, "JP"},   
    {400, "JO"},
    {398, "KZ"},
    {408, "KP"},
    {410, "KR"},
    {414, "KW"},
    {428, "LV"},
    {422, "LB"},
    {438, "LI"},
    {440, "LT"},
    {442, "LU"},
    {446, "MO"},
    {807, "MK"},
    {458, "MY"},
    {484, "MX"},
    {492, "MC"},
    {504, "MA"},
    {528, "NL"},
    {554, "NZ"},
    {578, "NO"},
    {512, "OM"},
    {586, "PK"},
    {591, "PA"},
    {604, "PE"},
    {608, "PH"},
    {616, "PL"},
    {620, "PT"},
    {630, "PR"},
    {634, "QA"},
    {642, "RO"},
    {643, "RU"},
    {682, "SA"},
    {702, "SG"},
    {703, "SK"},
    {705, "SI"},
    {710, "ZA"},
    {724, "ES"},
    {752, "SE"},
    {756, "CH"},
    {760, "SY"},
    {158, "TW"},
    {764, "TH"},
    {780, "TT"},
    {788, "TN"},
    {792, "TR"},
    {804, "UA"},
    {784, "AE"},
    {826, "GB"},
    {840, "US"},
    {858, "UY"},
    {860, "UZ"},
    {862, "VE"},
    {704,"VN"},
    {887, "YE"},
    {716, "ZW"},
    {0, NULL},
};

#ifdef SUPPORT_RTERMINAL_MODE
_config_map mapAuthType[] = {
    {D_AUTH_NONE, "None"},
    {D_AUTH_LOCAL, "Local"},
    {D_AUTH_RADIUS, "RADIUS"},
    {0, NULL}
};

_config_map mapMapKeys[] = {
    {DCF_CRLF_TO_CRLF, "CR-LF"},
    {DCF_CRLF_TO_CR, "CR"},
    {DCF_CRLF_TO_LF, "LF"},
    {0, NULL}
};

_config_map mapAuthServerPort[] = {
    {0, "1645"},
    {1, "1812"},
    {0, NULL},
};

_config_map mapModbusFunctionCodeFull[] = {
    {0, "01:COIL STATUS     "},
    {1, "02:INPUT STATUS    "},
    {2, "03:HOLDING REGISTER"},
    {3, "04:INPUT REGISTER  "},
    {0, NULL}
};

_config_map mapModbusFunctionCode0304[] = {
    {2, "03:HOLDING REGISTER"},
    {3, "04:INPUT REGISTER  "},
    {0, NULL}
};

_config_map mapModbusFunctionCode0204[] = {
    {1, "02:INPUT STATUS    "},
	{3, "04:INPUT REGISTER	"},
    {0, NULL}
};

_config_map mapModbusFunctionCode04[] = {
    {3, "04:INPUT REGISTER  "},
    {0, NULL}
};



_config_map mapDIChannelMode[] = {
    {0, "DI"},
    {1, "Counter"},	
    {0, NULL}
};

_config_map mapDOChannelMode[] = {
    {0, "DO"},
    {1, "Pulse Output"},	
    {0, NULL}
};

_config_map mapDICntTrigger[] = {
    {0, "Lo to Hi"},
    {1, "Hi to Lo"},	
    {2, "Both"},	
    {0, NULL}
};

_config_map mapDIOStatus[] = {
    {0, "OFF"},
    {1, "ON"},
    {0, NULL}
};

_config_map mapDOSafeStatusSetting[] = {
    {0, "OFF"},
    {1, "ON"},
    {2, "HOLD LAST"},
    {0, NULL}
};

#endif

char* search_map_by_index(int idx, _config_map_t map)
{
    while (map->name) {
        if (map->index == idx)
            return (char*)map->name;
        ++map;
    }
    return NULL;
}

int search_map_by_name(char* name, _config_map_t map)
{
    while (map->name) {
        if (strcmp(name, (char*)map->name) == 0)
            return map->index;
        ++map;
    }
    return 0;
}

int configmap_index(_config_map_t map, int id, char *str)
{
	int	i;
	for (i = 0; map[i].name; i++) {
		if ((str && strcmp(map[i].name, str) == 0) || (!str && map[i].index == id))
			return i;
	}
	return -1;
}
#if 0
_config_map_t sys_get_adhoc_map(int opmode)
{
    int region;
    _config_map_t map = NULL;

    region = Scf_getModelRegion();
    switch (region) {
        // 0: US
        case 0:
            if ((opmode == DWLAN_MODE_802_11A) || (opmode == DWLAN_MODE_802_11AN))
                map = map_wlan_a_channel_adhoc_us;
            else if ((opmode == DWLAN_MODE_802_11BG) || (opmode == DWLAN_MODE_802_11BGN)) 
                map = map_wlan_bg_channel_adhoc_us;
            break;
        // 1: EU
        case 1:
            if ((opmode == DWLAN_MODE_802_11A) || (opmode == DWLAN_MODE_802_11AN))
                map = map_wlan_a_channel_adhoc_eu;
            else if ((opmode == DWLAN_MODE_802_11BG) || (opmode == DWLAN_MODE_802_11BGN)) 
                map = map_wlan_bg_channel_adhoc_eu;
            break;
        // 2: JP
        case 2:
            if ((opmode == DWLAN_MODE_802_11A) || (opmode == DWLAN_MODE_802_11AN))
                map = map_wlan_a_channel_adhoc_jp;
            else if ((opmode == DWLAN_MODE_802_11BG) || (opmode == DWLAN_MODE_802_11BGN)) 
                map = map_wlan_bg_channel_adhoc_jp;
            break;
        // 3: CN
        case 3:
            if ((opmode == DWLAN_MODE_802_11A) || (opmode == DWLAN_MODE_802_11AN))
                map = map_wlan_a_channel_adhoc_cn;
            else if ((opmode == DWLAN_MODE_802_11BG) || (opmode == DWLAN_MODE_802_11BGN)) 
                map = map_wlan_bg_channel_adhoc_cn;
            break;	
        default:
            if ((opmode == DWLAN_MODE_802_11A) || (opmode == DWLAN_MODE_802_11AN))
                map = map_wlan_a_channel_adhoc_all;
            else if ((opmode == DWLAN_MODE_802_11BG) || (opmode == DWLAN_MODE_802_11BGN)) 
                map = map_wlan_bg_channel_adhoc_all;
            break;
    }
    return map;
}

#endif