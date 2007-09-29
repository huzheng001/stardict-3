#ifndef _STARDICT_COURT_WIDGET_H_
#define _STARDICT_COURT_WIDGET_H_

#include <gtk/gtk.h>
#include <string>
#include <vector>

#include "partic.hpp"
#include "newton.hpp"
#include "newton_env.hpp"
#include "scene.hpp"

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
	virtual void draw(cairo_t *cr) = 0;
	virtual const char *get_text() = 0;
	static void draw_spring(cairo_t *cr, const spring_t & s);
protected:
	partic_t & _p;
	unsigned int _t;
	static void draw_ball(cairo_t *cr, double x, double y);
	static void draw_line(cairo_t *cr, double x1, double y1, double x2, double y2);
	static void draw_text(cairo_t *cr, double x, double y, double w, double h, PangoLayout * layout);
};

class ball_t: public wnobj {
public:
	ball_t(partic_t & p, const char *text_): wnobj(p, et_ball | et_normal), text(text_) {}
	virtual ~ball_t() {}
	void draw(cairo_t *cr);
	const char *get_text();
private:
	std::string text;
};

class word_t: public wnobj {
public:
	word_t(partic_t & p, PangoLayout *layout): wnobj(p, et_word | et_normal), _layout(layout) {}
	virtual ~word_t();
	void draw(cairo_t *cr);
	const char *get_text();
private:
	PangoLayout * _layout;
};

class wncourt_t {
public:
	wncourt_t();
	~wncourt_t();
	word_t *create_word(PangoLayout *layout);
	ball_t *create_ball(const char *text);
	void create_spring(const wnobj * w, const wnobj * b, float springlength, float coeff = 0.4f);
	void set_center(wnobj * cb);
	void clear();
	void update(float t) { _newton.update(t); }
	bool hit(int x, int y, wnobj ** b);
	scene_t & get_scene() { return _scene; }
	newton_env_t & get_env() { return _env; }
	std::vector<wnobj *> & get_wnobjs() { return _wnobjs; }
private:
	newton_env_t _env;
	scene_t _scene;
	newton_t _newton;
	std::vector<wnobj *> _wnobjs;
	wnobj * the_center;
};

class WnCourt {
public:
	typedef void (*lookup_dict_func_t)(size_t dictid, const char *word, char ****Word, char *****WordData);
	typedef void (*FreeResultData_func_t)(size_t dictmask_size, char ***pppWord, char ****ppppWordData);
	WnCourt(size_t dictid, lookup_dict_func_t lookup_dict_, FreeResultData_func_t FreeResultData_);
	~WnCourt();
	GtkWidget *get_widget();
	void set_word(const gchar *orig_word, gchar **Word = NULL, gchar ***WordData = NULL);
private:
	size_t _dictid;
	lookup_dict_func_t lookup_dict;
	FreeResultData_func_t FreeResultData;
	GtkWidget *drawing_area;
	int timeout;
	wnobj * newobj;
	wncourt_t * _court;
	std::vector<wnobj *> _wnstack;
	char _init_angle;
	int init_spring_length;
	int widget_width, widget_height;
	int oldX, oldY;
	bool panning;
	wnobj * dragball;
	static gboolean expose_event_callback (GtkWidget *widget, GdkEventExpose *event, WnCourt *wncourt);
	static void on_destroy_callback (GtkObject *object, WnCourt *wncourt);
	static void on_realize_callback(GtkWidget *widget, WnCourt *wncourt);
	static gboolean on_button_press_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt);
	static gboolean on_button_release_event_callback(GtkWidget * widget, GdkEventButton *event, WnCourt *wncourt);
	static gboolean on_motion_notify_event_callback(GtkWidget * widget, GdkEventMotion * event , WnCourt *wncourt);
	static gint do_render_scene(gpointer data);
	void ClearScene();
	void CreateWord(const char *text);
	void CreateNode(const char *text);
	void Push();
	void Pop();
	wnobj *get_top();
	vector_t get_center_pos();
	vector_t get_next_pos(vector_t & center);
	void draw_wnobjs(cairo_t *cr);
	void render_scene();
};

#endif
