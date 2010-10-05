#ifndef _IFO_FILE_HPP_
#define _IFO_FILE_HPP_

#include <glib.h>
#include <list>
#include <string>

/* keep the following files in sync:
 * src/lib/ifo_file.hpp
 * stardict-tools/src/ifo_file.hpp
 */

typedef void (*print_info_t)(const char *info, ...);

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
	void set_print_info(print_info_t func);
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

	print_info_t print_info;
};

#undef GET_METHOD_TEMPL
#undef SET_METHOD_TEMPL
#undef UNSET_METHOD_TEMPL
#undef IS_METHOD_TEMPL
#undef ALL_METHOD_TEMPL

#endif//!_IFO_FILE_HPP_
