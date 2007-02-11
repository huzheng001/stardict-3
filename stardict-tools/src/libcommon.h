#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <glib.h>

typedef void (*print_info_t)(const char *info);

extern gint stardict_strcmp(const gchar *s1, const gchar *s2);


#endif

