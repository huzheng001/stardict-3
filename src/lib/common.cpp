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

/*
 * implementation of methods of common for dictionaries structures
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include "common.hpp"
#include "utils.h"

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
 * key=value\n
 * Empty lines and lines containing only spaces and tabs are skipped.
 * key-value pair may be followed by end-of-line or end-of-file.
 * 
 * Parameters:
 * p1 - beginning of the string.
 * 
 * Return value:
 * NULL if key - value pair was not found
 * != NULL otherwise. It is a pointer to the beginning of the next string if it 
 * exists or a pointer to the '\0' char if that was the last string. */
static const char* get_key_value(const char *p1, std::string& key, 
	std::string& value)
{
	key.clear();
	value.clear();
	while(true) {
		// skip ``\t''   ``\n''    ``\v''    ``\f''    ``\r''    `` ''
		while(isspace(*p1))
			++p1;
		if(!*p1)
			return NULL;
	
		const char *p2 = strchr(p1, '\n');
		if(!p2)
			p2 = p1 + strlen(p1);
		// p1 != p2 here
		const char *p3 = p1;
		while(*p3 != '=' && p3 < p2)
			++p3;
		if(*p3 != '=') {
			g_warning("Incorrect string in ifo file, '=' not found.");
			p1 = p2; // not p2 + 1! *p2 may be '\0'
			continue;
		}
		key.assign(p1, p3-p1);
		++p3;
		value.assign(p3, p2-p3);
		return *p2 ? p2 + 1 : p2;
	}
}

DictInfo::DictInfo(void)
:
	wordcount(0),
	synwordcount(0),
	index_file_size(0)
{
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

	const gchar *magic_data = NULL;
	if(infotype == DictInfoType_NormDict)
		magic_data = "StarDict's dict ifo file\nversion=";
	else if(infotype == DictInfoType_TreeDict)
		magic_data = "StarDict's treedict ifo file\nversion=";
	else if(infotype == DictInfoType_ResDb)
		magic_data = "StarDict's storage ifo file\nversion=";
	else
		return false;
	if (!g_str_has_prefix(p1, magic_data))
		return false;

	std::string key, value;
	p1 = strchr(p1, '\n') + 1;
	while((p1 = get_key_value(p1, key, value))) {
		if(key == "version") {
			version = value;
			if(infotype == DictInfoType_NormDict) {
				if(version != "2.4.2" && version != "3.0.0") {
					g_print("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_TreeDict) {
				if(version != "2.4.2") {
					g_print("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			} else if(infotype == DictInfoType_ResDb) {
				if(version != "3.0.0") {
					g_print("Load %s failed: Unknown version.\n", ifofilename.c_str());
					return false;
				}
			}
		} else if(key == "idxoffsetbits") {
			if(value != "32") {
				// TODO
				g_print("Load %s failed: idxoffsetbits != 32 not supported presently.\n",
					ifofilename.c_str());
				return false;
			}
		} else if(key == "wordcount" && (infotype == DictInfoType_NormDict
			|| infotype == DictInfoType_TreeDict)) {
			wordcount = atol(value.c_str());
		} else if(key == "filecount" && infotype == DictInfoType_ResDb) {
			filecount = atol(value.c_str());
		} else if(key == "synwordcount") {
			synwordcount = atol(value.c_str());
		} else if(key == "tdxfilesize" && infotype == DictInfoType_TreeDict) {
			index_file_size = atol(value.c_str());
		} else if(key == "idxfilesize" && infotype == DictInfoType_NormDict) {
			index_file_size = atol(value.c_str());
		} else if(key == "ridxfilesize" && infotype == DictInfoType_ResDb) {
			index_file_size = atol(value.c_str());
		} else if(key == "dicttype" && infotype == DictInfoType_NormDict) {
			dicttype = value;
		} else if(key == "bookname") {
			bookname = value;
		} else if(key == "author") {
			author = value;
		} else if(key == "email") {
			email = value;
		} else if(key == "website") {
			website = value;
		} else if(key == "date") {
			date = value;
		} else if(key == "description") {
			parse_description(value.c_str(), value.length(), description);
		} else if(key == "sametypesequence") {
			sametypesequence = value;
		} else {
			g_print("Load %s warning: unknown key %s.\n", ifofilename.c_str(), 
				key.c_str());
		}
	}

	if(wordcount == 0 && ((infotype == DictInfoType_NormDict 
		|| infotype == DictInfoType_TreeDict))) {
		g_print("Load %s failed: wordcount not specified or 0.\n", 
			ifofilename.c_str());
		return false;
	}
	if(filecount == 0 && infotype == DictInfoType_ResDb) {
		g_print("Load %s failed: filecount not specified or 0.\n", 
			ifofilename.c_str());
		return false;
	}
	if(bookname.empty() && (infotype == DictInfoType_NormDict 
		|| infotype == DictInfoType_TreeDict)) {
		g_print("Load %s failed: bookname not specified.\n", 
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
		g_print("Load %s failed: %s not specified or 0.\n", 
			ifofilename.c_str(), kkey);
		return false;
	}

	return true;
}

void DictInfo::clear(void)
{
	ifo_file_name.clear();
	filecount = wordcount = 0;
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
}
