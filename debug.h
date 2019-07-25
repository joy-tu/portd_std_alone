#ifndef __DEBUG_H__
#define __DEBUG_H__

#define __CONFIG_DEBUG    0
#define __SIO_DEBUG       0



#if __CONFIG_DEBUG
#	define CONFIG_DEBUG    printf
#else
#	define CONFIG_DEBUG(...)
#endif

#if __SIO_DEBUG
#	define SIO_DEBUG       printf
#else
#	define SIO_DEBUG(...)
#endif

#endif
