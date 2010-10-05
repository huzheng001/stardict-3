#ifndef _STARDICT_NETDICT_CACHE_H_
#define _STARDICT_NETDICT_CACHE_H_

struct NetDictResponse;

extern NetDictResponse *netdict_get_cache_resp(const char *dict, const char *key);
extern void netdict_save_cache_resp(const char *dict, const char *key, NetDictResponse *resp);

#endif
