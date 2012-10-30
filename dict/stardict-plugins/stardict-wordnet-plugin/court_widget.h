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

#ifndef _STARDICT_COURT_WIDGET_H_
#define _STARDICT_COURT_WIDGET_H_

#include <gtk/gtk.h>
#include <string>
#include <vector>

#include "partic.h"
#include "newton.h"
#include "newton_env.h"
#include "scene.h"

struct BallColor {
	double red;
	double green;
	double blue;
};

class wnobj {
public:
	enum ball_type {
		et_unknown = 0,

		et_ball = 0x01000000,
		et_word = 0x02000000,

		et_normal = 0x00000001,
		et_adj = 0x00000002,
		et_noun = 0x00000004,
		et_verb = 0x00000008,

		et_fixed = 0x00000100,
		et_center = 0x00000200,
	};
	wnobj(partic_t & p, unsigned int t=et_normal);
	virtual ~wnobj() {}
	partic_t & getP() const { return _p; }
	unsigned int getT() const { return _t; }
	void set_center();
	void set_anchor(bool b);
	void set_highlight(bool b);
	virtual void draw(cairo_t *cr, gdouble alpha) = 0;
	virtual const char *get_text() = 0;
	static void draw_spring(cairo_t *cr, const spring_t & s, gdouble alpha);
protected:
	partic_t & _p;
	unsigned int _t;
	bool highlight;
	static void draw_ball(cairo_t *cr, double x, double y, BallColor &color, gdouble alpha, bool highlight);
	static void draw_line(cairo_t *cr, double x1, double y1, double x2, double y2, gdouble alpha);
	static void draw_text(cairo_t *cr, double x, double y, double w, double h, PangoLayout * layout, gdouble alpha, bool highlight);
};

class ball_t: public wnobj {
public:
	ball_t(partic_t & p, const char *text_, const char *type_);
	virtual ~ball_t() {}
	void draw(cairo_t *cr, gdouble alpha);
	const char *get_text();
	const char *get_type_str();
private:
	std::string text;
	std::string type;
	BallColor color;
};

class word_t: public wnobj {
public:
	word_t(partic_t & p, PangoLayout *layout): wnobj(p, et_word | et_normal), _layout(layout) {}
	virtual ~word_t();
	void draw(cairo_t *cr, gdouble alpha);
	const char *get_text();
private:
	PangoLayout * _layout;
};

class wncourt_t {
public:
	wncourt_t();
	~wncourt_t();
	word_t *create_word(PangoLayout *layout);
	ball_t *create_ball(const char *text, const char *type);
	void create_spring(const wnobj * w, const wnobj * b, float springlength, float coeff = 0.4f);
	void set_center(wnobj * cb);
	void clear();
	void update(float t) { _newton.update(t); }
	bool hit(int x, int y, wnobj ** b);
	scene_t & get_scene() { return _scene; }
	newton_env_t & get_env() { return _env; }
	std::vector<wnobj *> & get_wnobjs() { return _wnobjs; }
	unsigned char get_alpha() { return _alpha; }
	void updte_alpha(char d);
	bool need_draw() { return _newton.is_stat_changed(); }
private:
	newton_env_t _env;
	scene_t _scene;
	newton_t _newton;
	std::vector<wnobj *> _wnobjs;
	wnobj * the_center;
	unsigned char _alpha;
};

class WnCourt {
public:
	typedef void (*lookup_dict_func_t)(size_t dictid, const char *word, char ****Word, char *****WordData);
	typedef void (*FreeResultData_func_t)(size_t dictmask_size, char ***pppWord, char ****ppppWordData);
	typedef void (*ShowPangoTips_func_t)(const char *word, const char *text);
	WnCourt(size_t dictid, lookup_dict_func_t lookup_dict_, FreeResultData_func_t FreeResultData_, ShowPangoTips_func_t ShowPangoTips_, gint *widget_width_, gint *widget_height_);
	~WnCourt();
	GtkWidget *get_widget();
	void set_word(const gchar *orig_word, gchar **Word = NULL, gchar ***WordData = NULL);
private:
	size_t _dictid;
	lookup_dict_func_t lookup_dict;
	FreeResultData_func_t FreeResultData;
	ShowPangoTips_func_t ShowPangoTips;
	std::string CurrentWord;
	GtkWidget *drawing_area;
	gint *global_widget_width, *global_widget_height;
	gint widget_width, widget_height;
	int timeout;
	wnobj * newobj;
	wncourt_t * _court;
	wncourt_t * _secourt;
	std::vector<wnobj *> _wnstack;
	unsigned char _init_angle;
	int init_spring_length;
	int oldX, oldY;
	bool resizing;
	bool panning;
	wnobj * dragball;
	wnobj * overball;
#if GTK_MAJOR_VERSION >= 3
	static gboolean on_draw_callback (GtkWidget *widget, cairo_t *cr, WnCourt *wncourt);
#else
	static gboolean expose_event_callback (GtkWidget *widget, GdkEventExpose *event, WnCourt *wncourt);
#endif
	static void on_destroy_callback (GtkWidget *object, WnCourt *wncourt);
	static void on_realize_callback(GtkWidget *widget, WnCourt *wncourt);
	static gboolean on_button_press_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt);
	static gboolean on_button_release_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt);
	static gboolean on_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event , WnCourt *wncourt);
	static gint do_render_scene(gpointer data);
	void ClearScene();
	void CenterScene();
	void CreateWord(const char *text);
	void CreateNode(const char *text, const char *type);
	void Push();
	void Pop();
	wnobj *get_top();
	vector_t get_center_pos();
	vector_t get_next_pos(vector_t & center);
	void draw_wnobjs(cairo_t *cr, wncourt_t *court);
	void draw_dragbar(cairo_t *cr);
	bool need_draw();
};

#endif
