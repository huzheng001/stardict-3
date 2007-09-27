#ifndef PANGOVIEW_H
#define PAGNOVIEW_H

#include <list>
#include <string>
#include <gtk/gtk.h>
#include <memory>

#include "sigc++/sigc++.h"
#include "lib/parsedata_plugin.h"

/**
 * Base class for classes which can show pango formated text
 */
class PangoWidgetBase {
public:
	sigc::signal<void, const std::string &> on_link_click_;//!< Emitted on link click

	PangoWidgetBase(): update_(false) {}
	virtual ~PangoWidgetBase() {}

	//TODO: make it not public and introduce function to work with it
	virtual GtkWidget *widget() = 0;

	void set_text(const char *str);
	void append_text(const char *str);
	void append_pango_text(const char *str);
	virtual void append_pango_text_with_links(const std::string&,
						  const LinksPosList&);
	void set_pango_text(const char *str);
	virtual std::string get_text() = 0;

	virtual void clear() = 0;
	virtual void append_mark(const char *mark) = 0;
	virtual void append_pixbuf(GdkPixbuf *pixbuf, const char *label) = 0;
	virtual void append_widget(GtkWidget *widget) = 0;
	virtual void begin_update();
	virtual void end_update();
	virtual void goto_begin() {}
	virtual void goto_end() {}
	virtual void modify_bg(GtkStateType state, const GdkColor *color) {}

	GtkWidget *vscroll_bar() { return scroll_win_->vscrollbar; }
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
	bool update_;
	std::string cache_;

	virtual void do_set_text(const char *str) = 0;
	virtual void do_append_text(const char *str) = 0;
	virtual void do_append_pango_text(const char *str) = 0;
	virtual void do_set_pango_text(const char *str) = 0;
private:
	static PangoWidgetBase *create(bool autoresize);
};


#endif//pangoview.h
