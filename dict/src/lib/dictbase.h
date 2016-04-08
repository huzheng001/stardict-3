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

#ifndef _DICTBASE_H_
#define _DICTBASE_H_

#include <glib.h>
#include <vector>
#include <string>
#include <memory>
#include <stdio.h>

#include "dictziplib.h"

enum InstantDictType {
	InstantDictType_UNKNOWN = 0,
	InstantDictType_LOCAL,
	InstantDictType_VIRTUAL,
	InstantDictType_NET,
};

struct InstantDictIndex {
	InstantDictType type;
	size_t index;
};

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
	std::unique_ptr<dictData> dictdzfile;
	cacheItem cache[WORDDATA_CACHE_NUM];
	gint cache_cur;
};

#endif//!_DICTBASE_H_
