#ifndef ARTICLEVIEW_H
#define ARTICLEVIEW_H

#include <string>
#include <gtk/gtk.h>

#include "pangoview.h" 
#include "lib/dictmask.h"

//class which show dictionary's aritcles
class ArticleView {
public:
    unsigned int bookindex;
	ArticleView(GtkContainer *owner, bool floatw=false): 
		bookindex(0), pango_view_(PangoWidgetBase::create(owner, floatw)), for_float_win(floatw) {}
	ArticleView(GtkBox *owner, bool floatw=false)
		: pango_view_(PangoWidgetBase::create(owner, floatw)), for_float_win(floatw) {}

	void SetDictIndex(InstantDictIndex index);
	void AppendHeaderMark();
	void AppendHeader(const char *dict_name);
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
	void clear() { pango_view_->clear(); bookindex = 0;}
	void append_mark(const char *mark) { pango_view_->append_mark(mark); }
	void begin_update() { pango_view_->begin_update(); }
	void end_update() { pango_view_->end_update(); }
	void goto_begin() { pango_view_->goto_begin(); }
	void goto_end() { pango_view_->goto_end(); }
	void modify_bg(GtkStateType state, const GdkColor *color) { pango_view_->modify_bg(state, color); } 
	GtkWidget *vscroll_bar() { return pango_view_->vscroll_bar(); }
	void set_size(gint w, gint h) { pango_view_->set_size(w, h); }
	gint scroll_space() { return pango_view_->scroll_space(); }
	GtkWidget *window() { return pango_view_->window(); }
	gdouble scroll_pos() { return pango_view_->scroll_pos(); }
	void connect_on_link(const sigc::slot<void, const std::string &>& s);
private:
	std::auto_ptr<PangoWidgetBase> pango_view_;
	bool for_float_win;
	InstantDictIndex dict_index;

	std::string xdxf2pango(const char *p, const gchar *oword, LinksPosList& links_list);
	void append_and_mark_orig_word(const std::string& mark,
				       const gchar *origword,
				       const LinksPosList& links);
};


#endif//ARTICLEVIEW_H
