

#ifndef __SNMPTRAP_H__
#define __SNMPTRAP_H__

#define D_TRAP_SYSTEM_DESC_EXTOID   ".1.3.6.1.2.1.1.1.0"

struct __autowarn_msg {
    int type;
    int version;
    char server[41];
    char community[41];
    char ext_oid[41];
    char ext_string[80];
};
typedef struct __autowarn_msg autowarn_msg;
typedef struct __autowarn_msg * autowarn_msg_t;


int send_trap(int flag, int version, char* server, char* community);
int send_trap_ex(int flag, int version, char* server, char* community, char* ext_oid, char* ext_string);
int process_trap(autowarn_msg_t msg);

#endif // __SNMPTRAP_H__
