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

#include <cstring>
#include <sys/stat.h>
#include <glib/gi18n.h>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include <algorithm>

#include "stardict.h"
#include "conf.h"
#include "desktop.h"
#include "ifo_file.h"
#include "lib/utils.h"
#include "lib/file-utils.h"
#include "dictmanage.h"

#include "dictmanagedlg.h"

enum DictListColumns {
	DICTLIST_TYPE_COLUMN, // 0 G_TYPE_INT
	DICTLIST_NAME_COLUMN, // 1
	DICTLIST_WORD_COUNT_COLUMN, // 2
	DICTLIST_AUTHOR_COLUMN, // 3
	DICTLIST_EMAIL_COLUMN, // 4
	DICTLIST_WEB_SITE_COLUMN, // 5
	DICTLIST_DESCRIPTION_COLUMN, // 6
	DICTLIST_DATE_COLUMN, // 7
	DICTLIST_ID_COLUMN, // 8
	DICTLIST_ID_TYPE_STR_COLUMN, // 9
	DICTLIST_COLUMN_NUMBER
};

enum TreeDictColumns {
	TREEDICT_ENABLED_COLUMN, // 0
	TREEDICT_NAME_COLUMN, // 1
	TREEDICT_WORD_COUNT_COLUMN, // 2
	TREEDICT_AUTHOR_COLUMN, // 3
	TREEDICT_EMAIL_COLUMN, // 4
	TREEDICT_WEB_SITE_COLUMN, // 5
	TREEDICT_DESCRIPTION_COLUMN, // 6
	TREEDICT_DATE_COLUMN, // 7
	TREEDICT_ID_COLUMN, // 8
	TREEDICT_COLUMN_NUMBER
};

enum NetworkDictColumns {
	NETWORKDICT_ID_COLUMN, // 0
	NETWORKDICT_NAME_COLUMN, // 1
	NETWORKDICT_WORD_COUNT_COLUMN, // 2
	NETWORKDICT_COLUMN_NUMBER
};

enum NetworkAddDlgDictColumns {
	NETWORK_ADD_DLG_NAME_COLUMN, // 0
	NETWORK_ADD_DLG_VISIBLE_COLUMN, // 1
	NETWORK_ADD_DLG_WORD_COUNT_COLUMN, // 2
	NETWORK_ADD_DLG_ID_COLUMN, // 3
	NETWORK_ADD_DLG_NEED_LEVEL_COLUMN, // 4 G_TYPE_INT
	NETWORK_ADD_DLG_COLUMN_NUMBER
};

enum DictManageColumns {
	DICT_MANAGE_ENABLE_COLUMN, // 0
	/* markup for row level 0, 1
	 * dictionary name for row level 2 */
	DICT_MANAGE_MARKUP_COLUMN, // 1
	DICT_MANAGE_WORD_COUNT_COLUMN, // 2
	/* plain text name for row level 0, 1
	 * author for row level 2 */
	DICT_MANAGE_AUTHOR_COLUMN, // 3
	DICT_MANAGE_EMAIL_COLUMN, // 4
	DICT_MANAGE_WEB_SITE_COLUMN, // 5
	DICT_MANAGE_DESCRIPTION_COLUMN, // 6
	DICT_MANAGE_DATE_COLUMN, // 7
	DICT_MANAGE_ID_COLUMN, // 8
	/* if true, show details information: checkbox, word count
	 * if false, show only markup */
	DICT_MANAGE_SHOW_DETAILS_COLUMN, // 9
	DICT_MANAGE_ROW_LEVEL_COLUMN, // 10 G_TYPE_INT
	DICT_MANAGE_EDITABLE_COLUMN, // 11
	DICT_MANAGE_TYPE_COLUMN, // 12 G_TYPE_INT
	DICT_MANAGE_COLUMN_NUMBER // 13
};

enum AddDictDlgColumns {
	ADD_DICT_TYPE_COLUMN, // 0 G_TYPE_INT
	ADD_DICT_NAME_COLUMN, // 1
	ADD_DICT_WORD_COUNT_COLUMN, // 2
	ADD_DICT_AUTHOR_COLUMN, // 3
	ADD_DICT_EMAIL_COLUMN, // 4
	ADD_DICT_WEB_SITE_COLUMN, // 5
	ADD_DICT_DESCRIPTION_COLUMN, // 6
	ADD_DICT_DATE_COLUMN, // 7
	ADD_DICT_ID_COLUMN, // 8
	ADD_DICT_TYPE_STR_COLUMN, // 9
	ADD_DICT_COLUMN_NUMBER // 10
};

NetworkAddDlg::NetworkAddDlg(DictManageDlg *dlg)
{
	dictdlg = dlg;
}

void NetworkAddDlg::on_network_adddlg_add_button_clicked(GtkWidget *widget, NetworkAddDlg *oNetworkAddDlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oNetworkAddDlg->treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gboolean visible;
		gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_VISIBLE_COLUMN, &visible, -1);
		if (visible) {
			gchar *uid;
			gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_ID_COLUMN, &uid, -1);
			bool added = false;
			int row_count = 0;
			GtkTreeIter iter2;
			if (gtk_tree_model_get_iter_first(oNetworkAddDlg->dictdlg->network_tree_model, &iter2)) {
				gchar *uid2;
				do {
					row_count++;
					gtk_tree_model_get (
						oNetworkAddDlg->dictdlg->network_tree_model, &iter2,
						NETWORKDICT_ID_COLUMN, &uid2,
						-1
					);
					if (strcmp(uid, uid2)==0) {
						added = true;
						g_free(uid2);
						break;
					}
					g_free(uid2);
				} while (gtk_tree_model_iter_next(oNetworkAddDlg->dictdlg->network_tree_model, &iter2));
			}
			if (!added) {
				gchar *msg;
				if (row_count < oNetworkAddDlg->dictdlg->max_dict_count) {
					gint need_level;
					gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_NEED_LEVEL_COLUMN, &need_level, -1);
					if (need_level > oNetworkAddDlg->dictdlg->user_level) {
						msg = g_strdup_printf(_("Only level %d user can choose this dictionary!"), need_level);
					} else {
						msg = NULL;
						gchar *bookname, *wordcount;
						gtk_tree_model_get (
							model, &iter,
							NETWORK_ADD_DLG_NAME_COLUMN, &bookname,
							NETWORK_ADD_DLG_WORD_COUNT_COLUMN, &wordcount,
							-1
						);
						gtk_list_store_append(GTK_LIST_STORE(oNetworkAddDlg->dictdlg->network_tree_model), &iter2);
						gtk_list_store_set(
							GTK_LIST_STORE(oNetworkAddDlg->dictdlg->network_tree_model),
							&iter2,
							NETWORKDICT_ID_COLUMN, uid,
							NETWORKDICT_NAME_COLUMN, bookname,
							NETWORKDICT_WORD_COUNT_COLUMN, wordcount,
							-1
						);
						g_free(bookname);
						g_free(wordcount);
						oNetworkAddDlg->dictdlg->network_dictmask_changed = true;
					}
				} else {
					msg = g_strdup_printf(_("You can only choose %d dictionaries."), oNetworkAddDlg->dictdlg->max_dict_count);
				}
				if (msg) {
					GtkWidget *message_dlg =
						gtk_message_dialog_new(
								GTK_WINDOW(oNetworkAddDlg->window),
								(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
								GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
								"%s", msg);
					g_free(msg);
					gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
					gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
					gtk_dialog_run(GTK_DIALOG(message_dlg));
					gtk_widget_destroy(message_dlg);
				}
			}
			g_free(uid);
		}
	}
}

void NetworkAddDlg::on_network_adddlg_info_button_clicked(GtkWidget *widget, NetworkAddDlg *oNetworkAddDlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oNetworkAddDlg->treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gboolean visible;
		gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_VISIBLE_COLUMN, &visible, -1);
		if (visible) {
			gchar *uid;
			gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_ID_COLUMN, &uid, -1);
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DICT_INFO, uid);
			if (!gpAppFrame->oStarDictClient.try_cache(c))
				gpAppFrame->oStarDictClient.send_commands(1, c);
			g_free(uid);
		}
	}
}

gboolean NetworkAddDlg::on_button_press(GtkWidget * widget, GdkEventButton * event, NetworkAddDlg *oNetworkAddDlg)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gboolean visible;
			gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_VISIBLE_COLUMN, &visible, -1);
			if (visible) {
				gchar *uid;
				gtk_tree_model_get (model, &iter, NETWORK_ADD_DLG_ID_COLUMN, &uid, -1);
				STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DICT_INFO, uid);
				if (!gpAppFrame->oStarDictClient.try_cache(c))
					gpAppFrame->oStarDictClient.send_commands(1, c);
				g_free(uid);
			} else {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW (widget), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW (widget), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW (widget), path, FALSE);
				gtk_tree_path_free(path);
			}
		}
		return true;
	} else {
		return false;
	}
}

void NetworkAddDlg::on_row_expanded(GtkTreeView *treeview, GtkTreeIter *arg1, GtkTreePath *arg2, NetworkAddDlg *oNetworkAddDlg)
{
	GtkTreeIter iter;
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(oNetworkAddDlg->model), &iter, arg1) == FALSE)
		return;
	gchar *word;
	gtk_tree_model_get (GTK_TREE_MODEL(oNetworkAddDlg->model), &iter,
		NETWORK_ADD_DLG_NAME_COLUMN, &word,
		-1
	);
	if (word[0] == '\0') {
		gchar *path;
		gtk_tree_model_get (GTK_TREE_MODEL(oNetworkAddDlg->model), arg1,
			NETWORK_ADD_DLG_ID_COLUMN, &path,
			-1
		);
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DIR_INFO, path);
		if (!gpAppFrame->oStarDictClient.try_cache(c))
			gpAppFrame->oStarDictClient.send_commands(1, c);
		g_free(path);
	}
	g_free(word);
}

void NetworkAddDlg::Show(GtkWindow *parent_win)
{
	window = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(window), parent_win);
	gtk_dialog_add_button (GTK_DIALOG (window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 6);
#endif
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, 350, 230);
	model = gtk_tree_store_new(NETWORK_ADD_DLG_COLUMN_NUMBER, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	g_object_unref (G_OBJECT (model));
#if GTK_MAJOR_VERSION >= 3
#else
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
#endif
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", NETWORK_ADD_DLG_NAME_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer,
		"text", NETWORK_ADD_DLG_WORD_COUNT_COLUMN,
		"visible", NETWORK_ADD_DLG_VISIBLE_COLUMN,
		NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_button_press), this);
	g_signal_connect (G_OBJECT (treeview), "row-expanded", G_CALLBACK (on_row_expanded), this);
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	gtk_box_pack_start (GTK_BOX (hbox), sw, true, true, 0);
	GtkWidget *vbox;
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox = gtk_vbox_new(false, 6);
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_adddlg_add_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
#ifdef CONFIG_MAEMO
	button = gtk_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
#else
	button = gtk_button_new_from_stock(GTK_STOCK_INFO);
#endif
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_adddlg_info_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, false, false, 0);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (window))), hbox, true, true, 0);
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG (window)));
	gtk_window_set_title(GTK_WINDOW (window), _("Browse Dictionaries"));
	STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DIR_INFO, "/");
	if (!gpAppFrame->oStarDictClient.try_cache(c))
		gpAppFrame->oStarDictClient.send_commands(1, c);
	gtk_dialog_run(GTK_DIALOG(window));
	gtk_widget_destroy(window);
}

struct dirinfo_ParseUserData {
	GtkTreeStore *model;
	GtkTreeIter *iter;
	std::string parent;
	unsigned int userlevel;
	bool in_dir;
	std::string dir_name;
	std::string dir_dirname;
	std::string dir_dictcount;
	bool in_dict;
	std::string dict_islink;
	std::string dict_level;
	std::string dict_uid;
	std::string dict_bookname;
	std::string dict_wordcount;
};

static void dirinfo_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dir")==0) {
		dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
		Data->in_dir = true;
		Data->dir_name.clear();
		Data->dir_dirname.clear();
		Data->dir_dictcount.clear();
	} else if (strcmp(element_name, "dict")==0) {
		dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
		Data->in_dict = true;
		Data->dict_islink.clear();
		Data->dict_level.clear();
		Data->dict_uid.clear();
		Data->dict_bookname.clear();
		Data->dict_wordcount.clear();
	}
}

static void dirinfo_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dir")==0) {
		dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
		Data->in_dir = false;
		GtkTreeIter iter;
		gtk_tree_store_append(Data->model, &iter, Data->iter);
		gtk_tree_store_set(
			Data->model, &iter,
			NETWORK_ADD_DLG_NAME_COLUMN, (Data->dir_dirname + " (" + Data->dir_dictcount + ')').c_str(),
			NETWORK_ADD_DLG_VISIBLE_COLUMN, FALSE,
			NETWORK_ADD_DLG_ID_COLUMN, (Data->parent + Data->dir_name + '/').c_str(),
			-1
		);
		GtkTreeIter iter1;
		gtk_tree_store_append(Data->model, &iter1, &iter);
		gtk_tree_store_set(
			Data->model, &iter1,
			NETWORK_ADD_DLG_NAME_COLUMN, "",
			NETWORK_ADD_DLG_VISIBLE_COLUMN, TRUE,
			-1
		);
	} else if (strcmp(element_name, "dict")==0) {
		dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
		Data->in_dict = false;
		GtkTreeIter iter;
		gtk_tree_store_append(Data->model, &iter, Data->iter);
		gint need_level;
		if (Data->dict_level.empty())
			need_level = 0;
		else
			need_level = atoi(Data->dict_level.c_str());
		gtk_tree_store_set(
			Data->model, &iter,
			NETWORK_ADD_DLG_NAME_COLUMN, Data->dict_bookname.c_str(),
			NETWORK_ADD_DLG_VISIBLE_COLUMN, TRUE,
			NETWORK_ADD_DLG_WORD_COUNT_COLUMN, Data->dict_wordcount.c_str(),
			NETWORK_ADD_DLG_ID_COLUMN, Data->dict_uid.c_str(),
			NETWORK_ADD_DLG_NEED_LEVEL_COLUMN, need_level,
			-1
		);
	}
}

static gboolean do_find_iter(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)data;
	gboolean visible;
	gtk_tree_model_get (model, iter, NETWORK_ADD_DLG_VISIBLE_COLUMN, &visible, -1);
	if (visible == FALSE) {
		gchar *dirpath;
		gtk_tree_model_get (model, iter, NETWORK_ADD_DLG_ID_COLUMN, &dirpath, -1);
		if (dirpath && Data->parent == dirpath) {
			Data->iter = (GtkTreeIter *)g_malloc(sizeof(GtkTreeIter));
			*(Data->iter) = *iter;
			g_free(dirpath);
			return TRUE;
		}
		g_free(dirpath);
	}
	return FALSE;
}

static void dirinfo_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
	if (strcmp(element, "parent")==0) {
		Data->parent.assign(text, text_len);
		if (Data->parent == "/") {
			Data->iter = NULL;
		} else {
			gtk_tree_model_foreach(GTK_TREE_MODEL(Data->model), do_find_iter, Data);
		}
	} else if (strcmp(element, "userlevel")==0) {
		std::string userlevel(text, text_len);
		Data->userlevel = atoi(userlevel.c_str());
	} else if (strcmp(element, "name")==0) {
		if (Data->in_dir)
			Data->dir_name.assign(text, text_len);
	} else if (strcmp(element, "dirname")==0) {
		if (Data->in_dir)
			Data->dir_dirname.assign(text, text_len);
	} else if (strcmp(element, "dictcount")==0) {
		if (Data->in_dir)
			Data->dir_dictcount.assign(text, text_len);
	} else if (strcmp(element, "islink")==0) {
		if (Data->in_dict)
			Data->dict_islink.assign(text, text_len);
	} else if (strcmp(element, "level")==0) {
		if (Data->in_dict)
			Data->dict_level.assign(text, text_len);
	} else if (strcmp(element, "uid")==0) {
		if (Data->in_dict)
			Data->dict_uid.assign(text, text_len);
	} else if (strcmp(element, "bookname")==0) {
		if (Data->in_dict)
			Data->dict_bookname.assign(text, text_len);
	} else if (strcmp(element, "wordcount")==0) {
		if (Data->in_dict)
			Data->dict_wordcount.assign(text, text_len);
	}
}

void NetworkAddDlg::network_getdirinfo(const char *xml)
{
	dirinfo_ParseUserData Data;
	Data.model = this->model;
	Data.iter = NULL;
	Data.userlevel = 0;
	Data.in_dir = false;
	Data.in_dict = false;
	GMarkupParser parser;
	parser.start_element = dirinfo_parse_start_element;
	parser.end_element = dirinfo_parse_end_element;
	parser.text = dirinfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, xml, -1, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	if (Data.iter) {
		GtkTreeIter iter;
		if (gtk_tree_model_iter_children(GTK_TREE_MODEL(Data.model), &iter, Data.iter))
			gtk_tree_store_remove(Data.model, &iter);
		g_free(Data.iter);
	}
	this->dictdlg->user_level = Data.userlevel;
}

DictManageDlg::DictManageDlg(GtkWindow *pw, GdkPixbuf *di,  GdkPixbuf *tdi) :
	parent_win(pw), dicts_icon(di), tree_dicts_icon(tdi), network_add_dlg(NULL), window(NULL)
{
}

void DictManageDlg::response_handler (GtkDialog *dialog, gint res_id, DictManageDlg *oDictManageDlg)
{
	if (res_id==GTK_RESPONSE_HELP)
		show_help("stardict-dictmanage");
}

void DictManageDlg::on_dict_list_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 0);
		gtk_widget_show(oDictManageDlg->download_hbox);
		gtk_widget_hide(oDictManageDlg->info_label);
		gtk_widget_hide(oDictManageDlg->upgrade_eventbox);
	}
}

void DictManageDlg::on_manage_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 1);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 1);
		gtk_widget_show(oDictManageDlg->download_hbox);
		gtk_widget_hide(oDictManageDlg->info_label);
		gtk_widget_hide(oDictManageDlg->upgrade_eventbox);
	}
}

void DictManageDlg::on_tree_dict_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 2);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 0);
		gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), _("These settings will take effect the next time you run StarDict."));
		gtk_widget_show(oDictManageDlg->info_label);
		gtk_widget_hide(oDictManageDlg->download_hbox);
		gtk_widget_hide(oDictManageDlg->upgrade_eventbox);
	}
}

void DictManageDlg::on_network_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 3);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 2);
		if (oDictManageDlg->max_dict_count == -1) {
			gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), _("Loading..."));
			STARDICT::Cmd *c1 = new STARDICT::Cmd(STARDICT::CMD_GET_DICT_MASK);
			STARDICT::Cmd *c2 = new STARDICT::Cmd(STARDICT::CMD_MAX_DICT_COUNT);

			gtk_widget_show(oDictManageDlg->info_label);
			gtk_widget_hide(oDictManageDlg->download_hbox);
			STARDICT::Cmd *c3 = new STARDICT::Cmd(STARDICT::CMD_GET_ADINFO);
			gpAppFrame->oStarDictClient.try_cache_or_send_commands(3, c1, c2, c3);
			gtk_widget_show(oDictManageDlg->upgrade_eventbox);
		} else {
			gchar *str = g_strdup_printf(_("You can only choose %d dictionaries."), oDictManageDlg->max_dict_count);
			gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), str);
			g_free(str);

			gtk_widget_show(oDictManageDlg->info_label);
			gtk_widget_hide(oDictManageDlg->download_hbox);
			STARDICT::Cmd *c3 = new STARDICT::Cmd(STARDICT::CMD_GET_ADINFO);
			gpAppFrame->oStarDictClient.try_cache_or_send_commands(1, c3);
			gtk_widget_show(oDictManageDlg->upgrade_eventbox);
		}
	}
}

static void create_dict_item_model(GtkTreeStore *model, GtkTreeIter *group_iter, std::list<DictManageItem> &dictitem, bool is_query)
{
	GtkTreeIter type_iter;
	gtk_tree_store_append(model, &type_iter, group_iter);
	if (is_query) {
		gtk_tree_store_set(
			model, &type_iter,
			DICT_MANAGE_MARKUP_COLUMN, _("<span foreground=\"red\">Query Dict</span>"),
			DICT_MANAGE_AUTHOR_COLUMN, "querydict",
			DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
			DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)1,
			DICT_MANAGE_EDITABLE_COLUMN, FALSE,
			-1
		);
	} else {
		gtk_tree_store_set(
			model, &type_iter,
			DICT_MANAGE_MARKUP_COLUMN, _("<span foreground=\"red\">Scan Dict</span>"),
			DICT_MANAGE_AUTHOR_COLUMN, "scandict",
			DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
			DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)1,
			DICT_MANAGE_EDITABLE_COLUMN, FALSE,
			-1
		);
	}
	GtkTreeIter dict_iter;
	for (std::list<DictManageItem>::iterator i = dictitem.begin(); i != dictitem.end(); ++i) {
		if (i->type == LOCAL_DICT) {
			DictInfo dictinfo;
			if (dictinfo.load_from_ifo_file(i->file_or_id.c_str(),
				DictInfoType_NormDict)) {
				gchar *markup = g_markup_escape_text(dictinfo.get_bookname().c_str(), dictinfo.get_bookname().length());
				gchar *wc = g_strdup_printf("%d", dictinfo.get_wordcount());
				gtk_tree_store_append(model, &dict_iter, &type_iter);
				gtk_tree_store_set(
					model, &dict_iter,
					DICT_MANAGE_ENABLE_COLUMN, i->enable,
					DICT_MANAGE_MARKUP_COLUMN, markup,
					DICT_MANAGE_WORD_COUNT_COLUMN, wc,
					DICT_MANAGE_AUTHOR_COLUMN, dictinfo.get_author().c_str(),
					DICT_MANAGE_EMAIL_COLUMN, dictinfo.get_email().c_str(),
					DICT_MANAGE_WEB_SITE_COLUMN, dictinfo.get_website().c_str(),
					DICT_MANAGE_DESCRIPTION_COLUMN, dictinfo.get_description().c_str(),
					DICT_MANAGE_DATE_COLUMN, dictinfo.get_date().c_str(),
					DICT_MANAGE_ID_COLUMN, i->file_or_id.c_str(),
					DICT_MANAGE_SHOW_DETAILS_COLUMN, true,
					DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)2,
					DICT_MANAGE_EDITABLE_COLUMN, FALSE,
					DICT_MANAGE_TYPE_COLUMN, (gint)LOCAL_DICT,
					-1
				);
				g_free(markup);
				g_free(wc);
			}
		} else if (i->type == VIRTUAL_DICT) {
			size_t iPlugin;
			if (gpAppFrame->oStarDictPlugins->VirtualDictPlugins.find_dict_by_id(i->file_or_id, iPlugin)) {
				const char *dictname = gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(iPlugin);
				gtk_tree_store_append(model, &dict_iter, &type_iter);
				gtk_tree_store_set(
					model, &dict_iter,
					DICT_MANAGE_ENABLE_COLUMN, i->enable,
					DICT_MANAGE_MARKUP_COLUMN, dictname,
					DICT_MANAGE_WORD_COUNT_COLUMN, "∞",
					DICT_MANAGE_AUTHOR_COLUMN, "",
					DICT_MANAGE_EMAIL_COLUMN, "",
					DICT_MANAGE_WEB_SITE_COLUMN, "",
					DICT_MANAGE_DESCRIPTION_COLUMN, _("Virtual Dictionary"),
					DICT_MANAGE_DATE_COLUMN, "",
					DICT_MANAGE_ID_COLUMN, i->file_or_id.c_str(),
					DICT_MANAGE_SHOW_DETAILS_COLUMN, true,
					DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)2,
					DICT_MANAGE_EDITABLE_COLUMN, FALSE,
					DICT_MANAGE_TYPE_COLUMN, (gint)VIRTUAL_DICT,
					-1
				);
			}
		} else if(i->type == NET_DICT) {
			size_t iPlugin;
			if (gpAppFrame->oStarDictPlugins->NetDictPlugins.find_dict_by_id(i->file_or_id, iPlugin)) {
				const char *dictname = gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(iPlugin);
				gtk_tree_store_append(model, &dict_iter, &type_iter);
				gtk_tree_store_set(
					model, &dict_iter,
					DICT_MANAGE_ENABLE_COLUMN, i->enable,
					DICT_MANAGE_MARKUP_COLUMN, dictname,
					DICT_MANAGE_WORD_COUNT_COLUMN, "∞",
					DICT_MANAGE_AUTHOR_COLUMN, "",
					DICT_MANAGE_EMAIL_COLUMN, "",
					DICT_MANAGE_WEB_SITE_COLUMN, "",
					DICT_MANAGE_DESCRIPTION_COLUMN, _("Network Dictionary"),
					DICT_MANAGE_DATE_COLUMN, "",
					DICT_MANAGE_ID_COLUMN, i->file_or_id.c_str(),
					DICT_MANAGE_SHOW_DETAILS_COLUMN, true,
					DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)2,
					DICT_MANAGE_EDITABLE_COLUMN, FALSE,
					DICT_MANAGE_TYPE_COLUMN, (gint)NET_DICT,
					-1
				);
			}
		} else {
			g_assert_not_reached();
		}
	}
}

GtkTreeModel* DictManageDlg::create_dictmanage_tree_model()
{
	GtkTreeStore *model = gtk_tree_store_new(DICT_MANAGE_COLUMN_NUMBER, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_BOOLEAN, G_TYPE_INT);
	GtkTreeIter group_iter;
	gchar *markup;
	for (std::list<DictManageGroup>::iterator i = gpAppFrame->dictinfo.groups.begin(); i != gpAppFrame->dictinfo.groups.end(); ++i) {
		markup = g_markup_printf_escaped("<span foreground=\"blue\">%s</span>", i->name.c_str());
		gtk_tree_store_append(model, &group_iter, NULL);
		gtk_tree_store_set(
			model, &group_iter,
			DICT_MANAGE_MARKUP_COLUMN, markup,
			DICT_MANAGE_AUTHOR_COLUMN, i->name.c_str(),
			DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
			DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)0,
			DICT_MANAGE_EDITABLE_COLUMN, TRUE,
			-1
		);
		g_free(markup);
		create_dict_item_model(model, &group_iter, i->querydict, true);
		create_dict_item_model(model, &group_iter, i->scandict, false);
	}
	return GTK_TREE_MODEL(model);
}

class GetInfo {
public:
	/* istreedict_:
	 * 		DictInfoType_TreeDict if true
	 * 		DictInfoType_NormDict if false
	 * */
	GetInfo(GtkListStore *model_, bool istreedict_):
		model(model_), istreedict(istreedict_) {}
	void operator()(const std::string& url, bool disable) {
		DictInfo dictinfo;
		if (dictinfo.load_from_ifo_file(url.c_str(),
			istreedict ? DictInfoType_TreeDict : DictInfoType_NormDict)) {
			GtkTreeIter iter;
			gtk_list_store_append(model, &iter);
			gchar *wc = g_strdup_printf("%d", dictinfo.get_wordcount());
			if(istreedict) {
				gtk_list_store_set(
					model, &iter,
					TREEDICT_ENABLED_COLUMN, (!disable),
					TREEDICT_NAME_COLUMN, dictinfo.get_bookname().c_str(),
					TREEDICT_WORD_COUNT_COLUMN, wc,
					TREEDICT_AUTHOR_COLUMN, dictinfo.get_author().c_str(),
					TREEDICT_EMAIL_COLUMN, dictinfo.get_email().c_str(),
					TREEDICT_WEB_SITE_COLUMN, dictinfo.get_website().c_str(),
					TREEDICT_DESCRIPTION_COLUMN, dictinfo.get_description().c_str(),
					TREEDICT_DATE_COLUMN, dictinfo.get_date().c_str(),
					TREEDICT_ID_COLUMN, url.c_str(),
					-1
				);
			} else {
				gtk_list_store_set(
					model, &iter,
					DICTLIST_TYPE_COLUMN, (gint)LOCAL_DICT,
					DICTLIST_NAME_COLUMN, dictinfo.get_bookname().c_str(),
					DICTLIST_WORD_COUNT_COLUMN, wc,
					DICTLIST_AUTHOR_COLUMN, dictinfo.get_author().c_str(),
					DICTLIST_EMAIL_COLUMN, dictinfo.get_email().c_str(),
					DICTLIST_WEB_SITE_COLUMN, dictinfo.get_website().c_str(),
					DICTLIST_DESCRIPTION_COLUMN, dictinfo.get_description().c_str(),
					DICTLIST_DATE_COLUMN, dictinfo.get_date().c_str(),
					DICTLIST_ID_COLUMN, url.c_str(),
					DICTLIST_ID_TYPE_STR_COLUMN, _("Local"),
					-1
				);
			}
			g_free(wc);
		}
	}
private:
	GtkListStore *model;
	bool istreedict;
};

GtkTreeModel* DictManageDlg::create_tree_model(TDictTree dicttree)
{
	GtkListStore *model = NULL;
	if (dicttree == DictTree_TreeDict) {
		model = gtk_list_store_new(TREEDICT_COLUMN_NUMBER,
			G_TYPE_BOOLEAN, // 0 - TREEDICT_ENABLED_COLUMN
			G_TYPE_STRING, // 1 - TREEDICT_NAME_COLUMN
			G_TYPE_STRING, // 2 - TREEDICT_WORD_COUNT_COLUMN
			G_TYPE_STRING, // 3 - TREEDICT_AUTHOR_COLUMN
			G_TYPE_STRING, // 4 - TREEDICT_EMAIL_COLUMN
			G_TYPE_STRING, // 5 - TREEDICT_WEB_SITE_COLUMN
			G_TYPE_STRING, // 6 - TREEDICT_DESCRIPTION_COLUMN
			G_TYPE_STRING, // 7 - TREEDICT_DATE_COLUMN
			G_TYPE_STRING // 8 - TREEDICT_ID_COLUMN
		);
		const std::list<std::string>& treedict_order_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_order_list");
		const std::list<std::string>& treedict_disable_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list");
		const std::list<std::string>& treedict_dirs_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_dirs_list");
#ifdef _WIN32
		std::list<std::string> treedict_order_list_abs;
		std::list<std::string> treedict_disable_list_abs;
		std::list<std::string> treedict_dirs_list_abs;
		abs_path_to_data_dir(treedict_order_list, treedict_order_list_abs);
		abs_path_to_data_dir(treedict_disable_list, treedict_disable_list_abs);
		abs_path_to_data_dir(treedict_dirs_list, treedict_dirs_list_abs);
		for_each_file(treedict_dirs_list_abs, ".ifo",
				treedict_order_list_abs,
				treedict_disable_list_abs,
				GetInfo(model, true));
#else
		for_each_file(treedict_dirs_list, ".ifo",
				treedict_order_list,
				treedict_disable_list,
				GetInfo(model, true));
#endif
	} else if (dicttree == DictTree_DictList) {
		model = gtk_list_store_new(DICTLIST_COLUMN_NUMBER,
			G_TYPE_INT, // 0 - DICTLIST_TYPE_COLUMN
			G_TYPE_STRING, // 1 - DICTLIST_NAME_COLUMN
			G_TYPE_STRING, // 2 - DICTLIST_WORD_COUNT_COLUMN
			G_TYPE_STRING, // 3 - DICTLIST_AUTHOR_COLUMN
			G_TYPE_STRING, // 4 - DICTLIST_EMAIL_COLUMN
			G_TYPE_STRING, // 5 - DICTLIST_WEB_SITE_COLUMN
			G_TYPE_STRING, // 6 - DICTLIST_DESCRIPTION_COLUMN
			G_TYPE_STRING, // 7 - DICTLIST_DATE_COLUMN
			G_TYPE_STRING, // 8 - DICTLIST_ID_COLUMN
			G_TYPE_STRING // 9 - DICTLIST_ID_TYPE_STR_COLUMN
		);
		std::list<std::string> dict_disable_list;
#ifdef _WIN32
		std::list<std::string> dict_order_list;
		std::list<std::string> dict_dirs_list;
		{
			const std::list<std::string>& dict_order_list_rel
				= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
			const std::list<std::string>& dict_dirs_list_rel
				= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
			abs_path_to_data_dir(dict_order_list_rel, dict_order_list);
			abs_path_to_data_dir(dict_dirs_list_rel, dict_dirs_list);
		}
#else
		const std::list<std::string>& dict_order_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list");
		const std::list<std::string>& dict_dirs_list
			= conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
#endif
		for_each_file_restricted(
			dict_dirs_list, ".ifo",
			dict_order_list,
			dict_disable_list, GetInfo(model, false));
		size_t n = gpAppFrame->oStarDictPlugins->VirtualDictPlugins.ndicts();
		const char *dictname, *dictid;
		GtkTreeIter iter;
		for (size_t i = 0; i < n; i++) {
			dictname = gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(i);
			dictid = gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_id(i);
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(
				model, &iter,
				DICTLIST_TYPE_COLUMN, (gint)VIRTUAL_DICT,
				DICTLIST_NAME_COLUMN, dictname,
				DICTLIST_WORD_COUNT_COLUMN, "∞",
				DICTLIST_AUTHOR_COLUMN, "",
				DICTLIST_EMAIL_COLUMN, "",
				DICTLIST_WEB_SITE_COLUMN, "",
				DICTLIST_DESCRIPTION_COLUMN, _("Virtual Dictionary"),
				DICTLIST_DATE_COLUMN, "",
				DICTLIST_ID_COLUMN, dictid,
				DICTLIST_ID_TYPE_STR_COLUMN, _("Virtual"),
				-1
			);
		}
		n = gpAppFrame->oStarDictPlugins->NetDictPlugins.ndicts();
		for (size_t i = 0; i < n; i++) {
			dictname = gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(i);
			dictid = gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_id(i);
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(
				model, &iter,
				DICTLIST_TYPE_COLUMN, (gint)NET_DICT,
				DICTLIST_NAME_COLUMN, dictname,
				DICTLIST_WORD_COUNT_COLUMN, "∞",
				DICTLIST_AUTHOR_COLUMN, "",
				DICTLIST_EMAIL_COLUMN, "",
				DICTLIST_WEB_SITE_COLUMN, "",
				DICTLIST_DESCRIPTION_COLUMN, _("Network Dictionary"),
				DICTLIST_DATE_COLUMN, "",
				DICTLIST_ID_COLUMN, dictid,
				DICTLIST_ID_TYPE_STR_COLUMN, _("Network"),
				-1
			);
		}
	} else if(dicttree == DictTree_NetworkDict) {
		model = gtk_list_store_new(NETWORKDICT_COLUMN_NUMBER,
			G_TYPE_STRING,
			G_TYPE_STRING,
			G_TYPE_STRING
		);
	}
	return GTK_TREE_MODEL(model);
}

void DictManageDlg::on_dictmanage_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->dictmanage_tree_model;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gboolean enable;
	gtk_tree_model_get (model, &iter, DICT_MANAGE_ENABLE_COLUMN, &enable, -1);
	enable = !enable;
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, DICT_MANAGE_ENABLE_COLUMN, enable, -1);
	gtk_tree_path_free (path);
	oDictManageDlg->dictmanage_config_changed = true;
}

void DictManageDlg::on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->treedict_tree_model;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean enable;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, TREEDICT_ENABLED_COLUMN, &enable, -1);

	enable = !enable;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, TREEDICT_ENABLED_COLUMN, enable, -1);

	gtk_tree_path_free (path);

	gboolean have_iter;
	gchar *filename;
	std::list<std::string> disable_list;
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, TREEDICT_ENABLED_COLUMN, &enable, -1);
		if (!enable) {
			gtk_tree_model_get (model, &iter, TREEDICT_ID_COLUMN, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
#ifdef _WIN32
	{
		std::list<std::string> disable_list_rel;
		rel_path_to_data_dir(disable_list, disable_list_rel);
		std::swap(disable_list, disable_list_rel);
	}
#endif
	conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list", disable_list);
}

void DictManageDlg::drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg)
{
	if (gtk_selection_data_get_target(data) == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE)) {
		GtkTreeRowReference *ref;
		GtkTreePath *source_row;

		ref = (GtkTreeRowReference *)g_object_get_data(G_OBJECT(ctx), "gtk-tree-view-source-row");
		source_row = gtk_tree_row_reference_get_path(ref);

		if (source_row == NULL)
			return;

		GtkTreeIter iter;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button)))
			gtk_tree_model_get_iter(oDictManageDlg->dict_list_tree_model, &iter, source_row);
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->manage_button)))
			gtk_tree_model_get_iter(oDictManageDlg->dictmanage_tree_model, &iter, source_row);
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->tree_dict_button)))
			gtk_tree_model_get_iter(oDictManageDlg->treedict_tree_model, &iter, source_row);
		else
			gtk_tree_model_get_iter(oDictManageDlg->network_tree_model, &iter, source_row);

		gtk_selection_data_set(data, gdk_atom_intern("STARDICT_DICTMANAGE", FALSE), 8, (const guchar *)&iter, sizeof(iter));

		gtk_tree_path_free(source_row);
	}
}

void DictManageDlg::dictmanage_drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg)
{
	if (gtk_selection_data_get_target(sd) == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE) && gtk_selection_data_get_data(sd)) {
		GtkTreePath *path = NULL;
		GtkTreeViewDropPosition position;

		GtkTreeIter drag_iter;
		memcpy(&drag_iter, gtk_selection_data_get_data(sd), sizeof(drag_iter));

		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &position)) {

			GtkTreeIter drop_iter;
			
			GtkTreeModel *model = oDictManageDlg->dictmanage_tree_model;
			gtk_tree_model_get_iter(model, &drop_iter, path);

			gint drop_type;
			gtk_tree_model_get (model, &drop_iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &drop_type, -1);
			gint drag_type;
			gtk_tree_model_get (model, &drag_iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &drag_type, -1);
			if (drop_type != drag_type || drop_type == 1) {
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			if (drop_type == 2) {
				GtkTreeIter drag_parent;
				gtk_tree_model_iter_parent(model, &drag_parent, &drag_iter);
				GtkTreePath* drag_parent_path = gtk_tree_model_get_path(model, &drag_parent);
				GtkTreeIter drop_parent;
				gtk_tree_model_iter_parent(model, &drop_parent, &drop_iter);
				GtkTreePath* drop_parent_path = gtk_tree_model_get_path(model, &drop_parent);
				if (gtk_tree_path_compare(drag_parent_path, drop_parent_path)!=0) {
					gtk_tree_path_free(drag_parent_path);
					gtk_tree_path_free(drop_parent_path);
					gtk_drag_finish (ctx, FALSE, FALSE, t);
					return;
				}
				gtk_tree_path_free(drag_parent_path);
				gtk_tree_path_free(drop_parent_path);
			}

			switch (position) {
				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_tree_store_move_after(GTK_TREE_STORE(model), &drag_iter, &drop_iter);
					break;

				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_tree_store_move_before(GTK_TREE_STORE(model), &drag_iter, &drop_iter);
					break;
				default:
					gtk_drag_finish (ctx, FALSE, FALSE, t);
					return;
			}
			oDictManageDlg->dictmanage_config_changed = true;
			gtk_drag_finish (ctx, TRUE, FALSE, t);
		}
	}
}

void DictManageDlg::drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg)
{
	if (gtk_selection_data_get_target(sd) == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE) && gtk_selection_data_get_data(sd)) {
		GtkTreePath *path = NULL;
		GtkTreeViewDropPosition position;

		GtkTreeIter drag_iter;
		memcpy(&drag_iter, gtk_selection_data_get_data(sd), sizeof(drag_iter));

		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &position)) {

			GtkTreeIter iter;
			
			GtkTreeModel *model;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button)))
				model = oDictManageDlg->dict_list_tree_model;
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->tree_dict_button)))
				model = oDictManageDlg->treedict_tree_model;
			else
				model = oDictManageDlg->network_tree_model;
			
			gtk_tree_model_get_iter(model, &iter, path);

			switch (position) {
				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_list_store_move_after(GTK_LIST_STORE(model), &drag_iter, &iter);
					break;

				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_list_store_move_before(GTK_LIST_STORE(model), &drag_iter, &iter);
					break;
				default:
					gtk_drag_finish (ctx, FALSE, FALSE, t);
					return;
			}
			if (model == oDictManageDlg->dict_list_tree_model)
				oDictManageDlg->dictmanage_list_changed = true;
			else if (model == oDictManageDlg->network_tree_model)
				oDictManageDlg->network_dictmask_changed = true;
			else
				oDictManageDlg->write_treedict_order_list();
			gtk_drag_finish (ctx, TRUE, FALSE, t);
		}
	}
}

gboolean DictManageDlg::on_network_treeview_button_press(GtkWidget * widget, GdkEventButton * event, DictManageDlg *oDictManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gchar *uid;
			gtk_tree_model_get (model, &iter, NETWORKDICT_ID_COLUMN, &uid, -1);
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DICT_INFO, uid);
			if (!gpAppFrame->oStarDictClient.try_cache(c))
				gpAppFrame->oStarDictClient.send_commands(1, c);
			g_free(uid);
		}
		return true;
	} else {
		return false;
	}
}

void DictManageDlg::show_dict_info()
{
	GtkWidget *treeview;
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == 0) {
		treeview = dict_list_treeview;
	} else if (gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook)) == 1) {
		treeview = dictmanage_treeview;
	} else {
		treeview = treedict_treeview;
	}
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		if (treeview == dictmanage_treeview) {
			gint type;
			gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
			if (type == 0) {
				GtkWidget *dialog = gtk_message_dialog_new(
					GTK_WINDOW(window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("You can have multiple groups of dictionaries, such as \"Study English\", \"Learn German\", and switch them instantly.")
				);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				return;
			} else if (type == 1) {
				GtkWidget *dialog = gtk_message_dialog_new(
					GTK_WINDOW(window),
					GTK_DIALOG_MODAL,
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK,
					_("The Query Dict will show in the main window, the Scan Dict will show in the floating window.")
				);
				gtk_dialog_run (GTK_DIALOG (dialog));
				gtk_widget_destroy (dialog);
				return;
			}
		}
		gchar *dictname, *wordcount, *author, *email, *website, *description, *date, *filename;
		if(treeview == dict_list_treeview) {
			gtk_tree_model_get (
				model, &iter,
				DICTLIST_NAME_COLUMN, &dictname,
				DICTLIST_WORD_COUNT_COLUMN, &wordcount,
				DICTLIST_AUTHOR_COLUMN, &author,
				DICTLIST_EMAIL_COLUMN, &email,
				DICTLIST_WEB_SITE_COLUMN, &website,
				DICTLIST_DESCRIPTION_COLUMN, &description,
				DICTLIST_DATE_COLUMN, &date,
				DICTLIST_ID_COLUMN, &filename,
				-1
			);
		} else if (treeview == treedict_treeview) {
			gtk_tree_model_get (
				model, &iter,
				TREEDICT_NAME_COLUMN, &dictname,
				TREEDICT_WORD_COUNT_COLUMN, &wordcount,
				TREEDICT_AUTHOR_COLUMN, &author,
				TREEDICT_EMAIL_COLUMN, &email,
				TREEDICT_WEB_SITE_COLUMN, &website,
				TREEDICT_DESCRIPTION_COLUMN, &description,
				TREEDICT_DATE_COLUMN, &date,
				TREEDICT_ID_COLUMN, &filename,
				-1
			);
		} else if (treeview == dictmanage_treeview) {
			gtk_tree_model_get (
				model, &iter,
				DICT_MANAGE_MARKUP_COLUMN, &dictname,
				DICT_MANAGE_WORD_COUNT_COLUMN, &wordcount,
				DICT_MANAGE_AUTHOR_COLUMN, &author,
				DICT_MANAGE_EMAIL_COLUMN, &email,
				DICT_MANAGE_WEB_SITE_COLUMN, &website,
				DICT_MANAGE_DESCRIPTION_COLUMN, &description,
				DICT_MANAGE_DATE_COLUMN, &date,
				DICT_MANAGE_ID_COLUMN, &filename,
				-1
			);
		}
		GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Dictionary Information"), 
			GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, 
			GTK_STOCK_CLOSE, GTK_RESPONSE_NONE, NULL);
		gtk_window_set_default_size (GTK_WINDOW(dialog), 400, 400);
		GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_container_set_border_width (GTK_CONTAINER (scrolled_window), 5);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),scrolled_window,TRUE,TRUE,0);
		GtkWidget *label = gtk_label_new(NULL);
		gchar *markup;
		markup = g_markup_printf_escaped (
			"<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s",
			_("Dictionary Name"), dictname,
			_("Word count"), wordcount,
			_("Author"), author,
			_("Email"), email,
			_("Website"), website,
			_("Description"), description,
			_("Date"), date,
			_("File name"), filename);
		g_free(dictname);
		g_free(wordcount);
		g_free(author);
		g_free(email);
		g_free(website);
		g_free(description);
		g_free(date);
		g_free(filename);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free (markup);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
		gtk_misc_set_alignment(GTK_MISC(label), 0., 0.);
		gtk_misc_set_padding(GTK_MISC(label), 5, 5);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), label);
		g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_widget_show_all(dialog);
	}
}

gboolean DictManageDlg::on_dictlist_treeview_button_press(GtkWidget * widget, GdkEventButton * event, DictManageDlg *oDictManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS && event->button == 1) {
		oDictManageDlg->show_dict_info();
		return true;
	} else if (event->button == 3) {
		gtk_menu_popup(GTK_MENU(oDictManageDlg->popup_menu1), NULL, NULL, NULL, NULL, event->button, event->time);
		return false; //So it can be selected.
	} else {
		return false;
	}
}

gboolean DictManageDlg::on_dicttree_and_manage_treeview_button_press(GtkWidget * widget, GdkEventButton * event, DictManageDlg *oDictManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS && event->button == 1) {
		oDictManageDlg->show_dict_info();
		return true;
	}else if (event->button == 3) {
		gtk_menu_popup(GTK_MENU(oDictManageDlg->popup_menu), NULL, NULL, NULL, NULL, event->button, event->time);
		return false; //So it can be selected.
	} else {
		return false;
	}
}

GtkWidget *DictManageDlg::create_dict_tree(TDictTree dicttree)
{
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 350, 230);

	GtkTreeModel *now_tree_model = create_tree_model (dicttree);
	if (dicttree == DictTree_DictList)
		dict_list_tree_model = now_tree_model;
	else if (dicttree == DictTree_TreeDict)
		treedict_tree_model = now_tree_model;
	else if(dicttree == DictTree_NetworkDict)
		network_tree_model = now_tree_model;

	GtkWidget *now_treeview = gtk_tree_view_new_with_model (now_tree_model);
	if (dicttree == DictTree_DictList) {
		g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
			G_CALLBACK(on_dictlist_treeview_button_press), this);
	} else if (dicttree == DictTree_TreeDict) {
		g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
			G_CALLBACK(on_dicttree_and_manage_treeview_button_press), this);
	} else if (dicttree == DictTree_NetworkDict) {
		g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
			G_CALLBACK(on_network_treeview_button_press), this);
	}
	if (dicttree == DictTree_DictList)
		dict_list_treeview = now_treeview;
	else if (dicttree == DictTree_TreeDict)
		treedict_treeview = now_treeview;
	else if (dicttree == DictTree_NetworkDict)
		network_treeview = now_treeview;
	g_object_unref (G_OBJECT (now_tree_model));
#if GTK_MAJOR_VERSION >= 3
#else
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (now_treeview), TRUE);
#endif
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	if (dicttree==DictTree_TreeDict) {
		renderer = gtk_cell_renderer_toggle_new ();
		g_signal_connect (renderer, "toggled", G_CALLBACK (on_treedict_enable_toggled), this);
		column = gtk_tree_view_column_new_with_attributes (_("Enable"), renderer, "active", TREEDICT_ENABLED_COLUMN, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
		gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	}
	
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	glong store_column;
	if (dicttree == DictTree_DictList)
		store_column = DICTLIST_NAME_COLUMN;
	else if (dicttree == DictTree_TreeDict)
		store_column = TREEDICT_NAME_COLUMN;
	else if (dicttree == DictTree_NetworkDict)
		store_column = NETWORKDICT_NAME_COLUMN;
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", store_column, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, store_column);
	if(dicttree == DictTree_DictList)
		g_signal_connect(column, "clicked", G_CALLBACK(on_dict_list_dict_name_column_clicked), this);
	else if(dicttree == DictTree_TreeDict)
		g_signal_connect(column, "clicked", G_CALLBACK(on_treedict_dict_name_column_clicked), this);
	else if(dicttree == DictTree_NetworkDict)
		g_signal_connect(column, "clicked", G_CALLBACK(on_network_dict_name_column_clicked), this);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	if (dicttree == DictTree_DictList)
		store_column = DICTLIST_WORD_COUNT_COLUMN;
	else if (dicttree == DictTree_TreeDict)
		store_column = TREEDICT_WORD_COUNT_COLUMN;
	else if (dicttree == DictTree_NetworkDict)
		store_column = NETWORKDICT_WORD_COUNT_COLUMN;
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer, "text", store_column, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	if(dicttree == DictTree_DictList)
	{
		renderer = gtk_cell_renderer_text_new ();
		g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
		column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer, "text", DICTLIST_ID_TYPE_STR_COLUMN, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
		gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	}

	GtkTargetEntry gte[] = {{(gchar *)"STARDICT_DICTMANAGE", GTK_TARGET_SAME_APP, 0}};
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(now_treeview), GDK_BUTTON1_MASK, gte, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(now_treeview), gte, 1, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE));

	g_signal_connect(G_OBJECT(now_treeview), "drag-data-received", G_CALLBACK(drag_data_received_cb), this);
	g_signal_connect(G_OBJECT(now_treeview), "drag-data-get", G_CALLBACK(drag_data_get_cb), this);

	gtk_container_add (GTK_CONTAINER (sw), now_treeview);
	return sw;
}

void DictManageDlg::on_group_name_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, DictManageDlg *oDictManageDlg)
{
	if (*new_text) {
		GtkTreeModel *model = oDictManageDlg->dictmanage_tree_model;
		GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
		GtkTreeIter iter;
		gtk_tree_model_get_iter (model, &iter, path);
		gchar *markup = g_markup_printf_escaped("<span foreground=\"blue\">%s</span>", new_text);
		gtk_tree_store_set(GTK_TREE_STORE(model), &iter,
			DICT_MANAGE_MARKUP_COLUMN, markup,
			DICT_MANAGE_AUTHOR_COLUMN, new_text,
		-1);
		g_free(markup);
		gtk_tree_path_free (path);
		oDictManageDlg->dictmanage_config_changed = true;
	}
}

GtkWidget *DictManageDlg::create_dictmanage_tree()
{	
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 350, 230);

	dictmanage_tree_model = create_dictmanage_tree_model ();

	dictmanage_treeview = gtk_tree_view_new_with_model (dictmanage_tree_model);
	g_signal_connect(G_OBJECT(dictmanage_treeview), "button_press_event",
		G_CALLBACK(on_dicttree_and_manage_treeview_button_press), this);
	g_object_unref (G_OBJECT (dictmanage_tree_model));
#if GTK_MAJOR_VERSION >= 3
#else
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dictmanage_treeview), TRUE);
#endif
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dictmanage_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (on_dictmanage_enable_toggled), this);
	column = gtk_tree_view_column_new_with_attributes (
		_("Enable"), renderer,
		"active", DICT_MANAGE_ENABLE_COLUMN,
		"visible", DICT_MANAGE_SHOW_DETAILS_COLUMN,
		NULL
	);
	gtk_tree_view_append_column (GTK_TREE_VIEW(dictmanage_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (
		_("Dictionary Name"), renderer,
		"markup", DICT_MANAGE_MARKUP_COLUMN,
		"editable", DICT_MANAGE_EDITABLE_COLUMN,
		NULL
	);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	g_signal_connect (renderer, "edited", G_CALLBACK (on_group_name_cell_edited), this);
	gtk_tree_view_append_column (GTK_TREE_VIEW(dictmanage_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (
		_("Word count"), renderer,
		"text", DICT_MANAGE_WORD_COUNT_COLUMN,
		"visible", DICT_MANAGE_SHOW_DETAILS_COLUMN,
		NULL
	);
	gtk_tree_view_append_column (GTK_TREE_VIEW(dictmanage_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	GtkTargetEntry gte[] = {{(gchar *)"STARDICT_DICTMANAGE", GTK_TARGET_SAME_APP, 0}};
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(dictmanage_treeview), GDK_BUTTON1_MASK, gte, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(dictmanage_treeview), gte, 1, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE));

	g_signal_connect(G_OBJECT(dictmanage_treeview), "drag-data-received", G_CALLBACK(dictmanage_drag_data_received_cb), this);
	g_signal_connect(G_OBJECT(dictmanage_treeview), "drag-data-get", G_CALLBACK(drag_data_get_cb), this);

	gtk_tree_view_expand_all(GTK_TREE_VIEW (dictmanage_treeview));
	gtk_container_add (GTK_CONTAINER (sw), dictmanage_treeview);
	return sw;
}

void DictManageDlg::write_treedict_order_list()
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar *filename;
	std::list<std::string> order_list;
	
	have_iter = gtk_tree_model_get_iter_first(treedict_tree_model, &iter);
	while (have_iter) {
		gtk_tree_model_get (treedict_tree_model, &iter, TREEDICT_ID_COLUMN, &filename, -1);
		order_list.push_back(filename);
		g_free(filename);
		have_iter = gtk_tree_model_iter_next(treedict_tree_model, &iter);
	}

#ifdef _WIN32
	{
		std::list<std::string> order_list_rel;
		rel_path_to_data_dir(order_list, order_list_rel);
		std::swap(order_list, order_list_rel);
	}
#endif
	conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_order_list", order_list);
}

void DictManageDlg::ChangeNetworkDictMask()
{
	GtkTreeIter iter;
	std::string dictmask;
	if (gtk_tree_model_get_iter_first(network_tree_model, &iter)) {
		gchar *uid;
		gtk_tree_model_get (network_tree_model, &iter, NETWORKDICT_ID_COLUMN, &uid, -1);
		dictmask = uid;
		g_free(uid);
		while (gtk_tree_model_iter_next(network_tree_model, &iter)) {
			gtk_tree_model_get (network_tree_model, &iter, NETWORKDICT_ID_COLUMN, &uid, -1);
			dictmask += ' ';
			dictmask += uid;
			g_free(uid);
		}
	}
	STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_SET_DICT_MASK, dictmask.c_str());
	gpAppFrame->oStarDictClient.send_commands(1, c);
}

static void process_dictmanage_type_iter(GtkTreeModel *model, GtkTreeIter *parent_iter, std::string &configxml)
{
	GtkTreeIter iter;
	gboolean have_next = gtk_tree_model_iter_children(model, &iter, parent_iter);
	while (have_next) {
		gboolean enable;
		DictManageItemType dicttype;
		gint intdicttype;
		gchar *file;
		gtk_tree_model_get (
			model, &iter,
			DICT_MANAGE_ENABLE_COLUMN, &enable,
			DICT_MANAGE_ID_COLUMN, &file,
			DICT_MANAGE_TYPE_COLUMN, &intdicttype,
			-1
		);
		dicttype = (DictManageItemType)intdicttype;
		if (dicttype == LOCAL_DICT) {
			configxml += "<localdict enable=\"";
			if (enable)
				configxml += "true";
			else
				configxml += "false";
#ifdef _WIN32
			gchar *estr = g_markup_escape_text(rel_path_to_data_dir(file).c_str(), -1);
#else
			gchar *estr = g_markup_escape_text(file, -1);
#endif
			g_free(file);
			configxml += "\" file=\"";
			configxml += estr;
			configxml += "\"/>";
			g_free(estr);
		} else if (dicttype == VIRTUAL_DICT) {
			configxml += "<virtualdict enable=\"";
			if (enable)
				configxml += "true";
			else
				configxml += "false";
#ifdef _WIN32
			gchar *estr = g_markup_escape_text(rel_path_to_data_dir(file).c_str(), -1);
#else
			gchar *estr = g_markup_escape_text(file, -1);
#endif
			g_free(file);
			configxml += "\" id=\"";
			configxml += estr;
			configxml += "\"/>";
			g_free(estr);
		} else if (dicttype == NET_DICT) {
			configxml += "<netdict enable=\"";
			if (enable)
				configxml += "true";
			else
				configxml += "false";
#ifdef _WIN32
			gchar *estr = g_markup_escape_text(rel_path_to_data_dir(file).c_str(), -1);
#else
			gchar *estr = g_markup_escape_text(file, -1);
#endif
			g_free(file);
			configxml += "\" id=\"";
			configxml += estr;
			configxml += "\"/>";
			g_free(estr);
		} else {
			g_assert_not_reached();
		}
		have_next = gtk_tree_model_iter_next(model, &iter);
	}
}

static void process_dictmanage_group_iter(GtkTreeModel *model, GtkTreeIter *parent_iter, std::string &configxml)
{
	GtkTreeIter iter;
	gboolean have_next = gtk_tree_model_iter_children(model, &iter, parent_iter);
	while (have_next) {
		gchar *type_name;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_AUTHOR_COLUMN, &type_name, -1);
		configxml += "<";
		configxml += type_name;
		configxml += ">";
		process_dictmanage_type_iter(model, &iter, configxml);
		configxml += "</";
		configxml += type_name;
		configxml += ">";
		g_free(type_name);
		have_next = gtk_tree_model_iter_next(model, &iter);
	}
}

void DictManageDlg::SaveDictManageConfig()
{
	std::string configxml;
	GtkTreeIter iter;
	gboolean have_next = gtk_tree_model_get_iter_first(dictmanage_tree_model, &iter);
	while (have_next) {
		gchar *groupname;
		gtk_tree_model_get (dictmanage_tree_model, &iter, DICT_MANAGE_AUTHOR_COLUMN, &groupname, -1);
		gchar *estr = g_markup_escape_text(groupname, -1);
		g_free(groupname);
		configxml += "<dictgroup name=\"";
		configxml += estr;
		configxml += "\">";
		g_free(estr);
		process_dictmanage_group_iter(dictmanage_tree_model, &iter, configxml);
		configxml += "</dictgroup>";
		have_next = gtk_tree_model_iter_next(dictmanage_tree_model, &iter);
	}
	conf->set_string("/apps/stardict/manage_dictionaries/dict_config_xml", configxml);
	LoadDictInfo();
}

void DictManageDlg::SaveDictManageList()
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar *filename;
	DictManageItemType dicttype;
	std::list<std::string> order_list;
	
	have_iter = gtk_tree_model_get_iter_first(dict_list_tree_model, &iter);
	while (have_iter) {
		gint intdicttype;
		gtk_tree_model_get (dict_list_tree_model, &iter, DICTLIST_TYPE_COLUMN, &intdicttype, -1);
		dicttype = (DictManageItemType)intdicttype;
		if (dicttype == LOCAL_DICT) {
			gtk_tree_model_get (dict_list_tree_model, &iter, DICTLIST_ID_COLUMN, &filename, -1);
			order_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(dict_list_tree_model, &iter);
	}
#ifdef _WIN32
	{
		std::list<std::string> order_list_rel;
		rel_path_to_data_dir(order_list, order_list_rel);
		std::swap(order_list, order_list_rel);
	}
#endif
	conf->set_strlist("/apps/stardict/manage_dictionaries/dict_order_list", order_list);
}

void DictManageDlg::on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button));
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_list_treeview;
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* first_path = gtk_tree_path_new_first();
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_compare(first_path, now_path)!=0) {
			gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, NULL);
			
			gtk_tree_selection_select_path(selection, first_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), first_path, NULL, false, 0, 0);
			if (istreedict)
				oDictManageDlg->write_treedict_order_list();
			else
				oDictManageDlg->dictmanage_list_changed = true;
		}
		gtk_tree_path_free(first_path);
		gtk_tree_path_free(now_path);
	}
}

void DictManageDlg::on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button));
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_list_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreeIter tmp,last;
		tmp = last = iter;
		while (gtk_tree_model_iter_next(model, &tmp))
			last = tmp;
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		GtkTreePath* last_path = gtk_tree_model_get_path(model, &last);
		if (gtk_tree_path_compare(last_path, now_path)!=0) {
			gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, &last);
			
			gtk_tree_selection_select_path(selection, last_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), last_path, NULL, false, 0, 0);
			if (istreedict)
				oDictManageDlg->write_treedict_order_list();
			else
				oDictManageDlg->dictmanage_list_changed = true;
		}
		gtk_tree_path_free(last_path);
		gtk_tree_path_free(now_path);
	}	
}

void DictManageDlg::on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	TDictTree dicttree;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button)))
		dicttree = DictTree_DictList;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->tree_dict_button)))
		dicttree = DictTree_TreeDict;
	else
		dicttree = DictTree_NetworkDict;
	GtkWidget *now_treeview;
	if (dicttree == DictTree_TreeDict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else if (dicttree == DictTree_DictList)
		now_treeview = oDictManageDlg->dict_list_treeview;
	else
		now_treeview = oDictManageDlg->network_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter prev;
			gtk_tree_model_get_iter(model, &prev, path);
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &prev);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			if (dicttree == DictTree_NetworkDict)
				oDictManageDlg->network_dictmask_changed = true;
			else if (dicttree == DictTree_DictList)
				oDictManageDlg->dictmanage_list_changed = true;
			else
				oDictManageDlg->write_treedict_order_list();
		}
		gtk_tree_path_free(path);
	}	
}

void DictManageDlg::on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	TDictTree dicttree;
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->dict_list_button)))
		dicttree = DictTree_DictList;
	else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->tree_dict_button)))
		dicttree = DictTree_TreeDict;
	else
		dicttree = DictTree_NetworkDict;
	GtkWidget *now_treeview;
	if (dicttree == DictTree_TreeDict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else if (dicttree == DictTree_DictList)
		now_treeview = oDictManageDlg->dict_list_treeview;
	else
		now_treeview = oDictManageDlg->network_treeview;

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_next(path);
		GtkTreeIter next;
		if (gtk_tree_model_get_iter(model, &next, path)) {
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &next);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			if (dicttree == DictTree_NetworkDict)
				oDictManageDlg->network_dictmask_changed = true;
			else if (dicttree == DictTree_DictList)
				oDictManageDlg->dictmanage_list_changed = true;
			else
				oDictManageDlg->write_treedict_order_list();
		}
		gtk_tree_path_free(path);
	}
}

void DictManageDlg::show_delete_group_dialog(GtkTreeIter *iter)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(window),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			_("Are you sure you want to delete this group of dictionaries?")
	);
	gint response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_YES) {
		gtk_tree_store_remove(GTK_TREE_STORE(dictmanage_tree_model), iter);
		dictmanage_config_changed = true;
	}
	gtk_widget_destroy (dialog);
}

void DictManageDlg::show_delete_subgroup_dialog(GtkTreeIter *first)
{
	GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(window),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_YES_NO,
			_("Are you sure you want to delete this sub-group of dictionaries?")
	);
	gint response = gtk_dialog_run (GTK_DIALOG (dialog));
	if (response == GTK_RESPONSE_YES) {
		while (true) {
			if (!gtk_tree_store_remove(GTK_TREE_STORE(dictmanage_tree_model), first))
				break;
		}
		dictmanage_config_changed = true;
	}
	gtk_widget_destroy (dialog);
}

static void on_add_group_entry_activated(GtkEntry *entry, GtkDialog *dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
}

void DictManageDlg::show_add_group_dialog(GtkTreeIter *sibling)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("New dict group"), GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),hbox,false,false,0);
	GtkWidget *label = gtk_label_new(_("Group name:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	GtkWidget *entry = gtk_entry_new();
	g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(on_add_group_entry_activated), dialog);
	gtk_box_pack_start(GTK_BOX(hbox),entry,false,false,0);
	gtk_widget_show_all(hbox);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		const char *name = gtk_entry_get_text(GTK_ENTRY(entry));
		if (name[0] != '\0') {
			GtkTreeIter group_iter;
			GtkTreeStore *model = GTK_TREE_STORE(dictmanage_tree_model);
			gtk_tree_store_insert_after(model, &group_iter, NULL, sibling);
			gchar *markup = g_markup_printf_escaped("<span foreground=\"blue\">%s</span>", name);
			gtk_tree_store_set(
				model, &group_iter,
				DICT_MANAGE_MARKUP_COLUMN, markup,
				DICT_MANAGE_AUTHOR_COLUMN, name,
				DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
				DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)0,
				DICT_MANAGE_EDITABLE_COLUMN, TRUE,
				-1
			);
			g_free(markup);
			GtkTreeIter type_iter;
			gtk_tree_store_append(model, &type_iter, &group_iter);
			gtk_tree_store_set(
				model, &type_iter,
				DICT_MANAGE_MARKUP_COLUMN, _("<span foreground=\"red\">Query Dict</span>"),
				DICT_MANAGE_AUTHOR_COLUMN, "querydict",
				DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
				DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)1,
				DICT_MANAGE_EDITABLE_COLUMN, FALSE,
				-1
			);
			gtk_tree_store_append(model, &type_iter, &group_iter);
			gtk_tree_store_set(
				model, &type_iter,
				DICT_MANAGE_MARKUP_COLUMN, _("<span foreground=\"red\">Scan Dict</span>"),
				DICT_MANAGE_AUTHOR_COLUMN, "scandict",
				DICT_MANAGE_SHOW_DETAILS_COLUMN, false,
				DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)1,
				DICT_MANAGE_EDITABLE_COLUMN, FALSE,
				-1
			);
			dictmanage_config_changed = true;
		}
	}
	gtk_widget_destroy(dialog);
}

static gboolean on_add_dict_dialog_treeview_button_press(GtkWidget * widget, GdkEventButton * event, GtkDialog *dialog)
{
	if (event->type==GDK_2BUTTON_PRESS && event->button == 1) {
		gtk_dialog_response(dialog, GTK_RESPONSE_ACCEPT);
		return true;
	}
	return false;
}

void DictManageDlg::show_add_dict_dialog(GtkTreeIter *parent_iter)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
		_("Add dictionary"),
		GTK_WINDOW(window),
		GTK_DIALOG_MODAL,
		GTK_STOCK_OK,
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_REJECT,
		NULL
	);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),vbox,TRUE,TRUE,0);
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox),sw,TRUE,TRUE,0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, 350, 230);
	GtkListStore *now_tree_model = gtk_list_store_new(
		ADD_DICT_COLUMN_NUMBER,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING,
		G_TYPE_STRING
	);
	std::list<std::string> added_dictlist;
	GtkTreeIter iter;
	gboolean have_next = gtk_tree_model_iter_children(dictmanage_tree_model, &iter, parent_iter);
	while (have_next) {
		gchar *file;
		gtk_tree_model_get (dictmanage_tree_model, &iter, DICT_MANAGE_ID_COLUMN, &file, -1);
		added_dictlist.push_back(file);
		g_free(file);
		have_next = gtk_tree_model_iter_next(dictmanage_tree_model, &iter);
	}
	have_next = gtk_tree_model_get_iter_first(dict_list_tree_model, &iter);
	while (have_next) {
		gchar *file;
		gtk_tree_model_get (dict_list_tree_model, &iter, DICTLIST_ID_COLUMN, &file, -1);
		bool added = false;
		for (std::list<std::string>::iterator i = added_dictlist.begin(); i != added_dictlist.end(); ++i) {
			if (*i == file) {
				added = true;
				break;
			}
		}
		if (!added) {
			DictManageItemType dicttype;
			gint intdicttype;
			gchar *bookname, *wordcount, *author, *email, *website, *description, *date, *type_str;
			gtk_tree_model_get (
				dict_list_tree_model, &iter,
				DICTLIST_TYPE_COLUMN, &intdicttype,
				DICTLIST_NAME_COLUMN, &bookname,
				DICTLIST_WORD_COUNT_COLUMN, &wordcount,
				DICTLIST_AUTHOR_COLUMN, &author,
				DICTLIST_EMAIL_COLUMN, &email,
				DICTLIST_WEB_SITE_COLUMN, &website,
				DICTLIST_DESCRIPTION_COLUMN, &description,
				DICTLIST_DATE_COLUMN, &date,
				DICTLIST_ID_TYPE_STR_COLUMN, &type_str,
				-1
			);
			dicttype = (DictManageItemType)intdicttype;
			GtkTreeIter new_iter;
			gtk_list_store_append(now_tree_model, &new_iter);
			gtk_list_store_set(
				now_tree_model, &new_iter,
				ADD_DICT_TYPE_COLUMN, (gint)dicttype,
				ADD_DICT_NAME_COLUMN, bookname,
				ADD_DICT_WORD_COUNT_COLUMN, wordcount,
				ADD_DICT_AUTHOR_COLUMN, author,
				ADD_DICT_EMAIL_COLUMN, email,
				ADD_DICT_WEB_SITE_COLUMN, website,
				ADD_DICT_DESCRIPTION_COLUMN, description,
				ADD_DICT_DATE_COLUMN, date,
				ADD_DICT_ID_COLUMN, file,
				ADD_DICT_TYPE_STR_COLUMN, type_str,
				-1
			);
			g_free(bookname);
			g_free(wordcount);
			g_free(author);
			g_free(email);
			g_free(website);
			g_free(description);
			g_free(date);
			g_free(type_str);
		}
		g_free(file);
		have_next = gtk_tree_model_iter_next(dict_list_tree_model, &iter);
	}
	GtkWidget *now_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(now_tree_model));
	g_object_unref (G_OBJECT (now_tree_model));
	g_signal_connect (G_OBJECT (now_treeview), "button_press_event", G_CALLBACK (on_add_dict_dialog_treeview_button_press), dialog);
#if GTK_MAJOR_VERSION >= 3
#else
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (now_treeview), TRUE);
#endif

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", ADD_DICT_NAME_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, ADD_DICT_NAME_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer, "text", ADD_DICT_WORD_COUNT_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer, "text", ADD_DICT_TYPE_STR_COLUMN, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Path"), renderer, "text", ADD_DICT_ID_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, ADD_DICT_ID_COLUMN);
	gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);

	gtk_container_add (GTK_CONTAINER (sw), now_treeview);
	gtk_widget_show_all(vbox);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_ACCEPT) {
		GList *selectlist = gtk_tree_selection_get_selected_rows(selection, NULL);
		if (selectlist) {
			GList *list = selectlist;
			GtkTreeIter select_iter;
			while (list) {
				gtk_tree_model_get_iter(GTK_TREE_MODEL(now_tree_model), &select_iter, (GtkTreePath *)(list->data));
				DictManageItemType dicttype;
				gint intdicttype;
				gchar *bookname, *wordcount, *author, *email, *website, *description, *date, *file;
				gtk_tree_model_get (
					GTK_TREE_MODEL(now_tree_model), &select_iter,
					ADD_DICT_TYPE_COLUMN, &intdicttype,
					ADD_DICT_NAME_COLUMN, &bookname,
					ADD_DICT_WORD_COUNT_COLUMN, &wordcount,
					ADD_DICT_AUTHOR_COLUMN, &author,
					ADD_DICT_EMAIL_COLUMN, &email,
					ADD_DICT_WEB_SITE_COLUMN, &website,
					ADD_DICT_DESCRIPTION_COLUMN, &description,
					ADD_DICT_DATE_COLUMN, &date,
					ADD_DICT_ID_COLUMN, &file,
					-1
				);
				dicttype = (DictManageItemType)intdicttype;
				GtkTreeIter new_iter;
				gtk_tree_store_insert_before(GTK_TREE_STORE(dictmanage_tree_model), &new_iter, parent_iter, NULL);
				gtk_tree_store_set(
					GTK_TREE_STORE(dictmanage_tree_model), &new_iter,
					DICT_MANAGE_ENABLE_COLUMN, TRUE,
					DICT_MANAGE_MARKUP_COLUMN, bookname,
					DICT_MANAGE_WORD_COUNT_COLUMN, wordcount,
					DICT_MANAGE_AUTHOR_COLUMN, author,
					DICT_MANAGE_EMAIL_COLUMN, email,
					DICT_MANAGE_WEB_SITE_COLUMN, website,
					DICT_MANAGE_DESCRIPTION_COLUMN, description,
					DICT_MANAGE_DATE_COLUMN, date,
					DICT_MANAGE_ID_COLUMN, file,
					DICT_MANAGE_SHOW_DETAILS_COLUMN, true,
					DICT_MANAGE_ROW_LEVEL_COLUMN, (gint)2,
					DICT_MANAGE_EDITABLE_COLUMN, FALSE,
					DICT_MANAGE_TYPE_COLUMN, (gint)dicttype,
					-1
				);
				g_free(bookname);
				g_free(wordcount);
				g_free(author);
				g_free(email);
				g_free(website);
				g_free(description);
				g_free(date);
				g_free(file);
				list = list->next;
			}
			g_list_foreach (selectlist, (GFunc)gtk_tree_path_free, NULL);
			g_list_free (selectlist);
			dictmanage_config_changed = true;
		}
	}
	gtk_widget_destroy(dialog);
}

void DictManageDlg::on_dictmanage_add_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oDictManageDlg->dictmanage_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (
			oDictManageDlg->dictmanage_tree_model, &iter,
			DICT_MANAGE_ROW_LEVEL_COLUMN, &type,
			-1
		);
		if (type == 0) {
			oDictManageDlg->show_add_group_dialog(&iter);
		} else if (type == 1) {
			oDictManageDlg->show_add_dict_dialog(&iter);
		} else {
			GtkTreeIter parent_iter;
			if (gtk_tree_model_iter_parent(oDictManageDlg->dictmanage_tree_model, &parent_iter, &iter))
				oDictManageDlg->show_add_dict_dialog(&parent_iter);
		}
	}
}

void DictManageDlg::on_dictmanage_delete_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oDictManageDlg->dictmanage_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
		if (type == 0) {
			if (gtk_tree_model_iter_n_children(model, NULL) > 1)
				oDictManageDlg->show_delete_group_dialog(&iter);
		} else if (type == 1) {
			GtkTreeIter first;
			if (gtk_tree_model_iter_children(model, &first, &iter)) {
				oDictManageDlg->show_delete_subgroup_dialog(&first);
			}
		} else if (type == 2) {
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			oDictManageDlg->dictmanage_config_changed = true;
		}
	}
}

void DictManageDlg::on_dictmanage_info_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	oDictManageDlg->show_dict_info();
}

void DictManageDlg::on_dictmanage_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
		if (type == 1)
			return;
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		GtkTreePath* first_path;
		if (type == 0) {
			first_path = gtk_tree_path_new_first();
		} else {
			GtkTreeIter parent;
			gtk_tree_model_iter_parent(model, &parent, &iter);
			GtkTreeIter first;
			gtk_tree_model_iter_children(model, &first, &parent);
			first_path = gtk_tree_model_get_path(model, &first);
		}
		if (gtk_tree_path_compare(first_path, now_path)!=0) {
			gtk_tree_store_move_after(GTK_TREE_STORE(model), &iter, NULL);
			gtk_tree_selection_select_path(selection, first_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), first_path, NULL, false, 0, 0);
			oDictManageDlg->dictmanage_config_changed = true;
		}
		gtk_tree_path_free(first_path);
		gtk_tree_path_free(now_path);
	}
}

void DictManageDlg::on_dictmanage_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
		if (type == 1)
			return;
		GtkTreePath* now_path = gtk_tree_model_get_path(model, &iter);
		GtkTreePath* last_path;
		GtkTreeIter last;
		if (type == 0) {
			gint n = gtk_tree_model_iter_n_children(model, NULL);
			gtk_tree_model_iter_nth_child(model, &last, NULL, n-1);
		} else {
			GtkTreeIter parent;
			gtk_tree_model_iter_parent(model, &parent, &iter);
			gint n = gtk_tree_model_iter_n_children(model, &parent);
			gtk_tree_model_iter_nth_child(model, &last, &parent, n-1);
		}
		last_path = gtk_tree_model_get_path(model, &last);
		if (gtk_tree_path_compare(last_path, now_path)!=0) {
			gtk_tree_store_move_after(GTK_TREE_STORE(model), &iter, &last);
			gtk_tree_selection_select_path(selection, last_path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), last_path, NULL, false, 0, 0);
			oDictManageDlg->dictmanage_config_changed = true;
		}
		gtk_tree_path_free(last_path);
		gtk_tree_path_free(now_path);
	}
}

void DictManageDlg::on_dictmanage_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
		if (type == 1)
			return;
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter prev;
			gtk_tree_model_get_iter(model, &prev, path);
			gtk_tree_store_swap(GTK_TREE_STORE(model), &iter, &prev);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			oDictManageDlg->dictmanage_config_changed = true;
		}
		gtk_tree_path_free(path);
	}
}

void DictManageDlg::on_dictmanage_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{  
	GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gint type;
		gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
		if (type == 1)
			return;
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_next(path);
		GtkTreeIter next;
		if (gtk_tree_model_get_iter(model, &next, path)) {
			gtk_tree_store_swap(GTK_TREE_STORE(model), &iter, &next);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (now_treeview), path, NULL, false, 0, 0);
			oDictManageDlg->dictmanage_config_changed = true;
		}
		gtk_tree_path_free(path);
	}
}

void DictManageDlg::on_network_add_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	if (oDictManageDlg->network_add_dlg)
		return;
	oDictManageDlg->network_add_dlg = new NetworkAddDlg(oDictManageDlg);
	oDictManageDlg->network_add_dlg->Show(GTK_WINDOW(oDictManageDlg->window));
	delete oDictManageDlg->network_add_dlg;
	oDictManageDlg->network_add_dlg = NULL;
}

void DictManageDlg::on_network_remove_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oDictManageDlg->network_treeview));
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
		gtk_list_store_remove(GTK_LIST_STORE(oDictManageDlg->network_tree_model), &iter);
		oDictManageDlg->network_dictmask_changed = true;
	}
}

void DictManageDlg::on_popup_menu_show_info_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg)
{
	oDictManageDlg->show_dict_info();
}

void DictManageDlg::on_popup_menu_select_all_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg)
{
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook)) == 1) {
		GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
		GtkTreeModel *model;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gint type;
			gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
			if (type == 0)
				return;
			GtkTreeIter first;
			if (type == 1) {
				if (!gtk_tree_model_iter_children(model, &first, &iter))
					return;
			} else {
				GtkTreeIter parent;
				gtk_tree_model_iter_parent(model, &parent, &iter);
				gtk_tree_model_iter_children(model, &first, &parent);
			}
			gboolean have_iter = true;
			while (have_iter) {
				gtk_tree_store_set (GTK_TREE_STORE (model), &first, DICT_MANAGE_ENABLE_COLUMN, TRUE, -1);
				have_iter = gtk_tree_model_iter_next(model, &first);
			}
			oDictManageDlg->dictmanage_config_changed = true;
		}
	} else if (gtk_notebook_get_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook)) == 2) {
		GtkTreeModel *model = oDictManageDlg->treedict_tree_model;
		gboolean have_iter;
		GtkTreeIter iter;
		have_iter = gtk_tree_model_get_iter_first(model, &iter);
		while (have_iter) {
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, TREEDICT_ENABLED_COLUMN, TRUE, -1);
			have_iter = gtk_tree_model_iter_next(model, &iter);
		}
		std::list<std::string> disable_list;
		conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list", disable_list);
	}
}

void DictManageDlg::on_popup_menu_unselect_all_activate(GtkMenuItem *menuitem, DictManageDlg *oDictManageDlg)
{
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook)) == 1) {
		GtkWidget *now_treeview = oDictManageDlg->dictmanage_treeview;
		GtkTreeSelection *selection;
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));
		GtkTreeModel *model;
		GtkTreeIter iter;
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gint type;
			gtk_tree_model_get (model, &iter, DICT_MANAGE_ROW_LEVEL_COLUMN, &type, -1);
			if (type == 0)
				return;
			GtkTreeIter first;
			if (type == 1) {
				if (!gtk_tree_model_iter_children(model, &first, &iter))
					return;
			} else {
				GtkTreeIter parent;
				gtk_tree_model_iter_parent(model, &parent, &iter);
				gtk_tree_model_iter_children(model, &first, &parent);
			}
			gboolean have_iter = true;
			while (have_iter) {
				gtk_tree_store_set (GTK_TREE_STORE (model), &first, DICT_MANAGE_ENABLE_COLUMN, FALSE, -1);
				have_iter = gtk_tree_model_iter_next(model, &first);
			}
			oDictManageDlg->dictmanage_config_changed = true;
		}
	} else if (gtk_notebook_get_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook)) == 2) {
		GtkTreeModel *model = oDictManageDlg->treedict_tree_model;
		gboolean have_iter;
		GtkTreeIter iter;
		have_iter = gtk_tree_model_get_iter_first(model, &iter);
		gchar *filename;
		std::list<std::string> disable_list;
		while (have_iter) {
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, TREEDICT_ENABLED_COLUMN, FALSE, -1);
			gtk_tree_model_get (model, &iter, TREEDICT_ID_COLUMN, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
			have_iter = gtk_tree_model_iter_next(model, &iter);
		}
#ifdef _WIN32
		{
			std::list<std::string> disable_list_rel;
			rel_path_to_data_dir(disable_list, disable_list_rel);
			std::swap(disable_list, disable_list_rel);
		}
#endif
		conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list", disable_list);
	}
}

GtkWidget *DictManageDlg::create_buttons()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#else
	vbox = gtk_vbox_new(false,2);
#endif
#else
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox = gtk_vbox_new(false,6);
#endif
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_TOP);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_top_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_BOTTOM);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_bottom_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_info_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return vbox;
}

GtkWidget *DictManageDlg::create_dictmanage_buttons()
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *image;
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	hbox = gtk_hbox_new(false, 6);
	vbox = gtk_vbox_new(false,6);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Add"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_add_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Delete"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_delete_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Information"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_info_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox = gtk_vbox_new(false,6);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_TOP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Move to top"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_move_top_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Move up"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Move down"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_BOTTOM, GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_widget_set_tooltip_text(button,_("Move to bottom"));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_dictmanage_move_bottom_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return hbox;
}

GtkWidget *DictManageDlg::create_network_buttons()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#else
	vbox = gtk_vbox_new(false,2);
#endif
#else
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox = gtk_vbox_new(false,6);
#endif
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_add_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_remove_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return vbox;
}

void DictManageDlg::on_download_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, DictManageDlg *oDictManageDlg)
{
	show_url("http://stardict-4.sourceforge.net");
}

void DictManageDlg::on_upgrade_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, DictManageDlg *oDictManageDlg)
{
	if (oDictManageDlg->upgrade_url.empty()) {
		show_url("http://www.stardict.net/finance.php");
	} else {
		show_url(oDictManageDlg->upgrade_url.c_str());
	}
}

void DictManageDlg::on_dict_list_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg)
{
	oDictManageDlg->dictmanage_list_changed = true;
}

void DictManageDlg::on_treedict_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg)
{
	oDictManageDlg->write_treedict_order_list();
}

void DictManageDlg::on_network_dict_name_column_clicked(GtkTreeViewColumn *treeviewcolumn, DictManageDlg *oDictManageDlg)
{
	oDictManageDlg->network_dictmask_changed = true;
}

bool DictManageDlg::ShowModal(bool &dictmanage_config_changed_)
{
	window = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(window), parent_win);

	gtk_dialog_add_button(GTK_DIALOG(window),
			GTK_STOCK_HELP,
			GTK_RESPONSE_HELP);
	
	gtk_dialog_add_button (GTK_DIALOG (window),
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response (GTK_DIALOG (window), GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(window), "response",
			G_CALLBACK(response_handler), this);
		
	GtkWidget *vbox;
#ifdef CONFIG_GPE
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_vbox_new (FALSE, 2);
#else
	vbox = gtk_vbox_new(false,2);
#endif
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
#else
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox = gtk_vbox_new(false,6);
#endif
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
#endif
		
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 3);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),hbox, false, false, 0);
		
	dict_list_button = gtk_radio_button_new(NULL);
	gtk_widget_set_can_focus (dict_list_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(dict_list_button), false);
	gtk_box_pack_start (GTK_BOX (hbox), dict_list_button, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	GtkWidget *hbox1 = gtk_hbox_new(false, 2);
#endif
	gtk_container_add (GTK_CONTAINER (dict_list_button), hbox1);
	GtkWidget *image = gtk_image_new_from_pixbuf(dicts_icon);
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	GtkWidget *label = gtk_label_new_with_mnemonic(_("D_ict List"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), dict_list_button);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dict_list_button), true);
	g_signal_connect(G_OBJECT(dict_list_button),"toggled", G_CALLBACK(on_dict_list_button_toggled), this);
		
	manage_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(dict_list_button));
	gtk_widget_set_can_focus (manage_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(manage_button), false);
	gtk_box_pack_start (GTK_BOX (hbox), manage_button, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	hbox1 = gtk_hbox_new(false, 2);
#endif
	gtk_container_add (GTK_CONTAINER (manage_button), hbox1);
	image = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("Manage _Dict"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), manage_button);
	g_signal_connect(G_OBJECT(manage_button),"toggled", G_CALLBACK(on_manage_button_toggled), this);

	tree_dict_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(manage_button));
	gtk_widget_set_can_focus (tree_dict_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(tree_dict_button), false);
	gtk_box_pack_start (GTK_BOX (hbox), tree_dict_button, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	hbox1 = gtk_hbox_new(false, 2);
#endif
	gtk_container_add (GTK_CONTAINER (tree_dict_button), hbox1);
	image = gtk_image_new_from_pixbuf(tree_dicts_icon);
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("T_ree dictionaries"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), tree_dict_button);
	g_signal_connect(G_OBJECT(tree_dict_button),"toggled", G_CALLBACK(on_tree_dict_button_toggled), this);
		
	GtkWidget *network_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(tree_dict_button));
	gtk_widget_set_can_focus (network_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(network_button), false);
	gtk_box_pack_start (GTK_BOX (hbox), network_button, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	hbox1 = gtk_hbox_new(false, 2);
#endif
	gtk_container_add (GTK_CONTAINER (network_button), hbox1);
	image = gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Network dictionaries"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), network_button);
	g_signal_connect(G_OBJECT(network_button),"toggled", G_CALLBACK(on_network_button_toggled), this);
#ifdef CONFIG_GPE
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
#else
	hbox = gtk_hbox_new (FALSE, 2);
#endif
#else
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 18);
#else
	hbox = gtk_hbox_new (FALSE, 18);
#endif
#endif
	gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);
		
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(hbox),notebook, true, true, 0);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), false);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), false);
		
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(DictTree_DictList), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dictmanage_tree(), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(DictTree_TreeDict), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(DictTree_NetworkDict), NULL);
		
	GtkWidget *menuitem;

	popup_menu1 = gtk_menu_new();
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Show information"));
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_popup_menu_show_info_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu1), menuitem);
	gtk_widget_show_all(popup_menu1);

	popup_menu = gtk_menu_new();
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Show information"));
	image = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_popup_menu_show_info_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("Select _All"));
	image = gtk_image_new_from_stock(GTK_STOCK_SELECT_ALL, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_popup_menu_select_all_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Unselect all"));
	image = gtk_image_new_from_stock(GTK_STOCK_CANCEL, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_popup_menu_unselect_all_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(popup_menu), menuitem);
	gtk_widget_show_all(popup_menu);

	button_notebook = gtk_notebook_new();
	gtk_box_pack_start (GTK_BOX (hbox), button_notebook, false, false, 0);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(button_notebook), false);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(button_notebook), false);
	gtk_notebook_append_page(GTK_NOTEBOOK(button_notebook), create_buttons(), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(button_notebook), create_dictmanage_buttons(), NULL);
	gtk_notebook_append_page(GTK_NOTEBOOK(button_notebook), create_network_buttons(), NULL);
		
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox = gtk_hbox_new (FALSE, 6);
#endif
	gtk_box_pack_start (GTK_BOX (vbox), hbox, false, false, 0);
#if GTK_MAJOR_VERSION >= 3
	download_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	download_hbox = gtk_hbox_new(FALSE, 0);
#endif
	gtk_box_pack_start (GTK_BOX (hbox), download_hbox, FALSE, FALSE, 0);
	label = gtk_label_new(_("Visit "));
	gtk_box_pack_start (GTK_BOX (download_hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<span foreground=\"blue\" underline=\"single\">http://stardict-4.sourceforge.net</span>");
	GtkWidget *download_eventbox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(download_eventbox),"button-release-event", G_CALLBACK(on_download_eventbox_clicked), this);
	gtk_container_add(GTK_CONTAINER(download_eventbox), label);
	gtk_box_pack_start (GTK_BOX (download_hbox), download_eventbox, FALSE, FALSE, 0);
	label = gtk_label_new(_(" to download dictionaries!"));
	gtk_box_pack_start (GTK_BOX (download_hbox), label, FALSE, FALSE, 0);

	info_label = gtk_label_new (NULL);
	gtk_label_set_justify (GTK_LABEL (info_label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (info_label), "xalign", 0.0, NULL);
	gtk_box_pack_start (GTK_BOX (hbox), info_label, FALSE, FALSE, 0);

	upgrade_label = gtk_label_new(NULL);
	gchar *markup = g_markup_printf_escaped("<span foreground=\"blue\" underline=\"single\">%s</span>", _("Upgrade Now!"));
	gtk_label_set_markup(GTK_LABEL(upgrade_label), markup);
	g_free(markup);
	upgrade_eventbox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(upgrade_eventbox),"button-release-event", G_CALLBACK(on_upgrade_eventbox_clicked), this);
	gtk_container_add(GTK_CONTAINER(upgrade_eventbox), upgrade_label);
	gtk_box_pack_start (GTK_BOX (hbox), upgrade_eventbox, FALSE, FALSE, 0);
		
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (window))), vbox, true, true, 0);
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG (window)));

	GdkCursor* cursor = gdk_cursor_new(GDK_HAND2);
	gtk_widget_realize(download_eventbox);
	gdk_window_set_cursor(gtk_widget_get_window(download_eventbox), cursor);
	gtk_widget_realize(upgrade_eventbox);
	gdk_window_set_cursor(gtk_widget_get_window(upgrade_eventbox), cursor);
#if GTK_MAJOR_VERSION >= 3
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif

	gtk_widget_hide(info_label);
	gtk_widget_hide(upgrade_eventbox);
	
	gtk_window_set_title(GTK_WINDOW (window), _("Manage Dictionaries"));

	max_dict_count = -1;
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(this->notebook)) == 3) {
		STARDICT::Cmd *c1 = new STARDICT::Cmd(STARDICT::CMD_GET_DICT_MASK);
		STARDICT::Cmd *c2 = new STARDICT::Cmd(STARDICT::CMD_MAX_DICT_COUNT);
		STARDICT::Cmd *c3 = new STARDICT::Cmd(STARDICT::CMD_GET_ADINFO);
		gpAppFrame->oStarDictClient.try_cache_or_send_commands(3, c1, c2, c3);
	}
	network_dictmask_changed = false;
	dictmanage_list_changed = false;
	dictmanage_config_changed = false;
	gint result;
	while ((result = gtk_dialog_run(GTK_DIALOG(window)))==GTK_RESPONSE_HELP)
		;
	if (network_dictmask_changed) {
		ChangeNetworkDictMask();
	}
	if (dictmanage_list_changed) {
		SaveDictManageList();
	}
	if (dictmanage_config_changed) {
		SaveDictManageConfig();
	}
	dictmanage_config_changed_ = dictmanage_config_changed;
	if (result == GTK_RESPONSE_NONE) {
		// Caused by gtk_widget_destroy(), quitting.
		return true;
	} else {
		gtk_widget_destroy(GTK_WIDGET(window));
		window = NULL;
		return false;
	}
}

void DictManageDlg::Close()
{
	if (window) {
		gtk_widget_destroy (window);
		gtk_widget_destroy(popup_menu1);
		gtk_widget_destroy(popup_menu);
		window = NULL;
	}
}

struct dictmask_ParseUserData {
	GtkListStore *model;
	std::string uid;
	std::string bookname;
	std::string wordcount;
};

static void dictmask_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dict")==0) {
		dictmask_ParseUserData *Data = (dictmask_ParseUserData *)user_data;
		Data->uid.clear();
		Data->bookname.clear();
		Data->wordcount.clear();
	}
}

static void dictmask_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dict")==0) {
		dictmask_ParseUserData *Data = (dictmask_ParseUserData *)user_data;
		GtkTreeIter iter;
		gtk_list_store_append(Data->model, &iter);
		gtk_list_store_set(
			Data->model, &iter,
			NETWORKDICT_ID_COLUMN, Data->uid.c_str(),
			NETWORKDICT_NAME_COLUMN, Data->bookname.c_str(),
			NETWORKDICT_WORD_COUNT_COLUMN, Data->wordcount.c_str(),
			-1
		);
	}
}

static void dictmask_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	dictmask_ParseUserData *Data = (dictmask_ParseUserData *)user_data;
	if (strcmp(element, "uid")==0) {
		Data->uid.assign(text, text_len);
	} else if (strcmp(element, "bookname")==0) {
		Data->bookname.assign(text, text_len);
	} else if (strcmp(element, "wordcount")==0) {
		Data->wordcount.assign(text, text_len);
	}
}

void DictManageDlg::network_getdictmask(const char *xml)
{
	if (!window)
		return;
	gtk_list_store_clear(GTK_LIST_STORE(network_tree_model));
	dictmask_ParseUserData Data;
	Data.model = GTK_LIST_STORE(network_tree_model);
	GMarkupParser parser;
	parser.start_element = dictmask_parse_start_element;
	parser.end_element = dictmask_parse_end_element;
	parser.text = dictmask_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, xml, -1, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

struct adinfo_ParseUserData {
	std::string title;
	std::string url;
};

static void adinfo_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	adinfo_ParseUserData *Data = (adinfo_ParseUserData *)user_data;
	if (strcmp(element, "title")==0) {
		Data->title.assign(text, text_len);
	} else if (strcmp(element, "url")==0) {
		Data->url.assign(text, text_len);
	}
}

void DictManageDlg::network_getadinfo(const char *xml)
{
	if (!window)
		return;
	adinfo_ParseUserData Data;
	GMarkupParser parser;
	parser.start_element = NULL;
	parser.end_element = NULL;
	parser.text = adinfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, xml, -1, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);

	gchar *markup = g_markup_printf_escaped("<span foreground=\"blue\" underline=\"single\">%s</span>", Data.title.c_str());
	gtk_label_set_markup(GTK_LABEL(upgrade_label), markup);
	g_free(markup);
	upgrade_url = Data.url;
}

void DictManageDlg::network_dirinfo(const char *xml)
{
	if (network_add_dlg)
		network_add_dlg->network_getdirinfo(xml);
}

struct dictinfo_ParseUserData {
	GtkWindow *parent;
	std::string dictinfo_bookname;
	std::string dictinfo_wordcount;
	std::string dictinfo_synwordcount;
	std::string dictinfo_author;
	std::string dictinfo_email;
	std::string dictinfo_website;
	std::string dictinfo_description;
	std::string dictinfo_date;
	std::string dictinfo_download;
};

static void dictinfo_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dictinfo")==0) {
		dictinfo_ParseUserData *Data = (dictinfo_ParseUserData *)user_data;
		Data->dictinfo_bookname.clear();
		Data->dictinfo_wordcount.clear();
		Data->dictinfo_synwordcount.clear();
		Data->dictinfo_author.clear();
		Data->dictinfo_email.clear();
		Data->dictinfo_website.clear();
		Data->dictinfo_description.clear();
		Data->dictinfo_date.clear();
		Data->dictinfo_download.clear();
	}
}

static void on_download_clicked(GtkWidget *widget, gpointer data)
{
	show_url((const gchar*)g_object_get_data(G_OBJECT(widget), "stardict_download"));
}

static void dictinfo_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dictinfo")==0) {
		dictinfo_ParseUserData *Data = (dictinfo_ParseUserData *)user_data;
		GtkWidget *dialog = gtk_dialog_new_with_buttons (
			_("Dictionary Information"),
			Data->parent,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_NONE,
			NULL
		);
		GtkWidget *label = gtk_label_new(NULL);
		char *markup;
		markup = g_markup_printf_escaped (
			"<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s",
			_("Dictionary Name"), Data->dictinfo_bookname.c_str(),
			_("Word count"), Data->dictinfo_wordcount.c_str(),
			_("Synonym word count"), Data->dictinfo_synwordcount.c_str(),
			_("Author"), Data->dictinfo_author.c_str(),
			_("Email"), Data->dictinfo_email.c_str(),
			_("Website"), Data->dictinfo_website.c_str(),
			_("Description"), Data->dictinfo_description.c_str(),
			_("Date"), Data->dictinfo_date.c_str()
		);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free (markup);
		gtk_label_set_selectable(GTK_LABEL(label), TRUE);
		gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),label,false,false,6);
		if (!Data->dictinfo_download.empty()) {
#if GTK_MAJOR_VERSION >= 3
			GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
			GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
#endif
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),hbox,false,false,6);
			GtkWidget *button = gtk_button_new_with_label(_("Download Now!"));
			g_object_set_data_full(G_OBJECT(button), "stardict_download", g_strdup(Data->dictinfo_download.c_str()), g_free);
			g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_download_clicked), NULL);
			gtk_box_pack_start(GTK_BOX(hbox),button,false,false,6);
		}

		g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_widget_destroy), dialog);
		gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
		gtk_widget_show_all(dialog);
	}
}

static void dictinfo_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	dictinfo_ParseUserData *Data = (dictinfo_ParseUserData *)user_data;
	if (strcmp(element, "bookname")==0) {
		Data->dictinfo_bookname.assign(text, text_len);
	} else if (strcmp(element, "wordcount")==0) {
		Data->dictinfo_wordcount.assign(text, text_len);
	} else if (strcmp(element, "synwordcount")==0) {
		Data->dictinfo_synwordcount.assign(text, text_len);
	} else if (strcmp(element, "author")==0) {
		Data->dictinfo_author.assign(text, text_len);
	} else if (strcmp(element, "email")==0) {
		Data->dictinfo_email.assign(text, text_len);
	} else if (strcmp(element, "website")==0) {
		Data->dictinfo_website.assign(text, text_len);
	} else if (strcmp(element, "description")==0) {
		Data->dictinfo_description.assign(text, text_len);
	} else if (strcmp(element, "date")==0) {
		Data->dictinfo_date.assign(text, text_len);
	} else if (strcmp(element, "download")==0) {
		Data->dictinfo_download.assign(text, text_len);
	}
}

void DictManageDlg::network_dictinfo(const char *xml)
{
	dictinfo_ParseUserData Data;
	if (network_add_dlg)
		Data.parent = GTK_WINDOW(network_add_dlg->window);
	else
		Data.parent = GTK_WINDOW(this->window);
	GMarkupParser parser;
	parser.start_element = dictinfo_parse_start_element;
	parser.end_element = dictinfo_parse_end_element;
	parser.text = dictinfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, xml, -1, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
}

void DictManageDlg::network_maxdictcount(int count)
{
	max_dict_count = count;
	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(this->notebook)) == 3) {
		gchar *str = g_strdup_printf(_("You can only choose %d dictionaries."), count);
		gtk_label_set_text(GTK_LABEL(this->info_label), str);
		g_free(str);
	}
}
