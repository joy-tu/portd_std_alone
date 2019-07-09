#ifndef __RIJNDAEL_ALG_H
#define __RIJNDAEL_ALG_H

#if 1// cybersecurity
#define MAXBC				8	//(256/32)
#define MAXKC				8	//(256/32)
#define MAXROUNDS			14	//(256/32) + 6

#define CFGBC				MAXBC
#define CFGKC				MAXKC
#define CFGROUNDS	MAXROUNDS
#else
#define MAXBC				4	//(128/32)
#define MAXKC				4	//(128/32)
#define MAXROUNDS			10	//(128/32) + 6
#endif

typedef unsigned char		word8;	
typedef unsigned short		word16;	
typedef unsigned long		word32;

int AESKeySched (word8 k[4][MAXKC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC]);
int AESEncrypt (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC]);
int AESEncryptRound (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC], int rounds);
int AESDecrypt (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC]);
int AESDecryptRound (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC], int rounds);
int NPortAESEncrypt (word8 data[16], word8 key[16]);
int NPortAESDecrypt (word8 data[16], word8 key[16]);

#if 1// cybersecurity
int CfgAESEncrypt (word8 data[32], word8 key[32]);
int CfgAESDecrypt (word8 data[32], word8 key[32]);
#endif

#endif /* __RIJNDAEL_ALG_H */
