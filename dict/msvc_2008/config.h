#ifndef CONFIG_H
#define CONFIG_H

#if defined(_MSC_VER)
#  include <glib.h>
#  include <glib/gstdio.h>

#  define S_IRWXU 0
#  define S_IRWXG 0
#  define S_IRWXO 0
#  define __attribute__(x) /**/
#  define g_fopen stardict_g_fopen

// defined in glib_wrapper.c
G_BEGIN_DECLS
FILE *
stardict_g_fopen (const gchar *filename,
						const gchar *mode);
G_END_DECLS

#endif

#include <../../lib/config-custom.h>

#endif/*!CONFIG_H*/
