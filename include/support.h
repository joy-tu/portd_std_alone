#ifndef SUPPORT_H
#define SUPPORT_H

/*=================================================================
	Support flags by model.
=================================================================*/
#ifdef w1
	#define SUPPORT_WLAN
	#define NO_MODULE
	#define NO_PPPOE
	#define NO_ROUTING
	#define NO_DDNS
	#define NO_DOUT
	#define NO_LCM
	#define NO_SDCARD
	#define SUPPORT_HTTPS
	#define SUPPORT_SSH
	#define NO_RESET_BUTTON
	#define NO_LCM
	#define NO_SECURE_MODE
	#define NO_USERTABLE
	#define NO_TACAS
	#define NO_RADIUS
	#define NO_WINS
	#define NO_INUSE_LED
	#define NO_CUSTOM_MIB

	#define NO_IPLOCATE
	#define SUPPORT_SERCMD
	#define SUPPORT_DIO
	#define SUPPORT_DIOD
    #define SUPPORT_CONNECT_GOESDOWN

	#define SUPPORT_DISABLE_MODE
	#define SUPPORT_ASPP_MODE
	#define SUPPORT_RFC2217_MODE
	#define SUPPORT_TCP_SERVER_MODE
	#define SUPPORT_TCP_CLIENT_MODE
	#define SUPPORT_UDP_MODE
	#define NO_PAIRCONN_MODE
	#define NO_EMODEM_MODE
	#define NO_TERMINAL_MODE
	#define NO_RTERMINAL_MODE
	#define NO_PRINTER_MODE
	#define NO_DIALINOUT_MODE

	#define NO_RECONNECT_RULE
	#define NO_AUTO_SWITCH
	#define SUPPORT_TURBO_ROAMING

	#define NO_MULTI_PROFILE
	#define NO_VERIFY_SERVER

	#define NO_LAN_SPEED
	#define NO_WLAN_OP_MODE

#elif defined(w2x50a)
	#define SUPPORT_WLAN
	#define NO_MODULE
	#define NO_PPPOE
	#define NO_ROUTING
	#define NO_DDNS
	#define NO_DOUT
	#define NO_LCM
	#define NO_SDCARD
	#define SUPPORT_HTTPS
	#define SUPPORT_SSH
	#define SUPPORT_RESET_BUTTON
	#define NO_LCM
	#define NO_SECURE_MODE
	#define NO_USERTABLE
	#define NO_TACAS
	#define NO_RADIUS
	#define NO_WINS
	#define NO_INUSE_LED
	#define NO_CUSTOM_MIB

	#define NO_IPLOCATE
	#define NO_DIO		// perry modify
	#define NO_DIOD		// perry modify
    #define SUPPORT_CONNECT_GOESDOWN

	#define SUPPORT_DISABLE_MODE
	#define SUPPORT_ASPP_MODE
	#define SUPPORT_RFC2217_MODE
	#define SUPPORT_TCP_SERVER_MODE
	#define SUPPORT_TCP_CLIENT_MODE
	#define SUPPORT_UDP_MODE
	#define SUPPORT_PAIRCONN_MODE	// perry modify
	#define SUPPORT_EMODEM_MODE
	#define SUPPORT_RTERMINAL_MODE
	#define NO_TERMINAL_MODE
	//#define NO_RTERMINAL_MODE
	#define NO_PRINTER_MODE
	#define NO_DIALINOUT_MODE

	#define NO_RECONNECT_RULE
	#define NO_AUTO_SWITCH
	#define SUPPORT_TURBO_ROAMING
	#define SUPPORT_VERIFY_SERVER

	#define NO_MULTI_PROFILE
	#define NO_LAN_SPEED
	#define SUPPORT_WLAN_OP_MODE
	#define SUPPORT_BRIDGE
	#define SUPPORT_STATIC_GARP
	#define __MTC
#elif defined(ia5x50aio)
	#define SUPPORT_WLAN
	#define NO_MODULE
	#define NO_PPPOE
	#define NO_ROUTING
	#define NO_DDNS
	#define NO_DOUT
	#define NO_LCM
	#define NO_SDCARD
	#define SUPPORT_HTTPS
	#define SUPPORT_SSH
	#define SUPPORT_RESET_BUTTON
	#define NO_LCM
	#define NO_SECURE_MODE
	#define NO_USERTABLE
	#define NO_TACAS
	#define NO_RADIUS
	#define NO_WINS
	#define NO_INUSE_LED
	#define NO_CUSTOM_MIB

	#define NO_IPLOCATE
	#define NO_DIO		// perry modify
	#define NO_DIOD		// perry modify
    #define SUPPORT_CONNECT_GOESDOWN

	#define SUPPORT_DISABLE_MODE
	#define SUPPORT_ASPP_MODE
	#define SUPPORT_RFC2217_MODE
	#define SUPPORT_TCP_SERVER_MODE
	#define SUPPORT_TCP_CLIENT_MODE
	#define SUPPORT_UDP_MODE
	#define SUPPORT_PAIRCONN_MODE	// perry modify
	#define SUPPORT_EMODEM_MODE
	#define SUPPORT_RTERMINAL_MODE
	#define NO_TERMINAL_MODE
	//#define NO_RTERMINAL_MODE
	#define NO_PRINTER_MODE
	#define NO_DIALINOUT_MODE

	#define NO_RECONNECT_RULE
	#define NO_AUTO_SWITCH
	#define SUPPORT_TURBO_ROAMING
	#define SUPPORT_VERIFY_SERVER

	#define NO_MULTI_PROFILE
	#define NO_LAN_SPEED
	#define SUPPORT_WLAN_OP_MODE
	#define SUPPORT_BRIDGE
	#define SUPPORT_STATIC_GARP
	#define _NO_WLAN
	#define __MTC
#endif


/*=================================================================
	End of support flags by model.
=================================================================*/

#endif
