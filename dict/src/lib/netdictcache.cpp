/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "netdictcache.h"
#include "netdictplugin.h"

#include <string>
#include <vector>
#include <map>

class RespCache {
public:
	RespCache();
	~RespCache();
	NetDictResponse *get_cache_resp(const char *key);
	void save_cache_resp(const char *key, NetDictResponse *resp);
private:
	static const unsigned int resp_pool_size = 50;
	struct RespElement {
		std::string key;
		NetDictResponse *resp;
	};
	std::vector<RespElement *> resp_pool;
	size_t cur_resp_pool_pos;
};

static std::map<std::string, RespCache> dictresp_map;

RespCache::RespCache()
{
	resp_pool.resize(resp_pool_size);
	for (size_t i = 0; i< resp_pool_size; i++) {
		resp_pool[i] = NULL;
	}
	cur_resp_pool_pos = 0;
}

RespCache::~RespCache()
{
	for (std::vector<RespElement *>::iterator i = resp_pool.begin(); i != resp_pool.end(); ++i) {
		if (*i) {
			delete (*i)->resp;
			delete *i;
		}
	}
}

NetDictResponse *RespCache::get_cache_resp(const char *key)
{
	for (std::vector<RespElement *>::iterator i = resp_pool.begin(); i != resp_pool.end(); ++i) {
		if (*i) {
			if ((*i)->key == key) {
				return (*i)->resp;
			}
		}
	}
	return NULL;
}

void RespCache::save_cache_resp(const char *key, NetDictResponse *resp)
{
	if (resp_pool[cur_resp_pool_pos]) {
		delete resp_pool[cur_resp_pool_pos]->resp;
		delete resp_pool[cur_resp_pool_pos];
	}
	resp_pool[cur_resp_pool_pos] = new RespElement();
	resp_pool[cur_resp_pool_pos]->key = key;
	resp_pool[cur_resp_pool_pos]->resp = resp;
	cur_resp_pool_pos++;
	if (cur_resp_pool_pos == resp_pool_size) {
		cur_resp_pool_pos = 0;
	}
}

static RespCache &get_response_cache(const char *dict)
{
	std::map<std::string, RespCache>::iterator i = dictresp_map.find(dict);
	if (i == dictresp_map.end()) {
		std::pair<std::map<std::string, RespCache>::iterator, bool> result;
		result = dictresp_map.insert(std::pair<std::string, RespCache>(dict, RespCache()));
		return result.first->second;
	} else {
		return i->second;
	}
}

NetDictResponse *netdict_get_cache_resp(const char *dict, const char *key)
{
	RespCache &resp_cache = get_response_cache(dict);
	return resp_cache.get_cache_resp(key);
}

void netdict_save_cache_resp(const char *dict, const char *key, NetDictResponse *resp)
{
	RespCache &resp_cache = get_response_cache(dict);
	resp_cache.save_cache_resp(key, resp);
}
