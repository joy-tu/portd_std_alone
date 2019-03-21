/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

/*
    confunc.h

    Routines to support console management

*/

#include	"console.h"
#include <stdlib.h>
#include <sysmaps.h>
#include <support.h>

#define WARN_GENERAL	1
#define WARN_NETWORK	2
#define WARN_SCM		4
#define WARN_PROFILE	8
#define WARN_PORT		16

#define STYLE_DEC	1
#define STYLE_HEX	2

/* conbasic.c */
int		con_basic(void);

/* confsubs.c */
int		config_loadstr(char *dstr, char *sstr, int len);
int		config_loadip(char *dstr, u_long ipaddr, int len);
int		config_savestr(char *sstr, char *dstr, int len);
int		config_getstr(char *dstr, char *sstr, int len);
int		config_hostinit(void);
int		config_hostname(int ndx, char *name);

/* console.c */
int		con_exit(void);
int		con_read(char *buf, int tout);
int		con_write(char *buf, int len);
int con_getright(void);
void	con_clearkey(void);
struct edit_item *con_find_first_item(struct edit_item *item, int first_x_plot, int first_y_plot);
void set_parameter_in_item(struct edit_item *item, char *str, char *title, int ndx);
int search_parameter_idx_from_items(struct edit_item *item, char *title,int max_idx);
int isNum(char* str);

/* conio.c */
void	conio_init(void);
int		conio_isconnected(void);
void	conio_disconnect(void);
void	conio_write(char *buf, int len);
void	conio_flush(void);
void	conio_reset_timeout(void);
int		conio_waitkey(int cmd, int tout);
int		conio_getch(void);
int 	conio_type(void);

/* conmain.c */
void	con_main(void);

/*	conterm.c	*/
void	con_attri(int attri);
void	con_clrscr(void);
void	con_clrtoeol(void);
void	con_clrtoeop(void);
void	con_dispstr(int x, int y, char *str);
void	con_dispstr1(int x, int y, char *str);
void	con_dispstr2(int x, int y, char *str, int len);
void 	con_dispitem(struct edit_item * itp);
int		con_getcsry(void);
int		con_getkey(void);
int		con_getkey_to(int to);
void	con_gotoxy(int x, int y);
void	con_putch(unsigned char ch);
void	con_puts(char *str);
void	con_savecursor(int *x, int *y);
void	con_scrn_restore(int x0, int y0, int x1, int y1);
void	con_scrn_save(int x0, int y0, char *text, int len);
void	con_scrn_store(int x0, int y0, int x1, int y1);
void	con_window_popup(struct edit_item * items, int itemno, char **menu, int x0, int y0, int x1, int y1, int x_data_ofst);
int		con_show_error(char *errstr, int y, int mode);
int		con_show_error2(char *str, int y);
void	con_show_line(int y);
void	con_show_region(int x0, int y0, int x1, int y1);
void	con_clear_region(int x0, int y0, int x1, int y1);
void	con_clear_line(int x, int y, int len);
int		con_show_warning(char *str, int y);
void	conterm_init(void);
int con_save_config(char *warning);
int con_save_config2(struct edit_item *items, struct edit_item *itemsb, int itemscnt, char *str, char *strb, int  slen, int flag);
int check_items(struct edit_item *items, struct edit_item *itemsb, int itemscnt, char *str, char *strb, int slen);
void con_clean_warning(void);
int check_str_style(char *str, int len, int style);

void con_show_pop_win(int x0, int y0, int x1, int y1);
void con_end_pop_win(int x0, int y0, int x1, int y1);

/* consubs.c */
void	con_putlong(long n);
int		get_ipaddr(u_char *str, u_char *ipaddr);
int		get_ipaddr2(u_char *str, u_char *ipaddr);
int		get_ipaddr3(u_char *str, u_char *ipaddr);
int		get_netmask(u_char *str, u_char *netmask, int mode);
int		Ssys_IPtostr(u_char *ip, char *str);
int		Ssys_space_str(u_char *str);
char 	*con_str_to_pwd(struct	edit_item	*items, char *pwdbuf, int buflen);
int		Ssys_strtoIP(u_char *str, u_long *ip);
int		Ssys_strtoIP_Zero(u_char *str, u_long *ip);
int		str_no_space_len(char *str, int len);
int		str_skip_end_space_len(char *str, int len);
int 		strtrim(char * dst, char * src);

/* conport.c */
int		con_line(void);
int		con_alias(void);
int		con_PortBuf(void);
int		con_mode(void);
void con_display_cipher(struct edit_item *itp, int start_item, int display_num);

/* conview.c */
int		con_view(void);

/* connetwork.c */
int con_net_general(void);
int con_net_ethernet(void);
int con_net_wlan(void);
int con_net_advance(void);
int con_net_wlan_log(void);

/* conprofile.c */
int		con_net_profile(void);
int		con_net_clear_sub_menu(void);
int		con_net_display_wep_menu(struct edit_item *current_items);
int		con_net_display_wpa_menu(struct edit_item *current_items);

/* consystem.c */
int		con_ipfilter(void);
int		con_host(void);
int		con_snmp(void);
#ifdef SUPPORT_RTERMINAL_MODE
int     con_user(void);
int     con_radius(void);
#endif
int		con_trap(void);
int		con_systemlog(void);
int		con_event(void);
int		con_portevent(void);
int		con_emailalert(void);
#ifdef SUPPORT_SERCMD
int		con_sercmd(void);
#endif
#ifdef SUPPORT_DIO
int		con_dio(void);
#endif
int		con_ping(void);
int		con_console(void);
int		con_default(void);
int		con_password(void);
int 	con_io_event(void);
int		con_sd_card_backup(void);

/* conload.c */
int		con_upgrade(void);
int		con_pre_shared_key(void);
int		con_export_conf(void);
int		con_import_conf(void);
int		con_import_eth_ssl(void);
int		con_import_wlan_ssl(void);
int		con_import_server_cert(void);
int		con_import_user_cert(void);
int		con_import_user_key(void);
int		con_erase_user_cert(void);

/* conmonitor.c */
int monitor_s2e(void);
int monitor_lstat(void);
#if defined (w2x50a) || defined (ia5x50aio)
int monitor_netstat(void);
#endif
int monitor_errcnt(void);
int monitor_lineconf(void);
int monitor_port_buffering(void);
int monitor_sockinfo(void);
int monitor_wlanstat(void);
int monitor_systemlog(void);
int monitor_dio(void);
int monitor_wlanlog(void);
int monitor_di(void);
int monitor_do(void);
int con_monitor_sys_alert(void);

/* conrestart.c */
int		con_restart_system(void);
int		con_restart_port(void);
void	con_do_reset(void);

/* conmodbus.c */
int	con_modbus(void);
int	con_default_modbus(void);

/* condio.c */
int con_di_setting(void);
int con_di_monitor(void);
int con_do_setting(void);
int con_do_monitor(void);
char *trim(char *str);
inline char *var2str(char *buff, int size, const char *fmt, ...);

struct sel_item	*sel_item_from_configmap(_config_map_t map);
void	drop_sel_item_from_configmap(struct sel_item *psi);
struct sel_item	*make_scan_channel_items(struct sel_item *psi);
