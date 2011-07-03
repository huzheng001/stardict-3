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

#ifndef _IFO_FILE_H_
#define _IFO_FILE_H_

#include <glib.h>
#include <list>
#include <string>
#include "libcommon.h"

#define GET_METHOD_TEMPL(type, field) \
	type get_##field(void) const \
	{ \
		return field; \
	}

#define SET_METHOD_TEMPL(type, field) \
	void set_##field(type field) \
	{ \
		this->field = field; \
		this->f_##field = true; \
	}

#define UNSET_METHOD_TEMPL(type, field, default_val) \
	void unset_##field(void) \
	{ \
		this->f_##field = false; \
		this->field = default_val; \
	}

#define IS_METHOD_TEMPL(type, field) \
	bool is_##field(void) const \
	{ \
		return f_##field; \
	}

#define ALL_METHOD_TEMPL(type, field, default_val) \
	GET_METHOD_TEMPL(type, field) \
	SET_METHOD_TEMPL(type, field) \
	UNSET_METHOD_TEMPL(type, field, default_val) \
	IS_METHOD_TEMPL(type, field)

enum DictInfoType {
	DictInfoType_NormDict,
	DictInfoType_TreeDict,
	DictInfoType_ResDb
};

// This structure contains all information about dictionary or Resource Storage
// database.
struct DictInfo {
	/* in file name encoding */
	std::string ifo_file_name;

	DictInfo(void);
	/* ifofilename in file name encoding */
	bool load_from_ifo_file(const std::string& ifofilename, DictInfoType infotype);
	bool save_ifo_file(void) const;
	void clear(void);
	DictInfo& operator=(const DictInfo& dict_info);

	ALL_METHOD_TEMPL(guint32, wordcount, 0)
	ALL_METHOD_TEMPL(guint32, filecount, 0)
	ALL_METHOD_TEMPL(guint32, synwordcount, 0)
	ALL_METHOD_TEMPL(const std::string&, bookname, "")
	ALL_METHOD_TEMPL(const std::string&, author, "")
	ALL_METHOD_TEMPL(const std::string&, email, "")
	ALL_METHOD_TEMPL(const std::string&, website, "")
	ALL_METHOD_TEMPL(const std::string&, date, "")
	ALL_METHOD_TEMPL(const std::string&, description, "")
	ALL_METHOD_TEMPL(guint32, index_file_size, 0)
	ALL_METHOD_TEMPL(const std::string&, sametypesequence, "")
	ALL_METHOD_TEMPL(const std::string&, dicttype, "")
	ALL_METHOD_TEMPL(const std::string&, version, "")
	ALL_METHOD_TEMPL(DictInfoType, infotype, DictInfoType_NormDict)
private:
	const char* get_key_value(const char *p1, std::string& key, 
		std::string& value);
	bool check_option_duplicate(bool& flag, const char* option);
	int lineno;
	// flags. true if corresponding item is set
	bool f_wordcount;
	bool f_filecount;
	bool f_synwordcount;
	bool f_bookname;
	bool f_author;
	bool f_email;
	bool f_website;
	bool f_date;
	bool f_description;
	bool f_index_file_size;
	bool f_sametypesequence;
	bool f_dicttype;
	bool f_version;
	bool f_idxoffsetbits;
	bool f_infotype;

	/* other strings in utf-8 */
	guint32 wordcount;
	guint32 filecount;
	guint32 synwordcount;
	std::string bookname;
	std::string author;
	std::string email;
	std::string website;
	std::string date;
	std::string description;
	guint32 index_file_size;
	std::string sametypesequence;
	std::string dicttype;
	std::string version;
	DictInfoType infotype;
};

#undef GET_METHOD_TEMPL
#undef SET_METHOD_TEMPL
#undef UNSET_METHOD_TEMPL
#undef IS_METHOD_TEMPL
#undef ALL_METHOD_TEMPL

#endif//!_IFO_FILE_H_
