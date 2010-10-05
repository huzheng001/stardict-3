#ifndef __SD_SPLASH_H__
#define __SD_SPLASH_H__

#include <string>

#include <gtk/gtk.h>

class splash_screen {
public:
	splash_screen();
	void display_action(const std::string& actname);
	void show();
private:
	static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , splash_screen *oWin);
	static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , splash_screen *oWin);
	GtkWidget *window;
	GtkLabel *text;
	GtkProgressBar *progress;
	gint press_x_root, press_y_root, press_window_x, press_window_y;
};

extern splash_screen stardict_splash;

#endif
