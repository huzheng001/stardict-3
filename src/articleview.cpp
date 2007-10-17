/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "conf.h"
#include "utils.h"
#include "stardict.h"
#include "lib/getuint32.h"

#include "articleview.h"


void ArticleView::append_and_mark_orig_word(const std::string& mark,
					    const gchar *origword,
					    const LinksPosList& links)
{	
	if (conf->get_bool_at("dictionary/markup_search_word") &&
	    origword && *origword) {
		std::string res;
		size_t olen = strlen(origword);
		glib::CharStr markoword(g_markup_escape_text(origword, olen));
		std::string::size_type pos, prev_pos = 0;
//TODO: mark word even if it looks like w<b>o</b>rd, or contains special xml
//characters, like &quote;
		while ((pos = mark.find(origword, prev_pos)) != std::string::npos) {
			res.append(mark, prev_pos, pos - prev_pos);
			res.append("<span background=\"yellow\">");
			res.append(get_impl(markoword));
			res.append("</span>");
			prev_pos = pos + olen;
		}
		res.append(mark, prev_pos, mark.length() - prev_pos);
		if (links.empty())
			append_pango_text(res.c_str());
		else
			pango_view_->append_pango_text_with_links(res, links);
	} else {
		if (links.empty())
			append_pango_text(mark.c_str());
		else
			pango_view_->append_pango_text_with_links(mark, links);
	}
}

void ArticleView::AppendData(gchar *data, const gchar *oword,
			     const gchar *real_oword)
{
	std::string mark;

	guint32 data_size,sec_size=0;
	data_size=get_uint32(data);
	data+=sizeof(guint32); //Here is a bug fix of 2.4.8, which make (guint32(p - data)<data_size) become correct, when data_size is the following data size, not the whole size as 4 bytes bigger.
	const gchar *p=data;
	bool first_time = true;
	size_t iPlugin;
	size_t nPlugins = gpAppFrame->oStarDictPlugins->ParseDataPlugins.nplugins();
	unsigned int parsed_size;
	ParseResult parse_result;
	while (guint32(p - data)<data_size) {
		if (first_time)
			first_time=false;
		else
			mark+= "\n";
		for (iPlugin = 0; iPlugin < nPlugins; iPlugin++) {
			parse_result.clear();
			if (gpAppFrame->oStarDictPlugins->ParseDataPlugins.parse(iPlugin, p, &parsed_size, parse_result, oword)) {
				p += parsed_size;
				break;
			}
		}
		if (iPlugin != nPlugins) {
			for (std::list<ParseResultItem>::iterator it = parse_result.item_list.begin(); it != parse_result.item_list.end(); ++it) {
				switch (it->type) {
					case ParseResultItemType_mark:
						mark += it->mark->pango;
						break;
					case ParseResultItemType_link:
					{
						append_and_mark_orig_word(mark, real_oword, LinksPosList());
						mark.clear();
						append_and_mark_orig_word(it->link->pango, real_oword, it->link->links_list);
						break;
					}
					case ParseResultItemType_res:
					{
						bool loaded = false;
						if (it->res->type == "image") {
							if (for_float_win) {
								loaded = true;
								append_and_mark_orig_word(mark, real_oword, LinksPosList());
								mark.clear();
								append_pixbuf(NULL, it->res->key.c_str());
							} else {
								GdkPixbuf* pixbuf = NULL;
								if (dict_index.type == InstantDictType_LOCAL) {
									int type = gpAppFrame->oLibs.GetStorageType(dict_index.index);
									if (type == 0) {
									} else if (type == 1) {
										const char *filename = gpAppFrame->oLibs.GetStorageFilePath(dict_index.index, it->res->key.c_str());
										if (filename) {
											pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
										}
									}
								}
								if (pixbuf) {
									loaded = true;
									append_and_mark_orig_word(mark, real_oword, LinksPosList());
									mark.clear();
									append_pixbuf(pixbuf, it->res->key.c_str());
									g_object_unref(pixbuf);
								}
							}
						} else if (it->res->type == "sound") {
						} else if (it->res->type == "video") {
						} else {
						}
						if (!loaded) {
							mark += "<span foreground=\"red\">";
							gchar *m_str = g_markup_escape_text(it->res->key.c_str(), -1);
							mark += m_str;
							g_free(m_str);
							mark += "</span>";
						}
						break;
					}
					case ParseResultItemType_widget:
						append_widget(it->widget->widget);
						break;
					default:
						break;
				}
			}
			parse_result.clear();
			continue;
		}
		switch (*p) {
			case 'm':
			case 'l'://need more work...
				p++;
				sec_size = strlen(p);
				if (sec_size) {
					gchar *m_str = g_markup_escape_text(p, sec_size);
					mark+=m_str;
					g_free(m_str);
				}
				sec_size++;
				break;
			case 'g':
				p++;
				sec_size=strlen(p);
				if (sec_size) {
					mark+=p;
				}
				sec_size++;
				break;
			case 'x':
				p++;
				sec_size = strlen(p) + 1;
				mark+= _("XDXF data parsing plug-in is not found!");
				break;
			case 'k':
				p++;
				sec_size = strlen(p) + 1;
				mark+= _("PowerWord data parsing plug-in is not found!");
				break;
			case 'w':
				p++;
				sec_size = strlen(p) + 1;
				mark+= _("Wiki data parsing plug-in is not found!");
				break;
			case 'h':
				p++;
				sec_size = strlen(p) + 1;
				mark+= _("HTML data parsing plug-in is not found!");
				break;
			case 'n':
				p++;
				sec_size = strlen(p) + 1;
				mark+= _("WordNet data parsing plug-in is not found!");
				break;
			case 't':
				p++;
				sec_size = strlen(p);
				if (sec_size) {
					mark += "[<span foreground=\"blue\">";
					gchar *m_str = g_markup_escape_text(p, sec_size);
					mark += m_str;
					g_free(m_str);
					mark += "</span>]";
				}
				sec_size++;
				break;
			case 'y':
				p++;
				sec_size = strlen(p);
				if (sec_size) {
					mark += "[<span foreground=\"red\">";
					gchar *m_str = g_markup_escape_text(p, sec_size);
					mark += m_str;
					g_free(m_str);
					mark += "</span>]";
				}
				sec_size++;
				break;
			case 'W':
				p++;
				sec_size=g_ntohl(get_uint32(p));
				//enbale sound button.
				sec_size += sizeof(guint32);
				break;
			case 'P':
				{
				p++;
				sec_size=g_ntohl(get_uint32(p));
				if (sec_size) {
					if (for_float_win) {
						append_and_mark_orig_word(mark, real_oword, LinksPosList());
						mark.clear();
						append_pixbuf(NULL);
					} else {
						GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
						gdk_pixbuf_loader_write(loader, (const guchar *)(p+sizeof(guint32)), sec_size, NULL);
						gdk_pixbuf_loader_close(loader, NULL);
						GdkPixbuf* pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
						if (pixbuf) {
							append_and_mark_orig_word(mark, real_oword, LinksPosList());
							mark.clear();
							append_pixbuf(pixbuf);
						} else {
							mark += _("<span foreground=\"red\">[Load image error!]</span>");
						}
						g_object_unref(loader);
					}
				} else {
					mark += _("<span foreground=\"red\">[Missing Image]</span>");
				}
				sec_size += sizeof(guint32);
				}
				break;
			default:
				if (g_ascii_isupper(*p)) {
					p++;
					sec_size=g_ntohl(get_uint32(p));
					sec_size += sizeof(guint32);
				} else {
					p++;
					sec_size = strlen(p)+1;
				}
				mark += _("Unknown data type, please upgrade StarDict!");
				break;
		}
		p += sec_size;
	}

	append_and_mark_orig_word(mark, real_oword, LinksPosList());
}

void ArticleView::AppendNewline()
{
	append_pango_text("\n");
}

void ArticleView::AppendDataSeparate()
{
	append_pango_text("<span foreground=\"gray\"><s>     </s></span>\n");
}

void ArticleView::SetDictIndex(InstantDictIndex index)
{
	dict_index = index;
}

void ArticleView::AppendHeaderMark()
{
	if (!for_float_win) {
		gchar *mark = g_strdup_printf("%d", bookindex);
		pango_view_->append_mark(mark);
		g_free(mark);
		bookindex++;
	}
}

void ArticleView::AppendHeader(const char *dict_name)
{
	AppendHeaderMark();
	std::string mark= "<span foreground=\"blue\">";
#ifdef CONFIG_GPE
	mark+= "&lt;- ";
#else
	mark+= "&lt;--- ";
#endif
	gchar *m_str = g_markup_escape_text(dict_name, -1);
	mark += m_str;
	g_free(m_str);
#ifdef CONFIG_GPE
	mark += " -&gt;</span>\n";
#else
	mark += " ---&gt;</span>\n";
#endif
	append_pango_text(mark.c_str());
}

void ArticleView::AppendWord(const gchar *word)
{
	std::string mark;
	mark += for_float_win ? "<span foreground=\"purple\">" : "<b><span size=\"x-large\">";
	gchar *m_str = g_markup_escape_text(word, -1);
	mark += m_str;
	g_free(m_str);
	mark += for_float_win ? "</span>\n" : "</span></b>\n";
	append_pango_text(mark.c_str());
}

void ArticleView::connect_on_link(const sigc::slot<void, const std::string &>& s)
{
	pango_view_->on_link_click_.connect(s);
}
