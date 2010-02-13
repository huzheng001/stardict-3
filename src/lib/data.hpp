#ifndef _DATA_HPP_
#define _DATA_HPP_

#include <glib.h>
#include <vector>
#include <string>
#include <memory>
#include <stdio.h>

#include "dictziplib.hpp"

struct cacheItem {
  guint32 offset;
	gchar *data;
  cacheItem() {data= NULL;}
  ~cacheItem() {g_free(data);}
};

const int WORDDATA_CACHE_NUM = 10;
const int UNSET_INDEX = -1;
const int INVALID_INDEX=-100;
extern const gchar* const DICT_DATA_TYPE_SEARCH_DATA_STR;


class DictBase {
public:
	DictBase();
	~DictBase();
	bool load(const std::string& filebasename, const char* mainext);
	gchar * GetWordData(guint32 idxitem_offset, guint32 idxitem_size);
	bool containSearchData() {
		if (sametypesequence.empty())
			return true;

		return sametypesequence.find_first_of(DICT_DATA_TYPE_SEARCH_DATA_STR) !=
			std::string::npos;
	}
	bool SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data);
protected:
	std::string sametypesequence;
private:
	FILE *dictfile;
	std::auto_ptr<dictData> dictdzfile;
	cacheItem cache[WORDDATA_CACHE_NUM];
	gint cache_cur;
};

#endif//!_DATA_HPP_
