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

#include "stardict_xdxf_parsedata.h"
#include <glib/gi18n.h>
#include <cstring>
#include <vector>
#include <string>
#include <iostream>

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const StarDictPluginSystemInfo *plugin_info = NULL;
static IAppDirs* gpAppDirs = NULL;
const char config_section[] = "xdxf";

struct ColorScheme {
	guint32 abr;
	guint32 ex;
	guint32 k;
	guint32 c;
	guint32 ref;
};

ColorScheme color_scheme;

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

/* concatenate path1 and path2 inserting a path separator in between if needed. */
static std::string build_path(const std::string& path1, const std::string& path2)
{
	std::string res;
	res.reserve(path1.length() + 1 + path2.length());
	res = path1;
	if(!res.empty() && res[res.length()-1] != G_DIR_SEPARATOR)
		res += G_DIR_SEPARATOR_S;
	if(!path2.empty() && path2[0] == G_DIR_SEPARATOR)
		res.append(path2, 1, std::string::npos);
	else
		res.append(path2);
	return res;
}

static std::string get_cfg_filename()
{
	return build_path(gpAppDirs->get_user_config_dir(), "xdxf_parser.cfg");
}

static std::string print_pango_color(guint32 c)
{
	char buf[8]; // #001122
	gint n = g_snprintf(buf, sizeof(buf), "#%06x", c & 0xffffff);
	if(n != sizeof(buf)-1)
		return "";
	else
		return buf;
}

static GdkRGBA guint32_2_gdkrgba(guint32 c)
{
	GdkRGBA gdkrgba;
	gdkrgba.red = ((c & 0xff0000) >> 16) / (gdouble)0xff;
	gdkrgba.green = ((c & 0x00ff00) >> 8) / (gdouble)0xff;
	gdkrgba.blue = ((c & 0x0000ff) >> 0) / (gdouble)0xff;
	gdkrgba.alpha = 1;
	return gdkrgba;
}

static guint32 gdkrgba_2_guint32(GdkRGBA c)
{
	return (guint32((c.red * 0xff)) << 16)
		| (guint32((c.green * 0xff)) << 8)
		| (guint32(c.blue * 0xff));
}

static std::string generate_config_content(const ColorScheme& cs)
{
	gchar *data = g_strdup_printf("[%s]\n"
		"abr_color=%u\n"
		"ex_color=%u\n"
		"k_color=%u\n"
		"c_color=%u\n"
		"ref_color=%u\n",
		config_section,
		cs.abr,
		cs.ex,
		cs.k,
		cs.c,
		cs.ref
	);
	std::string res = data;
	g_free(data);
	return res;
}

static void load_config_file(ColorScheme& cs)
{
	std::string confPath = get_cfg_filename();
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, confPath.c_str(), G_KEY_FILE_NONE, NULL);
	GError *err = NULL;
	gint value;
	value = g_key_file_get_integer(keyfile, config_section, "abr_color", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
	} else
		cs.abr = value;
	value = g_key_file_get_integer(keyfile, config_section, "ex_color", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
	} else
		cs.ex = value;
	value = g_key_file_get_integer(keyfile, config_section, "k_color", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
	} else
		cs.k = value;
	value = g_key_file_get_integer(keyfile, config_section, "c_color", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
	} else
		cs.c = value;
	value = g_key_file_get_integer(keyfile, config_section, "ref_color", &err);
	if (err) {
		g_error_free (err);
		err = NULL;
	} else
		cs.ref = value;
	g_key_file_free(keyfile);
}

static void set_default_color_scheme(void)
{
	// ABBYY Lingvo 12 default color scheme
	color_scheme.abr = 0x007F00;
	color_scheme.ex = 0x7F7F7F;
	color_scheme.k = 0x000000;
	color_scheme.c = 0x0066FF;
	color_scheme.ref = 0x00007F;
}

struct ReplaceTag {
	ReplaceTag(const char* match, int match_len, const std::string& replace, int char_len)
	:
		match_(match),
		match_len_(match_len),
		replace_(replace),
		char_len_(char_len)
	{
	}
	const char *match_;
	int match_len_;
	std::string replace_;
	int char_len_;
};

class XDXFParser {
public:
	XDXFParser(const char *p, ParseResult &result);
	static void fill_replace_arr(void);
private:
	void flush(void);
private:
	ParseResult& result_;
	LinksPosList links_list_;
	std::string res_;
	std::string::size_type cur_pos_;

	static std::vector<ReplaceTag> replace_arr_;
};

std::vector<ReplaceTag> XDXFParser::replace_arr_;

void XDXFParser::fill_replace_arr(void)
{
	replace_arr_.clear();
	std::string value;
	replace_arr_.push_back(ReplaceTag("abr>", 4,
		std::string("<span foreground=\"") + print_pango_color(color_scheme.abr) + "\" style=\"italic\">",
		0));
	replace_arr_.push_back(ReplaceTag("/abr>", 5, "</span>", 0));
	replace_arr_.push_back(ReplaceTag("b>", 2, "<b>", 0));
	replace_arr_.push_back(ReplaceTag("/b>", 3, "</b>", 0));
	replace_arr_.push_back(ReplaceTag("i>", 2, "<i>", 0));
	replace_arr_.push_back(ReplaceTag("/i>", 3, "</i>", 0));
	replace_arr_.push_back(ReplaceTag("sub>", 4, "<sub>", 0));
	replace_arr_.push_back(ReplaceTag("/sub>", 5, "</sub>", 0));
	replace_arr_.push_back(ReplaceTag("sup>", 4, "<sup>", 0));
	replace_arr_.push_back(ReplaceTag("/sup>", 5, "</sup>", 0));
	replace_arr_.push_back(ReplaceTag("tt>", 3, "<tt>", 0));
	replace_arr_.push_back(ReplaceTag("/tt>", 4, "</tt>", 0));
	replace_arr_.push_back(ReplaceTag("big>", 4, "<big>", 0));
	replace_arr_.push_back(ReplaceTag("/big>", 5, "</big>", 0));
	replace_arr_.push_back(ReplaceTag("small>", 6, "<small>", 0));
	replace_arr_.push_back(ReplaceTag("/small>", 7, "</small>", 0));
	replace_arr_.push_back(ReplaceTag("tr>", 3, "<b>[", 1));
	replace_arr_.push_back(ReplaceTag("/tr>", 4, "]</b>", 1));
	replace_arr_.push_back(ReplaceTag("ex>", 3,
		std::string("<span foreground=\"") + print_pango_color(color_scheme.ex) + "\">",
		0));
	replace_arr_.push_back(ReplaceTag("/ex>", 4, "</span>", 0));
	replace_arr_.push_back(ReplaceTag("/c>", 3, "</span>", 0));
}

XDXFParser::XDXFParser(const char *p, ParseResult &result) :
	result_(result)
{
	const char *tag, *next;
	std::string name;
	int i;

	bool is_first_k = true;
	for (cur_pos_ = 0; *p && (tag = strchr(p, '<')) != NULL;) {
		//TODO: do not create chunk
		std::string chunk(p, tag - p);
		res_ += chunk;
		cur_pos_ += xml_strlen(chunk);

		p = tag;
		for (i = 0; i < static_cast<int>(replace_arr_.size()); ++i)
			if (strncmp(replace_arr_[i].match_, p + 1,
						replace_arr_[i].match_len_) == 0) {
				res_ += replace_arr_[i].replace_;
				p += 1 + replace_arr_[i].match_len_;
				cur_pos_ += replace_arr_[i].char_len_;
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
					res_ += std::string("<span foreground=\"") + print_pango_color(color_scheme.k) + "\">";
					std::string chunk(p+3, next-(p+3));
					res_ += chunk;
					size_t xml_len = xml_strlen(chunk);
					cur_pos_ += xml_len;
					res_ += "</span>";
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
					res_ += "<span foreground=\"" + color + "\">";
				else
					res_ += "<span>";
			} else
				res_ += std::string("<span foreground=\"") + print_pango_color(color_scheme.c) + "\">";
			p = next + 1;
		} else if (*(p + 1) == 'r' && *(p + 2) == 'r' && *(p + 3) == 'e' 
			&& *(p + 4) == 'f' && (*(p + 5) == ' ' || *(p + 5) == '>')) {
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
				if (g_str_has_suffix(chunk.c_str(), ".jpg") 
					|| g_str_has_suffix(chunk.c_str(), ".png")
					|| g_str_has_suffix(chunk.c_str(), ".bmp")) {
					type = "image";
				} else if (g_str_has_suffix(chunk.c_str(), ".wav") 
					|| g_str_has_suffix(chunk.c_str(), ".mp3") 
					|| g_str_has_suffix(chunk.c_str(), ".ogg")) {
					type = "sound";
				} else if (g_str_has_suffix(chunk.c_str(), ".avi") 
					|| g_str_has_suffix(chunk.c_str(), ".mpeg")
					|| g_str_has_suffix(chunk.c_str(), ".mpg")) {
					type = "video";
				} else {
					type = "attach";
				}
			}
			flush();
			ParseResultItem item;
			item.type = ParseResultItemType_res;
			item.res = new ParseResultResItem;
			item.res->type = type;
			item.res->key = chunk;
			result_.item_list.push_back(item);
		} else if ((*(p + 1) == 'k' || *(p + 1) == 'i') && *(p + 2) == 'r' 
			&& *(p + 3) == 'e' && *(p + 4) == 'f' && (*(p + 5) == ' ' 
			|| *(p + 5) == '>')) {
			// kref and iref
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

			res_ += std::string("<span foreground=\"") + print_pango_color(color_scheme.ref) + "\" underline=\"single\">";
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
			links_list_.push_back(LinkDesc(cur_pos_, xml_len, link));
			res_ += chunk;
			cur_pos_ += xml_len;
			res_ += "</span>";
			if (is_k_or_i)
				p = next + sizeof("</kref>") - 1;
			else
				p = next + sizeof("</iref>") - 1;
		} else if (strncmp("blockquote", p + 1, 10) == 0 && (*(p + 11) == ' '
				|| *(p + 11) == '>')) {
			next = strchr(p, '>');
			if (!next) {
				++p;
				continue;
			}
			p = next + 1;
			flush();
			ParseResultItem item;
			item.type = ParseResultItemType_FormatBeg;
			item.format_beg = new ParseResultFormatBegItem;
			item.format_beg->type = ParseResultItemFormatType_Indent;
			result_.item_list.push_back(item);
		} else if (strncmp("/blockquote>", p + 1, 12) == 0) {
			p += sizeof("/blockquote>");
			flush();
			ParseResultItem item;
			item.type = ParseResultItemType_FormatEnd;
			item.format_end = new ParseResultFormatEndItem;
			item.format_end->type = ParseResultItemFormatType_Indent;
			result_.item_list.push_back(item);
		} else {
			next = strchr(p+1, '>');
			if (!next) {
				p++;
				res_ += "&lt;";
				cur_pos_++;
				continue;
			}
			p = next + 1;
		}
cycle_end:
		;
	}
	res_ += p;
	flush();
}

void XDXFParser::flush(void) 
{
	if (res_.empty()) {
		g_assert(cur_pos_ == 0);
		g_assert(links_list_.empty());
		return;
	}
	ParseResultItem item;
	if(links_list_.empty()) {
		item.type = ParseResultItemType_mark;
		item.mark = new ParseResultMarkItem;
		item.mark->pango = res_;
	} else {
		item.type = ParseResultItemType_link;
		item.link = new ParseResultLinkItem;
		item.link->pango = res_;
		item.link->links_list = links_list_;
	}
	result_.item_list.push_back(item);
	res_.clear();
	cur_pos_ = 0;
	links_list_.clear();
}

static bool parse(const char *p, unsigned int *parsed_size, ParseResult &result, 
	const char *oword)
{
	if (*p != 'x')
		return false;
	p++;
	size_t len = strlen(p);
	if (len) {
		XDXFParser(p, result);
	}
	*parsed_size = 1 + len + 1;
	return true;
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("XDXF parser configuration"), 
		GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, 
		GTK_STOCK_OK, GTK_RESPONSE_OK, 
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
		NULL);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(vbox),5);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 5);
#endif
	GtkWidget *label = gtk_label_new(_("Abbreviation"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	GdkRGBA color;
	color = guint32_2_gdkrgba(color_scheme.abr);
	GtkWidget *colorbutton_abr = gtk_color_button_new_with_rgba(&color);
	gtk_box_pack_end(GTK_BOX(hbox), colorbutton_abr, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	label = gtk_label_new(_("Example"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	color = guint32_2_gdkrgba(color_scheme.ex);
	GtkWidget *colorbutton_ex = gtk_color_button_new_with_rgba(&color);
	gtk_box_pack_end(GTK_BOX(hbox), colorbutton_ex, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	label = gtk_label_new(_("Extra key phrase"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	color = guint32_2_gdkrgba(color_scheme.k);
	GtkWidget *colorbutton_k = gtk_color_button_new_with_rgba(&color);
	gtk_box_pack_end(GTK_BOX(hbox), colorbutton_k, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	label = gtk_label_new(_("Emphasize"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	color = guint32_2_gdkrgba(color_scheme.c);
	GtkWidget *colorbutton_c = gtk_color_button_new_with_rgba(&color);
	gtk_box_pack_end(GTK_BOX(hbox), colorbutton_c, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	label = gtk_label_new(_("Reference"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	color = guint32_2_gdkrgba(color_scheme.ref);
	GtkWidget *colorbutton_ref = gtk_color_button_new_with_rgba(&color);
	gtk_box_pack_end(GTK_BOX(hbox), colorbutton_ref, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (gtk_dialog_get_content_area(GTK_DIALOG(window))), vbox);
	gint result = gtk_dialog_run(GTK_DIALOG(window));
	if(result == GTK_RESPONSE_OK) {
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorbutton_abr), &color);
		color_scheme.abr = gdkrgba_2_guint32(color);
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorbutton_ex), &color);
		color_scheme.ex = gdkrgba_2_guint32(color);
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorbutton_k), &color);
		color_scheme.k = gdkrgba_2_guint32(color);
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorbutton_c), &color);
		color_scheme.c = gdkrgba_2_guint32(color);
		gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(colorbutton_ref), &color);
		color_scheme.ref = gdkrgba_2_guint32(color);
		XDXFParser::fill_replace_arr();
		const std::string confPath = get_cfg_filename();
		const std::string contents(generate_config_content(color_scheme));
		g_file_set_contents(confPath.c_str(), contents.c_str(), -1, NULL);
	}
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading XDXF data parsing plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: XDXF data parsing plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_PARSEDATA;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("XDXF data parsing"), _("XDXF data parsing engine."), _("Parse the XDXF data."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	gpAppDirs = appDirs;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	plugin_info = NULL;
	gpAppDirs = NULL;
}

DLLIMPORT bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj)
{
	set_default_color_scheme();
	const std::string confPath = get_cfg_filename();
	if (!g_file_test(confPath.c_str(), G_FILE_TEST_EXISTS)) {
		const std::string contents(generate_config_content(color_scheme));
		g_file_set_contents(confPath.c_str(), contents.c_str(), -1, NULL);
	} else
		load_config_file(color_scheme);
	XDXFParser::fill_replace_arr();
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
