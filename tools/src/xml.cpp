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

#include <cstring>
#include <glib/gi18n.h>

#include "xml.h"

static const char raw_entrs[] = { '<',   '>',   '&',    '\'',    '\"',    0 };
static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
static const int xml_ent_len[] = { 3,     3,     4,      5,       5 };

void xml::add_and_encode(std::string& str, char ch)
{
	const char *res = strchr(raw_entrs, ch);
	if (!res)
		str += ch;
	else {
		str += '&';
		str += xml_entrs[res - raw_entrs];
	}
}

void xml::encode(const std::string& str, std::string& res)
{
	std::string::size_type irep = str.find_first_of(raw_entrs);
	if (irep == std::string::npos) {
		res = str;
		return;
	}

	res = std::string(str, 0, irep);
	std::string::size_type isize = str.size();

	while (irep != isize) {
		std::string::size_type ient;
		for (ient = 0; raw_entrs[ient] != 0; ++ient)
			if (str[irep] == raw_entrs[ient]) {
				res += '&';
				res += xml_entrs[ient];
				break;
			}
		if (raw_entrs[ient] == 0)
			res += str[irep];
		++irep;
	}
}

void xml::decode(std::string& str)
{
	std::string::size_type iamp = str.find('&');
	if (iamp == std::string::npos) {
		return;
	}
    
	std::string decoded(str, 0, iamp);
	std::string::size_type iSize = str.size();
	decoded.reserve(iSize);
  
	const char* ens = str.c_str();
	while (iamp != iSize) {
		if (str[iamp] == '&' && iamp+1 < iSize) {
			int ient;
			for (ient = 0; xml_entrs[ient] != 0; ++ient)
				if (strncmp(ens+iamp+1, xml_entrs[ient], xml_ent_len[ient]) == 0) {
					decoded += raw_entrs[ient];
					iamp += xml_ent_len[ient]+1;
					break;
				} 
			if (xml_entrs[ient] == 0)    // unrecognized sequence
				decoded += str[iamp++];   

		} else {
			decoded += str[iamp++];
		} 
	} 
   
	str = decoded;
}

void xml::Parser::xml_error(const std::string& line) const 
{
	g_critical("XML parser error: %s\nCan not parse such line: %s\n",
		      XML_ErrorString(XML_GetErrorCode(xmlp)), line.c_str());
}

xml::Parser::Parser(XML_StartElementHandler on_start,
		    XML_EndElementHandler on_end,
		    XML_CharacterDataHandler on_char,
		    void *data)
{
	xmlp = XML_ParserCreate("UTF-8");
	XML_SetElementHandler(xmlp, on_start, on_end);
	XML_SetCharacterDataHandler(xmlp, on_char);
	XML_SetUserData(xmlp, data);
}
