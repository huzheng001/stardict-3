#ifndef MD5_H
#define MD5_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_STDINT_H
	#include <stdint.h>
	typedef uint32_t uint32;
#else
	/* A.Leo.: this wont work on 16 bits platforms ;) */
	typedef unsigned uint32;
#endif

struct MD5Context {
	uint32 buf[4];
	uint32 bits[2];
	unsigned char in[64];
};

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, unsigned char const *buf,
	       unsigned len);
void MD5Final(unsigned char digest[16], struct MD5Context *context);
void MD5Transform(uint32 buf[4], uint32 const in[16]);

/*
 * This is needed to make RSAREF happy on some MS-DOS compilers.
 */
typedef struct MD5Context MD5_CTX;

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif /* !MD5_H */
