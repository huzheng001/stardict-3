#ifndef __SD_DICT_MANAGE_DLG_H__
#define __SD_DICT_MANAGE_DLG_H__

#include <gtk/gtk.h>

class DictManageDlg;

class NetworkAddDlg {
private:
    DictManageDlg *dictdlg;
	GtkTreeStore *model;
    GtkWidget *treeview;
	static void on_network_adddlg_add_button_clicked(GtkWidget *widget, NetworkAddDlg *oNetworkAddDlg);
	static void on_network_adddlg_info_button_clicked(GtkWidget *widget, NetworkAddDlg *oNetworkAddDlg);
    static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, NetworkAddDlg *oNetworkAddDlg);
    static void on_row_expanded(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, NetworkAddDlg *oNetworkAddDlg);
public:
    GtkWidget *window;
	NetworkAddDlg(DictManageDlg *dlg);
	void Show(GtkWindow *parent_win);
	void network_getdirinfo(const char *xml);
};

class DictManageDlg {
private:	
	GtkWidget *wazard_button;
	GtkWidget *appendix_button;
	GtkWidget *notebook;
	GtkWidget *button_notebook;
	GtkWidget *info_label;
    GtkWidget *upgrade_eventbox;
	GtkWidget *dict_treeview;
	GtkTreeModel *dict_tree_model;
	GtkWidget *treedict_treeview;
	GtkTreeModel *treedict_tree_model;
	GtkWidget *network_treeview;
	GtkTreeModel *network_tree_model;
	GtkWindow *parent_win;
	GdkPixbuf *dicts_icon, *tree_dicts_icon;
	GtkWidget *window;
	NetworkAddDlg *network_add_dlg;
    int max_dict_count;
    int user_level;
    bool dictmask_changed;

	static GtkTreeModel* create_dict_tree_model(int istreedict);
	GtkWidget *create_dict_tree(int istreedict);
		
	GtkWidget *create_buttons();
	GtkWidget *create_network_buttons();

	void write_order_list(bool istreedict);
    void ChangeDictMask();

	static void on_wazard_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_appendix_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_network_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_upgrade_eventbox_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);

	static gboolean on_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static gboolean on_network_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static void response_handler (GtkDialog *dialog, gint res_id, DictManageDlg *oDictManageDlg);
	static void on_network_add_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_network_remove_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);
	static void on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);

	static void drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg);
	static void drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg);
public:
	DictManageDlg(GtkWindow *parent_win, GdkPixbuf *dicts_icon, GdkPixbuf *tree_dicts_icon);
	bool Show();
	void Close();
	void network_getdictmask(const char *xml);
	void network_dirinfo(const char *xml);
	void network_dictinfo(const char *xml);
	void network_maxdictcount(int count);

    friend class NetworkAddDlg;
};

#endif
