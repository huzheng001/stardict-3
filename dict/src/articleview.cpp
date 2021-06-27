/*
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <iostream>
#include <vector>

#include "conf.h"
#include "lib/utils.h"
#include "stardict.h"
#include "lib/xml_str.h"

#include "articleview.h"
#include "desktop.h"

//#define DEBUG
//#define DDEBUG

/* Helper class.
 * Provides a means to generate a unique mark name and insert it at the 
 * specified point. All marks inserted by this class are deleted when this 
 * object is destroyed. */
class Marks {
public:
	Marks(PangoWidgetBase* pango_view)
	: 
		mark_num_(-1), 
		pango_view_(pango_view),
		insert_last_where_mark_(),
		insert_last_offset_(-1),
		insert_last_gravity_(false),
		insert_last_mark_num_(-1),
		clone_last_where_mark_(),
		clone_last_gravity_(false),
		clone_last_mark_num_(-1)
	{
	}
	~Marks(void)
	{
		int size = static_cast<int>(marks_list_.size());
		for(int i=0; i<size; ++i) {
			if(marks_list_[i] > 0)
				pango_view_->delete_mark(gen_mark_name(i).c_str());
		}
	}
	// return value: name of the mark inserted
	std::string append_mark(bool left_gravity = true)
	{
		std::string mark(gen_mark_name(++mark_num_));
#ifdef DDEBUG
		std::cout << "article::append_mark " << mark << std::endl;
#endif
		pango_view_->append_mark(mark.c_str(), left_gravity);
		marks_list_.push_back(1);
#ifdef DEBUG
		if(mark_num_ + 1 != static_cast<int>(marks_list_.size()))
			g_warning("article::Marks. incorrect list size.");
#endif
		return mark;
	}
	// return value: name of the mark inserted
	std::string insert_mark(const std::string& where_mark_name, 
		int char_offset = 0, bool left_gravity = true)
	{
		if(where_mark_name == insert_last_where_mark_ 
			&& char_offset == insert_last_offset_ 
			&& left_gravity == insert_last_gravity_ 
			&& marks_list_[insert_last_mark_num_] > 0) {
			++marks_list_[insert_last_mark_num_];
#ifdef DDEBUG
		std::cout << "article::insert_mark hit" << std::endl;
#endif
			return gen_mark_name(insert_last_mark_num_);
		}
		std::string mark(gen_mark_name(++mark_num_));
		insert_last_where_mark_ = where_mark_name;
		insert_last_offset_ = char_offset;
		insert_last_gravity_ = left_gravity;
		insert_last_mark_num_ = mark_num_;
#ifdef DDEBUG
		std::cout << "article::insert_mark " << mark << std::endl;
#endif
		pango_view_->insert_mark(mark.c_str(), where_mark_name.c_str(), char_offset, 
			left_gravity);
		marks_list_.push_back(1);
#ifdef DEBUG
		if(mark_num_ + 1 != static_cast<int>(marks_list_.size()))
			g_warning("article::Marks. incorrect list size.");
#endif
		return mark;
	}
	/* In is not the same as insert_mark(where_mark_name, 0, left_gravity)! */
	std::string clone_mark(const std::string& where_mark_name, 
		bool left_gravity = true)
	{
		if(where_mark_name == clone_last_where_mark_
			&& left_gravity == clone_last_gravity_
			&& marks_list_[clone_last_mark_num_] > 0) {
			++marks_list_[clone_last_mark_num_];
#ifdef DDEBUG
		std::cout << "article::clone_mark hit" << std::endl;
#endif
			return gen_mark_name(clone_last_mark_num_);
		}
		std::string mark(gen_mark_name(++mark_num_));
		clone_last_where_mark_ = where_mark_name;
		clone_last_gravity_ = left_gravity;
		clone_last_mark_num_ = mark_num_;
#ifdef DDEBUG
		std::cout << "article::clone_mark " << mark << std::endl;
#endif
		pango_view_->clone_mark(mark.c_str(), where_mark_name.c_str(), left_gravity);
		marks_list_.push_back(1);
#ifdef DEBUG
		if(mark_num_ + 1 != static_cast<int>(marks_list_.size()))
			g_warning("article::Marks. incorrect list size.");
#endif
		return mark;
	}
private:
	std::string gen_mark_name(int num) const
	{
		glib::CharStr gmark(g_strdup_printf("_ArticleView_mark_%d", num));
		return get_impl(gmark);
	}
private:
	typedef std::vector<int> MarksList;
	/* marks_list_[i] - number of references to mark i, use gen_mark_name 
	 * function to get mark name: gen_mark_name(i) */
	MarksList marks_list_;
	int mark_num_; // last mark number used
	PangoWidgetBase* pango_view_;
	
	std::string insert_last_where_mark_;
	int insert_last_offset_;
	bool insert_last_gravity_;
	int insert_last_mark_num_;

	std::string clone_last_where_mark_;
	bool clone_last_gravity_;
	int clone_last_mark_num_;
};

/* resource data structure
 * A resource file is loaded only if necessary.
 * We postpone loading files till a user requests to use them.
 * That speeds up article loading, saves memory. */
class ResData {
public:
	ResData(size_t iLib_, const std::string& key_)
	:
		iLib(iLib_),
		key(key_)
	{
		
	}
	const char* get_url(void)
	{
		if(file.empty())
			file = gpAppFrame->oLibs.GetStorageFilePath(iLib, key);
		return file.get_url();
	}
	const char * get_content(void)
	{
		return gpAppFrame->oLibs.GetStorageFileContent(iLib, key);
	}
	const std::string& get_key(void) const
	{
		return key;
	}
private:
	size_t iLib;
	std::string key;
	FileHolder file;
};

struct ArticleView::ParseResultItemWithMark {
	ParseResultItem* item;
	std::string mark;
	int char_offset;
	// true if a tmp char is added after the mark
	bool tmp_char;
	ParseResultItemWithMark(ParseResultItem* item, int char_offset, 
		bool tmp_char=false)
	: item(item), char_offset(char_offset), tmp_char(tmp_char)
	{
	}
};

void ArticleView::append_and_mark_orig_word(const std::string& mark,
					    const gchar *origword,
					    const LinksPosList& links)
{
	if(mark.empty())
		return;
	if (conf->get_bool_at("dictionary/markup_search_word") &&
	    origword && *origword) {
		std::string res;
		XMLCharData xmlcd;
		xmlcd.assign_xml(mark.c_str());
		const char* const cd_str = xmlcd.get_char_data_str();
		if(cd_str) {
			const size_t cd_str_len = xmlcd.get_char_data_str_length();
			const size_t olen = strlen(origword);
			const char* cd_b, *cd_p;
			const char* const start_tag = "<span background=\"yellow\">";
			const char* const end_tag = "</span>";
			cd_b = cd_str;
			while(cd_b < cd_str + cd_str_len && (cd_p = strstr(cd_b, origword))) {
				xmlcd.copy_xml(res, cd_b - cd_str, cd_p - cd_str);
				xmlcd.mark_substring(res, start_tag, end_tag, cd_p - cd_str, olen);
				cd_b = cd_p + olen;
			}
			xmlcd.copy_xml(res, cd_b - cd_str, cd_str_len);
		} else // mark does not contain char data, only markup
			res = mark;
		if (links.empty())
			append_pango_text(res.c_str());
		else
			append_pango_text_with_links(res, links);
	} else {
		if (links.empty())
			append_pango_text(mark.c_str());
		else
			append_pango_text_with_links(mark, links);
	}
}

void ArticleView::AppendData(gchar *data, const gchar *oword,
			     const gchar *real_oword)
{
	std::string mark;

	guint32 sec_size=0;
	const guint32 data_size=get_uint32(data);
	data+=sizeof(guint32);
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
			append_and_mark_orig_word(mark, real_oword, LinksPosList());
			mark.clear();
			append_data_parse_result(real_oword, parse_result);
			parse_result.clear();
			continue;
		}
		switch (*p) {
			case 'm':
			//case 'l': //TODO: convert from local encoding to utf-8
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
			case 'r':
				p++;
				sec_size = strlen(p);
				if(sec_size) {
					append_and_mark_orig_word(mark, real_oword, LinksPosList());
					mark.clear();
					append_resource_file_list(p);
				}
				sec_size++;
				break;
			/*case 'W':
				{
				p++;
				sec_size=g_ntohl(get_uint32(p));
				//TODO: sound button.
				sec_size += sizeof(guint32);
				}
				break;*/
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

void ArticleView::AppendHeader(const char *dict_name, const char *dict_link)
{
	if (bookname_style == BookNameStyle_Default) {
	} else if (bookname_style == BookNameStyle_OneBlankLine) {
		if(++headerindex > 0)
			append_pango_text("\n");
	} else {
		if(++headerindex > 0)
			append_pango_text("\n\n");
	}
	AppendHeaderMark();
	if (dict_link) {
		std::string mark;
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark= "<span foreground=\"blue\">&lt;--- <u>";
		} else {
			mark= "<span foreground=\"blue\"><u>";
		}
		LinksPosList links_list;
		std::string link(dict_link);
		links_list.push_back(LinkDesc(5, g_utf8_strlen(dict_name, -1), link));
		gchar *m_str = g_markup_escape_text(dict_name, -1);
		mark += m_str;
		g_free(m_str);
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark += "</u> ---&gt;</span>\n";
		} else {
			mark += "</u></span>\n";
		}
		append_pango_text_with_links(mark, links_list);
	} else {
		std::string mark= "<span foreground=\"blue\">";
#ifdef CONFIG_GPE
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark+= "&lt;- ";
		} else {
		}
#else
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark+= "&lt;--- ";
		} else {
		}
#endif
		gchar *m_str = g_markup_escape_text(dict_name, -1);
		mark += m_str;
		g_free(m_str);
#ifdef CONFIG_GPE
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark += " -&gt;</span>\n";
		} else {
			mark += "</span>\n";
		}
#else
		if ((bookname_style == BookNameStyle_Default) || (bookname_style == BookNameStyle_OneBlankLine)) {
			mark += " ---&gt;</span>\n";
		} else {
			mark += "</span>\n";
		}
#endif
		append_pango_text(mark.c_str());
	}
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

void ArticleView::append_resource_file_list(const gchar *p)
{
	const gchar *b = p, *e;
	std::string type, key;
	bool new_line_before = true, new_line_after;
	bool loaded;
	while(*b) {
		e = strchr(b, ':');
		if(!e) {
			pango_view_->append_pango_text(
				_("<span foreground=\"red\">[Resource file list: incorrect format!]</span>"));
			break;
		}
		type.assign(b, e-b);
		b = e+1;
		e = strchr(b, '\n');
		if(!e)
			e = strchr(b, '\0');
		key.assign(b, e-b);
		b = (*e) ? e + 1 : e;
		loaded = false;
		new_line_after = false;
		if(type == "img") {
			if(!new_line_before)
				pango_view_->append_text("\n");
			new_line_after = true;
			append_data_res_image(key, "", loaded);
		} else if(type == "snd") {
			append_data_res_sound(key, "", loaded);
		} else if(type == "vdo") {
			append_data_res_video(key, "", loaded);
		} else if(type == "att") {
			append_data_res_attachment(key, "", loaded);
		} else {
			pango_view_->append_pango_text(
				_("<span foreground=\"red\">[Resource file list: incorrect format!]</span>"));
			break;
		}
		if(!loaded) {
			glib::CharStr str(g_markup_escape_text(key.c_str(), -1));
			std::string mark;
			mark += "\n<span foreground=\"red\">";
			mark += get_impl(str);
			mark += "</span>";
			pango_view_->append_pango_text(mark.c_str());
			new_line_after = true;
		}
		if(new_line_after)
			pango_view_->append_text("\n");
		new_line_before = new_line_after;
	}
}

void ArticleView::append_data_parse_result(const gchar *real_oword, 
	ParseResult& parse_result)
{
	/* Why ParseResultItem's cannot be inserted into the pango_view_ in the 
	 * order they appear in the parse_result list? 
	 * 
	 * They can, but that limits use of the pango markup language when markup 
	 * intermixed with objects that are not expressed in pango markup. 
	 * For instance we cannot handle the following piece of data:
	 * markup: "<span foreground=\"purple\">some text"
	 * res: some image
	 * markup: "text continues</span>" 
	 * The first markup string cannot be committed because it is not a valid 
	 * markup - span tag is not closed. But if we piece two markup strings 
	 * together, commit markup and then insert the resource, everything will be
	 * fine.
	 * 
	 * Here is an outline of the rules parse_result list must adhere to.
	 * 
	 * - each list item with pango markup must contain only complete tags. Tag 
	 * may not be opened in one list item and closed in another. For example
	 * this list is not allowed:
	 * markup: "<span foreground=\"purple\" "
	 * markup: "size=\"x-large\">"
	 * 
	 * - after combining all list items with pango markup the resultant string
	 * must constitute a valid markup (correct order of tags, each tag must have
	 * a corresponding closing tag and so on). For example, the following text 
	 * is not allowed: "<b> bla bla </b><i>text end".
	 * Note: list item may contain incomplete markup like "<b>text begins".
	 * 
	 * - Delayed insert items must generate only valid markup. Items 
	 * representing images, widgets generate a pango-formated error message
	 * if the primary object cannot be inserted.
	 * 
	 * 
	 * Position in the pango markup string cannot be exactly specified by 
	 * character offset only. That is especially important for LabelPangoWidget
	 * where markup is stored in a string. In order to insert delayed insert 
	 * items in the correct place it was decided to add a temporary character
	 * in the string in the place where the delayed item must be inserted.
	 * Temporary characters are deleted at the end.
	 * 
	 * In addition to character offsets marks are used to specify position in 
	 * the text. Marks must be used instead of char offsets when possible.
	 * Marks are preferred over offsets because they preserve position across
	 * text modifications, they have a notion of right and left gravity.
	 * In general we do not know how many character would be inserted in the 
	 * text after the call the method insert_pixbuf. Normally it is only one,
	 * but it may be any number in case of error.
	 * 
	 * */
	Marks marks(pango_view_.get());
	std::string start_mark = marks.append_mark(true);
	std::list<ParseResultItemWithMark> delayed_insert_list;
	// compose markup
	{
		std::string markup_str;
		int char_offset = 0;
		const char tmp_char = 'x'; // may be any unicode char excluding '<', '&', '>'

		for (std::list<ParseResultItem>::iterator it = parse_result.item_list.begin(); 
			it != parse_result.item_list.end(); ++it) {
			switch (it->type) {
				case ParseResultItemType_mark:
					char_offset += xml_utf8_strlen(it->mark->pango.c_str());
					markup_str += it->mark->pango;
					break;
				case ParseResultItemType_link:
				{
					/* links do not insert any text, so exact mark position is
					 * not important. */
					ParseResultItemWithMark item(&*it, char_offset);
					delayed_insert_list.push_back(item);
					char_offset += xml_utf8_strlen(it->link->pango.c_str());
					markup_str += it->link->pango;
					break;
				}
				case ParseResultItemType_res:
				case ParseResultItemType_widget:
				{
					ParseResultItemWithMark item1(NULL, char_offset, true);
					delayed_insert_list.push_back(item1);
					char_offset += 1;
					markup_str += tmp_char;
					ParseResultItemWithMark item2(&*it, char_offset, true);
					delayed_insert_list.push_back(item2);
					char_offset += 1;
					markup_str += tmp_char;
					break;
				}
				case ParseResultItemType_FormatBeg:
				case ParseResultItemType_FormatEnd:
				{
					/* formats do not insert any text, so exact mark position is
					 * not important. */
					ParseResultItemWithMark item2(&*it, char_offset);
					delayed_insert_list.push_back(item2);
					break;
				}
				default:
					g_warning("Unsupported item type.");
					break;
			}
		}
		append_and_mark_orig_word(markup_str, real_oword, LinksPosList());
		markup_str.clear();
		pango_view_->flush();
	}
	// Marks that precede tmp chars. One mark - one char next to it that must be 
	// deleted. Different marks do not refer to the same char.
	std::list<std::string> tmp_char_mark_list;
	// insert marks
	for(std::list<ParseResultItemWithMark>::iterator it = delayed_insert_list.begin(); 
		it != delayed_insert_list.end(); ) {
		it->mark = marks.insert_mark(start_mark, it->char_offset, false);
		if(it->tmp_char)
			tmp_char_mark_list.push_back(it->mark);
		if(it->item)
			++it;
		else
			it = delayed_insert_list.erase(it);
	}
#ifdef DDEBUG
	std::cout << "ArticleView::append_data_parse_result. marks inserted." 
		<< std::endl; 
#endif
	// insert delayed items
	for(std::list<ParseResultItemWithMark>::iterator it = delayed_insert_list.begin(); 
		it != delayed_insert_list.end(); ) {
		bool EraseCurrent = true;
		switch(it->item->type) {
		case ParseResultItemType_link:
			pango_view_->insert_pango_text_with_links("", it->item->link->links_list, 
				it->mark.c_str());
			break;
		case ParseResultItemType_res:
		{
			bool loaded = false;
			if (it->item->res->type == "image") {
				append_data_res_image(it->item->res->key, it->mark, loaded);
			} else if (it->item->res->type == "sound") {
				append_data_res_sound(it->item->res->key, it->mark, loaded);
			} else if (it->item->res->type == "video") {
				append_data_res_video(it->item->res->key, it->mark, loaded);
			} else {
				append_data_res_attachment(it->item->res->key, it->mark, loaded);
			}
			if (!loaded) {
				std::string tmark;
				tmark += "<span foreground=\"red\">";
				glib::CharStr m_str(g_markup_escape_text(it->item->res->key.c_str(), -1));
				tmark += get_impl(m_str);
				tmark += "</span>";
				pango_view_->insert_pango_text(tmark.c_str(), it->mark.c_str());
			}
			break;
		}
		case ParseResultItemType_widget:
			pango_view_->insert_widget(it->item->widget->widget, it->mark.c_str());
			break;
		case ParseResultItemType_FormatBeg:
			// change gravity of the mark
			it->mark = marks.clone_mark(it->mark, true);
			EraseCurrent = false;
			break;
		case ParseResultItemType_FormatEnd:
		{
			// find paired ParseResultItemType_FormatBeg item
			std::list<ParseResultItemWithMark>::reverse_iterator it2(it);
			for(; it2 != delayed_insert_list.rend(); ++it2)
				if(it2->item->type == ParseResultItemType_FormatBeg)
					break;
			if(it2 != delayed_insert_list.rend() && it2->item->format_beg->type 
				== it->item->format_end->type) {
				if(it->item->format_end->type == ParseResultItemFormatType_Indent)
					pango_view_->indent_region(it2->mark.c_str(), 0, it->mark.c_str());
				std::list<ParseResultItemWithMark>::iterator it3(it2.base());
				delayed_insert_list.erase(--it3);
			} else
				g_warning("Not paired ParseResultItemType_FormatEnd item");
			break;
		}
		default:
			g_assert_not_reached();
			break;
		}
		if(EraseCurrent)
			it = delayed_insert_list.erase(it);
		else
			++it;
	}
	// remove tmp chars
	for(std::list<std::string>::iterator it = tmp_char_mark_list.begin(); 
		it != tmp_char_mark_list.end(); ++it) {
#ifdef DDEBUG
		std::cout << "tmp char mark " << *it << std::endl;
#endif
		pango_view_->delete_text(it->c_str(), 1, 0);
	}
	pango_view_->reindent();
	if(!delayed_insert_list.empty())
		g_warning("delayed_insert_list is not empty. "
			"parse_result contains not paired items.");
}

void ArticleView::append_data_res_image(
	const std::string& key, const std::string& mark,
	bool& loaded)
{
	if (for_float_win) {
		loaded = true;
		if(mark.empty())
			pango_view_->append_pixbuf(NULL, key.c_str());
		else
			pango_view_->insert_pixbuf(NULL, key.c_str(), mark.c_str());
	} else {
		GdkPixbuf* pixbuf = NULL;
		if (dict_index.type == InstantDictType_LOCAL) {
			StorageType type = gpAppFrame->oLibs.GetStorageType(dict_index.index);
			if (type == StorageType_DATABASE || type == StorageType_FILE) {
				if(const char* content = gpAppFrame->oLibs.GetStorageFileContent
					(dict_index.index, key)) {
					const guint32 size = get_uint32(content);
					const guchar *data = (const guchar *)(content+sizeof(guint32));
					GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
					gdk_pixbuf_loader_write(loader, data, size, NULL);
					gdk_pixbuf_loader_close(loader, NULL);
					pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
					if(pixbuf)
						g_object_ref(G_OBJECT(pixbuf));
					g_object_unref(loader);
				}
			}
		}
		if (pixbuf) {
			loaded = true;
			if(mark.empty())
				pango_view_->append_pixbuf(pixbuf, key.c_str());
			else
				pango_view_->insert_pixbuf(pixbuf, key.c_str(), mark.c_str());
			g_object_unref(pixbuf);
		}
	}
}

void ArticleView::append_data_res_sound(
	const std::string& key, const std::string& mark,
	bool& loaded)
{
	if (for_float_win) {
		loaded = true;
		if(mark.empty())
			pango_view_->append_widget(NULL);
		else
			pango_view_->insert_widget(NULL, mark.c_str());
	} else {
		GtkWidget *widget = NULL;
		if (dict_index.type == InstantDictType_LOCAL) {
			StorageType type = gpAppFrame->oLibs.GetStorageType(dict_index.index);
			if (type == StorageType_DATABASE || type == StorageType_FILE) {
				ResData *pResData
					= new ResData(dict_index.index, key);
				GtkWidget *button = NULL;
				GtkWidget *image = NULL;
				image = gtk_image_new_from_pixbuf(get_impl(
					gpAppFrame->oAppSkin.pronounce));
				button = gtk_button_new();
				/* We need an event box to associate a custom cursor with the
				 * button. */
				widget = gtk_event_box_new();
				g_object_ref_sink(G_OBJECT(widget));
				gtk_container_add(GTK_CONTAINER(button), image);
				gtk_container_add(GTK_CONTAINER(widget), button);
#ifdef DEBUG
				/* when the tooltip appears a number of gtk-warnings are generated:
				 * Gtk-WARNING **: IA__gtk_text_view_window_to_buffer_coords: 
				 * can't get coords for private windows */
				gtk_widget_set_tooltip_text(GTK_WIDGET(widget),
					pResData->get_key().c_str());
#endif
				gtk_event_box_set_above_child(GTK_EVENT_BOX(widget), FALSE);
				gtk_widget_show_all(widget);
				g_signal_connect(G_OBJECT(button), "destroy",
					G_CALLBACK(on_resource_button_destroy), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(on_sound_button_clicked), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "realize",
					G_CALLBACK(on_resource_button_realize), (gpointer)widget);
			}
		}
		if(widget) {
			loaded = true;
			if(mark.empty())
				pango_view_->append_widget(widget);
			else
				pango_view_->insert_widget(widget, mark.c_str());
			g_object_unref(widget);
		}
	}
}

void ArticleView::append_data_res_video(
	const std::string& key, const std::string& mark,
	bool& loaded)
{
	if (for_float_win) {
		loaded = true;
		if(mark.empty())
			pango_view_->append_widget(NULL);
		else
			pango_view_->insert_widget(NULL, mark.c_str());
	} else {
		GtkWidget *widget = NULL;
		if (dict_index.type == InstantDictType_LOCAL) {
			StorageType type = gpAppFrame->oLibs.GetStorageType(dict_index.index);
			if (type == StorageType_DATABASE || type == StorageType_FILE) {
				ResData *pResData
					= new ResData(dict_index.index, key);
				GtkWidget *button = NULL;
				GtkWidget *image = NULL;
				image = gtk_image_new_from_pixbuf(get_impl(
					gpAppFrame->oAppSkin.video));
				button = gtk_button_new();
				/* We need an event box to associate a custom cursor with the
				 * button. */
				widget = gtk_event_box_new();
				g_object_ref_sink(G_OBJECT(widget));
				gtk_container_add(GTK_CONTAINER(button), image);
				gtk_container_add(GTK_CONTAINER(widget), button);
#ifdef DEBUG
				/* when the tooltip appears a number of gtk-warnings are generated:
				 * Gtk-WARNING **: IA__gtk_text_view_window_to_buffer_coords: 
				 * can't get coords for private windows */
				gtk_widget_set_tooltip_text(GTK_WIDGET(widget),
					pResData->get_key().c_str());
#endif
				gtk_event_box_set_above_child(GTK_EVENT_BOX(widget), FALSE);
				gtk_widget_show_all(widget);
				g_signal_connect(G_OBJECT(button), "destroy",
					G_CALLBACK(on_resource_button_destroy), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(on_video_button_clicked), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "realize",
					G_CALLBACK(on_resource_button_realize), (gpointer)widget);
			}
		}
		if(widget) {
			loaded = true;
			if(mark.empty())
				pango_view_->append_widget(widget);
			else
				pango_view_->insert_widget(widget, mark.c_str());
			g_object_unref(widget);
		}
	}
}

void ArticleView::append_data_res_attachment(
	const std::string& key, const std::string& mark,
	bool& loaded)
{
	if (for_float_win) {
		loaded = true;
		if(mark.empty())
			pango_view_->append_widget(NULL);
		else
			pango_view_->insert_widget(NULL, mark.c_str());
	} else {
		GtkWidget *widget = NULL;
		if (dict_index.type == InstantDictType_LOCAL) {
			StorageType type = gpAppFrame->oLibs.GetStorageType(dict_index.index);
			if (type == StorageType_DATABASE || type == StorageType_FILE) {
				ResData *pResData
					= new ResData(dict_index.index, key);
				GtkWidget *button = NULL;
				GtkWidget *image = NULL;
				image = gtk_image_new_from_pixbuf(get_impl(
					gpAppFrame->oAppSkin.attachment));
				button = gtk_button_new();
				/* We need an event box to associate a custom cursor with the
				 * button. */
				widget = gtk_event_box_new();
				g_object_ref_sink(G_OBJECT(widget));
				gtk_container_add(GTK_CONTAINER(button), image);
				gtk_container_add(GTK_CONTAINER(widget), button);
#ifdef DEBUG
				/* when the tooltip appears a number of gtk-warnings are generated:
				 * Gtk-WARNING **: IA__gtk_text_view_window_to_buffer_coords: 
				 * can't get coords for private windows */
				gtk_widget_set_tooltip_text(GTK_WIDGET(widget),
					pResData->get_key().c_str());
#endif
				gtk_event_box_set_above_child(GTK_EVENT_BOX(widget), FALSE);
				gtk_widget_show_all(widget);
				g_signal_connect(G_OBJECT(button), "destroy",
					G_CALLBACK(on_resource_button_destroy), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(on_attachment_button_clicked), (gpointer)pResData);
				g_signal_connect(G_OBJECT(button), "realize",
					G_CALLBACK(on_resource_button_realize), (gpointer)widget);
			}
		}
		if(widget) {
			loaded = true;
			if(mark.empty())
				pango_view_->append_widget(widget);
			else
				pango_view_->insert_widget(widget, mark.c_str());
			g_object_unref(widget);
		}
	}
}

void ArticleView::on_resource_button_destroy(GtkWidget *object, gpointer user_data)
{
	delete (ResData*)user_data;
}

void ArticleView::on_sound_button_clicked(GtkWidget *object, gpointer user_data)
{
	ResData *pResData = (ResData*)user_data;
	if(const char *filename = pResData->get_url())
		play_sound_file(filename);
	else
		g_warning("Unable to load resource: %s", pResData->get_key().c_str());
}

void ArticleView::on_video_button_clicked(GtkWidget *object, gpointer user_data)
{
	ResData *pResData = (ResData*)user_data;
	if(const char *filename = pResData->get_url())
		play_video_file(filename);
	else
		g_warning("Unable to load resource: %s", pResData->get_key().c_str());
}

void ArticleView::on_attachment_button_clicked(GtkWidget *object, gpointer user_data)
{
	ResData *pResData = (ResData*)user_data;
	const std::string& key = pResData->get_key();
	std::string::size_type pos = key.rfind(DB_DIR_SEPARATOR);
	// in utf-8, yet a file name
	const char* default_file_name = key.c_str() + (pos == std::string::npos ? 0 : pos+1);
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new(_("Save File"),
		GTK_WINDOW(gpAppFrame->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
		gpAppFrame->last_selected_directory.c_str());
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), default_file_name);
	if(gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		glib::CharStr filename(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
		glib::CharStr selected_dir(g_path_get_dirname(get_impl(filename)));
		gpAppFrame->last_selected_directory = get_impl(selected_dir);
		if(const char* content = pResData->get_content()) {
			const guint32 size = get_uint32(content);
			const gchar *data = (const gchar *)(content+sizeof(guint32));
			if(!g_file_set_contents(get_impl(filename), data, size, NULL))
				g_warning("Fail to save file %s", get_impl(filename));
		} else {
			g_warning("Unable to load resource: %s", pResData->get_key().c_str());
		}
	}
	gtk_widget_destroy(dialog);
}

void ArticleView::on_resource_button_realize(GtkWidget *object, gpointer user_data)
{
	/* Event boxes are not automatically realized by GTK+, they must be realized
	 * explicitly. You need to make sure that an event box is already added as 
	 * a child to a top-level widget before realizing it. A child realize event 
	 * handler is a good place for that (imho). */
	GtkWidget *eventbox = GTK_WIDGET(user_data);
	gtk_widget_realize(eventbox);
	GdkCursor *cursor = gdk_cursor_new(GDK_HAND2);
	gdk_window_set_cursor(gtk_widget_get_window(eventbox), cursor);
#if GTK_MAJOR_VERSION >= 3
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
}
