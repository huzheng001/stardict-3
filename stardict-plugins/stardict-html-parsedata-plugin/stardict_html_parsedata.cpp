#include "stardict_html_parsedata.h"
#include <glib/gi18n.h>

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
		} else {
			p = g_utf8_next_char(q);
			pango.append(q, p-q);
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

static void html2result(const char *p, ParseResult &result)
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
		{ "b>", 2, "<b>", 0 },
		{ "/b>", 3, "</b>", 0 },
		{ "big>", 4, "<big>", 0},
		{ "/big>", 5, "</big>", 0},
		{ "i>", 2, "<i>", 0  },
		{ "/i>", 3, "</i>", 0 },
		{ "s>", 2, "<s>", 0  },
		{ "/s>", 3, "</s>", 0 },
		{ "sub>", 4, "<sub>", 0 },
		{ "/sub>", 5, "</sub>", 0},
		{ "sup>", 4, "<sup>", 0},
		{ "/sup>", 5, "</sup>", 0},
		{ "small>", 6, "<small>", 0},
		{ "/small>", 7, "</small>", 0},
		{ "tt>", 3, "<tt>", 0},
		{ "/tt>", 4, "</tt>", 0},
		{ "u>", 2, "<u>", 0  },
		{ "/u>", 3, "</u>", 0 },
		{ "br>", 3, "\n", 1 },
		{ "hr>", 3, "\n<span foreground=\"gray\"><s>     </s></span>\n", 7 },
		{ "/font>", 6, "</span>", 0 },
		{ NULL, 0, NULL },
	};

	for (cur_pos = 0; *p && (tag = strchr(p, '<')) != NULL;) {
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
				res += replace_arr[i].replace_;
				p += 1 + replace_arr[i].match_len_;
				cur_pos += replace_arr[i].char_len_;
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
				p1 += sizeof("color=") -1 +1;
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
					std::string color(p1, p2-p1);
					if (pango_color_parse(NULL, color.c_str())) {
						res += " foreground=\"";
						res += color;
						res += "\"";
					}
				}
			}
			res += ">";
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
		} else {
			p++;
			res += "&lt;";
			cur_pos++;
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
	item.type = ParseResultItemType_unknown;
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword)
{
	if (*p != 'h')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		html2result(p, result);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

static void configure()
{
}

bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: HTML data parse plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("HTML data parse"), _("HTML data parse engine."), _("Parse the HTML data."));
	obj->configure_func = configure;
	return false;
}

void stardict_plugin_exit(void)
{
}

bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	obj->parse_func = parse;
	g_print(_("HTML data parse plug-in loaded.\n"));
	return false;
}
