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
	GtkWidget *dict_list_button;
	GtkWidget *manage_button;
	GtkWidget *tree_dict_button;
	GtkWidget *notebook;
	GtkWidget *button_notebook;
	GtkWidget *download_hbox;
	GtkWidget *info_label;
	GtkWidget *upgrade_label;
	GtkWidget *upgrade_eventbox;
	GtkWidget *popup_menu1;
	GtkWidget *popup_menu;
	GtkWidget *dict_list_treeview;
	GtkTreeModel *dict_list_tree_model;
	GtkWidget *dictmanage_treeview;
	GtkTreeModel *dictmanage_tree_model;
	GtkWidget *treedict_treeview;
	GtkTreeModel *treedict_tree_model;
	GtkWidget *network_treeview;
	GtkTreeModel *network_tree_model;
	GtkWindow *parent_win;
	GdkPixbuf *dicts_icon, *tree_dicts_icon;
	NetworkAddDlg *network_add_dlg;
	int max_dict_count;
	int user_level;
	std::string upgrade_url;
	bool network_dictmask_changed;
	bool dictmanage_list_changed;
	bool dictmanage_config_changed;

	enum TDictTree { DictTree_DictList, DictTree_TreeDict, DictTree_NetworkDict };
	static GtkTreeModel* create_tree_model(TDictTree dicttree);
	GtkWidget *create_dict_tree(TDictTree dicttree);
	static GtkTreeModel* create_dictmanage_tree_model();
	GtkWidget *create_dictmanage_tree();

	GtkWidget *create_buttons();
	GtkWidget *create_dictmanage_buttons();
	GtkWidget *create_network_buttons();

	void write_treedict_order_list();
	void ChangeNetworkDictMask();
	void SaveDictManageList();
	void SaveDictManageConfig();
	void show_dict_info();

	void show_add_group_dialog(GtkTreeIter *iter);
	void show_delete_group_dialog(GtkTreeIter *iter);
	void show_delete_subgroup_dialog(GtkTreeIter *iter);
	void show_add_dict_dialog(GtkTreeIter *iter);

	static void on_dict_list_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_manage_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_tree_dict_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_network_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg);
	static void on_download_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static void on_upgrade_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);

	static void on_popup_menu_show_info_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg);
	static void on_popup_menu_select_all_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg);
	static void on_popup_menu_unselect_all_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg);

	static gboolean on_dictlist_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static gboolean on_dicttree_and_manage_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static gboolean on_network_treeview_button_press(GtkWidget * widget, GdkEventButton *event, DictManageDlg *oDictManageDlg);
	static void response_handler (GtkDialog *dialog, gint res_id, DictManageDlg *oDictManageDlg);
	static void on_network_add_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_network_remove_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_add_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_delete_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_info_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg);
	static void on_group_name_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, DictManageDlg *oDictManageDlg);
	static void on_dictmanage_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);
	static void on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg);
	static void on_dict_list_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg);
	static void on_treedict_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg);
	static void on_network_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg);

	static void drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg);
	static void drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg);
	static void dictmanage_drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg);

public:
	GtkWidget *window;

	DictManageDlg(GtkWindow *parent_win, GdkPixbuf *dicts_icon, GdkPixbuf *tree_dicts_icon);
	bool ShowModal(bool &dictmanage_config_changed_);
	void Close();
	void network_getdictmask(const char *xml);
	void network_getadinfo(const char *xml);
	void network_dirinfo(const char *xml);
	void network_dictinfo(const char *xml);
	void network_maxdictcount(int count);

	friend class NetworkAddDlg;
};

#endif
