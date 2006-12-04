#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <sys/stat.h>
#include <glib/gi18n.h>
#include <algorithm>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include "stardict.h"
#include "conf.h"
#include "desktop.hpp"
#include "lib/common.hpp"
#include "utils.h"
#include "lib/file.hpp"

#include "dictmanagedlg.h"

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
        gtk_tree_model_get (model, &iter, 1, &visible, -1);
        if (visible) {
            gchar *uid;
            gtk_tree_model_get (model, &iter, 3, &uid, -1);
            bool added = false;
            int row_count = 0;
            GtkTreeIter iter2;
            if (gtk_tree_model_get_iter_first(oNetworkAddDlg->dictdlg->network_tree_model, &iter2)) {
                gchar *uid2;
                do {
                    row_count++;
                    gtk_tree_model_get (oNetworkAddDlg->dictdlg->network_tree_model, &iter2, 0, &uid2, -1);
                    if (strcmp(uid, uid2)==0) {
                        added = true;
                        g_free(uid2);
                        break;
                    }
                    g_free(uid2);
                } while (gtk_tree_model_iter_next(oNetworkAddDlg->dictdlg->network_tree_model, &iter2) == TRUE);
            }
            if (!added) {
                gchar *msg;
                if (row_count < oNetworkAddDlg->dictdlg->max_dict_count) {
                    int need_level;
                    gtk_tree_model_get (model, &iter, 4, &need_level, -1);
                    if (need_level > oNetworkAddDlg->dictdlg->user_level) {
                        msg = g_strdup_printf(_("Only level %d user can choose this dictionary!"), need_level);
                    } else {
                        msg = NULL;
                        gchar *bookname;
                        glong wordcount;
                        gtk_tree_model_get (model, &iter, 0, &bookname, 2, &wordcount, -1);
                        gtk_list_store_append(GTK_LIST_STORE(oNetworkAddDlg->dictdlg->network_tree_model), &iter2);
                        gtk_list_store_set(GTK_LIST_STORE(oNetworkAddDlg->dictdlg->network_tree_model), &iter2, 0, uid, 1, bookname, 2, wordcount, -1);
                        g_free(bookname);
                        oNetworkAddDlg->dictdlg->dictmask_changed = true;
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
                                msg);
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
        gtk_tree_model_get (model, &iter, 1, &visible, -1);
        if (visible) {
            gchar *uid;
            gtk_tree_model_get (model, &iter, 3, &uid, -1);
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
            gtk_tree_model_get (model, &iter, 1, &visible, -1);
            if (visible) {
                gchar *uid;
                gtk_tree_model_get (model, &iter, 3, &uid, -1);
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
    gtk_tree_model_get (GTK_TREE_MODEL(oNetworkAddDlg->model), &iter, 0, &word, -1);
    if (word[0] == '\0') {
        gchar *path;
        gtk_tree_model_get (GTK_TREE_MODEL(oNetworkAddDlg->model), arg1, 3, &path, -1);
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
	GtkWidget *hbox = gtk_hbox_new(false, 6);
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, 350, 230);
	model = gtk_tree_store_new(5, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_LONG, G_TYPE_STRING, G_TYPE_INT);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	g_object_unref (G_OBJECT (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer, "text", 2, "visible", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
    g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_button_press), this);
    g_signal_connect (G_OBJECT (treeview), "row-expanded", G_CALLBACK (on_row_expanded), this);
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	gtk_box_pack_start (GTK_BOX (hbox), sw, true, true, 0);
	GtkWidget *vbox;
	vbox = gtk_vbox_new(false,6);
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_adddlg_add_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
#ifdef CONFIG_MAEMO
	button = gtk_button_new_from_stock(GTK_STOCK_DIALOG_INFO);
#else
	button = gtk_button_new_from_stock(GTK_STOCK_INFO);
#endif
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_adddlg_info_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, false, false, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), hbox, true, true, 0);
	gtk_widget_show_all(GTK_DIALOG (window)->vbox);
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
        gtk_tree_store_set(Data->model, &iter, 0, (Data->dir_dirname + " (" + Data->dir_dictcount + ')').c_str(), 1, FALSE, 3, (Data->parent + Data->dir_name + '/').c_str(), -1);
        GtkTreeIter iter1;
        gtk_tree_store_append(Data->model, &iter1, &iter);
        gtk_tree_store_set(Data->model, &iter1, 0, "", -1);
    } else if (strcmp(element_name, "dict")==0) {
        dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)user_data;
        Data->in_dict = false;
        GtkTreeIter iter;
        gtk_tree_store_append(Data->model, &iter, Data->iter);
        int need_level;
        if (Data->dict_level.empty())
            need_level = 0;
        else
            need_level = atoi(Data->dict_level.c_str());
        gtk_tree_store_set(Data->model, &iter, 0, Data->dict_bookname.c_str(), 1, TRUE, 2, atol(Data->dict_wordcount.c_str()), 3, Data->dict_uid.c_str(), 4, need_level, -1);
    }
}

static gboolean find_iter(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
    dirinfo_ParseUserData *Data = (dirinfo_ParseUserData *)data;
    gboolean visible;
    gtk_tree_model_get (model, iter, 1, &visible, -1);
    if (visible == FALSE) {
        gchar *path;
        gtk_tree_model_get (model, iter, 3, &path, -1);
        if (path && Data->parent == path) {
            Data->iter = (GtkTreeIter *)g_malloc(sizeof(GtkTreeIter));
            *(Data->iter) = *iter;
            g_free(path);
            return FALSE;
        }
        g_free(path);
    }
    return TRUE;
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
            gtk_tree_model_foreach(GTK_TREE_MODEL(Data->model), find_iter, Data);
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

void DictManageDlg::on_wazard_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 0);
        gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), _("Visit http://stardict.sourceforge.net to download dictionaries!"));
        gtk_widget_hide(oDictManageDlg->upgrade_eventbox);
    }
}

void DictManageDlg::on_appendix_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 1);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 0);
        gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), _("These settings will take effect the next time you run StarDict."));
        gtk_widget_hide(oDictManageDlg->upgrade_eventbox);
    }
}

void DictManageDlg::on_network_button_toggled(GtkToggleButton *button, DictManageDlg *oDictManageDlg)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->notebook), 2);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(oDictManageDlg->button_notebook), 1);
        if (oDictManageDlg->max_dict_count == -1) {
            gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), _("Loading..."));
    	    STARDICT::Cmd *c1 = new STARDICT::Cmd(STARDICT::CMD_GET_DICT_MASK);
            STARDICT::Cmd *c2 = new STARDICT::Cmd(STARDICT::CMD_MAX_DICT_COUNT);
            gpAppFrame->oStarDictClient.try_cache_or_send_commands(2, c1, c2);
        } else {
            gchar *str = g_strdup_printf(_("You can only choose %d dictionaries."), oDictManageDlg->max_dict_count);
            gtk_label_set_text(GTK_LABEL(oDictManageDlg->info_label), str);
            g_free(str);
        }
        gtk_widget_show(oDictManageDlg->upgrade_eventbox);
    }
}

class GetInfo {
public:
	GetInfo(GtkListStore *model_, bool istreedict_):
		model(model_), istreedict(istreedict_) {}
	void operator()(const std::string& url, bool disable) {
		DictInfo dictinfo;
		if (dictinfo.load_from_ifo_file(url.c_str(), istreedict)) {
			GtkTreeIter iter;
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter, 
												 0, !disable, 
												 1, dictinfo.bookname.c_str(), 
												 2, dictinfo.wordcount, 
												 3, dictinfo.author.c_str(), 
												 4, dictinfo.email.c_str(), 
												 5, dictinfo.website.c_str(),
												 6, dictinfo.description.c_str(), 
												 7, dictinfo.date.c_str(), 
												 8, url.c_str(), 
												 -1);
		}
	}
private:
	GtkListStore *model;
	bool istreedict;
};

static GtkTreeModel* __create_dict_tree_model(const std::list<std::string> &dirs,
																							const std::list<std::string>& order_list,
																							const std::list<std::string>& disable_list, bool istreedict)
{
	GtkListStore *model = 
		gtk_list_store_new(9, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_LONG, 
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
					 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	for_each_file(dirs, ".ifo", order_list, disable_list, 
								GetInfo(model, istreedict));
	
	return GTK_TREE_MODEL(model);
}

GtkTreeModel* DictManageDlg::create_dict_tree_model(int istreedict)
{
	if (istreedict == 1) {
		return __create_dict_tree_model(
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_dirs_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_order_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list"),
																		istreedict
																		);
	} else if (istreedict == 0) {
		return __create_dict_tree_model(
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list"),
																		conf->get_strlist("/apps/stardict/manage_dictionaries/dict_disable_list"),
																		istreedict
																		);
	} else {
		GtkListStore *model = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_LONG);
		return GTK_TREE_MODEL(model);
	}
}

void DictManageDlg::on_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->dict_tree_model;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean enable;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &enable, -1);

	enable = !enable;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, enable, -1);

	gtk_tree_path_free (path);

	gboolean have_iter;
	gchar *filename;
	std::list<std::string> disable_list;
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &enable, -1);
		if (!enable) {
			gtk_tree_model_get (model, &iter, 8, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist("/apps/stardict/manage_dictionaries/dict_disable_list", disable_list);
}

void DictManageDlg::on_treedict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, DictManageDlg *oDictManageDlg)
{
	GtkTreeModel *model = oDictManageDlg->treedict_tree_model;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	gboolean enable;

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, 0, &enable, -1);

	enable = !enable;

	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, enable, -1);

	gtk_tree_path_free (path);

	gboolean have_iter;
	gchar *filename;
	std::list<std::string> disable_list;
	
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &enable, -1);
		if (!enable) {
			gtk_tree_model_get (model, &iter, 8, &filename, -1);
			disable_list.push_back(filename);
			g_free(filename);
		}
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_disable_list", disable_list);
}

void DictManageDlg::drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, DictManageDlg *oDictManageDlg)
{
	if (data->target == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE)) {
		GtkTreeRowReference *ref;
		GtkTreePath *source_row;

		ref = (GtkTreeRowReference *)g_object_get_data(G_OBJECT(ctx), "gtk-tree-view-source-row");
		source_row = gtk_tree_row_reference_get_path(ref);

		if (source_row == NULL)
			return;

		GtkTreeIter iter;
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
			gtk_tree_model_get_iter(oDictManageDlg->dict_tree_model, &iter, source_row);
		else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->appendix_button)))
			gtk_tree_model_get_iter(oDictManageDlg->treedict_tree_model, &iter, source_row);
		else
			gtk_tree_model_get_iter(oDictManageDlg->network_tree_model, &iter, source_row);

		gtk_selection_data_set(data, gdk_atom_intern("STARDICT_DICTMANAGE", FALSE), 8, (const guchar *)&iter, sizeof(iter));

		gtk_tree_path_free(source_row);
	}
}

void DictManageDlg::drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, DictManageDlg *oDictManageDlg)
{
	if (sd->target == gdk_atom_intern("STARDICT_DICTMANAGE", FALSE) && sd->data) {
		GtkTreePath *path = NULL;
		GtkTreeViewDropPosition position;

		GtkTreeIter drag_iter;
		memcpy(&drag_iter, sd->data, sizeof(drag_iter));

		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &position)) {

			GtkTreeIter iter;
			
			GtkTreeModel *model;
			if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
				model = oDictManageDlg->dict_tree_model;
			else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->appendix_button)))
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
					//Bug: when move the entry to before the first item, it is moved to the end! But I think this is gtk's bug.
					break;
				default:
					return;
			}
			if (model == oDictManageDlg->network_tree_model)
                oDictManageDlg->dictmask_changed = true;
            else
				oDictManageDlg->write_order_list(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)));
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
            gtk_tree_model_get (model, &iter, 0, &uid, -1);
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

gboolean DictManageDlg::on_treeview_button_press(GtkWidget * widget, GdkEventButton * event, DictManageDlg *oDictManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;

		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
		if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
			gchar *dictname, *author, *email, *website, *description, *date, *filename;
			glong wordcount;
			gtk_tree_model_get (model, &iter, 1, &dictname, 2, &wordcount, 3, &author, 4, &email, 5, &website, 6, &description, 7, &date, 8, &filename, -1);
			GtkWidget *dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW(oDictManageDlg->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				"<b>%s:</b> %s\n<b>%s:</b> %ld\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s\n<b>%s:</b> %s",
				_("Dictionary Name"), dictname,
				_("Word count"), wordcount,
				_("Author"), author,
				_("Email"), email,
				_("Website"), website,
				_("Description"), description,
				_("Date"), date,
				_("File name"), filename);
			g_free(dictname);
			g_free(author);
			g_free(email);
			g_free(website);
			g_free(description);
			g_free(date);
			g_free(filename);
			if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_NONE)
				gtk_widget_destroy (dialog);
		}
		return true;
	} else {
		return false;
	}
}

GtkWidget *DictManageDlg::create_dict_tree(int istreedict)
{	
	GtkWidget *sw;	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
      	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 350, 230);

	GtkTreeModel *now_tree_model = create_dict_tree_model (istreedict);
	if (istreedict == 1)
		treedict_tree_model = now_tree_model;
	else if (istreedict == 0)
		dict_tree_model = now_tree_model;
	else
		network_tree_model = now_tree_model;

	GtkWidget *now_treeview = gtk_tree_view_new_with_model (now_tree_model);
    if (istreedict == 2) {
	    g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
                   G_CALLBACK(on_network_treeview_button_press), this);
    } else {
	    g_signal_connect(G_OBJECT(now_treeview), "button_press_event",
                   G_CALLBACK(on_treeview_button_press), this);
    }
	if (istreedict == 1)
		treedict_treeview = now_treeview;
	else if (istreedict == 0)
		dict_treeview = now_treeview;
	else
		network_treeview = now_treeview;
	g_object_unref (G_OBJECT (now_tree_model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (now_treeview), TRUE);
	
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (now_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	
	if (istreedict!=2) {
		renderer = gtk_cell_renderer_toggle_new ();
		if (istreedict)
			g_signal_connect (renderer, "toggled", G_CALLBACK (on_treedict_enable_toggled), this);
		else
			g_signal_connect (renderer, "toggled", G_CALLBACK (on_dict_enable_toggled), this);
		column = gtk_tree_view_column_new_with_attributes (_("Enable"), renderer, "active", 0, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	}
	
	renderer = gtk_cell_renderer_text_new ();
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  	
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Word count"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	/*renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Author"), renderer, "text", 3, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Email"), renderer, "text", 4, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Website"), renderer, "text", 5, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Description"), renderer, "text", 6, "editable", 9, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("Date"), renderer, "text", 7, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);  
	column = gtk_tree_view_column_new_with_attributes (_("File name"), renderer, "text", 8, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);*/

	GtkTargetEntry gte[] = {{"STARDICT_DICTMANAGE", GTK_TARGET_SAME_APP, 0}};
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(now_treeview), GDK_BUTTON1_MASK, gte, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(now_treeview), gte, 1, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE));

	g_signal_connect(G_OBJECT(now_treeview), "drag-data-received", G_CALLBACK(drag_data_received_cb), this);
	g_signal_connect(G_OBJECT(now_treeview), "drag-data-get", G_CALLBACK(drag_data_get_cb), this);

	gtk_container_add (GTK_CONTAINER (sw), now_treeview);
	return sw;
}

void DictManageDlg::write_order_list(bool istreedict)
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar *filename;
	std::list<std::string> order_list;
	GtkTreeModel *now_tree_model;
	
	if (istreedict)
		now_tree_model = treedict_tree_model;
	else
		now_tree_model = dict_tree_model;
	
	have_iter = gtk_tree_model_get_iter_first(now_tree_model, &iter);
	while (have_iter) {
		gtk_tree_model_get (now_tree_model, &iter, 8, &filename, -1);
		order_list.push_back(filename);
		g_free(filename);
		have_iter = gtk_tree_model_iter_next(now_tree_model, &iter);
	}
	
	if (istreedict)
	  conf->set_strlist("/apps/stardict/manage_dictionaries/treedict_order_list", order_list);
	else
	  conf->set_strlist("/apps/stardict/manage_dictionaries/dict_order_list", order_list);
}

void DictManageDlg::ChangeDictMask()
{
    GtkTreeIter iter;
    std::string dictmask;
    if (gtk_tree_model_get_iter_first(network_tree_model, &iter)) {
        gchar *uid;
        gtk_tree_model_get (network_tree_model, &iter, 0, &uid, -1);
        dictmask = uid;
        g_free(uid);
        while (gtk_tree_model_iter_next(network_tree_model, &iter)) {
            gtk_tree_model_get (network_tree_model, &iter, 0, &uid, -1);
            dictmask += ' ';
            dictmask += uid;
            g_free(uid);
        }
    }
    STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_SET_DICT_MASK, dictmask.c_str());
    gpAppFrame->oStarDictClient.send_commands(1, c);
}

void DictManageDlg::on_move_top_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;
	
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
			oDictManageDlg->write_order_list(istreedict);
		}
		gtk_tree_path_free(first_path);
		gtk_tree_path_free(now_path);
	}
}

void DictManageDlg::on_move_bottom_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
	gboolean istreedict = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button));	
	GtkWidget *now_treeview;
	if (istreedict)
		now_treeview = oDictManageDlg->treedict_treeview;
	else
		now_treeview = oDictManageDlg->dict_treeview;

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
			oDictManageDlg->write_order_list(istreedict);
		}
		gtk_tree_path_free(last_path);
		gtk_tree_path_free(now_path);
	}	
}

void DictManageDlg::on_move_up_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
    int istreedict;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
        istreedict = 0;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->appendix_button)))
        istreedict = 1;
    else
        istreedict = 2;
	GtkWidget *now_treeview;
	if (istreedict == 1)
		now_treeview = oDictManageDlg->treedict_treeview;
	else if (istreedict == 0)
		now_treeview = oDictManageDlg->dict_treeview;
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
			if (istreedict == 2)
                oDictManageDlg->dictmask_changed = true;
            else
                oDictManageDlg->write_order_list(istreedict);
		}		
		gtk_tree_path_free(path);		
	}	
}

void DictManageDlg::on_move_down_button_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
    int istreedict;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->wazard_button)))
        istreedict = 0;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oDictManageDlg->appendix_button)))
        istreedict = 1;
    else
        istreedict = 2;
	GtkWidget *now_treeview;
	if (istreedict == 1)
		now_treeview = oDictManageDlg->treedict_treeview;
	else if (istreedict == 0)
		now_treeview = oDictManageDlg->dict_treeview;
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
            if (istreedict == 2)
                oDictManageDlg->dictmask_changed = true;
            else
    			oDictManageDlg->write_order_list(istreedict);
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
        oDictManageDlg->dictmask_changed = true;
    }
}

GtkWidget *DictManageDlg::create_buttons()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
	vbox = gtk_vbox_new(false,2);
#else
	vbox = gtk_vbox_new(false,6);
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_TOP);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_top_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GOTO_BOTTOM);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_bottom_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return vbox;
}

GtkWidget *DictManageDlg::create_network_buttons()
{
	GtkWidget *vbox;
#ifdef CONFIG_GPE
	vbox = gtk_vbox_new(false,2);
#else
	vbox = gtk_vbox_new(false,6);
#endif
	GtkWidget *button;
	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_add_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_network_remove_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_UP);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_up_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_GO_DOWN);
	GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_move_down_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	return vbox;
}

void DictManageDlg::on_upgrade_eventbox_clicked(GtkWidget *widget, DictManageDlg *oDictManageDlg)
{
    show_url("http://www.stardict.org/finance.php");
}

bool DictManageDlg::Show()
{
	if (!window) {	
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
		vbox = gtk_vbox_new (FALSE, 2);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
#else
		vbox = gtk_vbox_new (FALSE, 6);
		gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
#endif
		
		GtkWidget *hbox = gtk_hbox_new(false, 3);
		gtk_box_pack_start(GTK_BOX(vbox),hbox, false, false, 0);
		
		wazard_button = gtk_radio_button_new(NULL);
		GTK_WIDGET_UNSET_FLAGS (wazard_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(wazard_button), false);
		gtk_box_pack_start (GTK_BOX (hbox), wazard_button, false, false, 0);	
		GtkWidget *hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (wazard_button), hbox1);
		GtkWidget *image = gtk_image_new_from_pixbuf(dicts_icon);
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		GtkWidget *label = gtk_label_new_with_mnemonic(_("D_ictionaries"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), wazard_button);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wazard_button), true);
		g_signal_connect(G_OBJECT(wazard_button),"toggled", G_CALLBACK(on_wazard_button_toggled), this);
		
		appendix_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
		GTK_WIDGET_UNSET_FLAGS (appendix_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(appendix_button), false);
		gtk_box_pack_start (GTK_BOX (hbox), appendix_button, false, false, 0);	
		hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (appendix_button), hbox1);
		image = gtk_image_new_from_pixbuf(tree_dicts_icon);
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		label = gtk_label_new_with_mnemonic(_("T_ree dictionaries"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), appendix_button);		
		g_signal_connect(G_OBJECT(appendix_button),"toggled", G_CALLBACK(on_appendix_button_toggled), this);
		
		GtkWidget *network_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(appendix_button));
		GTK_WIDGET_UNSET_FLAGS (network_button, GTK_CAN_FOCUS);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(network_button), false);
		gtk_box_pack_start (GTK_BOX (hbox), network_button, false, false, 0);	
		hbox1 = gtk_hbox_new(false, 2);
		gtk_container_add (GTK_CONTAINER (network_button), hbox1);
		image = gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_SMALL_TOOLBAR);
		gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
		label = gtk_label_new_with_mnemonic(_("_Network dictionaries"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
		gtk_label_set_mnemonic_widget(GTK_LABEL(label), network_button);		
		g_signal_connect(G_OBJECT(network_button),"toggled", G_CALLBACK(on_network_button_toggled), this);
#ifdef CONFIG_GPE
		hbox = gtk_hbox_new (FALSE, 2);
#else
		hbox = gtk_hbox_new (FALSE, 18);
#endif
		gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);
		
		notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(hbox),notebook, true, true, 0);
		gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), false);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), false);
		
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(0), NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(1), NULL);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_dict_tree(2), NULL);
		

        button_notebook = gtk_notebook_new();
		gtk_box_pack_start (GTK_BOX (hbox), button_notebook, false, false, 0);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(button_notebook), false);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(button_notebook), false);
        gtk_notebook_append_page(GTK_NOTEBOOK(button_notebook), create_buttons(), NULL);
        gtk_notebook_append_page(GTK_NOTEBOOK(button_notebook), create_network_buttons(), NULL);
		
		hbox = gtk_hbox_new (FALSE, 6);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, true, true, 0);
		info_label = gtk_label_new (_("Visit http://stardict.sourceforge.net to download dictionaries!"));
        gtk_label_set_selectable(GTK_LABEL (info_label), TRUE);
		gtk_label_set_justify (GTK_LABEL (info_label), GTK_JUSTIFY_LEFT);
		g_object_set (G_OBJECT (info_label), "xalign", 0.0, NULL);
		gtk_box_pack_start (GTK_BOX (hbox), info_label, FALSE, FALSE, 0);

        label = gtk_label_new(NULL);
        gtk_label_set_markup(GTK_LABEL(label), "<span foreground=\"blue\" underline=\"single\">Upgrade Now!</span>");
        upgrade_eventbox = gtk_event_box_new();
        g_signal_connect(G_OBJECT(upgrade_eventbox),"button-release-event", G_CALLBACK(on_upgrade_eventbox_clicked), this);
        gtk_container_add(GTK_CONTAINER(upgrade_eventbox), label);
        gtk_box_pack_start (GTK_BOX (hbox), upgrade_eventbox, FALSE, FALSE, 0);
		
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox,
												true, true, 0);
		
		
		gtk_widget_show_all(GTK_DIALOG (window)->vbox);

        gtk_widget_realize(upgrade_eventbox);
        GdkCursor* cursor = gdk_cursor_new(GDK_HAND2);
        gdk_window_set_cursor(upgrade_eventbox->window, cursor);
        gdk_cursor_unref(cursor);
        gtk_widget_hide(upgrade_eventbox);
	
		gtk_window_set_title(GTK_WINDOW (window), _("Manage Dictionaries"));	
	}
	max_dict_count = -1;
    	if (gtk_notebook_get_current_page(GTK_NOTEBOOK(this->notebook)) == 2) {
		STARDICT::Cmd *c1 = new STARDICT::Cmd(STARDICT::CMD_GET_DICT_MASK);
		STARDICT::Cmd *c2 = new STARDICT::Cmd(STARDICT::CMD_MAX_DICT_COUNT);
		gpAppFrame->oStarDictClient.try_cache_or_send_commands(2, c1, c2);
	}
    dictmask_changed = false;
	gint result;
	while ((result = gtk_dialog_run(GTK_DIALOG(window)))==GTK_RESPONSE_HELP)
		;
    if (dictmask_changed == true) {
        ChangeDictMask();
    }
	if (result == GTK_RESPONSE_NONE) {
		// Caused by gtk_widget_destroy(), quiting.
		return true;
	} else {
		gtk_widget_hide(GTK_WIDGET(window));
		return false;
	}
}

void DictManageDlg::Close()
{
	if (window) {
		gtk_widget_destroy (window);
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
		gtk_list_store_set(Data->model, &iter, 0, Data->uid.c_str(), 1, Data->bookname.c_str(), 2, atol(Data->wordcount.c_str()), -1);
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
	show_url((const gchar*)g_object_get_data(G_OBJECT(widget), "download"));
}

static void dictinfo_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
    if (strcmp(element_name, "dictinfo")==0) {
        dictinfo_ParseUserData *Data = (dictinfo_ParseUserData *)user_data;
	GtkWidget *dialog = gtk_dialog_new_with_buttons (_("Dictionary Information"), Data->parent, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_NONE, NULL);
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
		    _("Date"), Data->dictinfo_date.c_str());
	gtk_label_set_markup(GTK_LABEL(label), markup);
	g_free (markup);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),label,false,false,6);
	if (!Data->dictinfo_download.empty()) {
		GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),hbox,false,false,6);
		GtkWidget *button = gtk_button_new_with_label("Download Now!");
		g_object_set_data_full(G_OBJECT(button), "download", g_strdup(Data->dictinfo_download.c_str()), g_free);
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
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(this->notebook)) == 2) {
        gchar *str = g_strdup_printf(_("You can only choose %d dictionaries."), count);
        gtk_label_set_text(GTK_LABEL(this->info_label), str);
        g_free(str);
    }
}
