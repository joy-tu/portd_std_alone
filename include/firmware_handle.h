
#include <stdint.h>

#ifndef __FIRMWARE_HANDLE_H__
#define __FIRMWARE_HANDLE_H__

#ifdef CROSS
    #if defined(imx25) || defined (imx_EVB) || defined(imx_AR6K)
        #define MAGIC_0 0x89191230L
        #define MAGIC_1 0x28877620L
    #elif defined(w1) // MIINE_W1
        #define MAGIC_0 0x4D49494EL
        #define MAGIC_1 0x455F5731L
    #elif defined(w2x50a) /* IAW5x5xAIO */
//        #define MAGIC_0 0x4E505732L
//        #define MAGIC_1 0x58353041L
        #define MAGIC_0 0x4E505749L
        #define MAGIC_1 0x4F355835L
    #elif defined(ia5x50aio)	/* IA5x50AIO Joy */
//        #define MAGIC_0 0x4E505749L
//        #define MAGIC_1 0x4F355835L	
        #define MAGIC_0 0x4E50494FL
        #define MAGIC_1 0x35583530L	
    #else
        #error Undefined module.
    #endif
#else
    #define MAGIC_0 0x89191230L
    #define MAGIC_1 0x00000000L
#endif // CROSS

#define D_FIRMWARE_SECTION  4
//#define D_FIRMWARE_SECTION  3

struct __dc_version {
    uint8_t major;    // version major number
    uint8_t minor;    // version minor number
    uint8_t cv_bata;  // version cv/bata number
    uint8_t reserve;  // reserve
    uint32_t  build;  // build data.
};
typedef struct __dc_version dc_version;
typedef struct __dc_version* dc_version_t;


struct __firmware_handle {
    uint8_t magic[8];       // magic code
    uint32_t totalLen;     // total firmware size
    uint8_t hdrVersion;     // firmware handle version
    uint8_t sectionNumber;  // with section number
    uint8_t encryptVersion; // version of firmware encryption
    uint8_t reserve;     // unuse
    dc_version mVersion;    // firmware version
    dc_version oVersion;    // OEM firmware version
    uint8_t companyId;      // company ID
    uint8_t reserve2[3];    // unuse
    /* CRC */
    uint32_t crc;           // firmware CRC checksum
};
typedef struct __firmware_handle handle;
typedef struct __firmware_handle* handle_t;

struct __fw_section_handle {
    uint8_t sName[16];      // section name
    uint32_t len;           // section length
    dc_version sVersion;    // section version
    uint8_t reserve[4];     // unuse
    uint32_t crc;           // srction CRC shecksum
};
typedef struct __fw_section_handle sHandle;
typedef struct __fw_section_handle* sHandle_t;


struct __uFilePost {
    char *buf;
    int len;
};
typedef struct __uFilePost uFilePost;
typedef struct __uFilePost* uFilePost_t;


int fw_decrypt(unsigned char *buf, int *len);
int fw_check(unsigned char *buf, int len);
int fw_write(unsigned char *buf, int len, int *percentage);

#define FIRM_UP_KEY (1234)

int fw_up_sem_create(int *sid, key_t key, int members);
int fw_up_sem_open(int *sid, key_t key);
int fw_up_sem_up(int semid, int member);
int fw_up_sem_down(int semid, int member);
int fw_up_sem_wait(int semid, int member);


#endif // __FIRMWARE_HANDLE_H__
