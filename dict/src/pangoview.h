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

#ifndef PANGOVIEW_H
#define PAGNOVIEW_H

#include <list>
#include <string>
#include <gtk/gtk.h>
#include <memory>

#include "lib/stardict-sigc++.h"
#include "lib/parsedata_plugin.h"

//#define DEBUG

/**
 * Base class for classes which can show pango formated text.
 * 
 * Caching updates
 * This class provides a facility to cache updates that consists in appending
 * pango markup or normal text. beging_update starts and end_updates ends the
 * cached updates mode. In update mode text is appended to the cache_ variable.
 * Cached updates are applied when 1) end_updates is invoked, or 2) a user 
 * calls a method modifying the buffer in a way that cannon be expressed in
 * appending text, for instance, a pixbuf is appended. To force buffer update
 * call the flush method.
 */
class PangoWidgetBase {
public:
	sigc::signal<void, const std::string &> on_link_click_;//!< Emitted on link click

	PangoWidgetBase(): update_(false), append_cache_(true) {}
	virtual ~PangoWidgetBase() {}

	//TODO: make it not public and introduce function to work with it
	virtual GtkWidget *widget() = 0;

	virtual std::string get_text() = 0;
	void set_text(const char *str);
	void append_text(const char *str);
	virtual void insert_text(const char *str, const char *where_mark_name, 
		int char_offset = 0);
	void append_pango_text(const char *str);
	virtual void insert_pango_text(const char *str, const char *where_mark_name, 
		int char_offset = 0) = 0;
	virtual void append_pango_text_with_links(const std::string&,
						  const LinksPosList&);
	virtual void insert_pango_text_with_links(const std::string& str, 
		const LinksPosList& links, const char *where_mark_name, int char_offset = 0
		) = 0;
	void set_pango_text(const char *str);
	/* delete text char_length chars length starting char_offset chars forward
	 * from the mark named where_mark_name. */
	virtual void delete_text(const char *where_mark_name, int char_length, 
		int char_offset = 0) = 0;

	virtual void clear();
	virtual void append_mark(const char *mark_name, bool left_gravity = true) = 0;
	/* Insert a mark mark_name at the point char_offset characters forward from 
	 * the where_mark_name mark. Offset may be negative. */
	virtual void insert_mark(const char *mark_name, const char *where_mark_name, 
		int char_offset = 0, bool left_gravity = true) = 0;
	virtual void clone_mark(const char *mark_name, const char *where_mark_name, 
		bool left_gravity = true) = 0;
	/* return value: whether the mark existed. */
	virtual bool delete_mark(const char *mark_name) { return false; }
	virtual void append_pixbuf(GdkPixbuf *pixbuf, const char *label) = 0;
	virtual void insert_pixbuf(GdkPixbuf *pixbuf, const char *label, 
		const char *where_mark_name, int char_offset = 0) = 0;
	virtual void append_widget(GtkWidget *widget) = 0;
	virtual void insert_widget(GtkWidget *widget, const char *where_mark_name, 
		int char_offset = 0) = 0;
	virtual void begin_update();
	virtual void end_update();
	virtual void goto_begin() {}
	virtual void goto_end() {}
#if GTK_MAJOR_VERSION >= 3
	virtual void modify_bg(GtkStateFlags state, const GdkRGBA *color) {}
#else
	virtual void modify_bg(GtkStateType state, const GdkColor *color) {}
#endif
	/* Indent a region specified by two marks. If the mark_end is invalid,
	 * for instance NULL, assume the end mark is at the end of the buffer. */
	virtual void indent_region(const char *mark_begin, int char_offset_begin = 0, 
		const char *mark_end = NULL, int char_offset_end = 0) {}
	virtual void reindent(void) {}
	/* flush cache_ */
	virtual void flush(void);

	GtkWidget *vscroll_bar() { return gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(scroll_win_)); }
	void set_size(gint w, gint h) {
		gtk_widget_set_size_request(GTK_WIDGET(scroll_win_), w, h);
	}
	gint scroll_space() {
		gint val;
		gtk_widget_style_get(GTK_WIDGET(scroll_win_),
			 "scrollbar_spacing", &val, NULL);
		return val;
	}
	GtkWidget *window() { return GTK_WIDGET(scroll_win_); }
	void scroll_to(gdouble val) {
		gtk_adjustment_set_value(
			gtk_scrolled_window_get_vadjustment(scroll_win_), val
			);
	}
	gdouble scroll_pos() {
		return  gtk_adjustment_get_value(
			gtk_scrolled_window_get_vadjustment(scroll_win_)
			);
	}

	static PangoWidgetBase *create(GtkBox *owner, bool autoresize) {
		PangoWidgetBase *res = create(autoresize);
		gtk_box_pack_start(owner, res->window(), TRUE, TRUE, 0);
		return res;
	}

	static PangoWidgetBase *create(GtkContainer *owner, bool autoresize) {
		PangoWidgetBase *res = create(autoresize);
		gtk_container_add(owner, res->window());
		return res;
	}
protected:
	GtkScrolledWindow *scroll_win_;
	/* This variable specifies should updates be cached or applied immediately.
	 * It is set in the begin_update and end_update methods. */
	bool update_;
	std::string cache_;
	/* Specifies whether the cached text must be appended to the committed text
	 * or it must overwrite the committed text. */
	bool append_cache_;

	virtual void do_set_text(const char *str) = 0;
	virtual void do_append_text(const char *str) = 0;
	virtual void do_append_pango_text(const char *str) = 0;
	virtual void do_set_pango_text(const char *str) = 0;
	
private:
	static PangoWidgetBase *create(bool autoresize);
#ifdef DEBUG
public:
	virtual void print_string_with_marks(void) {}
#endif
};


#endif //pangoview.h
