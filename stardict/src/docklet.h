#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>
#ifdef _WIN32
#include <windows.h>
#endif
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
#ifdef _WIN32
	HWND systray_hwnd;
	HMENU systray_menu; // gtk menu don't work fine here.
#else
	GtkWidget *menu_;
	GtkWidget *scan_menuitem_;
#endif

	void create_docklet(void);
	void create_popup_menu(void);

#ifdef _WIN32
	HWND create_hiddenwin();
	static LRESULT CALLBACK mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#else
	static void on_menu_scan(GtkCheckMenuItem *, gpointer);
	static void on_menu_quit(GtkMenuItem *, gpointer);
#endif
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
