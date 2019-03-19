/*  libpwd.h
 *          
 *  Copyright c 2010-2011 Moxa Technologies Co., Ltd.
 *     
 *  Show dialog for user to set Communication parameter
 *  
 *  11/17/2010 1.0 first released.
 *  11/18/2010 1.1 bug fixed.
 *  11/22/2010 1.2 rename function modified.
 *  11/23/2010 1.3 option to authenticate without PAM
 *             
 *              
 */


#ifndef MY_PWD_H
#define MY_PWD_H

/* define USE_PAM if you want to use pam */
//#define USE_PAM

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <shadow.h>
#include <unistd.h>
#include <crypt.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#endif 

/* 
 * User should put the module file "check_user_auth" into 
 * /etc/pam.d/ to enable our setting of PAM.
 * Otherwise you can just use the default "other" module setting.
 */
#ifdef USE_PAM
#define AUTH_SERVICE_MODULE     "check_user_auth"
#endif

/* USE_MD5, USE_SHA256, USE_SHA512 */
#define USE_MD5
#define SHADOW_PATH "/etc/shadow"
#define SHADOW_PATH1 "/configData/etc/shadow"
#define TEMP_SHADOW_PATH "/etc/shadow+"
#define TEMP_SHADOW_PATH1 "/configData/etc/shadow+"



/* don't modify */
#define ID_MD5      "$1$"
#define ID_SHA256   "$5$"
#define ID_SHA512   "$6$"

#ifdef USE_MD5
#define CRYPT_ID ID_MD5
#endif
#ifdef USE_SHA256
#define CRYPT_ID ID_SHA256
#endif
#ifdef USE_SHA512
#define CRYPT_ID ID_SHA512
#endif

#define SALT_LEN        128
#define BUF_LEN         512

#define UPDATE_SUCCESS                  1
#define ERR_UNKNOW_UNAME                -1
#define ERR_DENIED_ACCESS               -2
#define ERR_NO_PW                       -3
#define ERR_CUR_PW                      -4
#define ERR_UPDATE_FILE_OPEN            -5
#define ERR_UPDATE_FILE_OPEN_SHADOW     -6
#define ERR_SYS_CP                      -7
#define ERR_SYS_MOUNT                   -8
#define ERR_SYS_UMOUNT                  -9
#define ERR_SYS_RM                      -10

#define AUTH_SUCCESS            1
#define ERR_PAM_START           -1
#define ERR_PAM_AUTH            -2
#define ERR_PAM_END             -3
#define ERR_PAM_ACC_UNHEALTH    -4
#define ERR_PAM_SET_ITEM        -5
#define ERR_PAM_PIPE            -6
#define ERR_PAM_FORK            -7
#define ERR_PAM_WAIT            -8


/* Lib Funciton*/
int smg_passwd(const char *uname, const char *oldpw, const char *newpw);
int smg_user_auth(const char *username, const char *pwd);



#endif
