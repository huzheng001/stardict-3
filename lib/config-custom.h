#ifndef CONFIG_CUSTOM_H
#define CONFIG_CUSTOM_H

/* include this file at the bottom of config.h 
the following config.h files should be affected:
lib/config.h
dict/config.h
dict/msvc_2008/config.h
tools/config.h
*/

#include <glib.h>
#include <glib/gstdio.h>

/* g_stat function is declared differently depending on the version of glib 
GLIB_MICRO_VERSION version number may be incorrect, adjust it if needed. 
Use stardict_stat_t in the source code to define a structure for g_stat. */
#if GLIB_CHECK_VERSION(2, 25, 0)
  #if defined(G_OS_UNIX) && !defined(G_STDIO_NO_WRAP_ON_UNIX) 
    // int g_stat (const gchar *filename, struct stat *buf);
    typedef struct stat stardict_stat_t;
  #else
    // int g_stat (const gchar *filename, GStatBuf *buf);
    typedef GStatBuf stardict_stat_t;
  #endif
#elif GLIB_CHECK_VERSION(2, 24, 0)
  #if defined(G_OS_UNIX) && !defined(G_STDIO_NO_WRAP_ON_UNIX)
    // #define g_stat    stat
    typedef struct stat stardict_stat_t;
  #else /* ! G_OS_UNIX */
    #ifdef G_OS_WIN32
      #if defined (_MSC_VER) && !defined(_WIN64)
        // #define _g_stat_struct _stat32
      #else
        // #define _g_stat_struct stat
      #endif
      // int g_stat      (const gchar           *filename,
      //                   struct _g_stat_struct *buf);
      typedef struct _g_stat_struct stardict_stat_t;
    #else
      // int g_stat      (const gchar *filename,
      //                  struct stat *buf);
      typedef struct stat stardict_stat_t;
    #endif
  #endif /* G_OS_UNIX */
#elif GLIB_CHECK_VERSION(2, 20, 0)
  #if defined(G_OS_UNIX) && !defined(G_STDIO_NO_WRAP_ON_UNIX)
    // #define g_stat    stat
    typedef struct stat stardict_stat_t;
  #else
    // int g_stat      (const gchar *filename,
    //                  struct stat *buf);
    typedef struct stat stardict_stat_t;
  #endif
#else
  // int g_stat (const gchar *filename, struct stat *buf);
  typedef struct stat stardict_stat_t;
#endif

#endif // CONFIG_CUSTOM_H
