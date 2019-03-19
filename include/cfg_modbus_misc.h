#ifndef CFG_MBUS_MISC_H
#define CFG_MBUS_MISC_H

#define MODBUS_DO 1
#define MODBUS_DI 1
#define MODBUS_COL (7-5)
#define DO_ROW 3
#define DI_ROW 5

#define MBUS_USED_ADDR_FILE "/var/tmp/mbus_used_address"
//#define MBUS_CFG_FILE "/etc/devsvr/cf_io_mbus.conf"
//#define MBUS_BACKUP_CFG_FILE "/etc/devsvr/b_cf_io_mbus.conf"

char *G_mbus_dsc[DO_ROW+DI_ROW];
char *modbus_config_name[DO_ROW+DI_ROW][MODBUS_COL];
	
int (*GScf_mbus_get[DO_ROW+DI_ROW][MODBUS_COL])(void);

int (*GScf_mbus_get_content[DO_ROW+DI_ROW])(int*, unsigned long*, int*, int*);
	
int (*GScf_mbus_set[DO_ROW+DI_ROW][MODBUS_COL])(int);

struct modbus_address{
int index;
unsigned long start;
unsigned long end;
};

struct modbus_info{
int start_addr;
int func_code;
};

int Scf_get_modbus_used_range(int index);
int Scf_set_modbus_single_info(int index, int info, int value);
int Scf_set_modbus_info(int index, int start_addr, int func_code);
int Scf_set_all_modbus_info(struct modbus_info *info, int size);
char *Scf_get_modbus_dsc(int index);
int Scf_get_default_modbus_total_ch(int index);
#endif