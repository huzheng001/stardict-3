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

#ifndef _STARDICT_PLUGIN_MANAGE_DIALOG_H_
#define _STARDICT_PLUGIN_MANAGE_DIALOG_H_

#include <gtk/gtk.h>

class PluginManageDlg {
public:
	PluginManageDlg();
	~PluginManageDlg();
	bool ShowModal(GtkWindow *parent_win, bool &dict_changed, bool &order_changed);
private:
	enum {
		STARDICT_RESPONSE_CONFIGURE = 100,
	};
	GtkWidget *window;
	GtkWidget *treeview;
	GtkWidget *detail_label;
	GtkWidget *pref_button;
	GtkTreeStore *plugin_tree_model;
	bool dict_changed_;
	bool order_changed_;
	GtkWidget *create_plugin_list();
	void write_order_list();
	static void response_handler (GtkDialog *dialog, gint res_id, PluginManageDlg *oPluginManageDlg);
	static void on_plugin_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, PluginManageDlg *oPluginManageDlg);
	static void on_plugin_treeview_selection_changed(GtkTreeSelection *selection, PluginManageDlg *oPluginManageDlg);
	static gboolean on_treeview_button_press(GtkWidget * widget, GdkEventButton * event, PluginManageDlg *oPluginManageDlg);
	static void drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, PluginManageDlg *oPluginManageDlg);
	static void drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, PluginManageDlg *oPluginManageDlg);
};

#endif
