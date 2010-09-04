#ifndef _IFO_FILE_HPP_
#define _IFO_FILE_HPP_

#include <glib.h>
#include <list>
#include <string>
#include "libcommon.h"

// keep this file in sync with src/lib/ifo_file.hpp

void encode_description(const char *p, long len, std::string &description);

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

#define UNSET_METHOD_TEMPL(type, field) \
	void unset_##field(void) \
	{ \
		this->f_##field = false; \
	}

#define IS_METHOD_TEMPL(type, field) \
	bool is_##field(void) const \
	{ \
		return f_##field; \
	}

#define ALL_METHOD_TEMPL(type, field) \
	GET_METHOD_TEMPL(type, field) \
	SET_METHOD_TEMPL(type, field) \
	UNSET_METHOD_TEMPL(type, field) \
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
	void clear(void);
	void set_print_info(print_info_t func);

	ALL_METHOD_TEMPL(guint32, wordcount)
	ALL_METHOD_TEMPL(guint32, filecount)
	ALL_METHOD_TEMPL(guint32, synwordcount)
	ALL_METHOD_TEMPL(const std::string&, bookname)
	ALL_METHOD_TEMPL(const std::string&, author)
	ALL_METHOD_TEMPL(const std::string&, email)
	ALL_METHOD_TEMPL(const std::string&, website)
	ALL_METHOD_TEMPL(const std::string&, date)
	ALL_METHOD_TEMPL(const std::string&, description)
	ALL_METHOD_TEMPL(guint32, index_file_size)
	ALL_METHOD_TEMPL(const std::string&, sametypesequence)
	ALL_METHOD_TEMPL(const std::string&, dicttype)
	ALL_METHOD_TEMPL(const std::string&, version)
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

	print_info_t print_info;
};

#undef GET_METHOD_TEMPL
#undef SET_METHOD_TEMPL
#undef UNSET_METHOD_TEMPL
#undef IS_METHOD_TEMPL
#undef ALL_METHOD_TEMPL

#endif//!_IFO_FILE_HPP_
