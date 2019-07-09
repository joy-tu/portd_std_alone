/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/
/*
    ipc_cfg_api.c

    ipc_config would be clear when rebooting the system.

    2010-12-30	James Wang
		developing...
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sem.h>
#include <debug.h>
#include <header.h>
#include <config.h>
#include "ipc_cfg_api.h"

static void _get_filename(char* item, char* file);
static void _remove_comment(char* str);
static void _trim_left_space(char* line);


// FUNCTION HEADER ----------------------------------------------------------
//
// FUNCTION NAME: config_get
// FUNCTION DESCRIPTION
//   Parameters:
//   Return valie:
//       CFG_OK  0
//       CFG_ITEM_NOT_FOUND      -1
//       CFG_FILE_NOT_FOUND      -2
//       CFG_BUFFER_IS_NULL      -3
//       CFG_BUFFER_TOO_SMALL    -4
//       CFG_INVALID_PARAMETER   -5
// --------------------------------------------------------------------------

int ipc_config_get(char* item, int data_type, void* buf, int size)
{
    char fname[2][64], cfg_file[64];
    char line[128];
    FILE *fp, *fpw;
    char *token = NULL;
    int val, i;
	int gettimes;

    if (strlen(item) < 4)
        return CFG_ITEM_NOT_FOUND;

    if (!buf)
        return CFG_BUFFER_IS_NULL;

    _get_filename(item, cfg_file);
    if (strlen(cfg_file) == 0)
        return CFG_ITEM_NOT_FOUND;

	sprintf(fname[0], "%s/%s", IPC_CONFIG_FILEPATH, cfg_file);
//	sprintf(fname[1], "%s/%s", CONFIG_DEFAULTFILEPATH, cfg_file);
		
	for(gettimes = 0; gettimes < 1; gettimes++) {
		fp = fopen(fname[gettimes], "r");
	    if (fp == NULL)
	        return CFG_FILE_NOT_FOUND;

	    while(fgets(line, sizeof(line), fp))
	    {
			
			_remove_comment(line);
	        _trim_left_space(line);
	        for (i=0; i<strlen(line); i++)
	        {
	            if (line[i]==' ' || line[i]=='\t' || line[i]=='\n' || line[i]=='\r')
	                break;
	        }

	        if (!i)
	            continue;

			if (strlen(item)==i && !strncmp(line, item, i))
	        {
	            token = NULL;
	            for (i=strlen(item); i<strlen(line); i++)
	            {
	                if (!token && line[i]!='\t' && line[i]!=' ' && line[i]!='\n' && line[i]!='\r')
	                    token = line+i;

	                if (line[i] == '\r' || line[i] == '\n')
	                    line[i] = 0;
	            }
	            break;
	        }
	    }
	
		if (feof(fp))
		{
			fclose(fp);
			if(gettimes) {
		    	return CFG_ITEM_NOT_FOUND;
			}
		}
		else
		{
			if(gettimes) {			
				fpw = fopen(fname[0], "a");
	    		if (fpw == NULL)
	        		return CFG_FILE_NOT_FOUND;
				//fseek(fp, 0, SEEK_END);
				fputs(line, fpw);
				fputc('\n', fpw);
				fclose(fpw);
			}
			else {
				break;
			}
		}
	}

	if (!token)
        val = 0;
    else
        val = atoi(token);

    switch (data_type)
    {
        case DATATYPE_INT32S:
            *((INT32S*)buf) = val;
            break;

        case DATATYPE_INT32U:
            *((INT32U*)buf) = val;
            break;

        case DATATYPE_INT16U:
            *((INT16U*)buf) = val;
            break;

        case DATATYPE_INT16S:
            *((INT16S*)buf) = val;
            break;

        case DATATYPE_INT8U:
            *((INT8U*)buf) = val;
            break;

        case DATATYPE_INT8S:
            *((INT8S*)buf) = val;
            break;

        case DATATYPE_STRING:
            if (!token)
                strcpy(buf, "");
            else
            {
                if (size <= strlen(token))
                {
                    fclose(fp);
                    return CFG_BUFFER_TOO_SMALL;
                }
                //ptr = strstr(line, token);
                //strcpy(buf, ptr);
                strcpy(buf, token);
            }
            break;

        default:
            fclose(fp);
            return CFG_INVALID_PARAMETER;
    }
    fclose(fp);
    return CFG_OK;
}


// FUNCTION HEADER ----------------------------------------------------------
//
// FUNCTION NAME: config_set
// FUNCTION DESCRIPTION
//   Parameters:
//   Return valie:
//       CFG_OK  0
//       CFG_ITEM_NOT_FOUND      -1
//       CFG_FILE_NOT_FOUND      -2
//       CFG_BUFFER_IS_NULL      -3
//       CFG_INVALID_PARAMETER   -5
// --------------------------------------------------------------------------
int ipc_config_set(char* item, int data_type, void* buf, int len)
{
    char fname[64], fname_tmp[64], cfg_file[32];
    char line[128];
    FILE *fpr, *fpw;
    int i, semset_id, found=0;
    key_t key;
	
    if (strlen(item) < 4)
        return CFG_ITEM_NOT_FOUND;
    if (!buf)
        return CFG_BUFFER_IS_NULL;
    _get_filename(item, cfg_file);
    if (strlen(cfg_file) == 0)
        return CFG_ITEM_NOT_FOUND;
    /* Semphore protection */
    sprintf(fname_tmp, "%s/%s", CONFIG_IPC_PATH, cfg_file);
    if ((key=ftok(fname_tmp, 's')) == -1)
    {
        ipc_create_file(fname_tmp);
        key = ftok(fname_tmp, 's');
        ipc_open_sem(&semset_id, key);
        semctl(semset_id, 0, IPC_RMID, 0);
        ipc_create_sem(&semset_id, key, 1);
    }
    if (ipc_open_sem(&semset_id, key) == -1)
    {
        ipc_create_sem(&semset_id, key, 1);
        ipc_open_sem(&semset_id, key);
    }
    for (;;)
    {
        if (ipc_sem_down(semset_id, 0) == -1)
        {
            usleep(1000);
            continue;
        }
        break;
    }
    sprintf(fname, "%s/%s", IPC_CONFIG_FILEPATH, cfg_file);
    fpr = fopen(fname, "r");
    if (fpr == NULL)
    {
    
        dbg_printf("open file(r) failed\n");
        ipc_sem_up(semset_id, 0);
        return CFG_FILE_NOT_FOUND;
    }
    sprintf(fname_tmp, "%s.tmp", fname);

    fpw = fopen(fname_tmp, "w");
    if (fpw == NULL)
    {
        fclose(fpr);
        dbg_printf("open file(w) failed\n");
        ipc_sem_up(semset_id, 0);
        return CFG_FILE_NOT_FOUND;
    }
    while (fgets(line, sizeof(line), fpr))
    {
		_trim_left_space(line);
        if (line[0]=='#' || line[0]=='\r' || line[0]=='\n')	// comment
        {
            fputs(line, fpw);
            continue;
        }
        for (i=0; i<strlen(line); i++)
        {
            if (line[i]==' ' || line[i]=='\t' || line[i]=='\n' || line[i]=='\r')
                break;
        }
        if (!i)
            continue;

        if (strlen(item)==i && !strncmp(line, item, i))
        {
            found = 1;
            line[i] = '\t';
            switch (data_type)
            {
                case DATATYPE_INT32S:
                    sprintf(line, "%s\t%d\n", item, *((INT32S*)buf));
                    break;
                case DATATYPE_INT32U:
                    sprintf(line, "%s\t%d\n", item, *((INT32U*)buf));
                    break;
                case DATATYPE_INT16U:
                    sprintf(line, "%s\t%d\n", item, *((INT16U*)buf));
                    break;
                case DATATYPE_INT16S:
                    sprintf(line, "%s\t%d\n", item, *((INT16S*)buf));
                    break;
                case DATATYPE_INT8U:
                    sprintf(line, "%s\t%d\n", item, *((INT8U*)buf));
                    break;
                case DATATYPE_INT8S:
                    sprintf(line, "%s\t%d\n", item, *((INT8S*)buf));
                    break;
                case DATATYPE_STRING:
                    memset(line, 0, sizeof(line));
                    sprintf(line, "%s\t", item);
                    memcpy(line+strlen(item)+1, (char*)buf, len);
                    line[strlen(line)] = '\n';
                    break;
                default:
                    dbg_printf("Invalid data_type\n");
                    continue;
            }
        }
        fputs(line, fpw);
    }
    fclose(fpr);
    fclose(fpw);
    if (!found)
    {
        remove(fname_tmp);
        ipc_sem_up(semset_id, 0);
        return CFG_ITEM_NOT_FOUND;
    }
    remove(fname);
    rename(fname_tmp, fname);
    //system("sync");
    ipc_sem_up(semset_id, 0);
    return CFG_OK;
}

static void _remove_comment(char* str)
{
    int len, i;
    len = strlen(str);
    i=0;

    while (len && str[i])
    {
        if (str[i] == '#')
        {
            str[i] = 0;
            break;
        }
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n' && str[i] != '\r') //vince
            break;
        i++;
    }
    return;
}

static void _trim_left_space(char* line)
{
    int i, j, len;
    len = strlen(line);

    if (!len)
        return;

    i=0;
    while(line[i] == ' ')
        i++;
    if (i)
    {
        for (j=0; j<len; j++, i++)
            line[j] = line[i];

        line[j] = 0;
    }
    return;
}

static void _get_filename(char* item, char* file)
{
#if 1
	if (!strncmp(item, "ipc", 3))
		sprintf(file, "%s", IPC_CONFIG_FILE_IO);
#else
    if (!strncmp(item, "syst", 4))
        sprintf(file, "%s", CONFIG_FILE_SYSTEM);

    else if (!strncmp(item, "port", 4))
        sprintf(file, "%s", CONFIG_FILE_PORT);

    else if (!strncmp(item, "misc", 4))
        sprintf(file, "%s", CONFIG_FILE_MISC);

    else if (!strncmp(item, "warn", 4) || !strncmp(item, "mail", 4))
        sprintf(file, "%s", CONFIG_FILE_WARN);

    else if(strncmp(item, "wireless", 8) == 0)
        sprintf(file, CONFIG_FILE_WIRELESS);

    else if (strncmp(item, "io_", 3) == 0)
	 sprintf(file, CONFIG_FILE_IO);
#endif	
    return;
}


int ipc_create_sem(int *sid, key_t key, int members)
{
    int cntr;
    union semun semopts;

    if((*sid = semget(key, members, IPC_CREAT|IPC_EXCL|0666)) == -1)
    {
#ifdef DEBUG_W1
        perror("semget(create)");
#endif
        return -1;
    }

    semopts.val = 1;
    // Initialize all members (could be done with SETALL)
    for(cntr=0; cntr<members; cntr++)
    {
        semctl(*sid, cntr, SETVAL, semopts);
    }

    return 0;
}

int ipc_open_sem(int *sid, key_t key)
{
    /* Open the semaphore set - do not create! */

    if((*sid = semget(key, 0, 0666)) == -1)
    {
#ifdef DEBUG_W1
        perror("semget(open)");
#endif
        return(-1);
    }
    return 0;
}



int ipc_sem_up(int semid, int member)
{
    struct sembuf sem_lock={ member, 1, IPC_NOWAIT};
    if((semop(semid, &sem_lock, 1)) == -1)
    {
#ifdef DEBUG_W1
        perror("semop(up)");
#endif
        return -1;
    }
    else
	return 0;
}


int ipc_sem_down(int semid, int member)
{
    struct  sembuf sem_lock= {member, -1, IPC_NOWAIT};

    if((semop(semid, &sem_lock, 1)) == -1)
    {
#ifdef DEBUG_W1
        perror("semop(down)");
#endif
        return -1;
    }
    else
        return 0;
}

int ipc_create_file(char *path)
{
    int i;
    char tmpstr[50], name[50];

    memset(tmpstr, '\0', 50);
    memset(name,   '\0', 50);
    for (i=strlen(path)-1; i>=0; i--)
    {
        if (path[i] == '/')
        memcpy(name, path, i);
    }
    sprintf(tmpstr, "mkdir -p %s", name);
    system(tmpstr);
    memset(tmpstr, '\0', 50);
    strcpy(name, path);
    sprintf(tmpstr, "touch %s", name);
    system(tmpstr);

    return 0;
}

