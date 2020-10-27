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

#include "stardict_html_parsedata.h"
#include <glib/gi18n.h>
#include <cstring>
#include <cstdlib>
#include <vector>

#ifdef _WIN32
#include <windows.h>

#ifdef _MSC_VER
#  define strncasecmp _strnicmp
#endif

static char *strcasestr (const char *phaystack, const char *pneedle)
{
	register const unsigned char *haystack, *needle;
	register char b, c;

	haystack = (const unsigned char *) phaystack;
	needle = (const unsigned char *) pneedle;

	b = tolower(*needle);
	if (b != '\0') {
		haystack--;             /* possible ANSI violation */
		do {
			c = *++haystack;
			if (c == '\0')
				goto ret0;
		} while (tolower(c) != (int) b);

		c = tolower(*++needle);
		if (c == '\0')
			goto foundneedle;
		++needle;
		goto jin;

		for (;;) {
			register char a;
			register const unsigned char *rhaystack, *rneedle;

			do {
				a = *++haystack;
				if (a == '\0')
					goto ret0;
				if (tolower(a) == (int) b)
					break;
				a = *++haystack;
				if (a == '\0')
					goto ret0;
			shloop:
				;
			}
			while (tolower(a) != (int) b);

		jin:      a = *++haystack;
			if (a == '\0')
				goto ret0;

			if (tolower(a) != (int) c)
				goto shloop;

			rhaystack = haystack-- + 1;
			rneedle = needle;
			a = tolower(*rneedle);

			if (tolower(*rhaystack) == (int) a)
				do {
					if (a == '\0')
						goto foundneedle;
					++rhaystack;
					a = tolower(*++needle);
					if (tolower(*rhaystack) != (int) a)
						break;
					if (a == '\0')
						goto foundneedle;
					++rhaystack;
					a = tolower(*++needle);
				} while (tolower (*rhaystack) == (int) a);

			needle = rneedle;             /* took the register-poor approach */

			if (a == '\0')
				break;
		}
	}
 foundneedle:
	return (char*) haystack;
 ret0:
	return 0;
}
#endif

/* pango_len length of the produced pango text in chars, but '&lt;', '&gt;', '&amp;', etc
 * are counted as one char. */
static void html_topango(const std::string& str, std::string &pango, size_t &pango_len)
{
	const char *q, *p;
	static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
	static const int xml_ent_len[] = { 3,     3,     4,      5,       5 };
	static const char* html_entries[] = {"nbsp;", 0};
	static const int html_entry_len[] = {5};
	static const char* html_values[] = {" "};
	static const int html_value_len[] = {1};
	size_t cur_pos;
	int i;
	char *etext;

	pango.clear();
	for (cur_pos = 0, q = str.c_str(); *q; ++cur_pos) {
		if (*q == '&') {
			for (i = 0; xml_entrs[i]; ++i) {
				if (strncasecmp(xml_entrs[i], q + 1,
					    xml_ent_len[i]) == 0) {
					q += xml_ent_len[i] + 1;
					pango += '&';
					pango += xml_entrs[i];
					break;
				}
			}
			if (xml_entrs[i] == NULL) {
				for (i = 0; html_entries[i]; ++i) {
					if (strncasecmp(html_entries[i], q+1, html_entry_len[i])==0) {
						q += html_entry_len[i] + 1;
						pango += html_values[i];
						cur_pos += (html_value_len[i] -1);
						break;
					}
				}
				if (html_entries[i] == NULL) {
					if (*(q+1)=='#' && (p = strchr(q+2, ';'))) {
						std::string str(q+2, p-(q+2));
						gunichar uc;
						uc = atoi(str.c_str());
						gchar utf8[7];
						gint n = g_unichar_to_utf8(uc, utf8);
						utf8[n] = '\0';
						pango += utf8;
						q = p+1;
					} else {
						++q;
						pango += "&amp;";
					}
				}
			}
		} else if (*q == '\r' || *q == '\n') {
			q++;
			cur_pos--;
		} else {
			p = g_utf8_next_char(q);
			etext = g_markup_escape_text(q, p-q);
			pango += etext;
			g_free(etext);
			q = p;
		}
	}

	pango_len = cur_pos;
}

static void xml_decode(const char *str, std::string& decoded)
{
	static const char raw_entrs[] = {
		'<',   '>',   '&',    '\'',    '\"',    0 
	};
	static const char* xml_entrs[] = {
		"lt;", "gt;", "amp;", "apos;", "quot;", 0 
	};
	static const int xml_ent_len[] = {
		3,     3,     4,      5,       5 
	};
	int ient;
	const char *amp = strchr(str, '&');

	if (amp == NULL) {
		decoded = str;
		return;
	}
	decoded.assign(str, amp - str);

	while (*amp)
		if (*amp == '&') {
			for (ient = 0; xml_entrs[ient] != 0; ++ient) {
				if (strncmp(amp + 1, xml_entrs[ient], xml_ent_len[ient]) == 0) {
					decoded += raw_entrs[ient];
					amp += xml_ent_len[ient]+1;
					break;
				}
			}
			if (xml_entrs[ient] == 0)    // unrecognized sequence
				decoded += *amp++;
		} else {
			decoded += *amp++;
		}
}

class HtmlParser
{
private:
	enum TagType { elOpen, elClose, elEmpty, elNone };
	enum Tag { tagNone, tagB, tagBig, tagI, tagS, tagSub, tagSup, tagSmall, tagTT, tagU, tagFont };
	struct ReplaceTag {
		const char *match_;
		int match_len_;
		const char *replace_;
		int char_len_;
		Tag tag;
		TagType type;
	};
	static const ReplaceTag replace_arr[];
	typedef std::vector<Tag> TTagStack;
	TTagStack tagStack;
	std::string res;
	std::string::size_type cur_pos;

	void add_tag(Tag tag, TagType type);
	const ReplaceTag* find_tag(Tag tag, TagType type);
	const ReplaceTag* find_tag(Tag tag);
	void apply_tag(const ReplaceTag* p);
	void end_of_input();
public:
	void html2result(const char *p, ParseResult &result);
};

const HtmlParser::ReplaceTag HtmlParser::replace_arr[] = {
	{ "b>", 2, "<b>", 0, tagB, elOpen },
	{ "/b>", 3, "</b>", 0, tagB, elClose },
	{ "big>", 4, "<big>", 0, tagBig, elOpen },
	{ "/big>", 5, "</big>", 0, tagBig, elClose },
	{ "i>", 2, "<i>", 0, tagI, elOpen },
	{ "/i>", 3, "</i>", 0, tagI, elClose },
	{ "s>", 2, "<s>", 0, tagS, elOpen },
	{ "/s>", 3, "</s>", 0, tagS, elClose },
	{ "sub>", 4, "<sub>", 0, tagSub, elOpen },
	{ "/sub>", 5, "</sub>", 0, tagSub, elClose },
	{ "sup>", 4, "<sup>", 0, tagSup, elOpen },
	{ "/sup>", 5, "</sup>", 0, tagSup, elClose },
	{ "small>", 6, "<small>", 0, tagSmall, elOpen },
	{ "/small>", 7, "</small>", 0, tagSmall, elClose },
	{ "tt>", 3, "<tt>", 0, tagTT, elOpen },
	{ "/tt>", 4, "</tt>", 0, tagTT, elClose },
	{ "u>", 2, "<u>", 0, tagU, elOpen },
	{ "/u>", 3, "</u>", 0, tagU, elClose },
	{ "br>", 3, "\n", 1, tagNone, elNone },
	{ "nl>", 3, "", 0, tagNone, elNone },
	{ "hr>", 3, "\n<span foreground=\"gray\"><s>     </s></span>\n", 7, tagNone, elNone },
	{ "/font>", 6, "</span>", 0, tagFont, elClose },
	{ NULL, 0, NULL, 0, tagNone, elNone },
};

void HtmlParser::add_tag(Tag tag, TagType type)
{
	switch(type)
	{
	case elNone:
	case elEmpty:
	{
		const ReplaceTag* p = find_tag(tag);
		g_assert(p);
		if(p)
			apply_tag(p);
		break;
	}
	case elOpen:
	{
		const ReplaceTag* p = find_tag(tag, elOpen);
		g_assert(p);
		if(p) {
			apply_tag(p);
			tagStack.push_back(tag);
		}
		break;
	}
	case elClose:
	{
		// search the paired open tag
		int openTagInd = -1;
		for(int i=static_cast<int>(tagStack.size())-1; i>=0; --i)
			if(tagStack[i] == tag) {
				openTagInd = i;
				break;
			}
		if (openTagInd < 0) {
			/* no matching open tag. Skip this close tag. */
		} else {
			/* close tags with indexes [openTagInd, tagStack.size()) */
			for(int i=static_cast<int>(tagStack.size())-1; i>=openTagInd; --i) {
				const ReplaceTag* p = find_tag(tagStack[i], elClose);
				g_assert(p);
				if(p)
					apply_tag(p);
			}
			tagStack.resize(openTagInd);
		}
		break;
	}
	}
}

void HtmlParser::end_of_input()
{
	// close all open tags
	for(int i=static_cast<int>(tagStack.size())-1; i>=0; --i) {
		const ReplaceTag* p = find_tag(tagStack[i], elClose);
		g_assert(p);
		if(p)
			apply_tag(p);
	}
	tagStack.clear();
}

const HtmlParser::ReplaceTag* HtmlParser::find_tag(Tag tag, TagType type)
{
	for (int i = 0; replace_arr[i].match_; ++i)
		if(replace_arr[i].tag == tag && replace_arr[i].type == type)
			return &replace_arr[i];
	return NULL;
}

const HtmlParser::ReplaceTag* HtmlParser::find_tag(Tag tag)
{
	for (int i = 0; replace_arr[i].match_; ++i)
		if(replace_arr[i].tag == tag)
			return &replace_arr[i];
	return NULL;
}

void HtmlParser::apply_tag(const ReplaceTag* p)
{
	res += p->replace_;
	cur_pos += p->char_len_;
}

void HtmlParser::html2result(const char *p, ParseResult &result)
{
	tagStack.clear();
	res.clear();
	cur_pos = 0;

	LinksPosList links_list;
	const char *tag, *next;
	std::string name;
	int i;

	for (; *p && (tag = strchr(p, '<')) != NULL;) {
		std::string chunk(p, tag - p);
		size_t pango_len;
		std::string pango;
		html_topango(chunk, pango, pango_len);
		res += pango;
		cur_pos += pango_len;

		p = tag;
		for (i = 0; replace_arr[i].match_; ++i)
			if (strncasecmp(replace_arr[i].match_, p + 1,
						replace_arr[i].match_len_) == 0) {
				add_tag(replace_arr[i].tag, replace_arr[i].type);
				p += 1 + replace_arr[i].match_len_;
				goto cycle_end;
			}

		if (strncasecmp(p+1, "font ", 5)==0) {
			next = strchr(p, '>');
			if (!next) {
				++p;
				continue;
			}
			res += "<span";
			name.assign(p + 6, next - (p + 6));
			const char *p1 = strcasestr(name.c_str(), "face=");
			if (p1) {
				p1 += sizeof("face=") -1 +1;
				const char *p2 = p1;
				while (true) {
					if (*p2 == '\0') {
						p2 = NULL;
						break;
					}
					if (*p2 == '\'' || *p2 == '"')
						break;
					p2++;
				}
				if (p2) {
					std::string face(p1, p2-p1);
					res += " face=\"";
					res += face;
					res += "\"";
				}
			}
			p1 = strcasestr(name.c_str(), "color=");
			if (p1) {
				p1 += sizeof("color=") -1;
				if (*p1 == '\'' || *p1 == '\"')
					p1++;
				const char *p2 = p1;
				while (true) {
					if (*p2 == '\0') {
						p2 = NULL;
						break;
					}
					if (*p2 == '\'' || *p2 == '"' || *p2 == ' ' || *p2 == '>')
						break;
					p2++;
				}
				if (p2) {
					std::string color(p1, p2-p1);
					if (pango_color_parse(NULL, color.c_str())) {
						res += " foreground=\"";
						res += color;
						res += "\"";
					}
				}
			}
			res += ">";
			tagStack.push_back(tagFont);
			p = next + 1;
		} else if ((*(p + 1) == 'a' || *(p + 1) == 'A') && *(p + 2) == ' ') {
			next = strchr(p, '>');
			if (!next) {
				p++;
				continue;
			}
			p+=3;
			name.assign(p, next - p);
			const char *p1 = strcasestr(name.c_str(), "href=");
			std::string link;
			if (p1) {
				p1 += sizeof("href=") -1 +1;
				const char *p2 = p1;
				while (true) {
					if (*p2 == '\0') {
						p2 = NULL;
						break;
					}
					if (*p2 == '\'' || *p2 == '"')
						break;
					p2++;
				}
				if (p2) {
					link.assign(p1, p2-p1);
				}
			}
			p = next + 1;
			next = strcasestr(p, "</a>");
			if (!next) {
				continue;
			}
			res += "<span foreground=\"blue\" underline=\"single\">";
			std::string::size_type link_len = next - p;
			std::string chunk(p, link_len);
			html_topango(chunk, pango, pango_len);
			links_list.push_back(LinkDesc(cur_pos, pango_len, link));
			res += pango;
			cur_pos += pango_len;
			res += "</span>";
			p = next + sizeof("</a>") - 1;
		} else if (strncasecmp(p+1, "ref>", 4)==0) {
			next = strcasestr(p, "</ref>");
			if (!next) {
				p++;
				continue;
			}
			p+=5;
			res += "<span foreground=\"blue\" underline=\"single\">";
			std::string::size_type link_len = next - p;
			std::string chunk(p, link_len);
			html_topango(chunk, pango, pango_len);
			std::string xml_enc;
			xml_decode(chunk.c_str(), xml_enc);
			std::string link;
			link = "query://";
			link += xml_enc;
			links_list.push_back(LinkDesc(cur_pos, pango_len, link));
			res += pango;
			cur_pos += pango_len;
			res += "</span>";
			p = next + sizeof("</ref>") - 1;
		} else if (strncasecmp(p+1, "img ", 4)==0) {
			next = strchr(p+5, '>');
			if (!next) {
				p++;
				continue;
			}
			name.assign(p+5, next - (p+5));
			p = next + 1;
			const char *p1 = strcasestr(name.c_str(), "src=");
			std::string src;
			if (p1) {
				p1 += sizeof("src=") -1 +1;
				const char *p2 = p1;
				while (true) {
					if (*p2 == '\0') {
						p2 = NULL;
						break;
					}
					if (*p2 == '\'' || *p2 == '"')
						break;
					p2++;
				}
				if (p2) {
					src.assign(p1, p2-p1);
				}
			}
			if (!src.empty()) {
				ParseResultItem item;
				item.type = ParseResultItemType_link;
				item.link = new ParseResultLinkItem;
				item.link->pango = res;
				item.link->links_list = links_list;
				result.item_list.push_back(item);
				res.clear();
				cur_pos = 0;
				links_list.clear();
				item.type = ParseResultItemType_res;
				item.res = new ParseResultResItem;
				item.res->type = "image";
				int n = src.length();
				if (src[0]==0x1e && src[n-1]==0x1f) {
					item.res->key.assign(src.c_str()+1, n-2);
				} else {
					item.res->key = src;
				}
				result.item_list.push_back(item);
			}
		} else {
			next = strchr(p+1, '>');
			if (!next) {
				p++;
				res += "&lt;";
				cur_pos++;
				continue;
			}
			p = next + 1;
		}
cycle_end:
		;
	}
	{
		size_t pango_len;
		std::string pango;
		html_topango(p, pango, pango_len);
		res += pango;
		cur_pos += pango_len;
	}
	end_of_input();
	ParseResultItem item;
	item.type = ParseResultItemType_link;
	item.link = new ParseResultLinkItem;
	item.link->pango = res;
	item.link->links_list = links_list;
	result.item_list.push_back(item);
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'h')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		HtmlParser parser;
		parser.html2result(p, result);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading HTML data parsing plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: HTML data parsing plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("HTML data parsing"), _("HTML data parsing engine."), _("Parse the HTML data."));
	obj->configure_func = NULL;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("HTML data parsing plug-in loaded.\n"));
	return false;
}

#ifdef _WIN32
BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
#endif
