#ifndef __SD_DOCKLET_H__
#define __SD_DOCKLET_H__

#include <gtk/gtk.h>
#include "tray.hpp"
#include "eggtrayicon.h"

enum DockLetIconType {
	DOCKLET_NORMAL_ICON,
	DOCKLET_SCAN_ICON,
	DOCKLET_STOP_ICON,
};

class DockLet : public TrayBase {
private:
	EggTrayIcon *docklet;
	GtkWidget *box;
	GtkWidget *image; //icon image.
	GtkWidget *menu,*scan_menuitem;
	DockLetIconType current_icon;
	gboolean embedded;

	static void EmbeddedCallback(GtkWidget *widget, gpointer data);
	static void DestroyedCallback(GtkWidget *widget, DockLet *oDockLet);
	static gboolean ButtonPressCallback(GtkWidget *button, GdkEventButton *event, DockLet *oDockLet);	

	static void MenuScanCallback(GtkCheckMenuItem *checkmenuitem, gpointer user_data);
	static void MenuQuitCallback(GtkMenuItem *menuitem, gpointer user_data);
	
	static gboolean docklet_create(gpointer data);

	void PopupMenu(GdkEventButton *event);
public:	


	DockLet(GtkWidget *mainwin);
	void Create(DockLetIconType iconType = DOCKLET_NORMAL_ICON);
	void End();
	void SetIcon(DockLetIconType icon_type);
	void minimize_to_tray();
};


#endif
