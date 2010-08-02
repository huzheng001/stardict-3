/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include "ifo_file.hpp"
#include "resourcewrap.hpp"

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

static void parse_description(const char *p, long len, std::string &description)
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

/* extract key - value pair having the following format:
 * key=value<new line>
 * Empty lines and lines containing only spaces and tabs are skipped.
 * 
 * Parameters:
 * p1 - beginning of the string.
 * 
 * Return value:
 * NULL if key - value pair was not found
 * != NULL otherwise. It is a pointer to the beginning of the next string. */
const char* DictInfo::get_key_value(const char *p1, std::string& key, 
	std::string& value)
{
	key.clear();
	value.clear();
	while(true) {
		const size_t s1 = strcspn(p1, "\r\n");
		const size_t s2 = strspn(p1, " \t");
		const char* p2 = p1 + s1;
		if(*p2 == '\0') { // EOF reached
			if(s1 != s2)
				print_info("%s: line %d: Last line is not terminated with new line char.\n",
					ifo_file_name.c_str(), lineno);
			return NULL;
		} else { // new line char found
			if(s1 == s2) { // empty line
				p1 = skip_new_line(p2);
				++lineno;
				continue;
			}
		}
		g_assert(*p2 == '\r' || *p2 == '\n');
		// p1 != p2 here
		const char *p3 = p1;
		while(*p3 != '=' && p3 < p2)
			++p3;
		if(*p3 != '=') {
			print_info("%s: line %d: '=' not found.\n", ifo_file_name.c_str(), lineno);
			p1 = skip_new_line(p2);
			++lineno;
			continue;
		}
		key.assign(p1, p3-p1);
		++p3;
		value.assign(p3, p2-p3);
		p1 = skip_new_line(p2);
		// no ++lineno; here
		return p1;
	}
}

DictInfo::DictInfo(void)
{
	set_print_info(NULL);
	clear();
}

bool DictInfo::load_from_ifo_file(const std::string& ifofilename,
	DictInfoType infotype)
{
	clear();
	ifo_file_name=ifofilename;
	glib::CharStr buffer;
	if (!g_file_get_contents(ifofilename.c_str(), get_addr(buffer), NULL, NULL))
		return false;
	const gchar *p1 = get_impl(buffer);
	
	if(g_str_has_prefix(p1, "\xEF\xBB\xBF")) // UTF-8 BOM
		p1 += 3;
	if(!g_utf8_validate(p1, -1, NULL)) {
		print_info("Load %s failed: Invalid UTF-8 encoded text.\n", ifofilename.c_str());
		return false;
	}
	lineno = 1;

	const gchar *magic_data = NULL;
	if(infotype == DictInfoType_NormDict)
		magic_data = "StarDict's dict ifo file";
	else if(infotype == DictInfoType_TreeDict)
		magic_data = "StarDict's treedict ifo file";
	else if(infotype == DictInfoType_ResDb)
		magic_data = "StarDict's storage ifo file";
	else
		return false;
	if (!g_str_has_prefix(p1, magic_data)) {
		print_info("Load %s failed: Incorrect magic data\n", ifofilename.c_str());
		return false;
	}
	p1 += strlen(magic_data);
	p1 = skip_new_line(p1);
	if(!p1) {
		print_info("Load %s failed: Incorrect magic data\n", ifofilename.c_str());
		return false;
	}

	std::string key, value;
	while(true) {
		++lineno;
		p1 = get_key_value(p1, key, value);
		if(!p1)
			break;

		// version must the first option
		if(version.empty()) {
			if(key != "version") {
				print_info("Load %s failed: \"version\" must be the first option.\n", ifofilename.c_str());
				return false;
			}
		}
		if(key == "version") {
			if(!check_option_duplicate(f_version, "version"))
				continue;
			version = value;
			if(infotype == DictInfoType_NormDict) {
				if(version != "2.4.2" && version != "3.0.0") {
					print_info("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_TreeDict) {
				if(version != "2.4.2") {
					print_info("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_ResDb) {
				if(version != "3.0.0") {
					print_info("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			}
		} else if(key == "idxoffsetbits") {
			if(!check_option_duplicate(f_idxoffsetbits, "idxoffsetbits"))
				continue;
			if(value != "32") {
				// TODO
				print_info("Load %s failed: idxoffsetbits != 32 not supported presently.\n",
					ifofilename.c_str());
				return false;
			}
		} else if(key == "wordcount" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_wordcount, "wordcount"))
				continue;
			wordcount = atol(value.c_str());
		} else if(key == "filecount" && infotype == DictInfoType_ResDb) {
			if(!check_option_duplicate(f_filecount, "filecount"))
				continue;
			filecount = atol(value.c_str());
		} else if(key == "synwordcount" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_synwordcount, "synwordcount"))
				continue;
			synwordcount = atol(value.c_str());
		} else if(key == "tdxfilesize" && infotype == DictInfoType_TreeDict) {
			if(!check_option_duplicate(f_index_file_size, "tdxfilesize"))
				continue;
			index_file_size = atol(value.c_str());
		} else if(key == "idxfilesize" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_index_file_size, "idxfilesize"))
				continue;
			index_file_size = atol(value.c_str());
		} else if(key == "ridxfilesize" && infotype == DictInfoType_ResDb) {
			if(!check_option_duplicate(f_index_file_size, "ridxfilesize"))
				continue;
			index_file_size = atol(value.c_str());
		} else if(key == "dicttype" && infotype == DictInfoType_NormDict) {
			if(!check_option_duplicate(f_dicttype, "dicttype"))
				continue;
			dicttype = value;
		} else if(key == "bookname" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_bookname, "bookname"))
				continue;
			bookname = value;
		} else if(key == "author" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_author, "author"))
				continue;
			author = value;
		} else if(key == "email" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_email, "email"))
				continue;
			email = value;
		} else if(key == "website" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_website, "website"))
				continue;
			website = value;
		} else if(key == "date" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_date, "date"))
				continue;
			date = value;
		} else if(key == "description" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_description, "description"))
				continue;
			parse_description(value.c_str(), value.length(), description);
		} else if(key == "sametypesequence" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			if(!check_option_duplicate(f_sametypesequence, "sametypesequence"))
				continue;
			sametypesequence = value;
		} else {
			print_info("Load %s warning: unknown option %s.\n", ifofilename.c_str(), 
				key.c_str());
		}
	}

	// check required options
	if(wordcount == 0 && ((infotype == DictInfoType_NormDict 
		|| infotype == DictInfoType_TreeDict))) {
		print_info("Load %s failed: wordcount not specified or 0.\n", 
			ifofilename.c_str());
		return false;
	}
	if(filecount == 0 && infotype == DictInfoType_ResDb) {
		print_info("Load %s failed: filecount not specified or 0.\n", 
			ifofilename.c_str());
		return false;
	}
	if(bookname.empty() && (infotype == DictInfoType_NormDict 
		|| infotype == DictInfoType_TreeDict)) {
		print_info("Load %s failed: bookname not specified.\n", 
			ifofilename.c_str());
		return false;
	}
	if(index_file_size == 0) {
		const char* kkey = NULL;
		if(infotype == DictInfoType_NormDict)
			kkey = "idxfilesize";
		else if(infotype == DictInfoType_TreeDict)
			kkey = "tdxfilesize";
		else if(infotype == DictInfoType_ResDb)
			kkey = "ridxfilesize";
		print_info("Load %s failed: %s not specified or 0.\n", 
			ifofilename.c_str(), kkey);
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
}

void DictInfo::set_print_info(print_info_t func)
{
	print_info = func ? func : g_print;
}

bool DictInfo::check_option_duplicate(bool& flag, const char* option)
{
	if(flag) {
		print_info("%s: line %d: duplicate option %s.\n", ifo_file_name.c_str(), lineno, option);
		return false;
	}
	flag = true;
	return true;
}
