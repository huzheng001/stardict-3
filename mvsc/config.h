#ifndef CONFIG_H
#define CONFIG_H

#if defined(_MSC_VER)
# include <stdlib.h>

#  define strcasecmp   _stricmp
#  define inline __inline
#  define S_IRWXU 0
#  define __attribute__(x) /**/
#  define g_fopen my_g_fopen

#endif

#endif/*!CONFIG_H*/
