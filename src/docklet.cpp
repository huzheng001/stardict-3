#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>

#include "skin.h"

#include "docklet.h"

DockLet::DockLet(GtkWidget *mainwin, bool is_scan_on,
		 const AppSkin& skin) : TrayBase(mainwin, is_scan_on)
{
	menu_ = NULL;
	image_ = NULL;
	embedded_ = false;

	normal_icon_ = get_impl(skin.docklet_normal_icon);
	scan_icon_ = get_impl(skin.docklet_scan_icon);
	stop_icon_ = get_impl(skin.docklet_stop_icon);

	create_docklet();
}

void DockLet::create_docklet()
{
	docklet_ = egg_tray_icon_new("StarDict");
	box_ = gtk_event_box_new();
	if (is_hide_state()) {
		gtk_widget_set_tooltip_text(box_, _("StarDict"));
		image_ = gtk_image_new_from_pixbuf(normal_icon_);
	} else if (is_scan_on()) {
		gtk_widget_set_tooltip_text(box_, _("StarDict - Scanning"));
		image_ = gtk_image_new_from_pixbuf(scan_icon_);
	} else {
		gtk_widget_set_tooltip_text(box_, _("StarDict - Stopped"));
		image_ = gtk_image_new_from_pixbuf(stop_icon_);
	}

	g_signal_connect(G_OBJECT(docklet_), "embedded",
			 G_CALLBACK(on_embedded), this);
	g_signal_connect(G_OBJECT(docklet_), "destroy",
			 G_CALLBACK(on_destroyed), this);
	g_signal_connect(G_OBJECT(box_), "button-press-event",
			 G_CALLBACK(on_btn_press), this);

	gtk_container_add(GTK_CONTAINER(box_), image_);
	gtk_container_add(GTK_CONTAINER(docklet_), box_);
	gtk_widget_show_all(GTK_WIDGET(docklet_));

	/* ref the docklet_ before we bandy it about the place */
	g_object_ref(G_OBJECT(docklet_));
}

DockLet::~DockLet()
{
	while (g_source_remove_by_user_data(&docklet_))
		;

	g_signal_handlers_disconnect_by_func(G_OBJECT(docklet_),
					     gpointer(on_destroyed), this);
	gtk_widget_destroy(GTK_WIDGET(docklet_));
	g_object_unref(G_OBJECT(docklet_));	
	if (menu_)
		gtk_widget_destroy(get_impl(menu_));
}

void DockLet::on_destroyed(GtkWidget *widget, DockLet *oDockLet)
{
	oDockLet->embedded_ = false;
	oDockLet->image_ = NULL;

	while (g_source_remove_by_user_data(&oDockLet->docklet_))
		;
	g_object_unref(G_OBJECT(oDockLet->docklet_));
	//when user add Nofification area applet again,it will show icon again.
	g_idle_add(on_docklet_create, oDockLet);
}

void DockLet::on_menu_scan(GtkCheckMenuItem *checkmenuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_change_scan_.emit(
                gtk_check_menu_item_get_active(checkmenuitem));
}

void DockLet::on_menu_quit(GtkMenuItem *menuitem, gpointer user_data)
{
	static_cast<DockLet *>(user_data)->on_quit_.emit();
}

void DockLet::popup_menu(GdkEventButton *event)
{
	if (!menu_) {
		menu_.reset(gtk_menu_new());

		scan_menuitem_ = gtk_check_menu_item_new_with_mnemonic(_("_Scan"));
		g_signal_connect(G_OBJECT(scan_menuitem_), "toggled",
				 G_CALLBACK(on_menu_scan), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(get_impl(menu_)),
				      scan_menuitem_);

		GtkWidget *menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(get_impl(menu_)),
				      menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		g_signal_connect(G_OBJECT(menuitem), "activate",
				 G_CALLBACK(on_menu_quit), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(get_impl(menu_)),
				      menuitem);

		gtk_widget_show_all(get_impl(menu_));
	}
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(scan_menuitem_),
				       is_scan_on());

	gtk_menu_popup(GTK_MENU(get_impl(menu_)), NULL, NULL, NULL, NULL,
		       event->button, event->time);
}

gboolean DockLet::on_docklet_create(gpointer data)
{
	DockLet *dock = static_cast<DockLet *>(data);
	dock->create_docklet();
	return FALSE; /* for when we're called by the glib idle handler */
}

void DockLet::on_embedded(GtkWidget *widget, gpointer data)
{
	static_cast<DockLet *>(data)->embedded_ = true;
}


gboolean DockLet::on_btn_press(GtkWidget *button, GdkEventButton *event,
			       DockLet *dock)
{
	if (event->type != GDK_BUTTON_PRESS)
		return FALSE;

	switch (event->button) {
	case 1:
		if ((event->state & GDK_CONTROL_MASK) &&
		    !(event->state & GDK_MOD1_MASK) &&
		    !(event->state & GDK_SHIFT_MASK)) {
			dock->on_change_scan_.emit(!dock->is_scan_on());
			return TRUE;
		} else {
			if (GTK_WIDGET_VISIBLE(dock->mainwin_))
				gtk_widget_hide(dock->mainwin_);
			else {
				dock->maximize_from_tray();
				dock->on_maximize_.emit();
			}
		}
		break;
	case 2:
		dock->on_middle_btn_click_.emit();
		return TRUE;
	case 3:
		dock->popup_menu(event);
		return TRUE;
	default:
		/* nothing */break;
	}

	return FALSE;
}

void DockLet::minimize_to_tray()
{
	if (embedded_)
		TrayBase::minimize_to_tray();
	else
		on_quit_.emit();
}

void DockLet::scan_on()
{
        gtk_widget_set_tooltip_text(box_, _("StarDict - Scanning"));
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_), scan_icon_);
}

void DockLet::scan_off()
{
        gtk_widget_set_tooltip_text(box_, _("StarDict - Stopped"));
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_), stop_icon_);
}

void DockLet::show_normal_icon()
{
        if (!image_)
                return;
        gtk_widget_set_tooltip_text(box_, _("StarDict"));
        gtk_image_set_from_pixbuf(GTK_IMAGE(image_), normal_icon_);
}

void DockLet::set_scan_mode(bool is_on)
{
        if (!image_)
                return;
        TrayBase::set_scan_mode(is_on);
}
