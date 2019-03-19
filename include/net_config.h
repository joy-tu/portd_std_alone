#ifndef __NET_CONFIG_H__
#define __NET_CONFIG_H__

#include "support.h"

#ifndef SUPORT_MULTI_PROFILE
#define DCF_WLAN_MAX_PROFILE        1
#else
#define DCF_WLAN_MAX_PROFILE        3
#endif
#define DCF_WLAN_INFRA_PROFILE_NUM	DCF_WLAN_MAX_PROFILE
#define DCF_WLAN_AD_HOC_PROFILE 	0

#define DCF_WLAN_PROFILE_NAME_LEN   32
#define DCF_WLAN_SSID_LEN           32
#define DCF_WLAN_WEP_KEY_NUM        4
#define DCF_WLAN_WEP_KEY_LEN        26
#define DCF_WLAN_WEP_PASSPHRASE_LEN 40
#define DCF_WLAN_PSK_PASSPHRASE_LEN 64
#define DCF_WLAN_WPA_USERNAME_LEN   40
#define DCF_WLAN_WPA_PASSWORD_LEN   40

#define DCF_WLAN_DISABLE    0
#define DCF_WLAN_ENABLE     1

#define DCF_WLAN_SCAN_CHANNEL_NUM	3
#define DCF_MAX_IPTABLES            16
#ifdef SUPPORT_RTERMINAL_MODE
#define DCF_MAX_USERTABLES          64
#define DCF_LOCAL_NAME_LEN 16
#define DCF_LOCAL_PSWD_LEN 16
#endif

#ifdef CROSS
#define WPA_SUPPLICANT_CONF_PATH "/configData/wireless/"
#else
#define WPA_SUPPLICANT_CONF_PATH "./"
#endif

#define DWLAN_BSSTYPE_INFRA		1	/* Intrastructure mode */
#define DWLAN_BSSTYPE_ADHOC		0	/* Ad-Hoc mode */

#define DWLAN_MODE_AUTO		0
#define DWLAN_MODE_802_11A		1
#define DWLAN_MODE_802_11BG		2
#define DWLAN_MODE_802_11AN		3 
#define DWLAN_MODE_802_11BGN	4

extern const char wlan_ca_cert[];
extern const char wlan_client_cert[];
extern const char wlan_private_key[];

struct wpa_config {
    /**
     * 0 = this network can be used (default)
     * 1 = this network block is disabled
     */
    int disabled;
    /**
     * priority group (integer)
     */
    int priority;
    /**
     * mode: IEEE 802.11 operation mode
     * 0 = infrastructure (Managed) mode, i.e., associate with an AP (default)
     * 1 = IBSS (ad-hoc, peer-to-peer)
     * 2 = AP (access point)
     */
    int mode;

    int scan_ssid;

    /**
     * Channel frequency in megahertz (MHz) for IBSS, e.g.,
     * 2412 = IEEE 802.11b/g channel 1.
     */
    int frequency;
    /**
     * SSID (mandatory); either as an ASCII string with double quotation or
     * as hex string; network name
     */
    char ssid[DCF_WLAN_SSID_LEN+1];

    /**
     * proto: list of accepted protocols
     * WPA = WPA/IEEE 802.11i/D3.0
     * RSN = WPA2/IEEE 802.11i (also WPA2 can be used as an alias for RSN)
     * If not set, this defaults to: WPA RSN
     */
    int proto;

    /**
     * key_mgmt: list of accepted authenticated key management protocols
     * WPA-PSK = WPA pre-shared key (this requires 'psk' field)
     * WPA-EAP = WPA using EAP authentication
     * IEEE8021X = IEEE 802.1X using EAP authentication and (optionally) dynamically generated WEP keys
     * NONE = WPA is not used; plaintext or static WEP could be used
     * WPA-PSK-SHA256 = Like WPA-PSK but using stronger SHA256-based algorithms
     * WPA-EAP-SHA256 = Like WPA-EAP but using stronger SHA256-based algorithms
     * If not set, this defaults to: WPA-PSK WPA-EAP
     */
    int key_mgmt;

    /**
     * auth_alg : list of allowed IEEE 802.11 authentication algorithms
     * OPEN = Open System authentication (required for WPA/WPA2)
     * SHARED = Shared Key authentication (requires static WEP keys)
     * LEAP = LEAP/Network EAP (only used with LEAP)
     * If not set, automatic selection is used (Open System with LEAP enabled if
     * LEAP is allowed as one of the EAP methods).
     */
    int auth_alg;

    /**
     * pairwise: list of accepted pairwise (unicast) ciphers for WPA
     * CCMP = AES in Counter mode with CBC-MAC [RFC 3610, IEEE 802.11i/D7.0]
     * TKIP = Temporal Key Integrity Protocol [IEEE 802.11i/D7.0]
     * NONE = Use only Group Keys (deprecated, should not be included if APs support
     *    pairwise keys)
     * If not set, this defaults to: CCMP TKIP
     */
    int pairwise;

    /**
     * group: list of accepted group (broadcast/multicast) ciphers for WPA
     * CCMP = AES in Counter mode with CBC-MAC [RFC 3610, IEEE 802.11i/D7.0]
     * TKIP = Temporal Key Integrity Protocol [IEEE 802.11i/D7.0]
     * WEP104 = WEP (Wired Equivalent Privacy) with 104-bit key
     * WEP40 = WEP (Wired Equivalent Privacy) with 40-bit key [IEEE 802.11]
     * If not set, this defaults to: CCMP TKIP WEP104 WEP40
     */
    int group;

    /**
     * psk: WPA preshared key; 256-bit pre-shared key
     * The key used in WPA-PSK mode can be entered either as 64 hex-digits, i.e.,
     * 32 bytes or as an ASCII passphrase (in which case, the real PSK will be
     * generated using the passphrase and SSID). ASCII passphrase must be between
     * 8 and 63 characters (inclusive).
     * This field is not needed, if WPA-EAP is used.
     * Note: Separate tool, wpa_passphrase, can be used to generate 256-bit keys
     * from ASCII passphrase. This process uses lot of CPU and wpa_supplicant
     * startup and reconfiguration time can be optimized by generating the PSK only
     * only when the passphrase or SSID has actually changed.
     */
    char psk[DCF_WLAN_PSK_PASSPHRASE_LEN+1];

    /**
     * wep_key0..3: Static WEP key (ASCII in double quotation, e.g. "abcde" or
     * hex without quotation, e.g., 0102030405)
     */
    char wep_key[DCF_WLAN_WEP_KEY_NUM][DCF_WLAN_WEP_KEY_LEN+1];

    /*
     * wep_tx_keyidx: Default WEP key index (TX) (0..3)
     */
    int wep_tx_keyidx;

    /**
     * eap: space-separated list of accepted EAP methods
     * MD5 = EAP-MD5 (unsecure and does not generate keying material
     *       cannot be used with WPA; to be used as a Phase 2 method
     *       with EAP-PEAP or EAP-TTLS)
     * MSCHAPV2 = EAP-MSCHAPv2 (cannot be used separately with WPA; to be used
     *              as a Phase 2 method with EAP-PEAP or EAP-TTLS)
     * OTP = EAP-OTP (cannot be used separately with WPA; to be used
     *         as a Phase 2 method with EAP-PEAP or EAP-TTLS)
     * GTC = EAP-GTC (cannot be used separately with WPA; to be used
     *         as a Phase 2 method with EAP-PEAP or EAP-TTLS)
     * TLS = EAP-TLS (client and server certificate)
     * PEAP = EAP-PEAP (with tunnelled EAP authentication)
     * TTLS = EAP-TTLS (with tunnelled EAP or PAP/CHAP/MSCHAP/MSCHAPV2
     *          authentication)
     * If not set, all compiled in methods are allowed.
     */
    int eap;

    /**
     * identity: Identity string for EAP
     * This field is also used to configure user NAI for
     * EAP-PSK/PAX/SAKE/GPSK.
     */
    char identity[DCF_WLAN_WPA_USERNAME_LEN+1];

    /**
     * password: Password string for EAP. This field can include either the
     *     plaintext password (using ASCII or hex string) or a NtPasswordHash
     *     (16-byte MD4 hash of password) in hash:<32 hex digits> format.
     *     NtPasswordHash can only be used when the password is for MSCHAPv2 or
     *     MSCHAP (EAP-MSCHAPv2, EAP-TTLS/MSCHAPv2, EAP-TTLS/MSCHAP, LEAP).
     *     EAP-PSK (128-bit PSK), EAP-PAX (128-bit PSK), and EAP-SAKE (256-bit
     *     PSK) is also configured using this field. For EAP-GPSK, this is a
     *     variable length PSK.
     */
    char password[DCF_WLAN_WPA_USERNAME_LEN+1];

    /**
     * anonymous_identity: Anonymous identity string for EAP (to be used as the
     *    unencrypted identity with EAP types that support different tunnelled
     *    identity, e.g., EAP-TTLS)
     */
    char anonymous_identity[DCF_WLAN_WPA_USERNAME_LEN+1];

    /**
     * Phase1 (outer authentication, i.e., TLS tunnel) parameters
     * (string with field-value pairs, e.g., "peapver=0" or "peapver=1 peaplabel=1")
     *
     * 'peapver' can be used to force which PEAP version (0 or 1) is used.
     *
     * 'peaplabel=1' can be used to force new label, "client PEAP encryption",
     *     to be used during key derivation when PEAPv1 or newer. Most existing
     *     PEAPv1 implementation seem to be using the old label, "client EAP
     *     encryption", and wpa_supplicant is now using that as the default value.
     *     Some servers, e.g., Radiator, may require peaplabel=1 configuration to
     *     interoperate with PEAPv1; see eap_testing.txt for more details.
     *
     * 'peap_outer_success=0' can be used to terminate PEAP authentication on
     *     tunneled EAP-Success. This is required with some RADIUS servers that
     *     implement draft-josefsson-pppext-eap-tls-eap-05.txt (e.g.,
     *     Lucent NavisRadius v4.4.0 with PEAP in "IETF Draft 5" mode)
     *     include_tls_length=1 can be used to force wpa_supplicant to include
     *     TLS Message Length field in all TLS messages even if they are not
     *     fragmented.
     *     sim_min_num_chal=3 can be used to configure EAP-SIM to require three
     *     challenges (by default, it accepts 2 or 3)
     *     result_ind=1 can be used to enable EAP-SIM and EAP-AKA to use
     *     protected result indication.
     *
     * 'crypto_binding' option can be used to control PEAPv0 cryptobinding behavior:
     *     * 0 = do not use cryptobinding (default)
     *     * 1 = use cryptobinding if server supports it
     *     * 2 = require cryptobinding
     *
     */
    int phase1;

    /**
     * phase2: Phase2 (inner authentication with TLS tunnel) parameters
     *     (string with field-value pairs, e.g., "auth=MSCHAPV2" for EAP-PEAP or
     *     "autheap=MSCHAPV2 autheap=MD5" for EAP-TTLS)
     */
    int phase2;

    char ca_cert[64];
    char client_cert[64];

    char private_key[64];
    char private_key_passwd[64];

    /**
     * freq_list: Array of allowed frequencies
     * Space-separated list of frequencies in MHz to allow for selecting the BSS. If
     * set, scan results that do not match any of the specified frequencies are not
     * considered when selecting a BSS.
     */
    char freq_list[64];
    char scan_freq[64];
    char bgscan[64];

};


char * inet_utos(unsigned long addr);
char *netmask_itos(int mask);
int netmask_utoi(unsigned long netmask);

int config_get_network(int interface, char* item, char* result);

/* ***************************************************
 * Configuration API
 * ***************************************************/
int Scf_getIfaddr(int interface, unsigned long* addr);
int Scf_setIfaddr(int interface, unsigned long addr);

int Scf_getNetmask(int interface, unsigned long* addr);
int Scf_setNetmask(int interface, unsigned long addr);

int Scf_getGateway(int interface, unsigned long* addr);
int Scf_setGateway(int interface, unsigned long addr);

int Scf_getIPConfig(int interface, int *config);
int Scf_setIPConfig(int interface, int config);

int Scf_getWlanConfig(int index, struct wpa_config *config);
int Scf_setWlanConfig(int index, struct wpa_config *config);

int Scf_cleanSecurity(struct wpa_config *config);


int Scf_getDNS(int index, unsigned long *ip);
int Scf_setDNS(int index, unsigned long ip);

int Scf_getNetworkType(void);
int Scf_setNetworkType(int mode);

int Scf_getReconnRule(void);
int Scf_setReconnRule(int mode);

int Scf_getTurboRoaming(void);
int Scf_setTurboRoaming(int enable);

int Scf_getExtDisconDetection(void);
int Scf_setExtDisconDetection(int enable);

int Scf_getScanChannel(int idx);
int Scf_setScanChannel(int idx, int channel);

int Scf_getProfileName(int idx, char* name);
int Scf_setProfileName(int idx, char *name);

int Scf_getProfileOPMode(int idx);
int Scf_setProfileOPMode(int idx, int mode);

int Scf_getProfilePriority(int idx);
int Scf_setProfilePriority(int idx, int priority);

int Scf_getAuthentication(int idx);
int Scf_setAuthentication(int idx, int auth, struct wpa_config* config);

int Scf_getEncryption(int idx);

int Scf_getWEPkeylen(int idx);
int Scf_setWEPkeylen(int idx, int len);

int Scf_getWEPkeyidx(int idx);
int Scf_setWEPkeyidx(int idx, int keyidx, struct wpa_config* config);

int Scf_getWEPKeyFormat(int idx);
int Scf_setWEPKeyFormat(int idx, int format);

int Scf_getEapMethod(int idx);

int Scf_getTunnAuth(int idx);

int Scf_getServerCert(int idx);
int Scf_setServerCert(int idx, int enable);

int Scf_setWEPKey(int idx, char wep_key[DCF_WLAN_WEP_KEY_NUM][DCF_WLAN_WEP_KEY_LEN]
    , int keylen, int format, struct wpa_config* config);

int Scf_getPrivate_key_passwd(int idx, char* passwd);
int Scf_setPrivate_key_passwd(int idx, char* passwd);

#define D_WLAN_CA_CERT              0x00000001L
#define D_WLAN_CLIENT_CERT          0x00000002L
#define D_WLAN_PRIVATE_KEY          0x00000004L
int Sys_CheckCertFileExist(int flag);
int Scf_setWLANCertFile(int idx, int flag, struct wpa_config* config);


void nwepgen(char *genstr, int keylen, char wep_key[DCF_WLAN_WEP_KEY_NUM][DCF_WLAN_WEP_KEY_LEN]);


/* string function */
char *next_word(char **buf);
char *next_word_with_space(char **buf);



/* ip tables */
int Scf_getIPTable(int idx, int *enable, unsigned long *ipaddr, unsigned long *netmask);
int Scf_setIPTable(int idx, int enable, unsigned long ipaddr, unsigned long netmask);

/* Gratuitous ARP */
int	Scf_getGratuitousArp_en(void);
int	Scf_setGratuitousArp_en(int enable);
int	Scf_getGratuitousArp_period(int *period);
int	Scf_setGratuitousArp_period(int period);

// SUPPORT_RTERMINAL_MODE
int Scf_getUserTable(int idx, char *username, char *password);
int Scf_setUserTable(int idx, char *username, char *password);

// SUPPORT_STATIC_GARP
int	Scf_getGratuitousArp_ip(int idx, char* ip, int len);
int	Scf_getGratuitousArp_mac(int idx, char* mac, int len);
int	Scf_setGratuitousArp_ip(int idx, char* ip, int len);
int	Scf_setGratuitousArp_mac(int idx, char* mac, int len);

int Scf_getRoamingThreshold(int *rssi);
int Scf_setRoamingThreshold(int rssi);
int Scf_getRoamingDifference(int *difference);
int Scf_setRoamingDifference(int difference);

#endif // __NET_CONFIG_H__
