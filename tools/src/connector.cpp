/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://xdxf.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gi18n.h>

#include "connector.h"

//#define DEBUG

bool Connector::set_dict_info(const std::string& name, const std::string& val)
{
	dict_info_[name] = val;
	return true;
}

const std::string& Connector::get_dict_info(const std::string& name) const
{
	static std::string empty;

	StringMap::const_iterator it = dict_info_.find(name);

	if (it != dict_info_.end())
		return it->second;

	return empty;
}

//TODO: this should not exists in IGeneratorDictOps interface?
bool Connector::get_meta_info()
{
	g_assert(FALSE);
	return true;
}

//TODO: this should not exists in IGeneratorDictOps interface?
bool Connector::get_info()
{
	g_assert(FALSE);
	return true;
}

bool Connector::send_meta_info()
{
	if (workdir_.empty())
		workdir_ = ".";
	return generator_.on_prepare_generator(workdir_, get_dict_info("basename"));
}

bool Connector::send_info()
{
	return generator_.on_new_dict_info("lang_from", dict_info_["lang_from"]) &&
		generator_.on_new_dict_info("lang_to", dict_info_["lang_to"]) &&
		generator_.on_new_dict_info("full_name", dict_info_["full_name"]) &&
		generator_.on_new_dict_info("description", dict_info_["description"]);
}

bool Connector::abbrs_begin()
{
	return generator_.on_abbr_begin();
}

bool Connector::abbrs_end()
{
	return generator_.on_abbr_end();
}

void Connector::fill_key(const std::string& keystr)
{
	key_.clear();
	key_.parts_.push_back(std::string());

	const char *p = keystr.c_str(), *q, *end;
	while ((q = strchr(p, '<')) != NULL) {
		if (strncmp(q + 1, "opt>", 4) == 0) {
#ifdef DEBUG
			StdErr << "Part: " << std::string(p, q - p) << "\n";
#endif
			key_.parts_.back().append(p, q - p);
			q += sizeof("<opt>") - 1;
			end = strstr(q, "</opt>");
			if (!end) {
				g_critical("Invalid key, there are no </opt>: %s\n",
					      keystr.c_str());
				exit(EXIT_FAILURE);
			}
			key_.parts_.push_back(std::string());
			key_.opts_.push_back(std::string());
#ifdef DEBUG
			StdErr << "Opt: " << std::string(q, end - q) << "\n";
#endif
			key_.opts_.back().append(q, end - q);
			p = end + sizeof("</opt>") - 1;
		} else if (strncmp(q + 1, "nu />", 5) == 0) {
			key_.parts_.back().append(p, q - p);
			q += sizeof("<nu />") - 1;
			end = strstr(q, "<nu />");
			if (!end) {
				g_critical("Invalid key, there are no close <nu />: %s\n",
					      keystr.c_str());
				exit(EXIT_FAILURE);
			}
#ifdef DEBUG
			StdErr << "We skip: " << std::string(q, end - q) << "\n";
#endif
			p = end + sizeof("<nu />") - 1;
		} else {
			g_critical("Unkown tag here: %s, line: %s\n", q, keystr.c_str());
			exit(EXIT_FAILURE);
		}
	}

	key_.parts_.back().append(p);
}

bool Connector::abbr(const StringList& keylist, const std::string& data)
{
	StringList real_keylist;
	for (StringList::const_iterator it = keylist.begin();
	     it != keylist.end(); ++it) {		
		fill_key(*it);
		generate_keys(real_keylist);
	}
	return generator_.on_have_data(real_keylist, data);
}

bool Connector::article(const StringList& keylist, const std::string& data, bool kia)
{
	std::string keystr;
	StringList real_keylist;
	for (StringList::const_iterator it = keylist.begin();
	     it != keylist.end(); ++it) {
		if (!kia)
			keystr += std::string("<k>") + *it + "</k>\n";
		fill_key(*it);
		generate_keys(real_keylist);
	}

	return generator_.on_have_data(real_keylist, keystr + data);
}

bool Connector::end()
{
	return generator_.generate() == EXIT_SUCCESS;
}

bool Connector::abbr(const std::string& key, const std::string& data)
{
	StringList real_keylist;

	fill_key(key);
	generate_keys(real_keylist);
	
	return generator_.on_have_data(real_keylist, data);
}

bool Connector::article(const std::string& key, const std::string& data,
			bool kia)
{
	std::string keystr;
	StringList real_keylist;
	
	if (!kia)
		keystr += std::string("<k>") + key + "</k>\n";
	fill_key(key);
	generate_keys(real_keylist);
	
	return generator_.on_have_data(real_keylist, keystr + data);
}
