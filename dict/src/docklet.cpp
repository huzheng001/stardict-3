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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "skin.h"
#include "lib/utils.h"
#ifdef _WIN32
#include "conf.h"
#endif
#include "docklet.h"

#ifdef _WIN32
enum SYSTRAY_CMND {
	SYSTRAY_CMND_MENU_QUIT=100,
	SYSTRAY_CMND_MENU_SCAN,
};
#endif

DockLet::DockLet(GtkWidget *mainwin, bool is_scan_on, const AppSkin& skin):
	TrayBase(mainwin, is_scan_on),
	normal_icon_(NULL),
	stop_icon_(NULL),
	scan_icon_(NULL),
	docklet_(NULL),
#ifdef _WIN32
	systray_hwnd(NULL),
	systray_menu(NULL)
#else
	menu_(NULL),
	scan_menuitem_(NULL)
#endif
{
	normal_icon_ = get_impl(skin.docklet_normal_icon);
	scan_icon_ = get_impl(skin.docklet_scan_icon);
	stop_icon_ = get_impl(skin.docklet_stop_icon);

	create_docklet();
}

DockLet::~DockLet() {
	while (g_source_remove_by_user_data(&docklet_))
		;
#ifdef _WIN32
	if(systray_menu)
		DestroyMenu(systray_menu);
	if(systray_hwnd)
		DestroyWindow(systray_hwnd);
#else
	if (menu_)
		gtk_widget_destroy(menu_);
#endif
	g_object_unref(G_OBJECT(docklet_));
}

void DockLet::minimize_to_tray()
{
	if (gtk_status_icon_is_embedded(docklet_))
		TrayBase::minimize_to_tray();
	else
		on_quit_.emit();
}

void DockLet::create_docklet(void)
{
	docklet_ = gtk_status_icon_new();
	gtk_status_icon_set_title(docklet_, _("StarDict"));
	if (is_hide_state()) {
		show_normal_icon();
	} else if (is_scan_on()) {
		scan_on();
	} else {
		scan_off();
	}

	g_signal_connect(G_OBJECT(docklet_), "popup-menu",
			 G_CALLBACK(on_popup_menu), this);
	g_signal_connect(G_OBJECT(docklet_), "button-press-event",
			 G_CALLBACK(on_button_press), this);

#ifdef _WIN32
	systray_hwnd = create_hiddenwin();
	::SetWindowLongPtr(systray_hwnd, GWL_USERDATA,
		reinterpret_cast<LONG_PTR>(this));
#endif
	gtk_status_icon_set_visible(docklet_, TRUE);
}

void DockLet::create_popup_menu(void)
{
#ifdef _WIN32
	if(!systray_menu) {
		systray_menu = CreatePopupMenu();
		if(!systray_menu)
			return;
		std_win_string menu_item_win;
		if(!utf8_to_windows(_("Scan"), menu_item_win))
			return ;
		AppendMenu(systray_menu, MF_CHECKED, SYSTRAY_CMND_MENU_SCAN,
			menu_item_win.c_str());
		AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
		if(!utf8_to_windows(_("Quit"), menu_item_win))
			return ;
		AppendMenu(systray_menu, MF_STRING, SYSTRAY_CMND_MENU_QUIT,
			menu_item_win.c_str());
	}
	if (is_scan_on())
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_UNCHECKED);
	/* need to call this so that the menu disappears if clicking outside
	of the menu scope */
	SetForegroundWindow(systray_hwnd);
#else
	if(!menu_) {
		menu_ = gtk_menu_new();

		scan_menuitem_ = gtk_check_menu_item_new_with_mnemonic(_("_Scan"));
		g_signal_connect(G_OBJECT(scan_menuitem_), "toggled",
			G_CALLBACK(on_menu_scan), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_),
			scan_menuitem_);

		GtkWidget *menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_),
			menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate",
			G_CALLBACK(on_menu_quit), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_),
			menuitem);

		gtk_widget_show_all(menu_);
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem_),
		is_scan_on());
#endif
}

#ifdef _WIN32
/* Create hidden window to process systray messages */
HWND DockLet::create_hiddenwin()
{
	WNDCLASSEX wcex;
	const TCHAR wname[] = TEXT("StarDictSystray");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)mainmsg_handler;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= stardictexe_hInstance;
	wcex.hIcon		= NULL;
	wcex.hCursor		= NULL,
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= wname;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	return CreateWindow(wname, TEXT(""), 0, 0, 0, 0, 0, GetDesktopWindow(),
		NULL, stardictexe_hInstance, 0);
}

LRESULT CALLBACK DockLet::mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	DockLet *dock =
		reinterpret_cast<DockLet *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));
	switch (msg) {
	case WM_COMMAND:
		switch(LOWORD(wparam)) {
		case SYSTRAY_CMND_MENU_SCAN:
			if (GetMenuState(dock->systray_menu, SYSTRAY_CMND_MENU_SCAN,
					MF_BYCOMMAND) & MF_CHECKED)
				dock->on_change_scan_.emit(false);
			else
				dock->on_change_scan_.emit(true);
			break;
		case SYSTRAY_CMND_MENU_QUIT:
			dock->on_quit_.emit();
			break;
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

#else
void DockLet::on_menu_scan(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_change_scan_.emit(
			gtk_check_menu_item_get_active(checkmenuitem)
	);
}

void DockLet::on_menu_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_quit_.emit();
}
#endif

void DockLet::on_popup_menu(GtkStatusIcon *status_icon,
	guint          button,
	guint          activate_time,
	gpointer       user_data)
{
	DockLet* pGtkTray = static_cast<DockLet*>(user_data);
	pGtkTray->create_popup_menu();

#ifdef _WIN32
	POINT mpoint;
	GetCursorPos(&mpoint);
	TrackPopupMenu(pGtkTray->systray_menu,         // handle to shortcut menu
		TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
		mpoint.x,                      // horizontal position, in screen coordinates
		mpoint.y,                      // vertical position, in screen coordinates
		0,                             // reserved, must be zero
		pGtkTray->systray_hwnd,        // handle to owner window
		NULL                 // ignored
		);
#else
	gtk_menu_popup(GTK_MENU(pGtkTray->menu_), NULL, NULL,
		gtk_status_icon_position_menu, status_icon,
		button, activate_time);
#endif
}

gboolean DockLet::on_button_press(GtkStatusIcon *status_icon,
		GdkEventButton      *event,
		gpointer       user_data)
{
	DockLet *pGtkTray = static_cast<DockLet*>(user_data);
	switch (event->button) {
	case 1:
		if ((event->state & GDK_CONTROL_MASK) &&
		    !(event->state & GDK_MOD1_MASK) &&
		    !(event->state & GDK_SHIFT_MASK)) {
			pGtkTray->on_change_scan_.emit(!pGtkTray->is_scan_on());
			return TRUE;
		} else {
			if (gtk_widget_get_visible(GTK_WIDGET(pGtkTray->mainwin_)))
				gtk_widget_hide(pGtkTray->mainwin_);
			else {
				pGtkTray->maximize_from_tray();
				pGtkTray->on_maximize_.emit();
			}
		}
		break;
	case 2:
		pGtkTray->on_middle_btn_click_.emit();
		return TRUE;
	default:
		/* nothing */break;
	}

	return FALSE;
}

void DockLet::scan_on()
{
	gtk_status_icon_set_tooltip_text(docklet_, _("StarDict - Scanning"));
	gtk_status_icon_set_from_pixbuf(docklet_, scan_icon_);
}

void DockLet::scan_off()
{
	gtk_status_icon_set_tooltip_text(docklet_, ("StarDict - Stopped"));
	gtk_status_icon_set_from_pixbuf(docklet_, stop_icon_);
}

void DockLet::show_normal_icon()
{
	gtk_status_icon_set_tooltip_text(docklet_, _("StarDict"));
	gtk_status_icon_set_from_pixbuf(docklet_, normal_icon_);
}
