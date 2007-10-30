#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>
#include "tray.hpp"
#include "eggtrayicon.h"
#include "utils.h"

class AppSkin;//forward declaration

class DockLet : public TrayBase {
public:
	DockLet(GtkWidget *, bool, const AppSkin&);
 	~DockLet();
	void minimize_to_tray();
	void set_scan_mode(bool);
private:
	EggTrayIcon *docklet_;
	GtkWidget *box_;
	GtkWidget *image_; //icon image.
	typedef  ResourceWrapper<GtkWidget, GtkWidget, gtk_widget_destroy> GMenu;
        GMenu menu_;
	GtkWidget *scan_menuitem_;
	bool embedded_;
	GdkPixbuf *normal_icon_, *stop_icon_, *scan_icon_;

	static void on_embedded(GtkWidget *widget, gpointer data);
	static void on_destroyed(GtkWidget *widget, DockLet *oDockLet);
	static gboolean on_btn_press(GtkWidget *, GdkEventButton *, DockLet *);
	static void on_menu_scan(GtkCheckMenuItem *, gpointer);
	static void on_menu_quit(GtkMenuItem *, gpointer);	
	static gboolean on_docklet_create(gpointer data);

	void popup_menu(GdkEventButton *event);
	void create_docklet();
	void scan_on();
	void scan_off();
        void show_normal_icon();
};


#endif
