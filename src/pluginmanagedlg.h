#ifndef _STARDICT_PLUGIN_MANAGE_DIALOG_H_
#define _STARDICT_PLUGIN_MANAGE_DIALOG_H_

#include <gtk/gtk.h>

class PluginManageDlg {
public:
	PluginManageDlg();
	~PluginManageDlg();
	bool ShowModal(GtkWindow *parent_win);
private:
	enum {
		STARDICT_RESPONSE_CONFIGURE = 100,
	};
	GtkWidget *window;
	GtkWidget *detail_label;
	GtkWidget *pref_button;
	GtkTreeStore *plugin_tree_model;
	GtkWidget *create_plugin_list();
	static void response_handler (GtkDialog *dialog, gint res_id, PluginManageDlg *oPluginManageDlg);
	static void on_plugin_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, PluginManageDlg *oPluginManageDlg);
	static void on_plugin_treeview_selection_changed(GtkTreeSelection *selection, PluginManageDlg *oPluginManageDlg);
};

#endif
