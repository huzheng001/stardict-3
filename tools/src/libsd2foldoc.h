#ifndef _LIBSD2FOLDOC_H_
#define _LIBSD2FOLDOC_H_
#include <glib.h>

typedef void (*print_info_t)(const char *info);

extern void sd2foldoc(const char *ifofilename, print_info_t print_info);

#endif
