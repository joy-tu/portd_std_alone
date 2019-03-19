#ifndef CFG_MTC_MISC_H
#define CFG_MTC_MISC_H

#include <stdlib.h>
#include <stdio.h>
#include <config.h>

#define MTC_CONF_MAX  32

#define MTC_ITEM_STR_LEN 32

#define defMsgMax 1024
#define MaxNumOfMsg ((32 * defMsgMax) * sizeof(struct mtc_msg_stru) - 1)

typedef struct _mtc_data_item {
	char id[MTC_ITEM_STR_LEN + 1];
	char name[MTC_ITEM_STR_LEN + 1];
	char type[MTC_ITEM_STR_LEN + 1];
	char subtype[MTC_ITEM_STR_LEN + 1];
	int cate;
	int device;
} mtc_data_item;

typedef struct _item_node {
	mtc_data_item     *item;
	struct _item_node *next;
} item_node;

typedef int (*fn)(mtc_data_item *,mtc_data_item *);

extern char *mtc_event_type;
extern char *mtc_condition_type;


#define UPPERCASE(tag, str) do { \
	int i; \
	for(i = 0; str[i] != 0; i++) { \
		(tag)[i] = toupper((str)[i]);} \
} while(0)

#define UPPERCASE_TRIM(tag, str) do { \
	int i, j; \
	for(i = 0, j = 0; i < strlen((str)); i++) { \
		if((str)[i] == '_') {continue;} \
		else {(tag)[j++] = toupper((str)[i]);} \
	} \
	(tag)[j] = 0; \
} while(0)

int cfg_mtc_get_device_by_id(char *tag_id);
int cfg_mtc_get_item_idx_by_id(char *item_id);
int cfg_mtc_get_id_by_device_and_name(char *item_id, int dev_id, char *item_name, int size);
int cfg_mtc_get_type_string(int idx, char *type_str, char *dst_str,int size);
int cfg_mtc_get_type_idx(char *key_str, char *type_str);
int cfg_mtc_get_unused_item_idx(void);
int cfg_mtc_is_used_device_id(int ignore_dev_num, char *id);
int cfg_mtc_is_used_device_name(int ignore_dev_num, char *name);

#if 1
int dump_item(mtc_data_item *item);
item_node *new_item_node(mtc_data_item *item);
int mtc_cmp(mtc_data_item *lhs, mtc_data_item *rhs);
int insert_item(item_node **in_head, mtc_data_item *item, fn cmpt);
int delete_item(item_node **in_head, int item_idx);
#endif
#endif
