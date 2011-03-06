#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "skin.h"

#include "docklet.h"

DockLet::DockLet(GtkWidget *mainwin, bool is_scan_on, const AppSkin& skin):
	TrayBase(mainwin, is_scan_on),
	normal_icon_(NULL),
	stop_icon_(NULL),
	scan_icon_(NULL),
	docklet_(NULL),
	menu_(NULL),
	scan_menuitem_(NULL)
{
	normal_icon_ = get_impl(skin.docklet_normal_icon);
	scan_icon_ = get_impl(skin.docklet_scan_icon);
	stop_icon_ = get_impl(skin.docklet_stop_icon);

	create_docklet();
}

DockLet::~DockLet() {
	while (g_source_remove_by_user_data(&docklet_))
		;
	if (menu_)
		gtk_widget_destroy(menu_);
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
	g_object_ref(G_OBJECT(docklet_));
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

	gtk_status_icon_set_visible(docklet_, TRUE);
}

void DockLet::create_popup_menu(void)
{
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
		g_signal_connect(G_OBJECT(menuitem), "activate",
			G_CALLBACK(on_menu_quit), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_),
			menuitem);

		gtk_widget_show_all(menu_);
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem_),
		is_scan_on());
}

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

void DockLet::on_popup_menu(GtkStatusIcon *status_icon,
	guint          button,
	guint          activate_time,
	gpointer       user_data)
{
	DockLet* pGtkTray = static_cast<DockLet*>(user_data);
	pGtkTray->create_popup_menu();

	gtk_menu_popup(GTK_MENU(pGtkTray->menu_), NULL, NULL,
		gtk_status_icon_position_menu, status_icon,
		button, activate_time);
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
