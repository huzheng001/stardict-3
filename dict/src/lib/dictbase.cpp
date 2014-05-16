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

/* implementation of class to work with dictionary data */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdio>
#include <cstring>

//#include "kmp.h"

#include "dictbase.h"
#include "utils.h"

/* may contain lower-case chars only, otherwise changes in DictBase::SearchData needed. */
const gchar* const DICT_DATA_TYPE_SEARCH_DATA_STR = "mgxtykwh";

/* return true if c is one of the lower-case character type identifies */
inline bool is_dict_data_type_lower_case(gchar c)
{
	return strchr("mtygxkwhnr", c);
}

/* return true if c is one of the upper-case character type identifies */
inline bool is_dict_data_type_upper_case(gchar c)
{
	return strchr("P", c);
}

/* return true if c is one of the character type identifies which data may be
 * searched for words */
inline bool is_dict_data_type_search_data(gchar c)
{
	return strchr(DICT_DATA_TYPE_SEARCH_DATA_STR, c);
}

DictBase::DictBase()
{
	dictfile = NULL;
	cache_cur =0;
}

DictBase::~DictBase()
{
	if (dictfile)
		fclose(dictfile);
}

/* load dictionary
 * filebasename - file name without extension.
 * We try filebasename + "." + mainext + ".dz" file first, 
 * then filebasename + "." + mainext. */
bool DictBase::load(const std::string& filebasename, const char* mainext)
{
	std::string fullfilename;
	fullfilename = filebasename + "." + mainext + ".dz";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		dictdzfile.reset(new dictData);
		if (!dictdzfile->open(fullfilename, 0)) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	} else {
		fullfilename = filebasename + "." + mainext;
		dictfile = fopen(fullfilename.c_str(),"rb");
		if (!dictfile) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	}
	return true;
}

gchar* DictBase::GetWordData(guint32 idxitem_offset, guint32 idxitem_size)
{
	for (int i=0; i<WORDDATA_CACHE_NUM; i++)
		if (cache[i].data && cache[i].offset == idxitem_offset)
			return cache[i].data;

	if (dictfile)
		fseek(dictfile, idxitem_offset, SEEK_SET);

	gchar *data;
	if (!sametypesequence.empty()) {
		gchar *origin_data = (gchar *)g_malloc(idxitem_size);

		if (dictfile) {
			size_t fread_size;
			fread_size = fread(origin_data, idxitem_size, 1, dictfile);
			if (fread_size != 1) {
				g_print("fread error!\n");
			}
		} else {
			dictdzfile->read(origin_data, idxitem_offset, idxitem_size);
		}

		const gint sametypesequence_len = sametypesequence.length();
		guint32 data_size = idxitem_size + sametypesequence_len;

		//if the last item's size is determined by the end up '\0',then +=sizeof(gchar);
		//if the last item's size is determined by the head guint32 type data,then +=sizeof(guint32);
		if(is_dict_data_type_lower_case(sametypesequence[sametypesequence_len-1])) {
			data_size += sizeof(gchar);
		} else if(is_dict_data_type_upper_case(sametypesequence[sametypesequence_len-1])) {
			data_size += sizeof(guint32);
		} else {
			if (g_ascii_isupper(sametypesequence[sametypesequence_len-1]))
				data_size += sizeof(guint32);
			else
				data_size += sizeof(gchar);
		}
		data = (gchar *)g_malloc(data_size + sizeof(guint32));
		gchar *p1,*p2;
		p1 = data + sizeof(guint32);
		p2 = origin_data;
		guint32 sec_size;
		//copy the head items.
		for (int i=0; i<sametypesequence_len-1; i++) {
			*p1=sametypesequence[i];
			p1+=sizeof(gchar);
			if(is_dict_data_type_lower_case(sametypesequence[i])) {
				sec_size = strlen(p2)+1;
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
			} else if(is_dict_data_type_upper_case(sametypesequence[i])) {
				sec_size = g_ntohl(get_uint32(p2));
				sec_size += sizeof(guint32);
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
			} else {
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = g_ntohl(get_uint32(p2));
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p2)+1;
				}
				memcpy(p1, p2, sec_size);
				p1+=sec_size;
				p2+=sec_size;
			}
		}
		//calculate the last item 's size.
		sec_size = idxitem_size - (p2-origin_data);
		*p1=sametypesequence[sametypesequence_len-1];
		p1+=sizeof(gchar);
		if(is_dict_data_type_lower_case(sametypesequence[sametypesequence_len-1])) {
			memcpy(p1, p2, sec_size);
			p1 += sec_size;
			*p1='\0';//add the end up '\0';
		} else if(is_dict_data_type_upper_case(sametypesequence[sametypesequence_len-1])) {
			guint32 t = g_htonl(sec_size);
			memcpy(p1, &t, sizeof(guint32));
			p1 += sizeof(guint32);
			memcpy(p1, p2, sec_size);
		} else {
			if (g_ascii_isupper(sametypesequence[sametypesequence_len-1])) {
				guint32 t = g_htonl(sec_size);
				memcpy(p1, &t, sizeof(guint32));
				p1 += sizeof(guint32);
				memcpy(p1, p2, sec_size);
			} else {
				memcpy(p1, p2, sec_size);
				p1 += sec_size;
				*p1='\0';
			}
		}
		g_free(origin_data);
		memcpy(data, &data_size, sizeof(guint32));
	} else {
		data = (gchar *)g_malloc(idxitem_size + sizeof(guint32));
		if (dictfile) {
			size_t fread_size;
			fread_size = fread(data+sizeof(guint32), idxitem_size, 1, dictfile);
			if (fread_size != 1) {
				g_print("fread error!\n");
			}
		} else {
			dictdzfile->read(data+sizeof(guint32), idxitem_offset, idxitem_size);
		}
		memcpy(data, &idxitem_size, sizeof(guint32));
	}
	g_free(cache[cache_cur].data);

	cache[cache_cur].data = data;
	cache[cache_cur].offset = idxitem_offset;
	cache_cur++;
	if (cache_cur==WORDDATA_CACHE_NUM)
		cache_cur = 0;
	return data;
}

bool DictBase::SearchData(std::vector<std::string> &SearchWords, guint32 idxitem_offset, guint32 idxitem_size, gchar *origin_data)
{
	const int nWord = SearchWords.size();
	std::vector<bool> WordFind(nWord, false);
	int nfound=0;

	if (dictfile)
		fseek(dictfile, idxitem_offset, SEEK_SET);
	if (dictfile) {
		size_t fread_size;
		fread_size = fread(origin_data, idxitem_size, 1, dictfile);
		if (fread_size != 1) {
			g_print("fread error!\n");
		}
	} else {
		dictdzfile->read(origin_data, idxitem_offset, idxitem_size);
	}
	gchar *p = origin_data;
	guint32 sec_size;
	int j;
	if (!sametypesequence.empty()) {
		const gint sametypesequence_len = sametypesequence.length();
		for (int i=0; i<sametypesequence_len-1; i++) {
			if(is_dict_data_type_search_data(sametypesequence[i])) {
				sec_size = strlen(p);
				for (j=0; j<nWord; j++)
					// KMP() is slower than strstr() if have no prepare data.
					//if (!WordFind[j] && KMP(p, sec_size, SearchWords[j].c_str())!=-1) {
					if (!WordFind[j] && g_strstr_len(p, sec_size, SearchWords[j].c_str())!=NULL) {
						WordFind[j] = true;
						++nfound;
					}

				if (nfound==nWord)
					return true;
				sec_size += sizeof(gchar);
				p+=sec_size;
			} else {
				if (g_ascii_isupper(sametypesequence[i])) {
					sec_size = g_ntohl(get_uint32(p));
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p)+1;
				}
				p+=sec_size;
			}
		}
		if(is_dict_data_type_search_data(sametypesequence[sametypesequence_len-1])) {
			sec_size = idxitem_size - (p-origin_data);
			for (j=0; j<nWord; j++)
				//if (!WordFind[j] && KMP(p, sec_size, SearchWords[j].c_str())!=-1) {
				if (!WordFind[j] && g_strstr_len(p, sec_size, SearchWords[j].c_str())!=NULL) {
					WordFind[j] = true;
					++nfound;
				}

			if (nfound==nWord)
				return true;
		}
	} else {
		while (guint32(p - origin_data)<idxitem_size) {
			if(is_dict_data_type_search_data(*p)) {
				for (j=0; j<nWord; j++)
					if (!WordFind[j] && strstr(p, SearchWords[j].c_str())) {
						WordFind[j] = true;
						++nfound;
					}

				if (nfound==nWord)
					return true;
				sec_size = strlen(p)+1;
				p+=sec_size;
			} else {
				if (g_ascii_isupper(*p)) {
					sec_size = g_ntohl(get_uint32(p));
					sec_size += sizeof(guint32);
				} else {
					sec_size = strlen(p)+1;
				}
				p+=sec_size;
			}
		}
	}
	return false;
}

