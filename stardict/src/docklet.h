#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>
#include "tray.hpp"

class AppSkin;

class DockLet : public TrayBase {
public:
	DockLet(GtkWidget *mainwin, bool is_scan_on, const AppSkin& skin);
	virtual ~DockLet();
	void minimize_to_tray();
private:
	GdkPixbuf *normal_icon_, *stop_icon_, *scan_icon_;
	GtkStatusIcon *docklet_;
	GtkWidget *menu_;
	GtkWidget *scan_menuitem_;

	void create_docklet(void);
	void create_popup_menu(void);

	static void on_menu_scan(GtkCheckMenuItem *, gpointer);
	static void on_menu_quit(GtkMenuItem *, gpointer);
	static void on_popup_menu(GtkStatusIcon *status_icon,
		guint          button,
		guint          activate_time,
		gpointer       user_data);
	static gboolean on_button_press(GtkStatusIcon *status_icon,
		GdkEventButton      *event,
		gpointer       user_data);

protected:
	void scan_on();
	void scan_off();
	void show_normal_icon();
};


#endif
