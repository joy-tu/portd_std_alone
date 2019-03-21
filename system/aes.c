/* rijndael-alg-ref.c   v2.2   March 2002
 * Reference ANSI C code
 * authors: Paulo Barreto
 *          Vincent Rijmen
 *
 * This code is placed in the public domain.
 */


#include "aes.h"

#define SC	((BC - 4) >> 1)

#include "boxes.h"

static word8 shifts[3][4][2] = {
   {{0, 0}, {1, 3}, {2, 2}, {3, 1}},
   {{0, 0},	{1, 5},	{2, 4},	{3, 3}},
   {{0, 0},	{1, 7},	{3, 5},	{4, 4}}
}; 


word8 mul(word8 a, word8 b) {
   /* multiply two elements of GF(2^m)
    * needed for MixColumn and InvMixColumn
    */
	if (a && b) return Alogtable[(Logtable[a] + Logtable[b])%255];
	else return 0;
}

void AddRoundKey(word8 a[4][MAXBC], word8 rk[4][MAXBC], word8 BC) {
	/* Exor corresponding text input and round key input bytes
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
   		for(j = 0; j < BC; j++) a[i][j] ^= rk[i][j];
}

void ShiftRows(word8 a[4][MAXBC], word8 d, word8 BC) {
	/* Row 0 remains unchanged
	 * The other three rows are shifted a variable amount
	 */
	word8 tmp[MAXBC];
	int i, j;
	
	for(i = 1; i < 4; i++) {
		for(j = 0; j < BC; j++) 
                	tmp[j] = a[i][(j + shifts[SC][i][d]) % BC];
		for(j = 0; j < BC; j++) a[i][j] = tmp[j];
	}
}

void Substitution(word8 a[4][MAXBC], word8 box[256], word8 BC) {
	/* Replace every byte of the input by the byte at that place
	 * in the nonlinear S-box.
         * This routine implements SubBytes and InvSubBytes
	 */
	int i, j;
	
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = box[a[i][j]] ;
}
   
void MixColumns(word8 a[4][MAXBC], word8 BC) {
        /* Mix the four bytes of every column in a linear way
	 */
	word8 b[4][MAXBC];
	int i, j;
		
	for(j = 0; j < BC; j++)
		for(i = 0; i < 4; i++)
			b[i][j] = mul(2,a[i][j])
				^ mul(3,a[(i + 1) % 4][j])
				^ a[(i + 2) % 4][j]
				^ a[(i + 3) % 4][j];
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = b[i][j];
}

void InvMixColumns(word8 a[4][MAXBC], word8 BC) {
        /* Mix the four bytes of every column in a linear way
	 * This is the opposite operation of Mixcolumns
	 */
	word8 b[4][MAXBC];
	int i, j;
	
	for(j = 0; j < BC; j++)
	for(i = 0; i < 4; i++)             
		b[i][j] = mul(0xe,a[i][j])
			^ mul(0xb,a[(i + 1) % 4][j])                 
			^ mul(0xd,a[(i + 2) % 4][j])
			^ mul(0x9,a[(i + 3) % 4][j]);                        
	for(i = 0; i < 4; i++)
		for(j = 0; j < BC; j++) a[i][j] = b[i][j];
}

int AESKeySched (word8 k[4][MAXKC], int keyBits, int blockBits, 
		word8 W[MAXROUNDS+1][4][MAXBC]) {
	/* Calculate the necessary round keys
	 * The number of calculations depends on keyBits and blockBits
	 */
	int KC, BC, ROUNDS;
	int i, j, t, rconpointer = 0;
	word8 tk[4][MAXKC];   

	switch (keyBits) {
	case 128: KC = 4; break;
	case 192: KC = 6; break;
	case 256: KC = 8; break;
	default : return (-1);
	}

	switch (blockBits) {
	case 128: BC = 4; break;
	case 192: BC = 6; break;
	case 256: BC = 8; break;
	default : return (-2);
	}

	switch (keyBits >= blockBits ? keyBits : blockBits) {
	case 128: ROUNDS = 10; break;
	case 192: ROUNDS = 12; break;
	case 256: ROUNDS = 14; break;
	default : return (-3); /* this cannot happen */
	}

	
	for(j = 0; j < KC; j++)
		for(i = 0; i < 4; i++)
			tk[i][j] = k[i][j];
	t = 0;
	/* copy values into round key array */
	for(j = 0; (j < KC) && (t < (ROUNDS+1)*BC); j++, t++)
		for(i = 0; i < 4; i++) W[t / BC][i][t % BC] = tk[i][j];
		
	while (t < (ROUNDS+1)*BC) { 
        	/* while not enough round key material calculated */
		/* calculate new values */
		for(i = 0; i < 4; i++)
			tk[i][0] ^= S[tk[(i+1)%4][KC-1]];
		tk[0][0] ^= rcon[rconpointer++];

		if (KC != 8)
			for(j = 1; j < KC; j++)
				for(i = 0; i < 4; i++) tk[i][j] ^= tk[i][j-1];
		else {
			for(j = 1; j < KC/2; j++)
				for(i = 0; i < 4; i++) tk[i][j] ^= tk[i][j-1];
			for(i = 0; i < 4; i++) 
                        	tk[i][KC/2] ^= S[tk[i][KC/2 - 1]];
			for(j = KC/2 + 1; j < KC; j++)
				for(i = 0; i < 4; i++) tk[i][j] ^= tk[i][j-1];
	}
	/* copy values into round key array */
	for(j = 0; (j < KC) && (t < (ROUNDS+1)*BC); j++, t++)
		for(i = 0; i < 4; i++) W[t / BC][i][t % BC] = tk[i][j];
	}		

	return 0;
}

int AESEncrypt (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC])
{
	/* Encryption of one block. 
	 */
	int r, ROUNDS;
	word8 BC;

	switch (blockBits) {
	case 128: BC = 4; break;
	case 192: BC = 6; break;
	case 256: BC = 8; break;
	default : return (-2);
	}

	switch (keyBits >= blockBits ? keyBits : blockBits) {
	case 128: ROUNDS = 10; break;
	case 192: ROUNDS = 12; break;
	case 256: ROUNDS = 14; break;
	default : return (-3); /* this cannot happen */
	}

	/* begin with a key addition
	 */
	AddRoundKey(a,rk[0],BC); 

        /* ROUNDS-1 ordinary rounds
	 */
	for(r = 1; r < ROUNDS; r++) {
		Substitution(a,S,BC);
		ShiftRows(a,0,BC);
		MixColumns(a,BC);
		AddRoundKey(a,rk[r],BC);
	}
	
	/* Last round is special: there is no MixColumns
	 */
	Substitution(a,S,BC);
	ShiftRows(a,0,BC);
	AddRoundKey(a,rk[ROUNDS],BC);

	return 0;
}   


#if 0
int AESEncryptRound (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC], int rounds)
/* Encrypt only a certain number of rounds.
 * Only used in the Intermediate Value Known Answer Test.
 */
{
	int r, ROUNDS;
	word8 BC;

	switch (blockBits) {
	case 128: BC = 4; break;
	case 192: BC = 6; break;
	case 256: BC = 8; break;
	default : return (-2);
	}

	switch (keyBits >= blockBits ? keyBits : blockBits) {
	case 128: ROUNDS = 10; break;
	case 192: ROUNDS = 12; break;
	case 256: ROUNDS = 14; break;
	default : return (-3); /* this cannot happen */
	}

	/* make number of rounds sane */
	if (rounds > ROUNDS) rounds = ROUNDS;

	/* begin with a key addition
	 */
	AddRoundKey(a,rk[0],BC);
        
	/* at most ROUNDS-1 ordinary rounds
	 */
	for(r = 1; (r <= rounds) && (r < ROUNDS); r++) {
		Substitution(a,S,BC);
		ShiftRows(a,0,BC);
		MixColumns(a,BC);
		AddRoundKey(a,rk[r],BC);
	}
	
	/* if necessary, do the last, special, round: 
	 */
	if (rounds == ROUNDS) {
		Substitution(a,S,BC);
		ShiftRows(a,0,BC);
		AddRoundKey(a,rk[ROUNDS],BC);
	}

	return 0;
}   
#endif

int AESDecrypt (word8 a[4][MAXBC], int keyBits, int blockBits, 
		word8 rk[MAXROUNDS+1][4][MAXBC])
{
	int r, ROUNDS;
	word8 BC;
	
	switch (blockBits) {
	case 128: BC = 4; break;
	case 192: BC = 6; break;
	case 256: BC = 8; break;
	default : return (-2);
	}

	switch (keyBits >= blockBits ? keyBits : blockBits) {
	case 128: ROUNDS = 10; break;
	case 192: ROUNDS = 12; break;
	case 256: ROUNDS = 14; break;
	default : return (-3); /* this cannot happen */
	}

	/* To decrypt: apply the inverse operations of the encrypt routine,
	 *             in opposite order
	 * 
	 * (AddRoundKey is an involution: it 's equal to its inverse)
	 * (the inverse of Substitution with table S is Substitution with 
         *           the inverse table of S)
	 * (the inverse of ShiftRows is ShiftRows over a suitable distance)
	 */

        /* First the special round:
	 *   without InvMixColumns
	 *   with extra AddRoundKey
	 */
	AddRoundKey(a,rk[ROUNDS],BC);
	Substitution(a,Si,BC);
	ShiftRows(a,1,BC);              
	
	/* ROUNDS-1 ordinary rounds
	 */
	for(r = ROUNDS-1; r > 0; r--) {
		AddRoundKey(a,rk[r],BC);
		InvMixColumns(a,BC);      
		Substitution(a,Si,BC);
		ShiftRows(a,1,BC);                
	}
	
	/* End with the extra key addition
	 */
	
	AddRoundKey(a,rk[0],BC);    

	return 0;
}

#if 0
int AESDecryptRound (word8 a[4][MAXBC], int keyBits, int blockBits, 
	word8 rk[MAXROUNDS+1][4][MAXBC], int rounds)
/* Decrypt only a certain number of rounds.
 * Only used in the Intermediate Value Known Answer Test.
 * Operations rearranged such that the intermediate values
 * of decryption correspond with the intermediate values
 * of encryption.
 */
{
	int r, ROUNDS;
	word8 BC;
	
	switch (blockBits) {
	case 128: BC = 4; break;
	case 192: BC = 6; break;
	case 256: BC = 8; break;
	default : return (-2);
	}

	switch (keyBits >= blockBits ? keyBits : blockBits) {
	case 128: ROUNDS = 10; break;
	case 192: ROUNDS = 12; break;
	case 256: ROUNDS = 14; break;
	default : return (-3); /* this cannot happen */
	}


	/* make number of rounds sane */
	if (rounds > ROUNDS) rounds = ROUNDS;

        /* First the special round:
	 *   without InvMixColumns
	 *   with extra AddRoundKey
	 */
	AddRoundKey(a,rk[ROUNDS],BC);
	Substitution(a,Si,BC);
	ShiftRows(a,1,BC);              
	
	/* ROUNDS-1 ordinary rounds
	 */
	for(r = ROUNDS-1; r > rounds; r--) {
		AddRoundKey(a,rk[r],BC);
		InvMixColumns(a,BC);      
		Substitution(a,Si,BC);
		ShiftRows(a,1,BC);                
	}
	
	if (rounds == 0) {
		/* End with the extra key addition
		 */	
		AddRoundKey(a,rk[0],BC);
	}    

	return 0;
}
#endif
int NPortAESEncrypt (word8 data[16], word8 key[16])
{
	int i, j, ret;
	word8 a[4][MAXKC], k[4][MAXKC], rk[MAXROUNDS+1][4][MAXBC];
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			a[i][j] = data[i * 4 + j];
			k[i][j] = key[i * 4 + j];
		}
	}
	
	ret = AESKeySched(k, 128, 128, rk);
	ret = AESEncrypt(a, 128, 128, rk);

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			data[i * 4 + j] = a[i][j];
		}
	}
	
	return ret;
}

int NPortAESDecrypt (word8 data[16], word8 key[16])
{
	int i, j, ret;
	word8 a[4][MAXKC], k[4][MAXKC], rk[MAXROUNDS+1][4][MAXBC];
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			a[i][j] = data[i * 4 + j];
			k[i][j] = key[i * 4 + j];
		}
	}

	ret = AESKeySched(k, 128, 128, rk);
	ret = AESDecrypt(a, 128, 128, rk);

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			data[i * 4 + j] = a[i][j];
		}
	}

	return ret;
}

int CfgAESEncrypt (word8 data[32], word8 key[32])
{
	int i, j, ret;
	word8 a[4][CFGKC], k[4][CFGKC], rk[CFGROUNDS+1][4][CFGBC];
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < CFGKC; j++)
		{
			a[i][j] = data[i * CFGKC + j];
			k[i][j] = key[i * CFGKC + j];
		}
	}
	
	ret = AESKeySched(k, 256, 256, rk);
	ret = AESEncrypt(a, 256, 256, rk);

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < CFGKC; j++)
		{
			data[i * CFGKC + j] = a[i][j];
		}
	}
	
	return ret;
}

int CfgAESDecrypt (word8 data[32], word8 key[32])
{
	int i, j, ret;
	word8 a[4][CFGKC], k[4][CFGKC], rk[CFGROUNDS+1][4][CFGBC];
	
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < CFGKC; j++)
		{
			a[i][j] = data[i * CFGKC + j];
			k[i][j] = key[i * CFGKC + j];
		}
	}

	ret = AESKeySched(k, 256, 256, rk);
	ret = AESDecrypt(a, 256, 256, rk);

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < CFGKC; j++)
		{
			data[i * CFGKC	 + j] = a[i][j];
		}
	}

	return ret;
}

