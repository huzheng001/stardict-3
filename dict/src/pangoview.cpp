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
#include <iostream>
#include <map>
#include "gtktextviewpango.h"
#include "lib/utils.h"
#include "skin.h"//for SkinCursor definition
#include "lib/xml_str.h"

#include "pangoview.h"

//#define DEBUG
//#define DDEBUG

class TextPangoWidget : public PangoWidgetBase {
public:
	TextPangoWidget();
	GtkWidget *widget() { return GTK_WIDGET(textview_); }
	GtkTextView *get_text_view() { return textview_; }

	std::string get_text();
	void insert_pango_text(const char *str, const char *where_mark_name, 
		int char_offset = 0);
	void append_pango_text_with_links(const std::string&, const LinksPosList&);
	void insert_pango_text_with_links(const std::string& str,
		const LinksPosList& links, const char *where_mark_name, int char_offset = 0);
	void delete_text(const char *where_mark_name, int char_length, 
		int char_offset = 0);
	void clear();
	void append_mark(const char *mark_name, bool left_gravity = true);
	void insert_mark(const char *mark_name, const char *where_mark_name, 
		int char_offset = 0, bool left_gravity = true);
	void clone_mark(const char *mark_name, const char *where_mark_name, 
		bool left_gravity = true);
	bool delete_mark(const char *mark);
	void append_pixbuf(GdkPixbuf *pixbuf, const char *label);
	void insert_pixbuf(GdkPixbuf *pixbuf, const char *label, 
		const char *where_mark_name, int char_offset = 0);
	void append_widget(GtkWidget *widget);
	void insert_widget(GtkWidget *widget, const char *where_mark_name, 
		int char_offset = 0);
	void begin_update();
	void end_update();
	void goto_begin();
	void goto_end();
#if GTK_MAJOR_VERSION >= 3
	void modify_bg(GtkStateFlags state, const GdkRGBA *color);
#else
	void modify_bg(GtkStateType state, const GdkColor *color);
#endif
	void indent_region(const char *mark_begin, int char_offset_begin = 0, 
		const char *mark_end = NULL, int char_offset_end = 0);
	void reindent(void);
protected:
	void do_set_text(const char *str);
	void do_append_text(const char *str);
	void do_append_pango_text(const char *str);
	void do_set_pango_text(const char *str);
private:
	GtkTextView *textview_;
	typedef std::list<GtkTextMark *> MarkList;
	MarkList marklist_;
	class TextBufPos {
	public:
		GtkTextMark *beg_, *end_;
		std::string link_;
		TextBufPos(gint beg, gint end, std::string link, TextPangoWidget *widget)
			: beg_(NULL), end_(NULL), link_(link), widget_(widget)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_offset(buffer, &iter, beg);
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
			gtk_text_buffer_get_iter_at_offset(buffer, &iter, end);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
		}
		TextBufPos(const TextBufPos& tbp)
			: beg_(NULL), end_(NULL), link_(tbp.link_), widget_(tbp.widget_)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(buffer, &iter, tbp.beg_);
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
			gtk_text_buffer_get_iter_at_mark(buffer, &iter, tbp.end_);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
		}
		TextBufPos& operator=(const TextBufPos& tbp)
		{
			clear();
			link_ = tbp.link_;
			widget_ = tbp.widget_;
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			GtkTextIter iter;
			gtk_text_buffer_get_iter_at_mark(buffer, &iter, tbp.beg_);
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, FALSE);
			gtk_text_buffer_get_iter_at_mark(buffer, &iter, tbp.end_);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
			return *this;
		}
		~TextBufPos(void)
		{
			clear();
		}
		void text_iter_beg(GtkTextIter* iter) const
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			gtk_text_buffer_get_iter_at_mark(buffer, iter, beg_);
		}
		void text_iter_end(GtkTextIter* iter) const
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			gtk_text_buffer_get_iter_at_mark(buffer, iter, end_);
		}
	private:
		TextBufPos(void);
		void clear(void)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			gtk_text_buffer_delete_mark(buffer, beg_);
			gtk_text_buffer_delete_mark(buffer, end_);
			beg_ = end_ = NULL;
		}
		
	private:
		TextPangoWidget *widget_;
	};
	typedef std::vector<TextBufPos> TextBufLinks;

	/* Strips of the text buffer containing links. A link is defined by two
	 * marks. When user clicks somewhere in the buffer, this list is searched 
	 * for a matching region. If a matching region is found, the user hit a link
	 * and the text contained in the link_ member is processed. See also the
	 * find_link member. */
	TextBufLinks tb_links_;
	GtkTextIter iter_;
	SkinCursor hand_cursor_, regular_cursor_;
	typedef std::vector<GtkTextTag*> IndentTags;
	/* An array of tags that indent text by the specified amount. 
	 * indent_tags_[i] - a tag that indent by i+1. Use the get_indent_tag
	 * method to get the tag desired. */
	IndentTags indent_tags_;
	// size of a single indent in pixels
	static const int indent_size_pxl_ = 10;
	static const int left_margin_size_pxl_ = 5;
	static const int right_margin_size_pxl_ = 5;
	class IndentedRegion {
	public:
		/* Both marks must have left gravity. So text added to the left will
		 * be included in the indented region, but the text added to the 
		 * right will not. In order to changes take effect, the region must
		 * be reindented!
		 * The beg_ mark must be preceded by a new line character or be at the
		 * beginning of the buffer, otherwise the first paragraph of the indent
		 * region will not be indented. */
		GtkTextMark *beg_, *end_;
		int indent_;
		IndentedRegion(const GtkTextIter *beg, const GtkTextIter *end, int indent, 
			TextPangoWidget *widget)
			: beg_(NULL), end_(NULL), indent_(indent), widget_(widget)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, beg, TRUE);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, end, TRUE);
		}
		IndentedRegion(const IndentedRegion& ir)
			: beg_(NULL), end_(NULL), indent_(ir.indent_), widget_(ir.widget_)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			GtkTextIter iter;
			ir.text_iter_beg(&iter);
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
			ir.text_iter_end(&iter);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
		}
		IndentedRegion& operator=(const IndentedRegion& ir)
		{
			clear();
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			widget_ = ir.widget_;
			indent_ = ir.indent_;
			GtkTextIter iter;
			ir.text_iter_beg(&iter);
			beg_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
			ir.text_iter_end(&iter);
			end_ = gtk_text_buffer_create_mark(buffer, NULL, &iter, TRUE);
			return *this;
		}
		~IndentedRegion(void)
		{
			clear();
		}
		void text_iter_beg(GtkTextIter* iter) const
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			gtk_text_buffer_get_iter_at_mark(buffer, iter, beg_);
		}
		void text_iter_end(GtkTextIter* iter) const
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			gtk_text_buffer_get_iter_at_mark(buffer, iter, end_);
		}
		/* This method was created for convenience only. It should be called
		 * explicitly by the IndentedRegions object user, it should not be
		 * called from a IndentedRegion constructor.  */
		void indent(void) const
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			GtkTextIter beg, end;
			text_iter_beg(&beg);
			text_iter_end(&end);
			GtkTextTag *tag = widget_->get_indent_tag(indent_);
			gtk_text_buffer_apply_tag(buffer, tag, &beg, &end);
		}
	private:
		IndentedRegion(void);
		void clear(void)
		{
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(widget_->get_text_view());
			// We do not remove the tag applied. Should we?
			gtk_text_buffer_delete_mark(buffer, beg_);
			gtk_text_buffer_delete_mark(buffer, end_);
			beg_ = end_ = NULL;
		}
	private:
		TextPangoWidget *widget_;
	};
	typedef std::list<IndentedRegion> IndentedRegions;
	/* a list of not overlapping indentation regions sorted in ascending order */
	IndentedRegions indented_regions_;
	/* counts gtk_text_buffer_begin_user_action and gtk_text_buffer_end_user_action
	 * functions calls */
	int buffer_user_action_cnt;

	static gboolean on_mouse_move(GtkWidget *, GdkEventMotion *, gpointer);
	static gboolean on_button_release(GtkWidget *, GdkEventButton *, gpointer);
	static void on_destroy(GtkWidget *widget, gpointer);

	bool goto_mark(const char *where_mark_name, int char_offset = 0);
	TextBufLinks::const_iterator find_link(gint x, gint y);
	MarkList::iterator find_mark(const char* mark);
	GtkTextTag* get_indent_tag(int indent);
	void indent_region(const GtkTextIter *begin, const GtkTextIter *end, 
		int indent_rel = 1);

#ifdef DEBUG
public:
	void print_indent_regions(void);
	void print_tb_links(void);
	void print_marks(void);
#endif
};

class LabelPangoWidget : public PangoWidgetBase {
public:
	LabelPangoWidget();
	GtkWidget *widget() { return GTK_WIDGET(label_); }

	std::string get_text();
	void insert_pango_text(const char *str, const char *where_mark_name, 
		int char_offset = 0);
	void insert_pango_text_with_links(const std::string& str, 
		const LinksPosList& links, const char *where_mark_name, int char_offset = 0);
	void delete_text(const char *where_mark_name, int char_length, 
		int char_offset = 0);
	void clear();
	void append_mark(const char *mark_name, bool left_gravity = true);
	void insert_mark(const char *mark_name, const char *where_mark_name, 
		int char_offset = 0, bool left_gravity = true);
	void clone_mark(const char *mark_name, const char *where_mark_name, 
		bool left_gravity = true);
	bool delete_mark(const char *mark_name);
	void append_pixbuf(GdkPixbuf *pixbuf, const char *label);
	void insert_pixbuf(GdkPixbuf *pixbuf, const char *label, 
		const char *where_mark_name, int char_offset = 0);
	void append_widget(GtkWidget *widget);
	void insert_widget(GtkWidget *widget, const char *where_mark_name, 
		int char_offset = 0);
	void begin_update();
	void end_update();
#if GTK_MAJOR_VERSION >= 3
	void modify_bg(GtkStateFlags state, const GdkRGBA *color);
#else
	void modify_bg(GtkStateType state, const GdkColor *color);
#endif
protected:
	void do_set_text(const char *str);
	void do_append_text(const char *str);
	void do_append_pango_text(const char *str);
	void do_set_pango_text(const char *str);
private:
	/* apply change stored in the pango_text_ field. */
	void flush_label(void);
	std::string get_pixbuf_replace_pango_text(GdkPixbuf *pixbuf, const char *label);
	std::string get_widget_replace_pango_text(GtkWidget *widget);
	size_t get_index_by_offset(const char *where_mark_name, int char_offset);

	struct Mark;
	static bool position_less(const Mark& mark1, const Mark& mark2);
	static bool position_equal(const Mark& mark1, const Mark& mark2);
	static bool position_more(const Mark& mark1, const Mark& mark2);
	static bool position_less(Mark * mark1, Mark * mark2);
private:
	/* Similar to GtkTextMark, marks a position in the text that is preserved 
	 * across text modifications. 
	 * 
	 * In string a mark points between characters.
	 * We have a string "01234", marks m_l, m_r point before the character 2,
	 * have left and right gravity respectively.
	 * |0|1|2|3|4|
	 *     ^
	 *     |
	 *    m_l
	 *    m_r
	 *
	 * after inserting string "xx" after 1 we have:
	 * |0|1|x|x|2|3|4|
	 *     ^   ^
	 *     |   |
	 *    m_l m_r
	 * 
	 * Back to the initial string, now remove substring "123", then we have:
	 * |0|4|
	 *   ^
	 *   |
	 *  m_l
	 *  m_r
	 *  
	 * */
	struct Mark
	{
		typedef std::string Name;
		typedef std::list<Mark*> MarksList;
		/* index of the character next to the mark 
		 * Must be in range [0, len], where len - pango text length. 
		 * We count raw characters, which may be part of utf8-encoded char. */
		size_t index_;
		bool left_gravity_;
		/* mark name, may be blank. */
		Name name_;
		/* reference to the place in the list where this mark is stored. */
		MarksList::iterator iter_;
		/* Do not forget to initialize iter_ after object construction! */
		Mark(size_t index, bool left_gravity, const Name& name = "");
		/* position_* functions compare relative position of two marks in the text */
		friend bool LabelPangoWidget::position_less(const Mark& mark1, 
			const Mark& mark2);
		friend bool LabelPangoWidget::position_equal(const Mark& mark1, 
			const Mark& mark2);
		friend bool LabelPangoWidget::position_more(const Mark& mark1, 
			const Mark& mark2);
		friend bool LabelPangoWidget::position_less(Mark * mark1, Mark * mark2);
	private:
		Mark(void);
	};
	class Marks
	{
	public:
		typedef Mark::MarksList MarksList;
		typedef std::map<Mark::Name, Mark*> MarksNamesMap;
	public:
		~Marks(void);
		// add an anonymous mark before the char with index
		Mark* add_mark(size_t index, bool left_gravity);
		// add a named mark
		Mark* add_mark(size_t index, bool left_gravity, const Mark::Name& name);
		/* return value: whether the mark existed. */
		bool delete_mark(Mark *pMark);
		/* return value: whether the mark existed. */
		bool delete_mark(const Mark::Name& name);
		/* where - index of the char before which text will be or has been 
		 * inserted 
		 * len - length of the inserted text in raw chars */
		void insert_text(size_t where, size_t len);
		/* where - index of the first char of the region that will be or
		 * has been deleted. 
		 * len - length of the inserted text in raw chars */
		void delete_text(size_t where, size_t len);
		// delete all text but preserve marks
		void delete_all_text(void);
		/* delete all text as well as all marks */
		void clear(void);
		Mark* find_mark(const Mark::Name& name) const;
	private:
		/* Mark can have a name that is represented by a string now. 
		 * Mark name if present must be unique across the Marks object. 
		 * Any number of marks may be anonymous. 
		 * Marks class consumers should refer to marks by their names or 
		 * Mark structure pointers. 
		 * A Mark object is always allocated in the heap with a help of new 
		 * operator. A Mark object is not copied and the pointer to the object
		 * remains consistent until the mark is deleted explicitly by the
		 * delete_mark method and similar or implicitly by the clear method or
		 * the Marks class destructor. */
		MarksList marks_list_;
		/* Provides mapping between marks names and marks themselves. The map
		 * contains only named marks. */
		MarksNamesMap marks_names_map_;
#ifdef DEBUG
	public:
		void print_string_with_marks(const std::string& pango_text);
#endif
	};
private:
	GtkLabel *label_;
	GtkWidget *viewport_;
	
	/* stores all label text. */
	std::string pango_text_;
	/* Similar to update_. First all changes are applied to pango_text_,
	 * then if update_mode_ is false the widget is updated. */
	bool update_mode_;
	Marks marks_;

#ifdef DEBUG
public:
	void print_string_with_marks(void);
#endif
};

LabelPangoWidget::Mark::Mark(size_t index, bool left_gravity, const Name& name)
:
	index_(index),
	left_gravity_(left_gravity),
	name_(name)
{
}

bool LabelPangoWidget::position_less(const LabelPangoWidget::Mark& mark1, 
	const LabelPangoWidget::Mark& mark2)
{
	if(mark1.index_ < mark2.index_)
		return true;
	if(mark1.index_ == mark2.index_)
		if(mark1.left_gravity_ && !mark2.left_gravity_)
			return true;
	return false;
}

bool LabelPangoWidget::position_equal(const LabelPangoWidget::Mark& mark1, 
	const LabelPangoWidget::Mark& mark2)
{
	return mark1.index_ == mark2.index_
		&& mark1.left_gravity_ == mark2.left_gravity_;
}

bool LabelPangoWidget::position_more(const LabelPangoWidget::Mark& mark1, 
	const LabelPangoWidget::Mark& mark2)
{
	return position_less(mark2, mark1);
}

bool LabelPangoWidget::position_less(LabelPangoWidget::Mark * mark1, 
	LabelPangoWidget::Mark * mark2)
{
	return position_less(*mark1, *mark2);
}

LabelPangoWidget::Marks::~Marks(void)
{
	clear();
}

LabelPangoWidget::Mark* LabelPangoWidget::Marks::add_mark(size_t index, 
	bool left_gravity)
{
#ifdef DDEBUG
	std::cout << "add_mark anonymous" << std::endl;
#endif
	std::unique_ptr<Mark> pMark(new Mark(index, left_gravity));
	pMark->iter_ = marks_list_.insert(marks_list_.end(), pMark.get());
	return pMark.release();
}

LabelPangoWidget::Mark* LabelPangoWidget::Marks::add_mark(size_t index, 
	bool left_gravity, const Mark::Name& name)
{
#ifdef DDEBUG
	std::cout << "add_mark " << name << std::endl;
#endif
	if(name.empty())
		return add_mark(index, left_gravity);
	std::unique_ptr<Mark> pMark(new Mark(index, left_gravity, name));
	std::pair<MarksNamesMap::iterator, bool> res 
		= marks_names_map_.insert(MarksNamesMap::value_type(name, pMark.get()));
	if(!res.second) {
		g_warning("mark %s already exists", name.c_str());
		return res.first->second;
	}
	pMark->iter_ = marks_list_.insert(marks_list_.end(), pMark.get());
	return pMark.release();
}

bool LabelPangoWidget::Marks::delete_mark(Mark *pMark)
{
#ifdef DDEBUG
	std::cout << "delete_mark unspecified" << std::endl;
#endif
	if(!pMark) {
		g_warning("mark does not exist");
		return false;
	}
	if(!pMark->name_.empty()) {
		if(marks_names_map_.erase(pMark->name_) != 1)
			g_warning("map corrupted, mark %s is not found in the map", 
				pMark->name_.c_str());
	}
	marks_list_.erase(pMark->iter_);
	delete pMark;
	return true;
}

bool LabelPangoWidget::Marks::delete_mark(const Mark::Name& name)
{
#ifdef DDEBUG
	std::cout << "delete_mark " << name << std::endl;
#endif
	MarksNamesMap::iterator it = marks_names_map_.find(name);
	if(it == marks_names_map_.end()) {
		g_warning("mark %s does not exist", name.c_str());
		return false;
	}
	Mark *pMark = it->second;
	marks_names_map_.erase(it);
	marks_list_.erase(pMark->iter_);
	delete pMark;
	return true;
}

void LabelPangoWidget::Marks::insert_text(size_t where, size_t len)
{
	Mark mark_beg(where, true);
	MarksList::iterator it;
	for(it = marks_list_.begin(); it != marks_list_.end(); ++it)
		if(position_less(mark_beg, **it))
			(*it)->index_ += len;
}

void LabelPangoWidget::Marks::delete_text(size_t where, size_t len)
{
	Mark mark_beg(where, true), mark_end(where+len, false);
	MarksList::iterator it;
	for(it = marks_list_.begin(); it != marks_list_.end(); ++it)
		if(position_less(mark_beg, **it)) {
			if(position_less(**it, mark_end))
				(*it)->index_ = where;
			else
				(*it)->index_ -= len;
		}
}

void LabelPangoWidget::Marks::delete_all_text(void)
{
	MarksList::iterator it;
	for(it = marks_list_.begin(); it != marks_list_.end(); ++it)
		(*it)->index_ = 0;
}

void LabelPangoWidget::Marks::clear(void)
{
	marks_names_map_.clear();
	MarksList::iterator it;
	for(it = marks_list_.begin(); it != marks_list_.end(); ++it)
		delete *it;
	marks_list_.clear();
}

LabelPangoWidget::Mark* LabelPangoWidget::Marks::find_mark(
	const Mark::Name& name) const
{
	MarksNamesMap::const_iterator it = marks_names_map_.find(name);
	if(it == marks_names_map_.end())
		return NULL;
	else
		return it->second;
}

#ifdef DEBUG
void LabelPangoWidget::Marks::print_string_with_marks(
	const std::string& pango_text)
{
	MarksList t_marks_list(marks_list_);
	t_marks_list.sort<bool (*)(Mark*, Mark*)>(LabelPangoWidget::position_less);
	std::string str;
	std::string mark_tag;
	Marks::MarksList::const_iterator it;
	size_t ind1, ind2;
	for(ind1 = 0, it = t_marks_list.begin(); it != t_marks_list.end(); ++it) {
		mark_tag = "<";
		if((*it)->name_.empty())
			mark_tag += "anonymous mark ";
		else
			mark_tag += (*it)->name_ + " ";
		mark_tag += (*it)->left_gravity_ ? "gr=l" : "gr=r";
		mark_tag += ">";
		ind2 = (*it)->index_;
		if(ind1<ind2)
			str.append(pango_text, ind1, ind2-ind1);
		str.append(mark_tag);
		ind1 = ind2;
	}
	ind2 = pango_text.length();
	if(ind1<ind2)
		str.append(pango_text, ind1, ind2-ind1);
	std::cout << "\nlabel text with marks:\n" << str;
	std::cout.flush();
}
#endif

void PangoWidgetBase::begin_update()
{
	update_ = true;
}

void PangoWidgetBase::end_update()
{
	if (update_) {
		update_ = false;
		flush();
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
	if (update_) {
		cache_ = str;
		append_cache_ = false;
	} else
		do_set_pango_text(str);
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
	gtk_text_view_set_left_margin(textview_, left_margin_size_pxl_);
	gtk_text_view_set_right_margin(textview_, right_margin_size_pxl_);

	g_signal_connect(textview_, "button-release-event",
			 G_CALLBACK(on_button_release), this);
	g_signal_connect(textview_, "motion-notify-event",
			 G_CALLBACK(on_mouse_move), this);
	g_signal_connect(textview_, "destroy",
			 G_CALLBACK(on_destroy), this);
	
	gtk_text_buffer_get_iter_at_offset(gtk_text_view_get_buffer(textview_),
					   &iter_, 0);
	scroll_win_ = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_widget_show(GTK_WIDGET(scroll_win_));


	gtk_scrolled_window_set_policy(scroll_win_,
				       //although textview's set_wrap_mode will cause
				       //this can be GTK_POLICY_NEVER,but...
				       //there are widgets that may make this broken.
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll_win_), GTK_WIDGET(textview_));
	gtk_scrolled_window_set_shadow_type(scroll_win_, GTK_SHADOW_IN);
	buffer_user_action_cnt = 0;
}

#if GTK_MAJOR_VERSION >= 3
void LabelPangoWidget::modify_bg(GtkStateFlags state, const GdkRGBA *color)
{
	gtk_widget_override_background_color(viewport_, state, color);
}
#else
void LabelPangoWidget::modify_bg(GtkStateType state, const GdkColor *color)
{
	gtk_widget_modify_bg(viewport_, state, color);
}
#endif

#if GTK_MAJOR_VERSION >= 3
void TextPangoWidget::modify_bg(GtkStateFlags state, const GdkRGBA *color)
{
	gtk_widget_override_background_color(widget(), state, color);
}
#else
void TextPangoWidget::modify_bg(GtkStateType state, const GdkColor *color)
{
	gtk_widget_modify_base(widget(), state, color);
}
#endif

LabelPangoWidget::LabelPangoWidget()
{
	update_mode_ = false;
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

void TextPangoWidget::begin_update()
{
	gtk_text_buffer_begin_user_action(gtk_text_view_get_buffer(textview_));
	++buffer_user_action_cnt;
	PangoWidgetBase::begin_update();
}

void LabelPangoWidget::begin_update()
{
	update_mode_ = true;
	PangoWidgetBase::begin_update();
}

void TextPangoWidget::end_update()
{
	PangoWidgetBase::end_update();
	--buffer_user_action_cnt;
	gtk_text_buffer_end_user_action(gtk_text_view_get_buffer(textview_));
}

void LabelPangoWidget::end_update()
{
	PangoWidgetBase::end_update();
	update_mode_ = false;
	flush_label();
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
	/* clear call may invalidate the text pointer. */
	std::string text2(text);
	clear();
	goto_begin();
	do_append_text(text2.c_str());
}

void LabelPangoWidget::do_set_text(const char *text)
{
	gchar *mstr = g_markup_escape_text(text, -1);
	do_set_pango_text(mstr);
	g_free(mstr);
}

void PangoWidgetBase::set_text(const char *str)
{
	if (update_) {
		gchar *mark = g_markup_escape_text(str, -1);
		cache_ = mark;
		g_free(mark);
		append_cache_ = false;
	} else {
		do_set_text(str);
	}
}

void TextPangoWidget::do_append_text(const char *str)
{
	goto_end();
	gtk_text_buffer_insert(gtk_text_view_get_buffer(textview_),
			       &iter_, str, strlen(str));
}

void LabelPangoWidget::do_append_text(const char *str)
{
	gchar *mstr = g_markup_escape_text(str, -1);
	do_append_pango_text(mstr);
	g_free(mstr);
}

void TextPangoWidget::do_append_pango_text(const char *str)
{
	goto_end();
	gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_),
				  &iter_, str);
}

void TextPangoWidget::do_set_pango_text(const char *str)
{
	/* clear call may invalidate the str pointer */
	std::string str2(str);
	clear();
	goto_begin();
	do_append_pango_text(str2.c_str());
}

void LabelPangoWidget::do_set_pango_text(const char *str)
{
	marks_.delete_all_text();
	marks_.insert_text(0, strlen(str));
	pango_text_ = str;
	if(!update_mode_)
		flush_label();
}

void LabelPangoWidget::do_append_pango_text(const char *str)
{
	marks_.insert_text(pango_text_.length(), strlen(str));
	pango_text_ += str;
	if(!update_mode_)
		flush_label();
}

void TextPangoWidget::append_mark(const char *mark_name, bool left_gravity)
{
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview_);
	flush();
	goto_end();
	marklist_.push_back(gtk_text_buffer_create_mark(buffer, mark_name, &iter_, 
		left_gravity ? TRUE : FALSE));
}

void LabelPangoWidget::append_mark(const char *mark_name, bool left_gravity)
{
	flush();
	marks_.add_mark(pango_text_.length(), left_gravity, mark_name);
}

void TextPangoWidget::insert_mark(const char *mark_name, 
	const char *where_mark_name, int char_offset, bool left_gravity)
{
	flush();
	goto_mark(where_mark_name, char_offset);
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview_);
	marklist_.push_back(gtk_text_buffer_create_mark(buffer, mark_name, &iter_, 
		left_gravity ? TRUE : FALSE));
}

void LabelPangoWidget::insert_mark(const char *mark_name, 
	const char *where_mark_name, int char_offset, bool left_gravity)
{
	flush();
	size_t ind = get_index_by_offset(where_mark_name, char_offset);
	if(ind != std::string::npos) {
		marks_.add_mark(ind, left_gravity, mark_name);
	} else
		g_warning("label::insert_mark incorrect index. Mark = %s", mark_name);
}

void TextPangoWidget::clone_mark(const char *mark_name, 
	const char *where_mark_name, bool left_gravity)
{
	insert_mark(mark_name, where_mark_name, 0, left_gravity);
}

void LabelPangoWidget::clone_mark(const char *mark_name, 
	const char *where_mark_name, bool left_gravity)
{
	flush();
	Mark *pMark = marks_.find_mark(where_mark_name);
	if(pMark == NULL) {
		g_warning("Mark \"%s\" does not exist", where_mark_name);
		return;
	}
	marks_.add_mark(pMark->index_, left_gravity, mark_name);
}

bool TextPangoWidget::delete_mark(const char *mark)
{
	MarkList::iterator it = find_mark(mark);
	if(it != marklist_.end()) {
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview_);
		gtk_text_buffer_delete_mark(buffer, *it);
		marklist_.erase(it);
		return true;
	}
	return false;
}

bool LabelPangoWidget::delete_mark(const char *mark_name)
{
	return marks_.delete_mark(mark_name);
}

void PangoWidgetBase::clear(void)
{
	cache_.clear();
	append_cache_ = true;
}

#if 0
void collectTags(GtkTextTag *tag, gpointer data)
{
	std::vector<GtkTextTag *>* allTags = (std::vector<GtkTextTag *>*)data;
	allTags->push_back(tag);
}
#endif

void TextPangoWidget::clear()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview_);

	MarkList::const_iterator it;
	for (it = marklist_.begin(); it != marklist_.end(); ++it)
		gtk_text_buffer_delete_mark(buffer, *it);

	marklist_.clear();
	tb_links_.clear();
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gtk_text_buffer_remove_all_tags(buffer, &start, &end);
	gtk_text_buffer_delete(buffer, &start, &end);
	scroll_to(0);
	indented_regions_.clear();
	goto_begin();
	// no need to clear indent_tags_ unless we recreate the buffer
	indent_tags_.clear();
	PangoWidgetBase::clear();
#if 0
	GtkTextTagTable * tagTable = gtk_text_buffer_get_tag_table(buffer);
	gint tagcnt = gtk_text_tag_table_get_size(tagTable);
	g_print("number of tags: %d\n", tagcnt);
#endif
#if 0
	std::vector<GtkTextTag *> allTags;
	allTags.reserve(tagcnt);
	gtk_text_tag_table_foreach(tagTable, collectTags, (gpointer)&allTags);
	g_print("number of tags collected: %lu\n", (unsigned long)allTags.size());
	for(size_t i = 0; i<allTags.size(); ++i) {
		//gtk_text_buffer_remove_tag(buffer, allTags[i], &start, &end);
		//gtk_text_tag_table_remove(tagTable, allTags[i]);
	}
#endif

	for(int i=0; i<buffer_user_action_cnt; ++i)
		gtk_text_buffer_end_user_action(buffer);

	/* recreating text buffer is the only way I found to clear the tag table
	 * gtk_text_buffer_get_tag_table(buffer)
	 * otherwise the tag table will grow and grow slowing down buffer update.
	 * Unfortunately I do not know other was to clear the table,
	 * gtk_text_buffer_remove_all_tags does not help. The tags in question are
	 * added by the gtk_text_buffer_real_insert_markup function. */
	GtkTextBuffer *new_buffer = gtk_text_buffer_new(NULL);
	gtk_text_view_set_buffer(textview_, new_buffer);
	g_object_unref(G_OBJECT(new_buffer));

	for(int i=0; i<buffer_user_action_cnt; ++i)
		gtk_text_buffer_begin_user_action(new_buffer);
}

void LabelPangoWidget::clear()
{
	pango_text_.clear();
	marks_.clear();
	PangoWidgetBase::clear();
	flush_label();
}

void TextPangoWidget::goto_begin()
{
	gtk_text_buffer_get_iter_at_offset(
		gtk_text_view_get_buffer(textview_), &iter_, 0
		);
}

void TextPangoWidget::goto_end()
{
	gtk_text_buffer_get_iter_at_offset(gtk_text_view_get_buffer(textview_), 
		&iter_, -1);
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
	return pango_text_;
}

void PangoWidgetBase::insert_text(const char *str, const char *where_mark_name, 
	int char_offset)
{
	gchar *mark = g_markup_escape_text(str, -1);
	insert_pango_text(mark, where_mark_name, char_offset);
	g_free(mark);
}

void TextPangoWidget::insert_pango_text(const char *str, const char *where_mark_name, 
	int char_offset)
{
	flush();
	goto_mark(where_mark_name, char_offset);
	gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_), &iter_, str);
}

void LabelPangoWidget::insert_pango_text(const char *str, 
	const char *where_mark_name, int char_offset)
{
	flush();
	size_t ind = get_index_by_offset(where_mark_name, char_offset);
	if(ind != std::string::npos) {
		pango_text_.insert(ind, str);
		marks_.insert_text(ind, strlen(str));
	} else {
		return;
	}
	if(!update_mode_)
		flush_label();
}

void TextPangoWidget::append_pango_text_with_links(const std::string& str,
						   const LinksPosList& links)
{
	if (links.empty()) {
		append_pango_text(str.c_str());
		return;
	}

	flush();
	goto_end();

	gint beg = gtk_text_iter_get_offset(&iter_);

	gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_),
				      &iter_, str.c_str());

	for (LinksPosList::const_iterator it = links.begin();
	     it != links.end(); ++it) {
		tb_links_.push_back(
			TextBufPos(beg + it->pos_, beg + it->pos_ + it->len_, it->link_, this));
	}
}

void TextPangoWidget::insert_pango_text_with_links(const std::string& str, 
	const LinksPosList& links, const char *where_mark_name, int char_offset)
{
	if(links.empty()) {
		insert_pango_text(str.c_str(), where_mark_name, char_offset);
		return;
	}
	flush();
	if(!goto_mark(where_mark_name, char_offset))
		return;

	gint beg = gtk_text_iter_get_offset(&iter_);
	gtk_text_buffer_insert_markup(gtk_text_view_get_buffer(textview_), &iter_, 
		str.c_str());

	for (LinksPosList::const_iterator it = links.begin();
	     it != links.end(); ++it) {
		tb_links_.push_back(
			TextBufPos(beg + it->pos_, beg + it->pos_ + it->len_, it->link_, this));
	}
}

void LabelPangoWidget::insert_pango_text_with_links(const std::string& str, 
	const LinksPosList& links, const char *where_mark_name, int char_offset)
{
	insert_pango_text(str.c_str(), where_mark_name, char_offset);
}

void TextPangoWidget::delete_text(const char *where_mark_name, int char_length, 
	int char_offset)
{
	flush();
	if(!goto_mark(where_mark_name, char_offset))
		return;
	GtkTextIter end = iter_;
	gtk_text_iter_forward_chars(&end, char_length);
	gtk_text_buffer_delete(gtk_text_view_get_buffer(textview_), &iter_, &end);
}

void LabelPangoWidget::delete_text(const char *where_mark_name, int char_length, 
	int char_offset)
{
	if(char_length < 0) {
		g_warning("incorrect char_length");
		return;
	}
	if(char_length  == 0)
		return;
	size_t beg_ind = get_index_by_offset(where_mark_name, char_offset);
	if(beg_ind == std::string::npos)
		return;
	const char * beg = pango_text_.c_str();
	// pointer to beginning of the last char
	const char * end = xml_utf8_offset_to_pointer(beg + beg_ind, char_length - 1);
	if(!end || !*end) {
		g_warning("incorrect char_length");
		return;
	}
	end = xml_utf8_end_of_char(end);
	if(!end) {
		g_warning("incorrect char_length");
		return;
	}
	size_t end_ind = end - beg;
	size_t len = 1 + end_ind - beg_ind;
	pango_text_.erase(beg_ind, len);
	marks_.delete_text(beg_ind, len);

	if(!update_mode_)
		flush_label();
}

void TextPangoWidget::append_pixbuf(GdkPixbuf *pixbuf, const char *label)
{
	flush();
	goto_end();
	gtk_text_buffer_insert_pixbuf (gtk_text_view_get_buffer(textview_), &iter_, 
		pixbuf);
}

void LabelPangoWidget::append_pixbuf(GdkPixbuf *pixbuf, const char *label)
{
	append_pango_text(get_pixbuf_replace_pango_text(pixbuf, label).c_str());
}

void TextPangoWidget::insert_pixbuf(GdkPixbuf *pixbuf, const char *label, 
	const char *where_mark_name, int char_offset)
{
	flush();
	if(!goto_mark(where_mark_name, char_offset))
		return;
	gtk_text_buffer_insert_pixbuf(gtk_text_view_get_buffer(textview_), &iter_, 
		pixbuf);
}

void LabelPangoWidget::insert_pixbuf(GdkPixbuf *pixbuf, const char *label, 
	const char *where_mark_name, int char_offset)
{
	insert_pango_text(get_pixbuf_replace_pango_text(pixbuf, label).c_str(), 
		where_mark_name, char_offset);
}

void TextPangoWidget::append_widget(GtkWidget *widget)
{
	flush();
	goto_end();
	GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor (
		gtk_text_view_get_buffer(textview_), &iter_);
	gtk_text_view_add_child_at_anchor (textview_, widget, anchor);
}

void LabelPangoWidget::append_widget(GtkWidget *widget)
{
	append_pango_text(get_widget_replace_pango_text(widget).c_str());
	if (widget) {
		gtk_widget_destroy(widget);
	}
}

void TextPangoWidget::insert_widget(GtkWidget *widget, 
	const char *where_mark_name, int char_offset)
{
	flush();
	if(!goto_mark(where_mark_name, char_offset))
		return;
	GtkTextChildAnchor *anchor = gtk_text_buffer_create_child_anchor (
		gtk_text_view_get_buffer(textview_), &iter_);
	gtk_text_view_add_child_at_anchor (textview_, widget, anchor);
}

void LabelPangoWidget::insert_widget(GtkWidget *widget, 
	const char *where_mark_name, int char_offset)
{
	insert_pango_text(get_widget_replace_pango_text(widget).c_str(), 
		where_mark_name, char_offset);
	if (widget) {
		gtk_widget_destroy(widget);
	}
}

void TextPangoWidget::indent_region(const char *mark_begin, 
	int char_offset_begin, const char *mark_end, int char_offset_end)
{
	flush();
	GtkTextBuffer *buffer=gtk_text_view_get_buffer(textview_);
	GtkTextMark *tm_begin = gtk_text_buffer_get_mark(buffer, mark_begin),
		*tm_end = gtk_text_buffer_get_mark(buffer, mark_end);
	GtkTextIter tit_begin, tit_end;
	if(tm_begin == NULL) {
		g_warning("Mark \"%s\" not found", mark_begin);
		return;
	} else {
		gtk_text_buffer_get_iter_at_mark(buffer, &tit_begin, tm_begin);
		gtk_text_iter_forward_chars(&tit_begin, char_offset_begin);
	}
	if(tm_end == NULL) {
		goto_end();
		tit_end = iter_;
	} else {
		gtk_text_buffer_get_iter_at_mark(buffer, &tit_end, tm_end);
		gtk_text_iter_forward_chars(&tit_end, char_offset_end);
	}
	indent_region(&tit_begin, &tit_end);
}

/* Reindent previously selected regions. That may be required if some text was 
 * inserted after indentation. */
void TextPangoWidget::reindent(void)
{
	IndentedRegions::iterator it;
	for(it = indented_regions_.begin(); it != indented_regions_.end(); ++it) {
		it->indent();
	}
}

void PangoWidgetBase::flush(void)
{
	if (!cache_.empty()) {
		if(append_cache_)
			do_append_pango_text(cache_.c_str());
		else
			do_set_pango_text(cache_.c_str());
		cache_.clear();
	}
	append_cache_ = true;
}

// return value: true - success
bool TextPangoWidget::goto_mark(const char *where_mark_name, int char_offset)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview_);
	GtkTextMark *tm_where = gtk_text_buffer_get_mark(buffer, where_mark_name);
	if(!tm_where) {
		g_warning("Mark \"%s\" does not exist", where_mark_name);
		return false;
	}
	gtk_text_buffer_get_iter_at_mark(buffer, &iter_, tm_where);
	gtk_text_iter_forward_chars(&iter_, char_offset);
	return true;
}

TextPangoWidget::TextBufLinks::const_iterator TextPangoWidget::find_link(gint x,
									 gint y)
{
	GtkTextIter iter;
	GtkTextIter beg, end;
	gtk_text_view_get_iter_at_location(textview_, &iter, x, y);
	TextBufLinks::const_iterator it;
	for (it = tb_links_.begin(); it != tb_links_.end(); ++it) {
		it->text_iter_beg(&beg);
		it->text_iter_end(&end);
		if (gtk_text_iter_compare(&iter, &beg) < 0)
			return tb_links_.end();
		if (gtk_text_iter_compare(&beg, &iter) <= 0 
				&& gtk_text_iter_compare(&iter, &end) < 0)
			break;
	}
	return it;
}

TextPangoWidget::MarkList::iterator TextPangoWidget::find_mark(const char* mark)
{
	if(mark == NULL)
		return marklist_.end();
	MarkList::iterator it;
	for (it = marklist_.begin(); it != marklist_.end(); ++it) {
		if(strcmp(gtk_text_mark_get_name(*it), mark) == 0)
			return it;
	}
	return marklist_.end();
}

GtkTextTag* TextPangoWidget::get_indent_tag(int indent)
{
	// Why 20? Unlikely someone needs deeper indentation.
	if(indent <= 0 || indent > 20) {
		g_warning("incorrect indent %d", indent);
		return NULL;
	}
	if(indent_tags_.size() < static_cast<size_t>(indent))
		indent_tags_.resize(indent, NULL);
	if(!indent_tags_[indent-1])
		indent_tags_[indent-1] = gtk_text_buffer_create_tag (
			gtk_text_view_get_buffer(textview_), NULL,
			"left_margin", left_margin_size_pxl_ + indent_size_pxl_*indent, NULL);
	return indent_tags_[indent-1];
}

/* indent_rel - relative indentation, normally 1. */
void TextPangoWidget::indent_region(const GtkTextIter *begin, 
	const GtkTextIter *end, int indent_rel)
{
	// new region that should be indented
	GtkTextIter new_reg_beg = *begin, new_reg_end = *end;
	// (list region) already indented region from indented_regions_
	GtkTextIter lst_reg_beg, lst_reg_end;
	// part of the new region
	GtkTextIter *sub_reg_beg, *sub_reg_end;

	gtk_text_iter_order(&new_reg_beg, &new_reg_end);
	if(gtk_text_iter_compare(&new_reg_beg, &new_reg_end) == 0)
		return;
	IndentedRegions::iterator it;
	// skip list regions that precede the new region
	for(it = indented_regions_.begin(); it != indented_regions_.end(); ++it) {
		it->text_iter_end(&lst_reg_end);
		if(gtk_text_iter_compare(&new_reg_beg, &lst_reg_end) < 0)
			break;
	}
	// handle overlapping regions
	while(it != indented_regions_.end()) {
		it->text_iter_beg(&lst_reg_beg);
		it->text_iter_end(&lst_reg_end);
		// after removing a part of the text some regions may become blank,
		// remove them
		if(gtk_text_iter_compare(&lst_reg_beg, &lst_reg_end) >= 0) {
			it = indented_regions_.erase(it);
			continue;
		}
		// calculate the subregion that precedes the list region
		sub_reg_beg = gtk_text_iter_min(&new_reg_beg, &lst_reg_beg);
		sub_reg_end = gtk_text_iter_min(&new_reg_end, &lst_reg_beg);
		if(gtk_text_iter_compare(sub_reg_beg, sub_reg_end) < 0) {
			IndentedRegion ir(sub_reg_beg, sub_reg_end, indent_rel, this);
			ir.indent();
			indented_regions_.insert(it, ir);
		}
		// calculate the not overlapped region at the begin 
		sub_reg_beg = &lst_reg_beg;
		sub_reg_end = gtk_text_iter_max(&new_reg_beg, &lst_reg_beg);
		if(gtk_text_iter_compare(sub_reg_beg, sub_reg_end) < 0) {
			IndentedRegion ir(sub_reg_beg, sub_reg_end, it->indent_, this);
			ir.indent();
			indented_regions_.insert(it, ir);
		}
		// calculate the subregion that overlaps the list region
		sub_reg_beg = gtk_text_iter_max(&new_reg_beg, &lst_reg_beg);
		sub_reg_end = gtk_text_iter_min(&new_reg_end, &lst_reg_end);
		if(gtk_text_iter_compare(sub_reg_beg, sub_reg_end) < 0) {
			IndentedRegion ir(sub_reg_beg, sub_reg_end, it->indent_ + indent_rel, 
				this);
			ir.indent();
			indented_regions_.insert(it, ir);
		}
		// calculate the not overlapped region at the end 
		sub_reg_beg = gtk_text_iter_min(&new_reg_end, &lst_reg_end);
		sub_reg_end = &lst_reg_end;
		if(gtk_text_iter_compare(sub_reg_beg, sub_reg_end) < 0) {
			IndentedRegion ir(sub_reg_beg, sub_reg_end, it->indent_, this);
			ir.indent();
			indented_regions_.insert(it, ir);
		}
		/* The current region is completely covered by newly created subregions
		 * and should be removed. */
		it = indented_regions_.erase(it);
		new_reg_beg = *gtk_text_iter_min(&lst_reg_end, &new_reg_end);
		if(gtk_text_iter_compare(&new_reg_beg, &new_reg_end) >= 0)
			break;
	}
	// handle the part of the new region that follows all regions in the list
	if(gtk_text_iter_compare(&new_reg_beg, &new_reg_end) < 0) {
		IndentedRegion ir(&new_reg_beg, &new_reg_end, indent_rel, this);
		ir.indent();
		indented_regions_.push_back(ir);
	}
}

void LabelPangoWidget::flush_label(void)
{
	scroll_to(0);
	// this should speed up the next two line.
	gtk_label_set_markup(label_, "");
	// so Popup()'s gtk_widget_size_request(label,&requisition); can
	gtk_widget_set_size_request(GTK_WIDGET(label_), -1, -1);
	// get its original width.
	gtk_label_set_line_wrap(label_, FALSE);
	gtk_label_set_markup(label_, pango_text_.c_str());
}

std::string LabelPangoWidget::get_pixbuf_replace_pango_text(GdkPixbuf *pixbuf, 
	const char *label)
{
	std::string res;
	if (label) {
		gchar *markup = g_markup_printf_escaped(
			"<span foreground=\"red\">[Image:%s]</span>", label);
		res.assign(markup);
		g_free(markup);
	} else {
		res.assign("<span foreground=\"red\">[Image]</span>");
	}
	return res;
}

std::string LabelPangoWidget::get_widget_replace_pango_text(GtkWidget *widget)
{
	return "<span foreground=\"red\">[Widget]</span>";
}

/* Returns index of the char having requested offset or std::string::npos 
 * if offset is out of range.
 * Index may be in range [0, pango_text_.length()] */
size_t LabelPangoWidget::get_index_by_offset(const char *where_mark_name, 
	int char_offset)
{
	if(char_offset < 0) {
		g_warning("Negative offsets are not supported");
		return std::string::npos;
	}
	Mark *pMark = marks_.find_mark(where_mark_name);
	if(pMark == NULL) {
		g_warning("Mark \"%s\" does not exist", where_mark_name);
		return std::string::npos;
	}
	const char *beg = pango_text_.c_str();
	const char *end = xml_utf8_offset_to_pointer(beg + pMark->index_, char_offset);
	if(end == NULL) {
		size_t len = xml_utf8_strlen(beg + pMark->index_);
		g_warning("Incorrect offset %d. string length = %zu", char_offset, len);
	}
	return end ? (end - beg) : std::string::npos;
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

	//gdk_window_get_pointer(widget->window, NULL, NULL, NULL);

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

void TextPangoWidget::on_destroy(GtkWidget *object, gpointer userdata)
{
	TextPangoWidget *tpw = static_cast<TextPangoWidget *>(userdata);
	/* The reason I need to clear indented regions here is the fact that
	 * IndentedRegion and TextBufPos classes destructors requires a valid widget
	 * to operate. */
	tpw->indented_regions_.clear();
	tpw->tb_links_.clear();
}

#ifdef DEBUG
void TextPangoWidget::print_indent_regions(void)
{
	GtkTextIter lst_reg_beg, lst_reg_end;
	IndentedRegions::iterator it;
	for(it = indented_regions_.begin(); it != indented_regions_.end(); ++it) {
		it->text_iter_beg(&lst_reg_beg);
		it->text_iter_end(&lst_reg_end);
		std::cout << "\nregions offsets (" << gtk_text_iter_get_offset(&lst_reg_beg)
			<< ", " << gtk_text_iter_get_offset(&lst_reg_end) << ")"
			<< " indent: " << it->indent_ << "\n"
			<< "text: \"" << gtk_text_iter_get_text(&lst_reg_beg, &lst_reg_end)
			<< "\"\n";
		
	}
	std::cout.flush();
}

void TextPangoWidget::print_tb_links(void)
{
	std::cout << "tb_links begin\n";
	size_t num = 0;
	for(TextBufLinks::const_iterator i = tb_links_.begin(); i != tb_links_.end(); 
		++i, ++num)
		std::cout << "link " << num << "\n"
							<< "beg: " << i->beg_ << "\n"
							<< "end: " << i->end_ << "\n"
							<< "link: " << i->link_.c_str() << "\n";
	std::cout << "tb_links end\n" << std::endl;
}

void TextPangoWidget::print_marks(void)
{
	std::cout << "marks begin\n";
	for(MarkList::const_iterator it = marklist_.begin(); it != marklist_.end(); 
		++it) {
		std::cout << gtk_text_mark_get_name(*it) << ", ";
	}
	std::cout << "\nmarks end" << std::endl;
}

void LabelPangoWidget::print_string_with_marks(void)
{
	marks_.print_string_with_marks(pango_text_);
}
#endif
