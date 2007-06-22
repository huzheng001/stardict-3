#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "pluginmanagedlg.h"
#include "desktop.hpp"
#include <glib/gi18n.h>
#include "stardict.h"

PluginManageDlg::PluginManageDlg()
{
}

PluginManageDlg::~PluginManageDlg()
{
}

void PluginManageDlg::response_handler (GtkDialog *dialog, gint res_id, PluginManageDlg *oPluginManageDlg)
{
	if (res_id == GTK_RESPONSE_HELP) {
		show_help("stardict-plugins");
	} else if (res_id == STARDICT_RESPONSE_CONFIGURE) {
		GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(oPluginManageDlg->treeview));
		GtkTreeModel *model;
		GtkTreeIter iter;
		if (! gtk_tree_selection_get_selected (selection, &model, &iter))
			return;
		if (!gtk_tree_model_iter_has_child(model, &iter)) {
			gchar *filename;
			StarDictPlugInType plugin_type;
			gtk_tree_model_get (model, &iter, 4, &filename, 5, &plugin_type, -1);
			gpAppFrame->oStarDictPlugins->configure_plugin(filename, plugin_type);
			g_free(filename);
		}
	}
}

static gboolean get_disable_list(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
	if (!gtk_tree_model_iter_has_child(model, iter)) {
		gboolean enable;
		gtk_tree_model_get (model, iter, 1, &enable, -1);
		if (!enable) {
			gchar *filename;
			gtk_tree_model_get (model, iter, 4, &filename, -1);
			std::list<std::string> *disable_list = (std::list<std::string> *)data;
			disable_list->push_back(filename);
			g_free(filename);
		}
	}
	return FALSE;
}

void PluginManageDlg::on_plugin_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, PluginManageDlg *oPluginManageDlg)
{
	GtkTreeModel *model = GTK_TREE_MODEL(oPluginManageDlg->plugin_tree_model);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter  iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_path_free (path);
	gboolean enable;
	gchar *filename;
	StarDictPlugInType plugin_type;
	gboolean can_configure;
	gtk_tree_model_get (model, &iter, 1, &enable, 4, &filename, 5, &plugin_type, 6, &can_configure, -1);
	enable = !enable;
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, 1, enable, -1);
	if (enable) {
		gpAppFrame->oStarDictPlugins->load_plugin(filename);
	} else {
		gpAppFrame->oStarDictPlugins->unload_plugin(filename, plugin_type);
	}
	g_free(filename);
	if (enable)
		gtk_widget_set_sensitive(oPluginManageDlg->pref_button, can_configure);
	else
		gtk_widget_set_sensitive(oPluginManageDlg->pref_button, FALSE);
	if (plugin_type == StarDictPlugInType_VIRTUALDICT) {
		oPluginManageDlg->dict_changed_ = true;
	} else if (plugin_type == StarDictPlugInType_TTS) {
		gpAppFrame->oMidWin.oToolWin.UpdatePronounceMenu();
	}

	std::list<std::string> disable_list;
	gtk_tree_model_foreach(model, get_disable_list, &disable_list);
	conf->set_strlist("/apps/stardict/manage_plugins/plugin_disable_list", disable_list);
}

struct plugininfo_ParseUserData {
	gchar *info_str;
	gchar *detail_str;
	std::string filename;
	std::string name;
	std::string version;
	std::string short_desc;
	std::string long_desc;
	std::string author;
	std::string website;
};

static void plugininfo_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "plugin_info")==0) {
		plugininfo_ParseUserData *Data = (plugininfo_ParseUserData *)user_data;
		Data->name.clear();
		Data->version.clear();
		Data->short_desc.clear();
		Data->long_desc.clear();
		Data->author.clear();
		Data->website.clear();
	}
}

static void plugininfo_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "plugin_info")==0) {
		plugininfo_ParseUserData *Data = (plugininfo_ParseUserData *)user_data;
		Data->info_str = g_markup_printf_escaped("<b>%s</b> %s\n%s", Data->name.c_str(), Data->version.c_str(), Data->short_desc.c_str());
		Data->detail_str = g_markup_printf_escaped(_("%s\n\n<b>Author:</b>\t%s\n<b>Website:</b>\t%s\n<b>Filename:</b>\t%s"), Data->long_desc.c_str(), Data->author.c_str(), Data->website.c_str(), Data->filename.c_str());
	}
}

static void plugininfo_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	plugininfo_ParseUserData *Data = (plugininfo_ParseUserData *)user_data;
	if (strcmp(element, "name")==0) {
		Data->name.assign(text, text_len);
	} else if (strcmp(element, "version")==0) {
		Data->version.assign(text, text_len);
	} else if (strcmp(element, "short_desc")==0) {
		Data->short_desc.assign(text, text_len);
	} else if (strcmp(element, "long_desc")==0) {
		Data->long_desc.assign(text, text_len);
	} else if (strcmp(element, "author")==0) {
		Data->author.assign(text, text_len);
	} else if (strcmp(element, "website")==0) {
		Data->website.assign(text, text_len);
	}
}

static void add_tree_model(GtkTreeStore *tree_model, GtkTreeIter*parent, std::list<StarDictPluginInfo> &infolist)
{
	plugininfo_ParseUserData Data;
	GMarkupParser parser;
	parser.start_element = plugininfo_parse_start_element;
	parser.end_element = plugininfo_parse_end_element;
	parser.text = plugininfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GtkTreeIter iter;
	for (std::list<StarDictPluginInfo>::iterator i = infolist.begin(); i != infolist.end(); ++i) {
		Data.info_str = NULL;
		Data.detail_str = NULL;
		Data.filename = i->filename;
		GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
		g_markup_parse_context_parse(context, i->info_xml.c_str(), -1, NULL);
		g_markup_parse_context_end_parse(context, NULL);
		g_markup_parse_context_free(context);
		gtk_tree_store_append(tree_model, &iter, parent);
		bool loaded = gpAppFrame->oStarDictPlugins->get_loaded(i->filename.c_str());
		gtk_tree_store_set(tree_model, &iter, 0, true, 1, loaded, 2, Data.info_str, 3, Data.detail_str, 4, i->filename.c_str(), 5, i->plugin_type, 6, i->can_configure, -1);
		g_free(Data.info_str);
		g_free(Data.detail_str);
	}
}

static void init_tree_model(GtkTreeStore *tree_model)
{
	std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > > plugin_list;
	gpAppFrame->oStarDictPlugins->get_plugin_list(plugin_list);
	GtkTreeIter iter;
	for (std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > >::iterator i = plugin_list.begin(); i != plugin_list.end(); ++i) {
		switch (i->first) {
			case StarDictPlugInType_VIRTUALDICT:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Virtual Dictionary</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_TTS:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>TTS Engine</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_MISC:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Misc</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			default:
				break;
		}
	}
}

void PluginManageDlg::on_plugin_treeview_selection_changed(GtkTreeSelection *selection, PluginManageDlg *oPluginManageDlg)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (! gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	if (gtk_tree_model_iter_has_child(model, &iter)) {
		gtk_widget_set_sensitive(oPluginManageDlg->pref_button, FALSE);
	} else {
		gboolean loaded;
		gchar *detail;
		gboolean can_configure;
		gtk_tree_model_get (model, &iter, 1, &loaded, 3, &detail, 6, &can_configure, -1);
		gtk_label_set_markup(GTK_LABEL(oPluginManageDlg->detail_label), detail);
		g_free(detail);
		if (loaded)
			gtk_widget_set_sensitive(oPluginManageDlg->pref_button, can_configure);
		else
			gtk_widget_set_sensitive(oPluginManageDlg->pref_button, FALSE);
	}
}

gboolean PluginManageDlg::on_treeview_button_press(GtkWidget * widget, GdkEventButton * event, PluginManageDlg *oPluginManageDlg)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		if (GTK_WIDGET_SENSITIVE(oPluginManageDlg->pref_button))
			gtk_dialog_response(GTK_DIALOG(oPluginManageDlg->window), STARDICT_RESPONSE_CONFIGURE);
		return true;
	} else {
		return false;
	}
}

GtkWidget *PluginManageDlg::create_plugin_list()
{
	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	plugin_tree_model = gtk_tree_store_new(7, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_BOOLEAN);
	init_tree_model(plugin_tree_model);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(plugin_tree_model));
	g_object_unref (G_OBJECT (plugin_tree_model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_treeview_button_press), this);
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	g_signal_connect (selection, "changed", G_CALLBACK (on_plugin_treeview_selection_changed), this);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (on_plugin_enable_toggled), this);
	column = gtk_tree_view_column_new_with_attributes (_("Enable"), renderer, "visible", 0, "active", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_expand(GTK_TREE_VIEW_COLUMN (column), FALSE);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Plug-in Name"), renderer, "markup", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	gtk_tree_view_column_set_expand(GTK_TREE_VIEW_COLUMN (column), TRUE);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	gtk_tree_view_expand_all(GTK_TREE_VIEW (treeview));
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	return sw;
}

bool PluginManageDlg::ShowModal(GtkWindow *parent_win, bool &dict_changed)
{
	window = gtk_dialog_new();
	gtk_window_set_transient_for(GTK_WINDOW(window), parent_win);
	gtk_dialog_set_has_separator(GTK_DIALOG(window), false);
	gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	pref_button = gtk_dialog_add_button(GTK_DIALOG(window), _("Configure Pl_ug-in"), STARDICT_RESPONSE_CONFIGURE);
	gtk_widget_set_sensitive(pref_button, FALSE);
	gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(response_handler), this);
	GtkWidget *vbox;
	vbox = gtk_vbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	GtkWidget *pluginlist = create_plugin_list();
	gtk_box_pack_start (GTK_BOX (vbox), pluginlist, true, true, 0);
	GtkWidget *expander = gtk_expander_new ("<b>Plug-in Details</b>");
	gtk_expander_set_use_markup(GTK_EXPANDER(expander), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), expander, false, false, 0);
	detail_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap(GTK_LABEL(detail_label), TRUE);
	gtk_label_set_selectable(GTK_LABEL (detail_label), TRUE);
	gtk_container_add (GTK_CONTAINER (expander), detail_label);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, true, true, 0);
	gtk_widget_show_all (GTK_DIALOG (window)->vbox);
	gtk_window_set_title (GTK_WINDOW (window), _("Manage Plugins"));
	gtk_window_set_default_size(GTK_WINDOW(window), 250, 350);
	dict_changed_ = false;
	gint result;
	while (true) {
		result = gtk_dialog_run(GTK_DIALOG(window));
		if (result ==GTK_RESPONSE_HELP || result == STARDICT_RESPONSE_CONFIGURE) {
		} else {
			break;
		}
	}
	if (result != GTK_RESPONSE_NONE) {
		dict_changed = dict_changed_;
		gtk_widget_destroy(GTK_WIDGET(window));
		return false;
	} else {
		return true;
	}
}
