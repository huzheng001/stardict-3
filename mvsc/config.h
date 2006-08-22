#ifndef CONFIG_H
#define CONFIG_H

#if defined(_MSC_VER)
# include <stdlib.h>

#  define strcasecmp   _stricmp
#  define inline __inline
#  define S_IRWXU 0
#  define __attribute__(x) /**/
#  define g_fopen my_g_fopen

static inline void *memrchr(const void *mem, int c, size_t len)
{
	char *res;
	char *cmem = (char *)mem;

	if (!len)
		return NULL;
	res = cmem + len - 1;
	while (res != cmem - 1 && *res != c)
		--res;
	return res == cmem - 1 ? NULL : res;
}

# include <math.h>
static inline double round(double d)
{
	double c = ceil(d);
    double f = floor(d);
    if (d - f < .5)
		return f;
     else
		return c;
}
#endif

#endif/*!CONFIG_H*/
