#ifndef _PROGRESSWIN_HPP_
#define _PROGRESSWIN_HPP_

#include <string>

class progress_win {
public:
	explicit progress_win(GtkWindow *parent_win);
	~progress_win();
	void display_action(const std::string& actname);
private:
	static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , progress_win *oWin);
	static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , progress_win *oWin);
	GtkLabel *text;
	GtkProgressBar *progress;
	GtkWidget *win;
	gint press_x_root, press_y_root, press_window_x, press_window_y;
};

#endif//!_PROGRESSWIN_HPP_
