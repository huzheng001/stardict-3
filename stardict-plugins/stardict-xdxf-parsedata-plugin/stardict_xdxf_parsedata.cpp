#include "stardict_xdxf_parsedata.h"
#include <glib/gi18n.h>

#ifdef _WIN32
#include <windows.h>
#endif

static size_t xml_strlen(const std::string& str)
{
	const char *q;
	static const char* xml_entrs[] = { "lt;", "gt;", "amp;", "apos;", "quot;", 0 };
	static const int xml_ent_len[] = { 3,     3,     4,      5,       5 };
	size_t cur_pos;
	int i;

	for (cur_pos = 0, q = str.c_str(); *q; ++cur_pos) {
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
                        for (ient = 0; xml_entrs[ient] != 0; ++ient)
                                if (strncmp(amp + 1, xml_entrs[ient],
					    xml_ent_len[ient]) == 0) {
                                        decoded += raw_entrs[ient];
                                        amp += xml_ent_len[ient]+1;
                                        break;
                                }
                        if (xml_entrs[ient] == 0)    // unrecognized sequence
                                decoded += *amp++;

                } else {
                        decoded += *amp++;
                }        
}

static void xdxf2result(const char *p, ParseResult &result)
{
	LinksPosList links_list;
	std::string res;
	const char *tag, *next;
	std::string name;
	std::string::size_type cur_pos;
	int i;

	struct ReplaceTag {
		const char *match_;
		int match_len_;
		const char *replace_;
		int char_len_;
	};
	static const ReplaceTag replace_arr[] = {
		{ "abr>", 4, "<span foreground=\"green\" style=\"italic\">", 0 },
		{ "/abr>", 5, "</span>", 0 },
		{ "b>", 2, "<b>", 0 },
		{ "/b>", 3, "</b>", 0 },
		{ "i>", 2, "<i>", 0  },
		{ "/i>", 3, "</i>", 0 },
		{ "sub>", 4, "<sub>", 0 },
		{ "/sub>", 5, "</sub>", 0},
		{ "sup>", 4, "<sup>", 0},
		{ "/sup>", 5, "</sup>", 0},
		{ "tt>", 3, "<tt>", 0},
		{ "/tt>", 4, "</tt>", 0},
		{ "big>", 4, "<big>", 0},
		{ "/big>", 5, "</big>", 0},
		{ "small>", 6, "<small>", 0},
		{ "/small>", 7, "</small>", 0},
		{ "tr>", 3, "<b>[", 1 },
		{ "/tr>", 4, "]</b>", 1 },
		{ "ex>", 3, "<span foreground=\"violet\">", 0 },
		{ "/ex>", 4, "</span>", 0 },
		{ "/c>", 3, "</span>", 0 },
		{ NULL, 0, NULL },
	};

	bool is_first_k = true;
	for (cur_pos = 0; *p && (tag = strchr(p, '<')) != NULL;) {
		//TODO: do not create chunk
		std::string chunk(p, tag - p);
		res += chunk;
		cur_pos += xml_strlen(chunk);

		p = tag;
		for (i = 0; replace_arr[i].match_; ++i)
			if (strncmp(replace_arr[i].match_, p + 1,
						replace_arr[i].match_len_) == 0) {
				res += replace_arr[i].replace_;
				p += 1 + replace_arr[i].match_len_;
				cur_pos += replace_arr[i].char_len_;
				goto cycle_end;
			}

		if (strncmp("k>", p + 1, 2) == 0) {
			next = strstr(p + 3, "</k>");
			if (next) {
				if (is_first_k) {
					is_first_k = false;
					if (*(next + 4) == '\n')
						next++;
				} else {
					res += "<span foreground=\"blue\">";
					std::string chunk(p+3, next-(p+3));
					res += chunk;
					size_t xml_len = xml_strlen(chunk);
					cur_pos += xml_len;
					res += "</span>";
				}
				p = next + sizeof("</k>") - 1;
			} else
				p += sizeof("<k>") - 1;
		} else if (*(p + 1) == 'c' && (*(p + 2) == ' ' || *(p + 2) == '>')) {
			next = strchr(p, '>');
			if (!next) {
				++p;
				continue;
			}
			name.assign(p + 1, next - p - 1);
			std::string::size_type pos = name.find("c=\"");
			if (pos != std::string::npos) {
				pos += sizeof("c=\"") - 1;
				std::string::size_type end_pos = name.find("\"", pos);
				if (end_pos == std::string::npos)
					end_pos = name.length();

				std::string color(name, pos, end_pos - pos);
				if (pango_color_parse(NULL, color.c_str()))
					res += "<span foreground=\"" + color + "\">";
				else
					res += "<span>";
			} else
				res += "<span foreground=\"blue\">";
			p = next + 1;
		} else if (*(p + 1) == 'r' && *(p + 2) == 'r' && *(p + 3) == 'e' && *(p + 4) == 'f' && (*(p + 5) == ' ' || *(p + 5) == '>')) {
			next = strchr(p, '>');
			if (!next) {
				++p;
				continue;
			}
			name.assign(p + 1, next - p - 1);
			std::string type;
			std::string::size_type pos = name.find("type=\"");
			if (pos != std::string::npos) {
				pos += sizeof("type=\"") - 1;
				std::string::size_type end_pos = name.find("\"", pos);
				if (end_pos == std::string::npos)
					end_pos = name.length();
				type.assign(name, pos, end_pos - pos);
			}
			p = next + 1;
			next = strstr(p, "</rref>");
			if (!next)
				continue;
			std::string chunk(p, next - p);
			p = next + sizeof("</rref>") - 1;
			if (type.empty()) {
				if (g_str_has_suffix(chunk.c_str(), ".jpg") || g_str_has_suffix(chunk.c_str(), ".png")) {
					type = "image";
				} else if (g_str_has_suffix(chunk.c_str(), ".wav") || g_str_has_suffix(chunk.c_str(), ".mp3") || g_str_has_suffix(chunk.c_str(), ".ogg")) {
					type = "sound";
				} else if (g_str_has_suffix(chunk.c_str(), ".avi") || g_str_has_suffix(chunk.c_str(), ".mpeg")) {
					type = "video";
				} else {
					type = "attach";
				}
			}
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
			item.res->type = type;
			item.res->key = chunk;
			result.item_list.push_back(item);
		} else if ((*(p + 1) == 'k' || *(p + 1) == 'i') && *(p + 2) == 'r' && *(p + 3) == 'e' && *(p + 4) == 'f' && (*(p + 5) == ' ' || *(p + 5) == '>')) {
			bool is_k_or_i = (*(p + 1) == 'k');
			next = strchr(p, '>');
			if (!next) {
				++p;
				continue;
			}
			name.assign(p + 1, next - p - 1);
			std::string key;
			std::string::size_type pos;
			if (is_k_or_i)
				pos = name.find("k=\"");
			else
				pos = name.find("href=\"");
			if (pos != std::string::npos) {
				if (is_k_or_i)
					pos += sizeof("k=\"") - 1;
				else
					pos += sizeof("href=\"") - 1;
				std::string::size_type end_pos = name.find("\"", pos);
				if (end_pos == std::string::npos)
					end_pos = name.length();
				key.assign(name, pos, end_pos - pos);
			}

			p = next + 1;
			if (is_k_or_i)
				next = strstr(p, "</kref>");
			else
				next = strstr(p, "</iref>");
			if (!next)
				continue;

			res += "<span foreground=\"blue\" underline=\"single\">";
			std::string::size_type link_len = next - p;
			std::string chunk(p, link_len);
			size_t xml_len = xml_strlen(chunk);
			std::string xml_enc;
			if (key.empty())
				xml_decode(chunk.c_str(), xml_enc);
			else
				xml_decode(key.c_str(), xml_enc);
			std::string link;
			if (is_k_or_i)
				link = "query://";
			link += xml_enc;
			links_list.push_back(LinkDesc(cur_pos, xml_len, link));
			res += chunk;
			cur_pos += xml_len;
			res += "</span>";
			if (is_k_or_i)
				p = next + sizeof("</kref>") - 1;
			else
				p = next + sizeof("</iref>") - 1;
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
	res += p;
	ParseResultItem item;
	item.type = ParseResultItemType_link;
	item.link = new ParseResultLinkItem;
	item.link->pango = res;
	item.link->links_list = links_list;
	result.item_list.push_back(item);
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'x')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		xdxf2result(p, result);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

static void configure()
{
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: XDXF data parsing plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("XDXF data parsing"), _("XDXF data parsing engine."), _("Parse the XDXF data."));
	obj->configure_func = configure;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("XDXF data parsing plug-in loaded.\n"));
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
