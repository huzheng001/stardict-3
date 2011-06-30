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
