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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <sstream>
#include <cstring>
#include <cstdlib>
#include "ifo_file.h"
#include "libcommon.h"

#define NORM_DICT_MAGIC_DATA "StarDict's dict ifo file"
#define TREE_DICT_MAGIC_DATA "StarDict's treedict ifo file"
#define RES_DB_MAGIC_DATA "StarDict's storage ifo file"

/* Skip new line (LF, CR+LF, CR), return pointer to the post- new line char.
Return NULL if no new line. */
static const char* skip_new_line(const char *p)
{
	if(!p)
		return NULL;
	if(*p == '\n')
		return ++p;
	if(*p == '\r') {
		++p;
		if(*p == '\n')
			++p;
		return p;
	}
	return NULL;
}

static void decode_description(const char *p, long len, std::string &description)
{
	description.clear();
	const char *p1 = p;
	while (p1 - p < len) {
		if (*p1 == '<') {
			p1++;
			if ((*p1 == 'b' || *p1 == 'B') && (*(p1+1)=='r' || *(p1+1)=='R') && *(p1+2)=='>') {
				description += '\n';
				p1+=3;
			} else {
				description += '<';
			}
		} else {
			description += *p1;
			p1++;
		}
	}
}

/* replace new lines with "<br>" sequence */
static void encode_description(const char *p, long len, std::string &description)
{
	description.clear();
	const char *p1 = p;
	while(p1 - p < len) {
		if(*p1 == '\r' || *p1 == '\n') {
			description += "<br>";
			p1 = skip_new_line(p1);
		} else {
			description += *p1;
			++p1;
		}
	}
}

/* extract key - value pair having the following format:
 * key=value<new line>
 * Empty lines and lines containing only spaces and tabs are skipped.
 * Leading and trailing blanks, as well as blanks around the equal sign are discarded.
 * 
 * Parameters:
 * p1 - beginning of the string.
 * 
 * Return value:
 * NULL if key - value pair was not found
 * != NULL otherwise. It is a pointer to the beginning of the next string. */
const char* DictInfo::get_key_value(const char *line_beg, std::string& key,
	std::string& value)
{
	key.clear();
	value.clear();
	while(true) {
		const size_t n1 = strcspn(line_beg, "\r\n");
		const size_t n2 = strspn(line_beg, " \t");
		const char* const line_end = line_beg + n1;
		if(*line_end == '\0') { // EOF reached
			if(n1 != n2)
				g_warning("%s: line %d: Last line is not terminated with new line char.",
					ifo_file_name.c_str(), lineno);
			return NULL;
		}
		// new line char found
		g_assert(*line_end == '\r' || *line_end == '\n');
		if(n1 == n2) { // empty line
			line_beg = skip_new_line(line_end);
			++lineno;
			continue;
		}
		const char* const key_beg = line_beg + n2; // first non-blank char
		const char *equal_sign = key_beg;
		while(*equal_sign != '=' && equal_sign < line_end)
			++equal_sign;
		if(*equal_sign != '=') {
			g_warning("%s: line %d: '=' not found.", ifo_file_name.c_str(), lineno);
			line_beg = skip_new_line(line_end);
			++lineno;
			continue;
		}
		const char *key_end=equal_sign;
		while(key_beg < key_end && (*(key_end-1) == ' ' || *(key_end-1) == '\t'))
			--key_end;
		key.assign(key_beg, key_end-key_beg);
		const char *val_beg = equal_sign+1;
		const char *val_end = line_end;
		while(val_beg < line_end && (*val_beg == ' ' || *val_beg == '\t'))
			++val_beg;
		while(val_beg < val_end && (*(val_end-1) == ' ' || *(val_end-1) == '\t'))
			--val_end;
		value.assign(val_beg, val_end-val_beg);
		line_beg = skip_new_line(line_end);
		// no ++lineno; here
		return line_beg;
	}
}

DictInfo::DictInfo(void)
{
	clear();
}

bool DictInfo::load_from_ifo_file(const std::string& ifofilename,
	DictInfoType infotype)
{
	clear();
	ifo_file_name=ifofilename;
	set_infotype(infotype);
	glib::CharStr buffer;
	glib::Error error;
	if (!g_file_get_contents(ifo_file_name.c_str(), get_addr(buffer), NULL, get_addr(error))) {
		g_critical("Load %s failed. Error: %s.", ifo_file_name.c_str(), error->message);
		return false;
	}
	const gchar *p1 = get_impl(buffer);
	
	if(g_str_has_prefix(p1, UTF8_BOM))
		p1 += 3;
	if(!g_utf8_validate(p1, -1, NULL)) {
		g_critical("Load %s failed: Invalid UTF-8 encoded text.", ifo_file_name.c_str());
		return false;
	}
	lineno = 1;

	const gchar *magic_data = NULL;
	if(infotype == DictInfoType_NormDict)
		magic_data = NORM_DICT_MAGIC_DATA;
	else if(infotype == DictInfoType_TreeDict)
		magic_data = TREE_DICT_MAGIC_DATA;
	else if(infotype == DictInfoType_ResDb)
		magic_data = RES_DB_MAGIC_DATA;
	else
		return false;
	if (!g_str_has_prefix(p1, magic_data)) {
		g_critical("Load %s failed: Incorrect magic data.", ifo_file_name.c_str());
		if(g_str_has_prefix(p1, NORM_DICT_MAGIC_DATA))
			g_message("File '%s' is an index-based dictionary.", ifo_file_name.c_str());
		else if(g_str_has_prefix(p1, TREE_DICT_MAGIC_DATA))
			g_message("File '%s' is a tree dictionary.", ifo_file_name.c_str());
		else if(g_str_has_prefix(p1, RES_DB_MAGIC_DATA))
			g_message("File '%s' is a resource database.", ifo_file_name.c_str());
		else
			g_message("File '%s' is not a StarDict dictionary or it's broken.", ifo_file_name.c_str());
		return false;
	}
	p1 += strlen(magic_data);
	p1 = skip_new_line(p1);
	if(!p1) {
		g_critical("Load %s failed: Incorrect magic data.", ifo_file_name.c_str());
		return false;
	}

	std::string key, value;
	while(true) {
		++lineno;
		p1 = get_key_value(p1, key, value);
		if(!p1)
			break;

		// version must the first option
		if(!is_version()) {
			if(key != "version") {
				g_critical("Load %s failed: \"version\" must be the first option.", ifo_file_name.c_str());
				return false;
			}
		}
		if(key == "version") {
			if(!check_option_duplicate(f_version, "version"))
				continue;
			set_version(value);
			if(infotype == DictInfoType_NormDict) {
				if(version != "2.4.2" && version != "3.0.0") {
					g_critical("Load %s failed: Unknown version.", ifo_file_name.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_TreeDict) {
				if(version != "2.4.2") {
					g_critical("Load %s failed: Unknown version.", ifo_file_name.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_ResDb) {
				if(version != "3.0.0") {
					g_critical("Load %s failed: Unknown version.", ifo_file_name.c_str());
					return false;
				}
			}
		} else if(key == "idxoffsetbits") {
			if(!check_option_duplicate(f_idxoffsetbits, "idxoffsetbits"))
				continue;
			if(value != "32") {
				// TODO
				g_critical("Load %s failed: idxoffsetbits != 32 not supported presently.",
					ifo_file_name.c_str());
				return false;
			}
		} else if(key == "wordcount" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_wordcount, "wordcount"))
				continue;
			set_wordcount(atol(value.c_str()));
		} else if(key == "filecount" && infotype == DictInfoType_ResDb) {
			if(!check_option_duplicate(f_filecount, "filecount"))
				continue;
			set_filecount(atol(value.c_str()));
		} else if(key == "synwordcount" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_synwordcount, "synwordcount"))
				continue;
			set_synwordcount(atol(value.c_str()));
		} else if(key == "tdxfilesize" && infotype == DictInfoType_TreeDict) {
			if(!check_option_duplicate(f_index_file_size, "tdxfilesize"))
				continue;
			set_index_file_size(atol(value.c_str()));
		} else if(key == "idxfilesize" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_index_file_size, "idxfilesize"))
				continue;
			set_index_file_size(atol(value.c_str()));
		} else if(key == "ridxfilesize" && infotype == DictInfoType_ResDb) {
			if(!check_option_duplicate(f_index_file_size, "ridxfilesize"))
				continue;
			set_index_file_size(atol(value.c_str()));
		} else if(key == "dicttype" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_dicttype, "dicttype"))
				continue;
			set_dicttype(value);
		} else if(key == "bookname" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_bookname, "bookname"))
				continue;
			set_bookname(value);
		} else if(key == "author" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_author, "author"))
				continue;
			set_author(value);
		} else if(key == "email" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_email, "email"))
				continue;
			set_email(value);
		} else if(key == "website" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_website, "website"))
				continue;
			set_website(value);
		} else if(key == "date" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_date, "date"))
				continue;
			set_date(value);
		} else if(key == "description" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_description, "description"))
				continue;
			std::string temp;
			decode_description(value.c_str(), value.length(), temp);
			set_description(temp);
		} else if(key == "sametypesequence" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_sametypesequence, "sametypesequence"))
				continue;
			set_sametypesequence(value);
		} else {
			g_message("Load %s warning: unknown option %s.", ifo_file_name.c_str(),
				key.c_str());
		}
	}

	// check required options
	if((!is_wordcount() || wordcount == 0) && ((infotype == DictInfoType_NormDict
		|| infotype == DictInfoType_TreeDict))) {
		g_critical("Load %s failed: wordcount not specified or 0.",
			ifo_file_name.c_str());
		return false;
	}
	if((!is_filecount() || filecount == 0) && infotype == DictInfoType_ResDb) {
		g_critical("Load %s failed: filecount not specified or 0.",
			ifo_file_name.c_str());
		return false;
	}
	if((!is_bookname() || bookname.empty()) && (infotype == DictInfoType_NormDict
		|| infotype == DictInfoType_TreeDict)) {
		g_critical("Load %s failed: bookname not specified.",
			ifo_file_name.c_str());
		return false;
	}
	if(!is_index_file_size() || index_file_size == 0) {
		const char* kkey;
		if(infotype == DictInfoType_NormDict)
			kkey = "idxfilesize";
		else if(infotype == DictInfoType_TreeDict)
			kkey = "tdxfilesize";
		else if(infotype == DictInfoType_ResDb)
			kkey = "ridxfilesize";
		else
			kkey = "";
		g_critical("Load %s failed: %s not specified or 0.",
			ifo_file_name.c_str(), kkey);
		return false;
	}

	return true;
}

bool DictInfo::save_ifo_file(void) const
{
	if(ifo_file_name.empty()) {
		g_critical("Fail to save ifo file. ifo file name is not specified.");
		return false;
	}
	std::stringstream str;
	//str << UTF8_BOM;
	if(!is_infotype()) {
		g_critical("Fail to save ifo file. Dict info type is not specified.");
		return false;
	}
	const gchar *magic_data = NULL;
	if(infotype == DictInfoType_NormDict)
		magic_data = NORM_DICT_MAGIC_DATA;
	else if(infotype == DictInfoType_TreeDict)
		magic_data = TREE_DICT_MAGIC_DATA;
	else if(infotype == DictInfoType_ResDb)
		magic_data = RES_DB_MAGIC_DATA;
	else
		return false;
	str << magic_data << '\n';
	if(!is_version()) {
		g_critical("Fail to save ifo file. version is not specified.");
		return false;
	}
	str << "version=" << version << '\n';
	if(infotype == DictInfoType_NormDict || infotype == DictInfoType_TreeDict) {
		if(!is_bookname()) {
			g_critical("Fail to save ifo file. bookname is not specified.");
			return false;
		}
		str << "bookname=" << bookname << '\n';
		if(!is_wordcount()) {
			g_critical("Fail to save ifo file. wordcount is not specified.");
			return false;
		}
		str << "wordcount=" << wordcount << '\n';
	}
	if(infotype == DictInfoType_NormDict) {
		if(is_synwordcount())
			str << "synwordcount=" << synwordcount << '\n';
	}
	if(infotype == DictInfoType_ResDb) {
		if(is_filecount())
			str << "filecount=" << filecount << '\n';
	}
	if(infotype == DictInfoType_NormDict || infotype == DictInfoType_TreeDict
			|| infotype == DictInfoType_ResDb) {
		if(!is_index_file_size()) {
			g_critical("Fail to save ifo file. index_file_size is not specified.");
			return false;
		}
		if(infotype == DictInfoType_NormDict)
			str << "idxfilesize=" << index_file_size << '\n';
		if(infotype == DictInfoType_TreeDict)
			str << "tdxfilesize=" << index_file_size << '\n';
		if(infotype == DictInfoType_ResDb)
			str << "ridxfilesize=" << index_file_size << '\n';
	}
	if(infotype == DictInfoType_NormDict || infotype == DictInfoType_TreeDict) {
		if(is_author())
			str << "author=" << author << '\n';
		if(is_email())
			str << "email=" << email << '\n';
		if(is_website())
			str << "website=" << website << '\n';
		if(is_description()) {
			std::string temp;
			encode_description(description.c_str(), description.length(), temp);
			str << "description=" << temp << '\n';
		}
		if(is_date())
			str << "date=" << date << '\n';
		if(is_sametypesequence())
			str << "sametypesequence=" << sametypesequence << '\n';
	}
	if(infotype == DictInfoType_NormDict) {
		if(is_dicttype())
			str << "dicttype=" << dicttype << '\n';
	}
	if(!g_file_set_contents(ifo_file_name.c_str(), str.str().c_str(), -1, NULL)) {
		g_critical("Fail to save ifo file." open_write_file_err, ifo_file_name.c_str());
		return false;
	}
	return true;
}

void DictInfo::clear(void)
{
	ifo_file_name.clear();
	wordcount = 0;
	filecount = 0;
	synwordcount = 0;
	bookname.clear();
	author.clear();
	email.clear();
	website.clear();
	date.clear();
	description.clear();
	index_file_size = 0;
	sametypesequence.clear();
	dicttype.clear();
	version.clear();
	lineno = -1;

	f_wordcount = false;
	f_filecount = false;
	f_synwordcount = false;
	f_bookname = false;
	f_author = false;
	f_email = false;
	f_website = false;
	f_date = false;
	f_description = false;
	f_index_file_size = false;
	f_sametypesequence = false;
	f_dicttype = false;
	f_version = false;
	f_idxoffsetbits = false;
	f_infotype = false;
}

DictInfo& DictInfo::operator=(const DictInfo& dict_info)
{
	clear();
	ifo_file_name = dict_info.ifo_file_name;

	if(dict_info.is_wordcount())
		set_wordcount(dict_info.get_wordcount());
	if(dict_info.is_filecount())
		set_filecount(dict_info.get_filecount());
	if(dict_info.is_synwordcount())
		set_synwordcount(dict_info.get_synwordcount());
	if(dict_info.is_bookname())
		set_bookname(dict_info.get_bookname());
	if(dict_info.is_author())
		set_author(dict_info.get_author());
	if(dict_info.is_email())
		set_email(dict_info.get_email());
	if(dict_info.is_website())
		set_website(dict_info.get_website());
	if(dict_info.is_date())
		set_date(dict_info.get_date());
	if(dict_info.is_description())
		set_description(dict_info.get_description());
	if(dict_info.is_index_file_size())
		set_index_file_size(dict_info.get_index_file_size());
	if(dict_info.is_sametypesequence())
		set_sametypesequence(dict_info.get_sametypesequence());
	if(dict_info.is_dicttype())
		set_dicttype(dict_info.get_dicttype());
	if(dict_info.is_version())
		set_version(dict_info.get_version());
	if(dict_info.is_infotype())
		set_infotype(dict_info.get_infotype());

	f_idxoffsetbits = dict_info.f_idxoffsetbits;
	return *this;
}

bool DictInfo::check_option_duplicate(bool& flag, const char* option)
{
	if(flag) {
		g_warning("%s: line %d: duplicate option %s.", ifo_file_name.c_str(), lineno, option);
		return false;
	}
	flag = true;
	return true;
}
