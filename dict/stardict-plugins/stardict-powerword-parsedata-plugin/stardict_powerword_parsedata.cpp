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

#include "stardict_powerword_parsedata.h"
#include <cstring>
#include <glib/gi18n.h>

#ifdef _WIN32
#include <windows.h>
#endif

static size_t xml_strlen(const char *xmlstr)
{
	const char *q;
	static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
	static const int xml_ent_len[] = { 3,     3,     4,      5,       5 };
	size_t cur_pos;
	int i;

	for (cur_pos = 0, q = xmlstr; *q; ++cur_pos) {
		if (*q == '&') {
			for (i = 0; xml_entrs[i]; ++i)
				if (strncmp(xml_entrs[i], q + 1,
					    xml_ent_len[i]) == 0) {
					q += xml_ent_len[i] + 1;
					break;
				}
			if (xml_entrs[i] == NULL)
				++q;
		} else if (*q == '<') {
			const char *p = strchr(q+1, '>');
			if (p)
				q = p + 1;
			else
				++q;
			--cur_pos;
		} else
			q = g_utf8_next_char(q);
	}

	return cur_pos;
}

static gchar* toUtfPhonetic(const gchar *text, gsize len)
{
	std::string p;
	gsize i;
	for (i=0;i<len;i++) {
		switch (text[i]) {
			case 'A':
				p+="æ"; break;
			case 'B':
				p+="ɑ"; break;
			case 'C':
				p+="ɔ"; break;
			case 'D':
				p+="ã"; break;
			case 'E':
				p+="ә"; break;
			case 'F':
				p+="ʃ"; break;
			case 'G':
				p+="ɣ"; break;
			case 'H':
				p+="Ч"; break;
			case 'I':
				p+="i"; break;
			case 'J':
				p+="ʊ"; break;
			case 'K':
				p+="ʏ"; break;
			case 'L':
				p+="ɚ"; break;
			case 'M':
				p+="ɲ"; break;
			case 'N':
				p+="ŋ"; break;
			case 'P':
				p+="ɵ"; break;
			case 'Q':
				p+="ʌ"; break;
			case 'R':
				p+="ɔ"; break;
			case 'T':
				p+="ð"; break;
			case 'V':
				p+="ʒ"; break;
			case 'W':
				p+="θ"; break;
			case 'X':
				p+="Ø"; break;
			case 'Z':
				p+="є"; break;
			case '5':
				p+="'"; break;
			case '6':
				p+="!"; break;
			case '7':
				p+="͵"; break;
			case '9':
				p+="ˏ"; break;
			case '_':
				p+="ˇ"; break;
			case '=':
				p+="ê"; break;
			case 'l':
				p+="l"; break;
			case '^':
				p+="ɡ"; break;
			case '\\':
				p+="ɜ"; break;
			case '?':
				p+="U"; break;
			case '@':
				p+="S"; break;
			case '[':
				p+="ə"; break;
			default:
				p+=text[i];
				break;
		}
	}
	return g_markup_escape_text(p.c_str(), -1);
}

static gchar* toUtfPhonetic2(const gchar *text, glong len)
{
	std::string p;
	const char *s = text;
	const char *n;
	std::string uc;
	while (s-text < len) {
		n = g_utf8_next_char(s);
		uc.assign(s, n-s);
		if (uc == "8")
			p+=":";
		else if (uc == "0")
			p+="Ŋ";
		else if (uc == "¾")
			p+="ǔ";
		else if (uc == "%")
			p+="ɔ";
		else if (uc == "µ")
			p+="ě";
		else if (uc == "³")
			p+="ā";
		else if (uc == "!")
			p+="I";
		else if (uc == "W")
			p+="ɛ";
		else if (uc == "&")
			p+="U";
		else if (uc == "…")
			p+="ə";
		else if (uc == "¹")
			p+="ǐ";
		else if (uc == "“")
			p+="′";
		else if (uc == "*")
			p+="ə";
		else if (uc == "6")
			p+="ˋ";
		else if (uc == "+")
			p+="ɚ";
		else if (uc == "”")
			p+="´";
		else if (uc == "‘")
			p+="KH";
		else if (uc == "$")
			p+="ɑ";
		else if (uc == "7")
			p+="͵";
		else if (uc == "'")
			p+="KH";
		else if (uc == "½")
			p+="ō";
		else if (uc == "¼")
			p+="ǒ";
		else if (uc == "¶")
			p+="ē";
		else if (uc == "º")
			p+="ī";
		else if (uc == "G")
			p+="θ";
		else if (uc == "9")
			p+="ʒ";
		else if (uc == ".")
			p+="ʃ";
		else if (uc == "/")
			p+="ʒ";
		else if (uc == "²")
			p+="ǎ";
		else if (uc == "#")
			p+="æ";
		else if (uc == "’")
			p+="N";
		else if (uc == "Y")
			p+="t";
		else if (uc == "H")
			p+="ð";
		else if (uc == "÷")
			p+="ń";
		else if (uc == "é")
			p+="ê";
		else if (uc == "¿")
			p+="ū";
		else if (uc == ")")
			p+="ɜ";
		else if (uc == "Ó")
			p+="ǒ";
		else if (uc == "ï")
			p+="Ś";
		else if (uc == "Ä")
			p+="ǐ";
		else if (uc == "\\")
			p+="ł";
		else if (uc == "›")
			p+="ōō";
		else if (uc == "‹")
			p+="ǒǒ";
		else if (uc == "ý")
			p+="V";
		else
			p+= uc;
		s = n;
	}
	return g_markup_escape_text(p.c_str(), -1);
}

static void powerword_markup_add_text(const gchar *text, gssize length, std::string *pango, std::string::size_type &cur_pos, LinksPosList *links_list)
{
	const gchar *p;
	const gchar *end;
	p = text;
	end = text + length;

	GString *str;
	str = g_string_sized_new (length);

	const gchar *n;
	bool find;
	bool previous_islink = false;
	std::string marktags;
	guint currentmarktag = 0;
	while (p != end) {
		const gchar *next;
		next = g_utf8_next_char (p);
		switch (*p) {
			case '}':
				if (currentmarktag==0) {
					g_string_append (str, "}");
					previous_islink = false;
				}
				else {
					currentmarktag--;
					switch (marktags[currentmarktag]) {
						case 'b':
						case 'B':
							g_string_append (str, "</b>");
							previous_islink = false;
							break;
						case 'I':
							g_string_append (str, "</i>");
							previous_islink = false;
							break;
						case '+':
							g_string_append (str, "</sup>");
							previous_islink = false;
							break;
						case '-':
							g_string_append (str, "</sub>");
							previous_islink = false;
							break;
						case 'x':
							g_string_append (str, "</span>");
							previous_islink = false;
							break;
						case 'l':
						case 'D':
						case 'L':
						case 'U':
							g_string_append (str, "</span>");
							previous_islink = true;
							break;
						default:
							previous_islink = false;
							break;
					}
				}
				break;
			case '&':
				find = false;
				if (next!=end) {
					n = g_utf8_next_char(next);
					if (n!=end && *n == '{') {
						find=true;
						currentmarktag++;
						if (marktags.length()<currentmarktag)
							marktags+=*next;
						else
							marktags[currentmarktag-1]=*next;
						switch (*next) {
							case 'b':
							case 'B':
								g_string_append (str, "<b>");
								next = n+1;
								break;
							case 'I':
								g_string_append (str, "<i>");
								next = n+1;
								break;
							case '+':
								g_string_append (str, "<sup>");
								next = n+1;
								break;
							case '-':
								g_string_append (str, "<sub>");
								next = n+1;
								break;
							case 'x':
								g_string_append (str, "<span foreground=\"blue\" underline=\"single\">");
								next = n+1;
								break;
							case 'X':
							case '2':
								{
								const gchar *tag_end = n+1;
								while (tag_end!=end) {
									if (*tag_end=='}')
										break;
									else
										tag_end++;
								}
								g_string_append (str, "<span foreground=\"blue\">");
								gchar *tag_str;
								if (*next == 'X') {
									tag_str = toUtfPhonetic(n+1, tag_end - (n+1));
								} else {
									tag_str = toUtfPhonetic2(n+1, tag_end - (n+1)); // Used by 金山词霸 2007 《美国传统词典[双解]》 dictionary.
								}
								g_string_append (str, tag_str);
								g_free(tag_str);
								g_string_append (str, "</span>");
								currentmarktag--;
								if (tag_end!=end)
									next = tag_end+1;
								else
									next = end;
								previous_islink = false;
								break;
								}
							case 'l':
							case 'D':
							case 'L':
							case 'U':
								if (previous_islink)
									g_string_append (str, "\t");
								if (*next == 'l' || *next == 'D')
									g_string_append (str, "<span foreground=\"blue\" underline=\"single\">");
								else
									g_string_append (str, "<span foreground=\"#008080\" underline=\"single\">");
								*pango += str->str;
								cur_pos += xml_strlen(str->str);
								g_string_erase(str, 0, -1);
								{
								const gchar *tag_end = n+1;
								while (tag_end!=end) {
									if (*tag_end=='}')
										break;
									else
										tag_end++;
								}
								char *tmpstr = g_markup_escape_text(n+1, tag_end - (n+1));
								size_t xml_len = xml_strlen(tmpstr);
								std::string link("query://");
								link.append(n+1, tag_end - (n+1));
								links_list->push_back(LinkDesc(cur_pos, xml_len, link));
								*pango += tmpstr;
								cur_pos += xml_len;
								g_free(tmpstr);
								g_string_append (str, "</span>");
								currentmarktag--;
								if (tag_end!=end)
									next = tag_end+1;
								else
									next = end;
								previous_islink = true;
								break;
								}
							/*case ' ':
							case '9':
							case 'S':*/
							default:
								next = n+1;
								break;
						}
					}
				}
				if (!find) {
					previous_islink = false;
					g_string_append (str, "&amp;");
				}
				break;
			case '<':
				previous_islink = false;
				g_string_append (str, "&lt;");
				break;
			case '>':
				previous_islink = false;
				g_string_append (str, "&gt;");
				break;
			case '\'':
				previous_islink = false;
				g_string_append (str, "&apos;");
				break;
			case '"':
				previous_islink = false;
				g_string_append (str, "&quot;");
				break;
			default:
				previous_islink = false;
				g_string_append_len (str, p, next - p);
				break;
		}
		p = next;
	}
	if (currentmarktag>0) {
		do {
			currentmarktag--;
			switch (marktags[currentmarktag]) {
				case 'b':
				case 'B':
					g_string_append (str, "</b>");
					break;
				case 'I':
					g_string_append (str, "</i>");
					break;
				case '+':
					g_string_append (str, "</sup>");
					break;
				case '-':
					g_string_append (str, "</sub>");
					break;
				case 'x':
				case 'l':
				case 'D':
				case 'L':
				case 'U':
					g_string_append (str, "</span>");
					break;
				default:
					break;
			}
		} while (currentmarktag>0);
	}
	*pango += str->str;
	cur_pos += xml_strlen(str->str);
	g_string_free (str, TRUE);
}

typedef struct _PwUserData {
	std::string *pango;
	LinksPosList *links_list;
	std::string::size_type cur_pos;
	const gchar *oword;
	bool first_jbcy;
} PwUserData;

static void func_parse_passthrough(GMarkupParseContext *context, const gchar *passthrough_text, gsize text_len, gpointer user_data, GError **error)
{
	if (!g_str_has_prefix(passthrough_text, "<![CDATA["))
		return;
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	const gchar *text = passthrough_text+9;
	gsize len = text_len-9-3;
	while (g_ascii_isspace(*text)) {
		text++;
		len--;
	}
	while (len>0 && g_ascii_isspace(*(text+len-1))) {
		len--;
	}
	if (len==0)
		return;
	std::string *pango = ((PwUserData*)user_data)->pango;
	std::string::size_type &cur_pos = ((PwUserData*)user_data)->cur_pos;
	if (strcmp(element, "词典音标")==0 || strcmp(element, "CB")==0) {
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		*pango+="[<span foreground=\"blue\">";
		cur_pos++;
		gchar *str = toUtfPhonetic(text, len);
		*pango+=str;
		cur_pos+=xml_strlen(str);
		g_free(str);
		*pango+="</span>]";
		cur_pos++;
	} else if (strcmp(element, "单词原型")==0 || strcmp(element, "YX")==0) {
		const gchar *oword = ((PwUserData*)user_data)->oword;
		if (strncmp(oword, text, len)) {
			if (!pango->empty()) {
				*pango+='\n';
				cur_pos++;
			}
			*pango+="<b>";
			gchar *str = g_markup_escape_text(text, len);
			pango->append(str);
			cur_pos+=xml_strlen(str);
			g_free(str);
			*pango+="</b>";
		}
	} else if (strcmp(element, "单词词性")==0 || strcmp(element, "DX")==0) {
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		*pango+="<i>";
		powerword_markup_add_text(text, len, pango, cur_pos, ((PwUserData*)user_data)->links_list);
		*pango+="</i>";
	} else if (strcmp(element, "汉语拼音")==0 || strcmp(element, "PY")==0) {
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		*pango+="<span foreground=\"blue\" underline=\"single\">";
		powerword_markup_add_text(text, len, pango, cur_pos, ((PwUserData*)user_data)->links_list);
		*pango+="</span>";
	} else if (strcmp(element, "例句原型")==0 || strcmp(element, "LY")==0) {
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		*pango+="<span foreground=\"#008080\">";
		powerword_markup_add_text(text, len, pango, cur_pos, ((PwUserData*)user_data)->links_list);
		*pango+="</span>";
	} else if (strcmp(element, "例句解释")==0 || strcmp(element, "LS")==0) {
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		*pango+="<span foreground=\"#01259A\">";
		powerword_markup_add_text(text, len, pango, cur_pos, ((PwUserData*)user_data)->links_list);
		*pango+="</span>";
	/*} else if (strcmp(element, "相关词")==0) {
		if (!res->empty())
			*res+='\n';
		std::string tabstr;
		tabstr+=text[0];
		for (gsize i=1;i<len;i++) {
			if (text[i]=='&')
				tabstr+="\t&";
			else
				tabstr+=text[i];
		}
		gchar *str = powerword_markup_escape_text(tabstr.c_str(), tabstr.length());
		res->append(str);
		g_free(str);*/
	} else
	/*} else if (
	strcmp(element, "解释项")==0 ||
	strcmp(element, "跟随解释")==0 ||
	strcmp(element, "相关词")==0 ||
	strcmp(element, "预解释")==0 ||
	strcmp(element, "繁体写法")==0 ||
	strcmp(element, "台湾音标")==0 ||
	strcmp(element, "图片名称")==0 ||
	strcmp(element, "跟随注释")==0 ||
	strcmp(element, "音节分段")==0 ||
	strcmp(element, "AHD音标")==0 ||
	strcmp(element, "国际音标")==0 ||
	strcmp(element, "美国音标")==0 ||
	strcmp(element, "子解释项")==0 ||
	strcmp(element, "同义词")==0 ||
	strcmp(element, "日文发音")==0 ||
	strcmp(element, "惯用型原型")==0 ||
	strcmp(element, "惯用型解释")==0 ||
	strcmp(element, "另见")==0
	) {*/
	{
		if (!pango->empty()) {
			*pango+='\n';
			cur_pos++;
		}
		powerword_markup_add_text(text, len, pango, cur_pos, ((PwUserData*)user_data)->links_list);
	}
}

static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	std::string res;
	if (strcmp(element_name, "基本词义")==0 || strcmp(element_name, "CY")==0) {
		if (((PwUserData*)user_data)->first_jbcy) {
			((PwUserData*)user_data)->first_jbcy = false;
		} else {
			res="\n<span foreground=\"blue\">&lt;基本词义&gt;</span>";
		}
	// ToDo: These need to fix!
	} else if (strcmp(element_name, "继承用法")==0) {
		res="\n<span foreground=\"blue\">&lt;继承用法&gt;</span>";
	} else if (strcmp(element_name, "习惯用语")==0) {
		res="\n<span foreground=\"blue\">&lt;习惯用语&gt;</span>";
	} else if (strcmp(element_name, "词性变化")==0) {
		res="\n<span foreground=\"blue\">&lt;词性变化&gt;</span>";
	} else if (strcmp(element_name, "特殊用法")==0) {
		res="\n<span foreground=\"blue\">&lt;特殊用法&gt;</span>";
	} else if (strcmp(element_name, "参考词汇")==0) {
		res="\n<span foreground=\"blue\">&lt;参考词汇&gt;</span>";
	} else if (strcmp(element_name, "常用词组")==0) {
		res="\n<span foreground=\"blue\">&lt;常用词组&gt;</span>";
	} else if (strcmp(element_name, "语源")==0) {
		res="\n<span foreground=\"blue\">&lt;语源&gt;</span>";
	} else if (strcmp(element_name, "派生")==0) {
		res="\n<span foreground=\"blue\">&lt;派生&gt;</span>";
	} else if (strcmp(element_name, "用法")==0) {
		res="\n<span foreground=\"blue\">&lt;用法&gt;</span>";
	} else if (strcmp(element_name, "注释")==0) {
		res="\n<span foreground=\"blue\">&lt;注释&gt;</span>";
	}
	if (!res.empty()) {
		*(((PwUserData*)user_data)->pango) += res;
		((PwUserData*)user_data)->cur_pos += xml_strlen(res.c_str());
	}
}

static void powerword2link(const char *p, guint32 sec_size, const gchar *oword, std::string *pango, LinksPosList *links_list)
{
	PwUserData Data;
	Data.pango = pango;
	Data.links_list = links_list;
	Data.cur_pos = 0;
	Data.oword = oword;
	Data.first_jbcy = true;

	GMarkupParser parser;
	parser.start_element = func_parse_start_element;
	parser.end_element = NULL;
	parser.text = NULL;
	parser.passthrough = func_parse_passthrough;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, p, sec_size, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'k')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		std::string pango;
		LinksPosList links_list;
		powerword2link(p, len, oword, &pango, &links_list);
		ParseResultItem item;
		item.type = ParseResultItemType_link;
		item.link = new ParseResultLinkItem;
		item.link->pango = pango;
		item.link->links_list = links_list;
		result.item_list.push_back(item);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading PowerWord data parsing plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: PowerWord data parsing plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>2.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("PowerWord data parsing"), _("PowerWord data parsing engine."), _("Parse the PowerWord data."));
	obj->configure_func = NULL;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("PowerWord data parsing plug-in loaded.\n"));
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
