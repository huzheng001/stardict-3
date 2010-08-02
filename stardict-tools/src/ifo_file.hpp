#ifndef _IFO_FILE_HPP_
#define _IFO_FILE_HPP_

#include <glib.h>
#include <list>
#include <string>
#include "libcommon.h"

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
	DictInfo(void);
	/* ifofilename in file name encoding */
	bool load_from_ifo_file(const std::string& ifofilename, DictInfoType infotype);
	void clear(void);
	void set_print_info(print_info_t func);
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
	print_info_t print_info;
};

#endif//!_IFO_FILE_HPP_
