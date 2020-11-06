/*  Copyright (C) MOXA Inc. All rights reserved.

    This software is distributed under the terms of the
    MOXA License.  See the file COPYING-MOXA for details.
*/

//#define TRACE_CODE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sio.h>
#include <datalog.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define	SIO_MAX_PORTS		2

#define DATA_LOG_MEM_SIZE_P   (64*1024)		/* each port 64K */

#define LOG_SHM_KEY			1090

#define SHMSZ				sizeof(dataLogBuf_t)
#define NUMSEMS				1	/* Num of sems in created sem set    */

/***********************
 * Data Log Definition *
 ***********************/
#define FNAME_LEN 16
/*
  Log Record Structure
  [len(4 bytes int)][tag(8 bytes)][data(1~n bytes)]
  len: length of (tag + data), not include len itself
  tag: [year(2)][mon(1)][day(1)][hour(1)][min(1)][sec(1)][dirction(1)]
*/
struct data_log_tag
{
  	unsigned short year;  /* real-year - 1900 */
  	char mon; /* 0 - 11 */  
  	char day;
  	char hour;
  	char min;
  	char sec;
  	char direction;
};

#define    DATA_LOG_LEN_FIELD_SIZE  sizeof(int) /* size of rec length field */
#define    DATA_LOG_TAG_LEN         sizeof(struct data_log_tag) /* timestamp(7) + T/R(1) */

typedef struct data_log_buf
{
    char   mem_buf[DATA_LOG_MEM_SIZE_P];    /* memory buffer */
    u_long mem_size;	/* memory buffer size */ 
    u_long mem_len;     /* buffer len */
    u_long mem_rstart;	/* buffer read start pointer */
    u_long mem_rndx;	/* buffer read index */
    u_long mem_wndx;	/* buffer write index */
} dataLogBuf_t;

#define LOG_CBUF_INC_READ(bp, len) \
  {bp->mem_rndx += len; \
   if (bp->mem_rndx >= bp->mem_size) \
      bp->mem_rndx -= bp->mem_size;}    

#define LOG_CBUF_INC_WRITE(bp, len) \
  {bp->mem_wndx += len; \
   if (bp->mem_wndx >= bp->mem_size) \
      bp->mem_wndx -= bp->mem_size;}      

static dataLogBuf_t *dataLogBufPtr[SIO_MAX_PORTS] = {(dataLogBuf_t *)-1, (dataLogBuf_t *)-1};
static char tmpbuf[256];

static int semWait(key_t semkey)
{
    int semid, rc;
    struct sembuf operations[2];

    /* Get the already created semaphore ID associated with key.     */
    /* If the semaphore set does not exist, then it will not be      */
    /* created, and an error will occur.                             */
    semid = semget(semkey, NUMSEMS, 0666);
    if ( semid == -1 )
    {
        perror("semget() failed 2");
        return -1;
    }

    /* First, check to see if the first semaphore is a zero.  If it  */
    /* is not, it is busy right now.  The semop() command will wait  */
    /* for the semaphore to reach zero before running the semop().   */
    /* When it is zero, increment the first semaphore to show that   */
    /* the shared memory segment is busy.                            */
    operations[0].sem_num = 0;		/* Operate on the first sem      */
    operations[0].sem_op =  0;		/* Wait for the value to be=0    */
    operations[0].sem_flg = 0;		/* Allow a wait to occur         */
    operations[1].sem_num = 0;		/* Operate on the first sem      */
    operations[1].sem_op =  1;		/* Increment the semval by one   */
    operations[1].sem_flg = 0;		/* Allow a wait to occur         */
    rc = semop( semid, operations, 2 );
    if (rc == -1)
    {
        perror("semop() failed\n");
        return -1;
    }

	return 0;
}

static int semPost(key_t semkey)
{
    int semid, rc;
    struct sembuf operations[1];

    /* Get the already created semaphore ID associated with key.     */
    /* If the semaphore set does not exist, then it will not be      */
    /* created, and an error will occur.                             */
    semid = semget(semkey, NUMSEMS, 0666);
    if ( semid == -1 )
    {
        perror("semget() failed 3");
        return -1;
    }

    /* Release the shared memory segment by decrementing the in-use  */
    /* semaphore (the first one).                                    */
    operations[0].sem_num = 0;		/* Operate on the first sem      */
    operations[0].sem_op = -1;		/* Decrement the semval by one   */
    operations[0].sem_flg = 0;		/* Allow a wait to occur         */
    rc = semop( semid, operations, 1 );
    if (rc == -1)
    {
        perror("semop() failed\n");
        return -1;
    }

	return 0;
}

static dataLogBuf_t *shmGet(int port)
{
    int shmid, pidx;
    key_t shmkey;
   
	if (!(1 <= port && port <=SIO_MAX_PORTS))
		return 0;

	pidx = port - 1;

	if (dataLogBufPtr[pidx] == (dataLogBuf_t *)-1)
	{
	    /*
	     * We'll name our shared memory segment
	     * LOG_SHM_KEY + port.
	     */
	    shmkey = LOG_SHM_KEY + port;
   
	    /* Get the already created shared memory ID associated with key. */
	    /* If the shared memory ID does not exist, then it will not be   */
	    /* created, and an error will occur.                             */
	    shmid = shmget(shmkey, SHMSZ, 0666);
	    if (shmid == -1)
	    {
	        perror("shmget() failed");
	        return (dataLogBuf_t *)-1;
	    }

	    /*
	     * Now we attach the segment to our data space.
	     */
	    if ((dataLogBufPtr[pidx] = (dataLogBuf_t *)shmat(shmid, NULL, 0)) == (dataLogBuf_t *) -1)
	    {
	        perror("shmat");
	        return (dataLogBuf_t *)-1;
	    }
	}

	return dataLogBufPtr[pidx];
}

/*
 * Called by portd
 */
void log_init(int port)
{
    int semid, shmid, rc, pidx;
    key_t key;
    short  sarray[NUMSEMS];

	if (!(1 <= port && port <=SIO_MAX_PORTS))
		return;

	pidx = port - 1;

    /*
     * We'll name our shared memory segment
     * LOG_SHM_KEY + port.
     */
    key = LOG_SHM_KEY + port;

    /* Create a semaphore set using the IPC key.  The number of      */
    /* semaphores in the set is one.  If a semaphore set already     */
    /* exists for the key, return an error. The specified permissions*/
    /* give everyone read/write access to the semaphore set.         */
    semid = semget( key, NUMSEMS, 0666 );
    if ( semid == -1 )
    {
        semid = semget( key, NUMSEMS, 0666 | IPC_CREAT | IPC_EXCL );
        if ( semid == -1 )
        {
            perror("semget() failed 1");
            return;
        }
    }

    /* Initialize the first semaphore in the set to 0                */
    /*                                                               */
    /* The first semaphore in the sem set means:                     */
    /*        '1' --  The shared memory segment is being used.       */
    /*        '0' --  The shared memory segment is freed.            */
    sarray[0] = 0;
    rc = semctl( semid, 0, SETALL, sarray);
    if(rc == -1)
    {
        perror("semctl() initialization failed");
        return;
    }

    /* Create a shared memory segment using the IPC key.  The        */
    /* size of the segment is a constant.  The specified permissions */
    /* give everyone read/write access to the shared memory segment. */
    /* If a shared memory segment already exists for this key,       */
    /* return an error.                                              */
    if ((shmid = shmget(key, SHMSZ, 0666)) < 0)
    {
        if ((shmid = shmget(key, SHMSZ, 0666 | IPC_CREAT | IPC_EXCL )) < 0)
        {
            perror("shmget");
            return;
        }
    }

    /*
     * Now we attach the segment to our data space.
     */
    if ((dataLogBufPtr[pidx] = (dataLogBuf_t *)shmat(shmid, NULL, 0)) == (dataLogBuf_t *) -1)
    {
        perror("shmat");
        return;
    }

	semWait(key);
	memset(dataLogBufPtr[pidx], 0, sizeof(dataLogBuf_t));
	dataLogBufPtr[pidx]->mem_size = DATA_LOG_MEM_SIZE_P;
//	dataLogBufPtr[pidx]->mem_buf = malloc(dataLogBufPtr[pidx]->mem_size);
	semPost(key);
}

int log_cbuf_read(dataLogBuf_t *bp, char *buf, int len)
{
  	int tail_len;
  
  	tail_len = bp->mem_size - bp->mem_rndx;
  	if (tail_len < len)
  	{ 
    	memcpy(buf, bp->mem_buf + bp->mem_rndx, tail_len);
    	memcpy(buf + tail_len, bp->mem_buf, len - tail_len);
  	}
  	else
    	memcpy(buf, bp->mem_buf + bp->mem_rndx, len);

  	LOG_CBUF_INC_READ(bp, len);

  	return len;    
}

void log_cbuf_write(dataLogBuf_t *bp, char *buf, int len)
{ 
  	int cbuf_free, tail_len, i, needed, rec_len, cnt; 

  	cbuf_free = bp->mem_size - bp->mem_len;
  	if (len > cbuf_free)
  	{    
    	needed = len - cbuf_free;
    	/* move rstart to next record which will not be overwritten by the new record*/
    	i = 0;
    	bp->mem_rndx = bp->mem_rstart;
    	cnt = 0;
    	do 
    	{      
      		log_cbuf_read(bp, (char*)&rec_len, DATA_LOG_LEN_FIELD_SIZE);
      		if ((rec_len < 0) || (rec_len > 2048) || (++cnt > 4*1024))
        		break;
      		LOG_CBUF_INC_READ(bp, rec_len); /* jump to next record */
      		i += DATA_LOG_LEN_FIELD_SIZE + rec_len;
    	} while (i < needed);    
    	bp->mem_len -= i;
    	bp->mem_rstart = bp->mem_rndx;
  	}

  	tail_len = bp->mem_size - bp->mem_wndx;

  	if (tail_len < len)
  	{    
	    memcpy(bp->mem_buf + bp->mem_wndx, buf, tail_len);
    	memcpy(bp->mem_buf, buf + tail_len, len - tail_len);   
  	}
  	else  
    	memcpy(bp->mem_buf + bp->mem_wndx, buf, len);

  	bp->mem_len += len;
    
  	LOG_CBUF_INC_WRITE(bp, len);
}    

//----------------------------------------------------------------
// Function Name: log_Port
// Function Desc:
//      Main function call to perform port data log. This function
//      will detect the relevant configuration of logging flag,
//      and do the logging if enabled. (both for SYSLOG, NFS, LOCAL)
//
//      ex.
//          char *msg = "hello world!!";
//          log_port(3, DIRECTION_T, msg, strlen(msg));
//
// Return Value:
//      0 -> Success;
//----------------------------------------------------------------
int log_port (int port, char direction, char *msg, int length)
{
  	int rec_len, pidx;
    time_t t;
    struct tm tm;
  	dataLogBuf_t *bp; 
  	struct data_log_tag tag;
    key_t key;

	if (!(1 <= port && port <=SIO_MAX_PORTS))
		return 0;

	pidx = port - 1;

  	if (length <= 0) return 0;

	bp = dataLogBufPtr[pidx];

  	if (bp == (dataLogBuf_t *)-1)
  	{
  		perror("Invalid dataLogBufPtr");
  		return 0;
  	}

    key = LOG_SHM_KEY + port;

	if (semWait(key) == -1)
		return 0;

  	/* write len */
  	rec_len = DATA_LOG_TAG_LEN + length;  
  	log_cbuf_write(bp, (char*)&rec_len, DATA_LOG_LEN_FIELD_SIZE);
  	/* write tag */
  	t = time(0);
    gmtime_r(&t, &tm);
  	tag.year = (unsigned short)tm.tm_year + 1900;
  	tag.mon = (char)tm.tm_mon;
  	tag.day = (char)tm.tm_mday;
  	tag.hour = (char)tm.tm_hour;
  	tag.min = (char)tm.tm_min;
  	tag.sec = (char)tm.tm_sec;
  	tag.direction = direction;    
  	log_cbuf_write(bp, (char*)&tag, DATA_LOG_TAG_LEN);
  	/* write data */
  	log_cbuf_write(bp, msg, length);

  	semPost(key);

  	return length;
}

int	log_sio_write(int port, char *buf, int len)
{
	int n;
	n = sio_write(port, buf, len);
	if (n > 0)
		log_port(port, 'T', buf, n);
	return n;
}

int	log_sio_read(int port, char *buf, int len)
{
	int n;
	n = sio_read(port, buf, len);
	if (n > 0)
		log_port(port, 'R', buf, n);
	return n; 
}

/*******************************
 * PARAMETERS:
 *    port:   SIO port number 1 ~ SIO_MAX_PORTS
 *    buf:    buffer to store read data
 *    size:   buffer size
 * RETURN VALUE:
 *    bytes of data successfully read
 *    0 is returned when error or EOF
 *******************************/
int log_read_data(int port, char *buf, int size, int hex_mode)
{
  	dataLogBuf_t *bp; 
  	int idx, rec_len, mem_len, i, str_len;
  	char ch, chstr[3];
  	struct data_log_tag tag;
    key_t key;

	bp = shmGet(port);
	if (bp == (dataLogBuf_t *)-1)
		return 0;

    key = LOG_SHM_KEY + port;

	if (semWait(key) == -1)
		return 0;

  	idx = 0;
  	bp->mem_rndx = bp->mem_rstart;
  	mem_len = bp->mem_len;
  	while (1)
  	{
    	if (mem_len <= 0)
      		break;      
    	/* read record length */
    	log_cbuf_read(bp, (char*)&rec_len, DATA_LOG_LEN_FIELD_SIZE);
    	/* read record tag */
    	log_cbuf_read(bp, (char*)&tag, DATA_LOG_TAG_LEN);        
    	sprintf(tmpbuf, "%04d/%02d/%02d %02d:%02d:%02d[%c] ", 
        	  	tag.year, tag.mon + 1, tag.day, 
          		tag.hour, tag.min, tag.sec, tag.direction);
    	str_len = strlen(tmpbuf);
    	if (((idx + str_len + (rec_len - DATA_LOG_TAG_LEN) + 1/*\n*/) > size) ||
        	(rec_len <= DATA_LOG_TAG_LEN))
      		break;     
    	memcpy(buf + idx, tmpbuf, str_len);      
    	idx += str_len;   
    	/* read record data */
    	if (hex_mode)
    	{
      		for (i = 0; i < rec_len - DATA_LOG_TAG_LEN; i++) 
      		{               
        		ch = bp->mem_buf[bp->mem_rndx];
        		LOG_CBUF_INC_READ(bp, 1);
        		sprintf(chstr, "%02X", ch);
        		memcpy(buf+idx, chstr, 2);
        		idx += 2;
        		buf[idx++] = ' ';
      		}
    	}
    	else
    	{
      		log_cbuf_read(bp, buf + idx, rec_len - DATA_LOG_TAG_LEN);
      		idx += rec_len - DATA_LOG_TAG_LEN;
    	}  
    	//buf[idx++] = '\r';
    	buf[idx++] = '\n';
    	mem_len -= rec_len + DATA_LOG_LEN_FIELD_SIZE/*length of rec_len */;
  	}

	semPost(key);
  
  	return idx;
}

int log_clear_data(int port)
{
  	dataLogBuf_t *bp; 
    key_t key;
        
	bp = shmGet(port);
	if (bp == (dataLogBuf_t *)-1)
		return -1;

    key = LOG_SHM_KEY + port;

	if (semWait(key) == -1)
		return -1;

  	bp->mem_len = 0;
  	bp->mem_rstart = 0;
  	bp->mem_rndx = 0;
  	bp->mem_wndx = 0;

	semPost(key);
  	
  	return 0;
}

#include <delimit.h>

static fdkparam_t Gdktab[SIO_MAX_PORTS] = {(fdkparam_t)-1, (fdkparam_t)-1};

long port_buffering_len(int port)
{
    key_t shmkey;
    int shmid, pidx;

	if (!(1 <= port && port <=SIO_MAX_PORTS))
		return 0;

	pidx = port - 1;

	if (Gdktab[pidx] == (fdkparam_t)-1)
	{
	    /*
	     * We'll name our shared memory segment
	     * LOG_SHM_KEY + port.
	     */
	    shmkey = DK_SHM_KEY + port;
    
	    /* Get the already created shared memory ID associated with key. */
	    /* If the shared memory ID does not exist, then it will not be   */
	    /* created, and an error will occur.                             */
	    shmid = shmget(shmkey, sizeof(dkparam), 0666);
	    if (shmid == -1)
	    {
	        perror("shmget() failed");
	        return 0;
	    }

	    /*
	     * Now we attach the segment to our data space.
	     */
	    if ((Gdktab[pidx] = (fdkparam_t)shmat(shmid, NULL, 0)) == (fdkparam_t) -1)
	    {
	        perror("shmat");
	        return 0;
	    }
	}

	if(Gdktab[pidx]->flag&DK_CAN_BUFFER)
	{
		return Gdktab[pidx]->s2e_len;
	}
	
	return 0;
}
