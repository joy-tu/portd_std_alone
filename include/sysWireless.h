
#ifndef __SYS_WIRELESS_H__
#define __SYS_WIRELESS_H__

#define VAR_CHANNEL "/var/channel"

int sys_getWirelessInfo(char *ifname, struct wireless_info* info);
int sys_WLANGetChannel(struct wireless_info* info);
int sys_WLANGetEncryption(struct wireless_info* info, char* buf, int buflen);
int sys_WLANGetSignalLevel(int *level);
int sys_WLANGetSpeed(struct wireless_info* info, char* buf, int buflen);
int sys_WLANGetSignalRssi(int *rssi);

#endif // __SYS_WIRELESS_H__
