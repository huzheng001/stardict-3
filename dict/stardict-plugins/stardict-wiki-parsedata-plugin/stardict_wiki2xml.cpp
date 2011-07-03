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

#include "stardict_wiki2xml.h"
#include "WIKI2XML.h"
#include <cstring>
#include <glib.h>
#include <string.h>

std::string wiki2xml(std::string &str)
{
	WIKI2XML w2x(str);
	w2x.parse () ;
	return w2x.get_xml ();
}

struct WikiXmlParseUserData {
	std::string *res;
};

static void wikixml_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	WikiXmlParseUserData *Data = (WikiXmlParseUserData *)user_data;
	if (strcmp(element_name, "wikilink")==0) {
		Data->res->append("<span foreground=\"blue\" underline=\"single\">");
	}
}

static void wikixml_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	WikiXmlParseUserData *Data = (WikiXmlParseUserData *)user_data;
	if (strcmp(element_name, "wikilink")==0) {
		Data->res->append("</span>");
	}
}

static void wikixml_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	WikiXmlParseUserData *Data = (WikiXmlParseUserData *)user_data;
	char *estr = g_markup_escape_text(text, text_len);
	Data->res->append(estr);
	g_free(estr);
}

std::string wikixml2pango(std::string &str)
{
	std::string res;
	WikiXmlParseUserData Data;
	Data.res = &res;
	GMarkupParser parser;
	parser.start_element = wikixml_parse_start_element;
	parser.end_element = wikixml_parse_end_element;
	parser.text =  wikixml_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, str.c_str(), str.length(), NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	return res;
}
