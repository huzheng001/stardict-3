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

#ifndef ARTICLEVIEW_H
#define ARTICLEVIEW_H

#include <string>
#include <gtk/gtk.h>

#include "pangoview.h" 
#include "lib/dictbase.h"

enum BookNameStyle
{
	BookNameStyle_Default,
	BookNameStyle_OneBlankLine,
	BookNameStyle_TwoBlankLines,
};

//class which show dictionary's articles
class ArticleView {
public:
	ArticleView(GtkContainer *owner, BookNameStyle booknamestyle, bool floatw=false)
		: bookindex(0), bookname_style(booknamestyle), pango_view_(PangoWidgetBase::create(owner, floatw)),
		for_float_win(floatw), headerindex(-1) {}
	ArticleView(GtkBox *owner, BookNameStyle booknamestyle, bool floatw=false)
		:  bookname_style(booknamestyle),
		pango_view_(PangoWidgetBase::create(owner, floatw)),
		for_float_win(floatw), headerindex(-1) {}

	void SetDictIndex(InstantDictIndex index);
	void AppendHeaderMark();
	void AppendHeader(const char *dict_name, const char *dict_link = NULL);
	void AppendWord(const gchar *word);
	void AppendData(gchar *data, const gchar *oword, const gchar *origword);
	void AppendNewline();
	void AppendDataSeparate();

	GtkWidget *widget() { return pango_view_->widget(); }
	void scroll_to(gdouble val) { pango_view_->scroll_to(val); }
	void set_text(const char *str) { pango_view_->set_text(str); }
	void append_text(const char *str) { pango_view_->append_text(str); }
	void append_pango_text(const char *str) { pango_view_->append_pango_text(str); }
	void append_pixbuf(GdkPixbuf *pixbuf, const char *label = NULL) { pango_view_->append_pixbuf(pixbuf, label); }
	void append_widget(GtkWidget *widget) { pango_view_->append_widget(widget); }
	void set_pango_text(const char *str) { pango_view_->set_pango_text(str); }
	std::string get_text() { return pango_view_->get_text(); }
	void append_pango_text_with_links(const std::string& str,
					  const LinksPosList& links) {
		pango_view_->append_pango_text_with_links(str, links);
	}
	void clear() { pango_view_->clear(); bookindex = 0; headerindex = -1; }
	void append_mark(const char *mark) { pango_view_->append_mark(mark); }
	void begin_update() { pango_view_->begin_update(); }
	void end_update() { pango_view_->end_update(); }
	void goto_begin() { pango_view_->goto_begin(); }
	void goto_end() { pango_view_->goto_end(); }
#if GTK_MAJOR_VERSION >= 3
	void modify_bg(GtkStateFlags state, const GdkRGBA *color) { pango_view_->modify_bg(state, color); } 
#else
	void modify_bg(GtkStateType state, const GdkColor *color) { pango_view_->modify_bg(state, color); }
#endif
	GtkWidget *vscroll_bar() { return pango_view_->vscroll_bar(); }
	void set_size(gint w, gint h) { pango_view_->set_size(w, h); }
	gint scroll_space() { return pango_view_->scroll_space(); }
	gdouble scroll_pos() { return pango_view_->scroll_pos(); }
	void connect_on_link(const sigc::slot<void, const std::string &>& s);
	unsigned int get_bookindex(void) { return bookindex; }
	void set_bookname_style(BookNameStyle style) { bookname_style = style; }
private:
	struct ParseResultItemWithMark;

	unsigned int bookindex;
	BookNameStyle bookname_style;
	std::unique_ptr<PangoWidgetBase> pango_view_;
	bool for_float_win;
	InstantDictIndex dict_index;
	/* Count headers. Add extra space before headers with index > 0. */
	int headerindex;

	std::string xdxf2pango(const char *p, const gchar *oword, LinksPosList& links_list);
	void append_and_mark_orig_word(const std::string& mark,
				       const gchar *origword,
				       const LinksPosList& links);
	void append_resource_file_list(const gchar *p);
	void append_data_parse_result(const gchar *real_oword, ParseResult& parse_result);
	void append_data_res_image(const std::string& key, const std::string& mark,
		bool& loaded);
	void append_data_res_sound(const std::string& key, const std::string& mark,
		bool& loaded);
	void append_data_res_video(const std::string& key, const std::string& mark,
		bool& loaded);
	void append_data_res_attachment(const std::string& key, const std::string& mark,
		bool& loaded);
	static void on_resource_button_destroy(GtkWidget *object, gpointer user_data);
	static void on_sound_button_clicked(GtkWidget *object, gpointer user_data);
	static void on_video_button_clicked(GtkWidget *object, gpointer user_data);
	static void on_attachment_button_clicked(GtkWidget *object, gpointer user_data);
	static void on_resource_button_realize(GtkWidget *object, gpointer user_data);
};


#endif//ARTICLEVIEW_H
