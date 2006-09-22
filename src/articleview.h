#ifndef ARTICLEVIEW_H
#define ARTICLEVIEW_H

#include <string>
#include <gtk/gtk.h>

#include "pangoview.h" 

//class which show dictionary's aritcles
class ArticleView {
public:
	ArticleView(GtkContainer *owner, bool floatw=false): 
		pango_view_(PangoWidgetBase::create(owner, floatw)), for_float_win(floatw) {}
	ArticleView(GtkBox *owner, bool floatw=false)
		: pango_view_(PangoWidgetBase::create(owner, floatw)), for_float_win(floatw) {}

	void AppendHeader(const std::string& dict_name, size_t i);
	void AppendWord(const gchar *word);
	void AppendData(gchar *data, const gchar *oword);
	void AppendNewline();
	void AppendDataSeparate();

	GtkWidget *widget() { return pango_view_->widget(); }
	void scroll_to(gdouble val) { pango_view_->scroll_to(val); }
	void set_text(const char *str) { pango_view_->set_text(str); }
	void append_text(const char *str) { pango_view_->append_text(str); }
	void append_pango_text(const char *str) { pango_view_->append_pango_text(str); }
	void set_pango_text(const char *str) { pango_view_->set_pango_text(str); }
	std::string get_text() { return pango_view_->get_text(); }
	void append_pango_text_with_links(const std::string& str,
					  const LinksPosList& links) {
		pango_view_->append_pango_text_with_links(str, links);
	}
	void clear() { pango_view_->clear(); }
	void append_mark(const char *mark) { pango_view_->append_mark(mark); }
	void begin_update() { pango_view_->begin_update(); }
	void end_update() { pango_view_->end_update(); }
	void goto_begin() { pango_view_->goto_begin(); }
	GtkWidget *vscroll_bar() { return pango_view_->vscroll_bar(); }
	void set_size(gint w, gint h) { pango_view_->set_size(w, h); }
	gint scroll_space() { return pango_view_->scroll_space(); }
	GtkWidget *window() { return pango_view_->window(); }
	gdouble scroll_pos() { return pango_view_->scroll_pos(); }
	void connect_on_link(const sigc::slot<void, const char *>& s);
private:
	std::auto_ptr<PangoWidgetBase> pango_view_;
	bool for_float_win;

	static std::string xdxf2pango(const char *p, LinksPosList& links_list);
};


#endif//ARTICLEVIEW_H
