#ifndef CONFIG_CUSTOM_H
#define CONFIG_CUSTOM_H

/* include this file at the bottom of config.h 
the following config.h files should be affected:
config.h
msvc_2005/config.h
stardict-tools/config.h
*/

#include <glib.h>
#include <glib/gstdio.h>

/* g_stat function is declared differently depending on the version of glib 
GLIB_MICRO_VERSION version number may be incorrect, adjust it if needed. 
Use stardict_stat_t in the source code to define a structure for g_stat. */
#if !GLIB_CHECK_VERSION(2, 20, 0)
  // int g_stat (const gchar *filename, struct stat *buf);
  typedef struct stat stardict_stat_t;
#elif !GLIB_CHECK_VERSION(2, 25, 0)
#  ifdef G_OS_WIN32
    // int g_stat (const gchar *filename, struct _g_stat_struct *buf);
    typedef struct _g_stat_struct stardict_stat_t;
#  else
    // int g_stat (const gchar *filename, struct stat *buf);
    typedef struct stat stardict_stat_t;
#  endif
#else
  #if defined(G_OS_UNIX) && !defined(G_STDIO_NO_WRAP_ON_UNIX) 
    // int g_stat (const gchar *filename, struct stat *buf);
    typedef struct stat stardict_stat_t;
  #else
    // int g_stat (const gchar *filename, GStatBuf *buf);
    typedef GStatBuf stardict_stat_t;
  #endif
#endif

#endif // CONFIG_CUSTOM_H
