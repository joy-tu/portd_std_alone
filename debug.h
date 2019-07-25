#ifndef __DEBUG_H__
#define __DEBUG_H__

#define __CONFIG_DEBUG    0



#if __CONFIG_DEBUG
#	define CONFIG_DEBUG    printf
#else
#	define CONFIG_DEBUG(...)
#endif

#endif
