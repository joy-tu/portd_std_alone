#include <sysapi.h>
#include <string.h>
#include <stdio.h>

/**
 * \brief Get firmware version number.
 * \param [out] main_ver Firmware major number.
 * \param [out] sub_ver Firmware minor number.
 * \param [out] ext_ver Frimware CV or beta version number.
 */
void sys_getVersionExt(int *main_ver, int *sub_ver, int *ext_ver)
{
    *main_ver = VERSION_MAJOR;
    *sub_ver = VERSION_MINOR;
    *ext_ver = VERSION_CV_BETA;
}

/**
 * \brief Get firmware version string with build date.
 * \param [out] buf Output firmware string. Ex: 1.0.2 Build 11012010
 * \param [in] size Buffer size.
 */
void sys_getVersionString(char *buf, int size)
{
    if(VERSION_CV_BETA == 0)
        snprintf(buf, size, "%d.%d Build %08d", VERSION_MAJOR, VERSION_MINOR, BUILD_DATE);
    else
        snprintf(buf, size, "%d.%d.%d Build %08d", VERSION_MAJOR, VERSION_MINOR, VERSION_CV_BETA, BUILD_DATE);
}

