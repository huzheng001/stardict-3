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

#include "gtktextviewpango.h"
#include "utils.h"
#include "skin.h"//for SkinCursor definition

#include "pangoview.h"

class TextPangoWidget : public PangoWidgetBase {
public:
	TextPangoWidget();
	GtkWidget *widget() { return GTK_WIDGET(textview_); }

	void clear();
	void append_mark(const char *mark);
	void append_pixbuf(GdkPixbuf *pixbuf, const char *label);
	void append_widget(GtkWidget *widget);
	void begin_update();
	void end_update();
	std::string get_text();
	void append_pango_text_with_links(const std::string&,
					  const LinksPosList&);
protected:
	void do_set_text(const char *str);
	void do_append_text(const char *str);
	void do_append_pango_text(const char *str);
	void do_set_pango_text(const char *str);
private:
	GtkTextView *textview_;
	std::list<GtkTextMark *> marklist_;
	struct TextBufPos {
		gint beg_;
		gint end_;
		std::string link_;
		TextBufPos(gint beg, gint end, std::string link): beg_(beg), end_(end), link_(link) {}
	};
	typedef std::vector<TextBufPos> TextBufLinks;

	TextBufLinks tb_links_;
	GtkTextIter iter_;
	SkinCursor hand_cursor_, regular_cursor_;

	static gboolean on_mouse_move(GtkWidget *, GdkEventMotion *, gpointer);
	static gboolean on_button_release(GtkWidget *, GdkEventButton *, gpointer);

	void goto_begin();
	void goto_end();
	TextBufLinks::const_iterator find_link(gint x, gint y);
};

class LabelPangoWidget : public PangoWidgetBase {
public:
	LabelPangoWidget();
	GtkWidget *widget() { return GTK_WIDGET(label_); }

	void clear();
	void append_mark(const char *mark) {}
	void append_pixbuf(GdkPixbuf *pixbuf, const char *label);
	void append_widget(GtkWidget *widget);
	std::string get_text();
	void modify_bg(GtkStateType state, const GdkColor *color);
protected:
	void do_set_text(const char *str);
	void do_append_text(const char *str);
	void do_append_pango_text(const char *str);
	void do_set_pango_text(const char *str);
private:
	GtkLabel *label_;
	GtkWidget *viewport_;
};


void PangoWidgetBase::begin_update()
{
	update_ = true;
}

void PangoWidgetBase::end_update()
{
	if (update_) {
		update_ = false;
		do_append_pango_text(cache_.c_str());
		cache_.clear();
	}
}

void PangoWidgetBase::append_text(const char *str)
{
	if (update_) {
		gchar *mark = g_markup_escape_text(str, -1);
		cache_ += mark;
		g_free(mark);
	} else {
		do_append_text(str);
	}
}

void PangoWidgetBase::append_pango_text(const char *str)
{
	if (update_)
		cache_ += str;
	else
		do_append_pango_text(str);
}

void PangoWidgetBase::append_pango_text_with_links(const std::string& str,
						   const LinksPosList&)
{
	append_pango_text(str.c_str());
}

void PangoWidgetBase::set_pango_text(const char *str)
{
	if (update_)
		cache_ = str;
	else
		do_set_pango_text(str);
}


void TextPangoWidget::begin_update()
{
	PangoWidgetBase::begin_update();
	gtk_text_buffer_begin_user_action(
		gtk_text_view_get_buffer(textview_));
}


void TextPangoWidget::end_update()
{
	PangoWidgetBase::end_update();
	gtk_text_buffer_end_user_action(gtk_text_view_get_buffer(textview_));
}


TextPangoWidget::TextPangoWidget()
{
	hand_cursor_.reset(gdk_cursor_new(GDK_HAND2));
	regular_cursor_.reset(gdk_cursor_new(GDK_XTERM));
	textview_ = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_widget_show(GTK_WIDGET(textview_));
	gtk_text_view_set_editable(textview_, FALSE);
	gtk_text_view_set_cursor_visible(textview_, FALSE);
	gtk_text_view_set_wrap_mode(textview_, GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin(textview_, 5);
	gtk_text_view_set_right_margin(textview_, 5);

	g_signal_connect(textview_, "button-release-event",
			 G_CALLBACK(on_button_release), this);
	g_signal_connect(textview_, "motion-notify-event",
			 G_CALLBACK(on_mouse_move), this);
	
	gtk_text_buffer_get_iter_at_offset(gtk_text_view_get_buffer(textview_),
					   &iter_, 0);
	scroll_win_ = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_widget_show(GTK_WIDGET(scroll_win_));


	gtk_scrolled_window_set_policy(scroll_win_,
				       //altought textview's set_wrap_mode will cause
				       //this can be GTK_POLICY_NEVER,but...
				       //there are widgets that may make this broken.
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll_win_), GTK_WIDGET(textview_));
	gtk_scrolled_window_set_shadow_type(scroll_win_, GTK_SHADOW_IN);
}

void LabelPangoWidget::modify_bg(GtkStateType state, const GdkColor *color)
{
	gtk_widget_modify_bg(viewport_, state, color);
}

LabelPangoWidget::LabelPangoWidget()
{
    label_ = GTK_LABEL(gtk_label_new(NULL));
    gtk_label_set_justify(label_, GTK_JUSTIFY_LEFT);
    scroll_win_ = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
    gtk_scrolled_window_set_shadow_type(scroll_win_, GTK_SHADOW_NONE);
    gtk_scrolled_window_set_policy(scroll_win_, GTK_POLICY_NEVER,
				   GTK_POLICY_AUTOMATIC);

    viewport_ =
	    gtk_viewport_new(gtk_scrolled_window_get_hadjustment(scroll_win_),
			     gtk_scrolled_window_get_vadjustment(scroll_win_));
    gtk_widget_add_events(viewport_, GDK_BUTTON1_MOTION_MASK);
    gtk_widget_add_events(viewport_, GDK_BUTTON_RELEASE_MASK);
    gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport_), GTK_SHADOW_NONE);
    gtk_container_add(GTK_CONTAINER(scroll_win_), viewport_);
    gtk_container_add(GTK_CONTAINER(viewport_), GTK_WIDGET(label_));
}

PangoWidgetBase *PangoWidgetBase::create(bool autoresize)
{
	if (!autoresize)
		return new TextPangoWidget;
	else
		return new LabelPangoWidget;
}

void TextPangoWidget::do_set_text(const char *text)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview_);

	std::list<GtkTextMark *>::const_iterator it;
	for (it = marklist_.begin(); it != marklist_.end(); ++it)
		gtk_text_buffer_delete_mark(buffer, *it);
	marklist_.clear();
	tb_links_.clear();

	gtk_text_buffer_set_text(buffer, text, -1);
}

void LabelPangoWidget::do_set_text(const char *text)
{
	scroll_to(0);
	// this should speed up the next two line.
	gtk_label_set_markup(label_, "");
	// so Popup()'s gtk_widget_size_request(label, &requisition); can
	gtk_widget_set_size_request(GTK_WIDGET(label_), -1, -1);
	// get its original width.
	gtk_label_set_line_wrap(label_, FALSE);
	gchar *mstr = g_markup_escape_text(text, -1);
	gtk_label_set_text(label_, mstr);
	g_free(mstr);
}

void LabelPangoWidget::append_pixbuf(GdkPixbuf *pixbuf, const char *label)
{
	if (label) {
		gchar *markup = g_markup_printf_escaped("<span foreground=\"red\">[Image:%s]</span>", label);
		append_pango_text(markup);
		g_free(markup);
	} else {
		append_pango_text("<span foreground=\"red\">[Image]</span>");
	}
}

void LabelPangoWidget::append_widget(GtkWidget *widget)
{
	append_pango_text("<span foreground=\"red\">[Widget]</span>");
	if (widget) {
		gtk_widget_destroy(widget);
	}
}

void PangoWidgetBase::set_text(const char *str)
{
	if (update_) {
		gchar *mark = g_markup_escape_text(str, -1);
		cache_ = mark;
		g_free(mark);
	} else {
		do_set_text(str);
	}
}

void TextPangoWidget::do_append_text(const char *str)
{
	gtk_text_buffer_insert(gtk_text_view_get_buffer(textview_),
			       &iter_, str, strlen(str));
}

void LabelPangoWidget::do_append_text(const char *str)
{
	set_text((std::string(gtk_label_get_text(label_)) + str).c_str());
}


void TextPangoWidget::do_append_pango_text(const char *str)
{

    gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_),
				  &iter_, str);
}

void TextPangoWidget::do_set_pango_text(const char *str)
{
	clear();
	goto_begin();
	do_append_pango_text(str);
}

void LabelPangoWidget::do_set_pango_text(const char *str)
{
	scroll_to(0);
	// this should speed up the next two line.
	gtk_label_set_markup(label_, "");
	// so Popup()'s gtk_widget_size_request(label,&requisition); can
	gtk_widget_set_size_request(GTK_WIDGET(label_), -1, -1);
	// get its original width.
	gtk_label_set_line_wrap(label_, FALSE);
	gtk_label_set_markup(label_, str);
}

void LabelPangoWidget::do_append_pango_text(const char *str)
{
   do_set_pango_text((std::string(gtk_label_get_label(label_)) + str).c_str());
}

void TextPangoWidget::append_mark(const char *mark)
{
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview_);
	if (update_) {
		if (!cache_.empty()) {
			do_append_pango_text(cache_.c_str());
			cache_.clear();
		}
	}
	marklist_.push_back(
		gtk_text_buffer_create_mark(buffer, mark, &iter_, TRUE));
}

void TextPangoWidget::clear()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview_);

	std::list<GtkTextMark *>::const_iterator it;
	for (it = marklist_.begin(); it != marklist_.end(); ++it)
		gtk_text_buffer_delete_mark(buffer, *it);

	marklist_.clear();
	tb_links_.clear();

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gtk_text_buffer_delete(buffer, &start, &end);
	scroll_to(0);
	cache_.clear();
}

void LabelPangoWidget::clear()
{
	do_set_text("");
	cache_.clear();
}

void TextPangoWidget::goto_begin()
{
	gtk_text_buffer_get_iter_at_offset(
		gtk_text_view_get_buffer(textview_), &iter_, 0
		);
}

void TextPangoWidget::goto_end()
{
    gtk_text_buffer_get_iter_at_offset(gtk_text_view_get_buffer(textview_), &iter_, -1);
}

std::string TextPangoWidget::get_text()
{
	std::string res;

	GtkTextIter start, end;
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview_);
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
	res = text;
	g_free(text);

	return res;
}

std::string LabelPangoWidget::get_text()
{
	return gtk_label_get_text(label_);
}


void TextPangoWidget::append_pango_text_with_links(const std::string& str,
						   const LinksPosList& links)
{
	if (links.empty()) {
		append_pango_text(str.c_str());
		return;
	}

	do_append_pango_text(cache_.c_str());
	cache_.clear();

	gint beg = gtk_text_iter_get_offset(&iter_);

	for (LinksPosList::const_iterator it = links.begin();
	     it != links.end(); ++it) {
		tb_links_.push_back(TextBufPos(beg + it->pos_, beg + it->pos_ + it->len_, it->link_));	
	}

	gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_),
				      &iter_, str.c_str());
}

void TextPangoWidget::append_pixbuf(GdkPixbuf *pixbuf, const char *label)
{
	do_append_pango_text(cache_.c_str());
	cache_.clear();
	gtk_text_buffer_insert_pixbuf (gtk_text_view_get_buffer(textview_), &iter_, pixbuf);
}

void TextPangoWidget::append_widget(GtkWidget *widget)
{
	do_append_pango_text(cache_.c_str());
	cache_.clear();
	GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor (gtk_text_view_get_buffer(textview_), &iter_);
	gtk_text_view_add_child_at_anchor (textview_, widget, anchor);
}

TextPangoWidget::TextBufLinks::const_iterator TextPangoWidget::find_link(gint x,
									 gint y)
{
	GtkTextIter iter;
	gtk_text_view_get_iter_at_location(textview_, &iter, x, y);
	gint pos = gtk_text_iter_get_offset(&iter);
	TextBufLinks::const_iterator it;
	for (it = tb_links_.begin(); it != tb_links_.end(); ++it) {
		if (pos < it->beg_)
			return tb_links_.end();
		if (it->beg_ <= pos && pos < it->end_) 
			break;
	}
	return it;
}

gboolean TextPangoWidget::on_mouse_move(GtkWidget *widget, GdkEventMotion *event,
					gpointer userdata)
{
	TextPangoWidget *tpw = static_cast<TextPangoWidget *>(userdata);
	GtkTextWindowType win_type =
		gtk_text_view_get_window_type(tpw->textview_, event->window);
	gint x, y;
	gtk_text_view_window_to_buffer_coords(tpw->textview_, win_type, 
					      gint(event->x), gint(event->y),
					      &x, &y);

	TextBufLinks::const_iterator it = tpw->find_link(x, y);
	if (it != tpw->tb_links_.end()) {
		gdk_window_set_cursor(
			gtk_text_view_get_window(tpw->textview_,
						 GTK_TEXT_WINDOW_TEXT),
			get_impl(tpw->hand_cursor_));
	} else {
		gdk_window_set_cursor(
			gtk_text_view_get_window(tpw->textview_,
						 GTK_TEXT_WINDOW_TEXT),
			get_impl(tpw->regular_cursor_));
	}

	gdk_window_get_pointer(widget->window, NULL, NULL, NULL);

	return FALSE;
}



gboolean TextPangoWidget::on_button_release(GtkWidget *, GdkEventButton *event,
					    gpointer userdata)
{
	if (event->button != 1)
		return FALSE;
	TextPangoWidget *tpw = static_cast<TextPangoWidget *>(userdata);
	GtkTextBuffer *buf = gtk_text_view_get_buffer(tpw->textview_);
	/* we shouldn't follow a link if the user has selected something */
	GtkTextIter beg, end;
	gtk_text_buffer_get_selection_bounds (buf, &beg, &end);
	if (gtk_text_iter_get_offset (&beg) != gtk_text_iter_get_offset (&end))
		return FALSE;
	GtkTextWindowType win_type =
		gtk_text_view_get_window_type(tpw->textview_, event->window);
	gint x, y;
	gtk_text_view_window_to_buffer_coords(tpw->textview_, win_type, 
					      gint(event->x), gint(event->y),
					      &x, &y);
	TextBufLinks::const_iterator it = tpw->find_link(x, y);
	if (it != tpw->tb_links_.end()) {
		tpw->on_link_click_.emit(it->link_);
	}	
	return FALSE;
}
