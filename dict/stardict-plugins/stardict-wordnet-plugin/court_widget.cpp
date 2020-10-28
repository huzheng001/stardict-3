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

#include "court_widget.h"
#include <math.h>
#include <list>
#include <cstring>


wnobj::wnobj(partic_t & p, unsigned int t) : _p(p), _t(t), highlight(false)
{
}

void wnobj::set_anchor(bool b)
{
	if (_t & et_center)
		return;
	_p.set_anchor(b);
}

void wnobj::set_highlight(bool b)
{
	highlight = b;
}

void wnobj::set_center()
{
	_t = _t | et_center;
	_p.set_anchor(true);
}

void wnobj::draw_ball(cairo_t *cr, double x, double y, BallColor &color, gdouble alpha, bool highlight)
{
	const double r = 5;
	cairo_save(cr);
	cairo_arc(cr, x + r/3, y + r/3, r, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, alpha);
	cairo_fill(cr);
	cairo_arc(cr, x, y, r, 0, 2 * M_PI);
	if (highlight) {
		cairo_set_source_rgb(cr, 1, 1, 0);
	} else {
		cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);
	}
	cairo_fill(cr);
	cairo_arc(cr, x - r/3, y - r/3, r/3, 0, 2 * M_PI);
	cairo_set_source_rgba(cr, 1, 1, 1, 0.8*alpha);
	cairo_fill(cr);
	cairo_restore(cr);
}

void wnobj::draw_line(cairo_t *cr, double x1, double y1, double x2, double y2, gdouble alpha)
{
	cairo_save(cr);
	cairo_set_source_rgba(cr, 0, 0, 0, alpha);
	cairo_move_to(cr, x1, y1);
	cairo_line_to(cr, x2, y2);
	cairo_stroke(cr);
	cairo_restore(cr);
}

void wnobj::draw_text(cairo_t *cr, double x, double y, double w, double h, PangoLayout * layout, gdouble alpha, bool highlight)
{
	cairo_save(cr);
	cairo_set_source_rgba(cr, 1, 1, 1, alpha);
	cairo_rectangle(cr, x, y, w, h);
	cairo_fill(cr);
	cairo_move_to(cr, x, y);
	if (highlight) {
		cairo_set_source_rgb(cr, 0, 0, 1);
	} else {
		cairo_set_source_rgba(cr, 0, 0, 0, alpha);
	}
	pango_cairo_show_layout(cr, layout);
	cairo_restore(cr);
}

void wnobj::draw_spring(cairo_t *cr, const spring_t & s, gdouble alpha)
{
	vector_t &a = s.getA().getP();
	vector_t &b = s.getB().getP();
	draw_line(cr, a.x, a.y, b.x, b.y, alpha);
}

ball_t::ball_t(partic_t & p, const char *text_, const char *type_): wnobj(p, et_ball | et_normal), text(text_), type(type_)
{
	if (strcmp(type_, "n")==0) {
		color.red = 0;
		color.green = 0;
		color.blue = 1;
	} else if (strcmp(type_, "v")==0) {
		color.red = 1;
		color.green = 0.5;
		color.blue = 0.25;
	} else if (strcmp(type_, "a")==0) {
		color.red = 0;
		color.green = 0;
		color.blue = 0.5;
	} else if (strcmp(type_, "s")==0) {
		color.red = 1;
		color.green = 0.25;
		color.blue = 0;
	} else if (strcmp(type_, "r")==0) {
		color.red = 0.8;
		color.green = 0.8;
		color.blue = 0;
	} else {
		color.red = 0;
		color.green = 0;
		color.blue = 0;
	}
}

void ball_t::draw(cairo_t *cr, gdouble alpha)
{
	vector_t v = getP().getP();
	draw_ball(cr, v.x, v.y, color, alpha, highlight);
}

const char *ball_t::get_text()
{
	return text.c_str();
}

const char *ball_t::get_type_str()
{
	if (type == "n")
		return "Noun";
	else if (type == "v")
		return "Verb";
	else if (type == "a")
		return "Adjective";
	else if (type == "s")
		return "Adjective satellite";
	else if (type == "r")
		return "Adverb";
	else
		return type.c_str();
}

word_t::~word_t()
{
	g_object_unref(_layout);
}

void word_t::draw(cairo_t *cr, gdouble alpha)
{
	point_t<single> left_top = _p.get_left_top();
	tsize_t<single> & size = _p.get_size();
	draw_text(cr, left_top.x, left_top.y, size.w, size.h, _layout, alpha, highlight);
}

const char *word_t::get_text()
{
	return pango_layout_get_text(_layout);
}

wncourt_t::wncourt_t() : _env(), _scene(), _newton(_scene, _env), _alpha(255)
{
}

wncourt_t::~wncourt_t()
{
	clear();
}

word_t * wncourt_t::create_word(PangoLayout *layout)
{
	int w, h;
	pango_layout_get_pixel_size(layout, &w, &h);
	partic_t * p = _scene.create_partic(10, w, h);
	word_t * b = new word_t(*p, layout);
	_wnobjs.push_back(b);
	return b;
}

ball_t * wncourt_t::create_ball(const char *text, const char *type)
{
	partic_t * p = _scene.create_partic(10, 10, 10);
	ball_t * b = new ball_t(*p, text, type);
	_wnobjs.push_back(b);
	return b;
}

void wncourt_t::create_spring(const wnobj * w, const wnobj * b, float springlength, float coeff)
{
	_scene.create_spring(w->getP(), b->getP(), springlength, coeff);
}

void wncourt_t::set_center(wnobj * cb)
{
	the_center = cb;
	the_center->set_center();
	_scene.set_center(&cb->getP());
}

void wncourt_t::clear()
{
	for (std::vector<wnobj *>::const_iterator it = _wnobjs.begin();it != _wnobjs.end(); ++it) {
		delete *it;
	}
	_wnobjs.clear();
	_scene.clear();
}

bool wncourt_t::hit(int x, int y, wnobj ** b)
{
	vector_t p((single)x, (single)y, 0);
	for(std::vector<wnobj *>::iterator it = _wnobjs.begin(); it != _wnobjs.end(); ++it) {
		if ((*it)->getP().hit(p)) {
			*b = *it;
			return true;
		}
	}
	*b = NULL;
	return false;
}

void wncourt_t::updte_alpha(char d)
{
	_alpha = _alpha - d < 0 ? 0 : _alpha - d;
}

#if GTK_MAJOR_VERSION >= 3
gboolean WnCourt::on_draw_callback (GtkWidget *widget, cairo_t *cr, WnCourt *wncourt)
#else
gboolean WnCourt::expose_event_callback (GtkWidget *widget, GdkEventExpose *event, WnCourt *wncourt)
#endif
{
#if GTK_MAJOR_VERSION >= 3
#else
	cairo_t *cr = gdk_cairo_create(gtk_widget_get_window(widget));
#endif
	if (wncourt->_secourt && wncourt->_secourt->get_alpha() != 0) {
		wncourt->_secourt->updte_alpha(16);
		if (wncourt->_secourt->get_alpha() != 0) {
			wncourt->draw_wnobjs(cr, wncourt->_secourt);
		}
	}
	wncourt->draw_wnobjs(cr, wncourt->_court);
	wncourt->draw_dragbar(cr);
#if GTK_MAJOR_VERSION >= 3
#else
	cairo_destroy(cr);
#endif
	return TRUE;
}

void WnCourt::draw_wnobjs(cairo_t *cr, wncourt_t *court)
{
	cairo_set_line_width(cr, 1);
	gdouble alpha = court->get_alpha()/(255.0f);
	vector<spring_t *> & springs = court->get_scene().get_springs();
	for(vector<spring_t *>::iterator it = springs.begin(); it != springs.end(); ++it) {
		wnobj::draw_spring(cr, **it, alpha);
	}
	std::vector<wnobj *> & partics = court->get_wnobjs();
	for(std::vector<wnobj *>::iterator it = partics.begin(); it != partics.end(); ++it) {
		(*it)->draw(cr, alpha);
	}
}

void WnCourt::draw_dragbar(cairo_t *cr)
{
	cairo_move_to(cr, widget_width -15, widget_height);
	cairo_line_to(cr, widget_width, widget_height - 15);
	cairo_line_to(cr, widget_width, widget_height);
	cairo_line_to(cr, widget_width -15, widget_height);
	cairo_set_source_rgba(cr, 0, 0, 1, 0.8);
	cairo_fill(cr);
}

void WnCourt::on_destroy_callback (GtkWidget *object, WnCourt *wncourt)
{
	delete wncourt;
}

void WnCourt::on_realize_callback(GtkWidget *widget, WnCourt *wncourt)
{
	GdkCursor* cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_LEFT_PTR);
	gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
#if GTK_MAJOR_VERSION >= 3
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
}

gboolean WnCourt::on_button_press_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt)
{
	if (wncourt->timeout == 0) {
		wncourt->timeout = g_timeout_add(int(1000/16), do_render_scene, wncourt);
	}
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 1) {
			wnobj * b;
			if (event->x > wncourt->widget_width - 15 && event->y > wncourt->widget_height - 15) {
				wncourt->resizing = true;
				GdkCursor* cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_SIZING);
				gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
#if GTK_MAJOR_VERSION >= 3
				g_object_unref(cursor);
#else
				gdk_cursor_unref(cursor);
#endif
			} else if (wncourt->_court->hit((int)(event->x), (int)(event->y), &b)) {
				wncourt->dragball = b;
				wncourt->dragball->set_anchor(true);
			} else {
				wncourt->panning = true;
			}
			wncourt->oldX = (int)(event->x);
			wncourt->oldY = (int)(event->y);
		} else if (event->button == 2) {
			return FALSE;
		}
	} else if (event->type == GDK_2BUTTON_PRESS) {
		if (event->button == 1) {
			wnobj * b;
			if (wncourt->_court->hit((int)(event->x), (int)(event->y), &b)) {
				if (b->getT() & wnobj::et_word) {
					char *sWord = g_strdup(b->get_text());
					char ***Word;
					char ****WordData;
					wncourt->lookup_dict(wncourt->_dictid, sWord, &Word, &WordData);
					wncourt->set_word(sWord, Word[0], WordData[0]);
					wncourt->FreeResultData(1, Word, WordData);
					g_free(sWord);
				}
			} else {
				wncourt->CenterScene();
			}
		}
	}
	return TRUE;
}

gboolean WnCourt::on_button_release_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt)
{
	if (event->button == 1) {
		if (wncourt->dragball) {
			wncourt->dragball->set_anchor(false);
			wncourt->_court->get_env().reset();
			wncourt->dragball = NULL;
		}
		if (wncourt->resizing) {
			GdkCursor* cursor = gdk_cursor_new_for_display(gdk_display_get_default(), GDK_LEFT_PTR);
			gdk_window_set_cursor(gtk_widget_get_window(widget), cursor);
#if GTK_MAJOR_VERSION >= 3
			g_object_unref(cursor);
#else
			gdk_cursor_unref(cursor);
#endif
			wncourt->resizing = false;
		}
		wncourt->panning = false;
	} else if (event->button == 2) {
		return FALSE;
	}
	return TRUE;
}

gboolean WnCourt::on_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event , WnCourt *wncourt)
{
	if (event->state & GDK_BUTTON1_MASK) {
		if (wncourt->dragball) {
			vector_t dv((single)(event->x - wncourt->oldX), (single)(event->y - wncourt->oldY), 0);
			wncourt->dragball->getP().getP().add(dv);
			if (wncourt->overball) {
				wncourt->overball->set_highlight(false);
				wncourt->overball = NULL;
			}
		} else if (wncourt->resizing) {
			wncourt->widget_width = (gint)event->x;
			wncourt->widget_height = (gint)event->y;
			if (wncourt->widget_width < 20)
				wncourt->widget_width = 20;
			if (wncourt->widget_height < 20)
				wncourt->widget_height = 20;
			wncourt->CenterScene();
			gtk_widget_set_size_request (wncourt->drawing_area, wncourt->widget_width, wncourt->widget_height);
		} else if (wncourt->panning) {
			wncourt->_court->get_scene().pan(vector_t((single)(event->x - wncourt->oldX), (single)(event->y - wncourt->oldY), 0));
		}
		wncourt->oldX = (int)(event->x);
		wncourt->oldY = (int)(event->y);
	} else {
		wnobj * b;
		if (wncourt->_court->hit((int)event->x, (int)event->y, &b)) {
			if (wncourt->overball != b) {
				wncourt->overball = b;
				wncourt->overball->set_anchor(true);
				wncourt->overball->set_highlight(true);
				gtk_widget_queue_draw(wncourt->drawing_area);
				if (wncourt->overball->getT() & wnobj::et_ball) {
					ball_t *ball = static_cast<ball_t *>(wncourt->overball);
					char *text = g_markup_printf_escaped("<i>%s</i>\n%s", ball->get_type_str(), ball->get_text());
					wncourt->ShowPangoTips(wncourt->CurrentWord.c_str(), text);
					g_free(text);
				}
			}
		} else {
			if (wncourt->overball) {
				wncourt->overball->set_anchor(false);
				wncourt->overball->set_highlight(false);
				wncourt->overball = NULL;
			}
		}
	}
	return TRUE;
}

gint WnCourt::do_render_scene(gpointer data)
{
	WnCourt *wncourt = static_cast<WnCourt *>(data);
	wncourt->_court->update(1.0f);
	if (wncourt->need_draw()) {
		gtk_widget_queue_draw(wncourt->drawing_area);
		return TRUE;
	} else {
		wncourt->timeout = 0;
		return FALSE;
	}
}

bool WnCourt::need_draw()
{
	return (_secourt && _secourt->get_alpha() != 0) ||
			dragball || panning || _court->need_draw();
}

WnCourt::WnCourt(size_t dictid, lookup_dict_func_t lookup_dict_, FreeResultData_func_t FreeResultData_, ShowPangoTips_func_t ShowPangoTips_, gint *widget_width_, gint *widget_height_) : _dictid(dictid), lookup_dict(lookup_dict_), FreeResultData(FreeResultData_), ShowPangoTips(ShowPangoTips_), global_widget_width(widget_width_), global_widget_height(widget_height_), _secourt(NULL), _init_angle(0), init_spring_length(81), resizing(false), panning(false), dragball(NULL), overball(NULL)
{
	_court = new wncourt_t();
	widget_width = *widget_width_;
	widget_height = *widget_height_;

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request (drawing_area, widget_width, widget_height);
	gtk_widget_add_events(drawing_area, GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK | GDK_POINTER_MOTION_MASK);
#if GTK_MAJOR_VERSION >= 3
/*
	GdkRGBA color;
	color.red = 1;
	color.green = 1;
	color.blue = 1;
	color.alpha = 1;
	gtk_widget_override_background_color(drawing_area, GTK_STATE_FLAG_NORMAL, &color);
*/
#else
	GdkColor color;
	color.red = 65535;
	color.green = 65535;
	color.blue = 65535;
	gtk_widget_modify_bg(drawing_area, GTK_STATE_NORMAL, &color);
#endif
#if GTK_MAJOR_VERSION >= 3
	g_signal_connect (G_OBJECT (drawing_area), "draw", G_CALLBACK (on_draw_callback), this);
#else
	g_signal_connect (G_OBJECT (drawing_area), "expose_event", G_CALLBACK (expose_event_callback), this);
#endif
	g_signal_connect (G_OBJECT (drawing_area), "destroy", G_CALLBACK (on_destroy_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "realize", G_CALLBACK (on_realize_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "button_press_event", G_CALLBACK (on_button_press_event_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "button_release_event", G_CALLBACK (on_button_release_event_callback), this);
	g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event", G_CALLBACK (on_motion_notify_event_callback), this);
	gtk_widget_show(drawing_area);
	timeout = g_timeout_add(int(1000/16), do_render_scene, this);
}

WnCourt::~WnCourt()
{
	if (timeout)
		g_source_remove(timeout);
	delete _court;
	delete _secourt;
	*global_widget_width = widget_width;
	*global_widget_height = widget_height;
}

struct WnUserData {
	WnUserData(const char *oword_, std::string &type_, std::list<std::string> &wordlist_, std::string &gloss_): oword(oword_), type(type_), wordlist(wordlist_), gloss(gloss_) {}
	const gchar *oword;
	std::string &type;
	std::list<std::string> &wordlist;
	std::string &gloss;
};

static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	WnUserData *Data = (WnUserData *)user_data;
	if (strcmp(element, "type")==0) {
		Data->type.assign(text, text_len);
	} else if (strcmp(element, "word")==0) {
		std::string word(text, text_len);
		if (word != Data->oword) {
			Data->wordlist.push_back(word);
		}
	} else if (strcmp(element, "gloss")==0) {
		Data->gloss.assign(text, text_len);
	}
}

static void wordnet2result(const gchar *Word, gchar *WordData, std::string &type, std::list<std::string> &wordlist, std::string &gloss)
{
	guint32 data_size = *reinterpret_cast<guint32 *>(WordData);
	type.clear();
	wordlist.clear();
	gloss.clear();
	WnUserData Data(Word, type, wordlist, gloss);
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, WordData + sizeof(guint32) + sizeof(char), data_size - 2, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

void WnCourt::set_word(const gchar *orig_word, gchar **Word, gchar ***WordData)
{
	ClearScene();
	CurrentWord = orig_word;
	CreateWord(orig_word);
	if (Word == NULL)
		return;
	Push();
	std::string type;
	std::list<std::string> wordlist;
	std::string gloss;
	size_t i = 0;
	size_t j;
	do {
		j = 0;
		do {
			wordnet2result(orig_word, WordData[i][j], type, wordlist, gloss);
			CreateNode(gloss.c_str(), type.c_str());
			Push();
			for (std::list<std::string>::iterator it = wordlist.begin(); it != wordlist.end(); ++it) {
				CreateWord(it->c_str());
			}
			Pop();
			j++;
		} while (WordData[i][j]);
		i++;
	} while (Word[i]);
}

GtkWidget *WnCourt::get_widget()
{
	return drawing_area;
}

void WnCourt::ClearScene()
{
	if (_secourt)
		delete _secourt;
	_secourt = _court;
	_court = new wncourt_t();
	dragball=NULL;
	overball=NULL;
	_wnstack.clear();
}

void WnCourt::CenterScene()
{
	if (!_court->get_scene().get_center())
		return;
	_court->get_scene().center_to(vector_t(widget_width/2, widget_height/2, 0));
}

void WnCourt::CreateWord(const char *text)
{
	wnobj *top = get_top();
	if (top) {
		PangoLayout *layout = gtk_widget_create_pango_layout(drawing_area, text);
		newobj = _court->create_word(layout);
		_court->create_spring(newobj, top, init_spring_length);
		newobj->getP().getP() = get_next_pos(get_top()->getP().getP());
	} else {
		PangoLayout *layout = gtk_widget_create_pango_layout(drawing_area, "");
		gchar *str = g_markup_printf_escaped("<big><b>%s</b></big>", text);
		pango_layout_set_markup(layout, str, -1);
		g_free(str);
		newobj = _court->create_word(layout);
		newobj->getP().getP() = get_center_pos();
		_court->set_center(newobj);
	}
}

void WnCourt::CreateNode(const char *text, const char *type)
{
	newobj = _court->create_ball(text, type);
	wnobj *top = get_top();
	if (top) {
		_court->create_spring(newobj, top, init_spring_length, 0.4f);
		newobj->getP().getP() = get_next_pos(top->getP().getP());
	} else {
		newobj->getP().getP() = get_center_pos();
	}
}

vector_t WnCourt::get_center_pos()
{
	return vector_t(widget_width/2, widget_height/2, 0);
}

vector_t WnCourt::get_next_pos(vector_t &center)
{
	vector_t d(init_spring_length, 0, 0);
	d.rot(M_PI_10*_init_angle++);
	return center+d;
}

void WnCourt::Push()
{
	_wnstack.push_back(newobj);
}

void WnCourt::Pop()
{
	newobj = get_top();
	_wnstack.pop_back();
}

wnobj *WnCourt::get_top()
{
	if (_wnstack.empty())
		return NULL;
	else
		return _wnstack.back();
}
