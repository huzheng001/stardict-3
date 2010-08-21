#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>
#include <glib/gstdio.h>

#if defined(_MSC_VER)
# include <stdlib.h>

#  define S_IRWXU 0
#  define __attribute__(x) /**/
#  define g_fopen stardict_g_fopen

// defined in glib_wrapper.c
G_BEGIN_DECLS
FILE *
stardict_g_fopen (const gchar *filename,
						const gchar *mode);
G_END_DECLS

#endif

/* g_stat function is declared differently depending on the version of glib 
GLIB_MICRO_VERSION version number may be incorrect, adjust it if needed. 
Use stardict_stat_t in the source code to define a structure for g_stat. */
#if !GLIB_CHECK_VERSION(2, 20, 0)
// int g_stat (const gchar *filename, struct stat *buf);
typedef struct stat stardict_stat_t;
#elif !GLIB_CHECK_VERSION(2, 25, 0)
// int g_stat (const gchar *filename, struct _g_stat_struct *buf);
typedef struct _g_stat_struct stardict_stat_t;
#else
// int g_stat (const gchar *filename, GStatBuf *buf);
typedef GStatBuf stardict_stat_t;
#endif

#endif/*!CONFIG_H*/
