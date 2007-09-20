#ifndef _SD_GET_UINT32_H_
#define _SD_GET_UINT32_H_

#ifdef ARM
static inline guint32 get_uint32(const gchar *addr)
{
	guint32 result;
	memcpy(&result, addr, sizeof(guint32));
	return result;
}
#else
#define get_uint32(x) *reinterpret_cast<const guint32 *>(x)
#endif

#endif
