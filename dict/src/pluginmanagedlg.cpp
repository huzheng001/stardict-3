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

#include "pluginmanagedlg.h"
#include "desktop.h"
#include <glib/gi18n.h>
#include "stardict.h"

PluginManageDlg::PluginManageDlg()
:
window(NULL),
treeview(NULL),
detail_label(NULL),
pref_button(NULL),
plugin_tree_model(NULL),
dict_changed_(false),
order_changed_(false)
{
}

PluginManageDlg::~PluginManageDlg()
{
	g_assert(!window);
	g_assert(!treeview);
	g_assert(!detail_label);
	g_assert(!pref_button);
	g_assert(!plugin_tree_model);
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
	if (plugin_type == StarDictPlugInType_VIRTUALDICT
		|| plugin_type == StarDictPlugInType_NETDICT) {
		oPluginManageDlg->dict_changed_ = true;
	} else if (plugin_type == StarDictPlugInType_TTS) {
		gpAppFrame->oMidWin.oToolWin.UpdatePronounceMenu();
	}

	std::list<std::string> disable_list;
	gtk_tree_model_foreach(model, get_disable_list, &disable_list);
#ifdef _WIN32
	{
		std::list<std::string> disable_list_rel;
		rel_path_to_data_dir(disable_list, disable_list_rel);
		std::swap(disable_list, disable_list_rel);
	}
#endif
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

static void add_tree_model(GtkTreeStore *tree_model, GtkTreeIter*parent, const std::list<StarDictPluginInfo> &infolist)
{
	plugininfo_ParseUserData Data;
	GMarkupParser parser;
	parser.start_element = plugininfo_parse_start_element;
	parser.end_element = plugininfo_parse_end_element;
	parser.text = plugininfo_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GtkTreeIter iter;
	for (std::list<StarDictPluginInfo>::const_iterator i = infolist.begin(); i != infolist.end(); ++i) {
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
	{
#ifdef _WIN32
		std::list<std::string> plugin_order_list;
		const std::list<std::string>& plugin_order_list_rel
			= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
		abs_path_to_data_dir(plugin_order_list_rel, plugin_order_list);
#else
		const std::list<std::string>& plugin_order_list
			= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
#endif
		gpAppFrame->oStarDictPlugins->get_plugin_list(plugin_order_list, plugin_list);
	}
	GtkTreeIter iter;
	for (std::list<std::pair<StarDictPlugInType, std::list<StarDictPluginInfo> > >::iterator i = plugin_list.begin(); i != plugin_list.end(); ++i) {
		switch (i->first) {
			case StarDictPlugInType_VIRTUALDICT:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Virtual Dictionary</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_NETDICT:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Network Dictionary</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_SPECIALDICT:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Special Dictionary</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_TTS:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>TTS Engine</b>"), -1);
				add_tree_model(tree_model, &iter, i->second);
				break;
			case StarDictPlugInType_PARSEDATA:
				gtk_tree_store_append(tree_model, &iter, NULL);
				gtk_tree_store_set(tree_model, &iter, 0, false, 2, _("<b>Data Parsing Engine</b>"), -1);
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
		if (gtk_widget_get_sensitive(GTK_WIDGET(oPluginManageDlg->pref_button)))
			gtk_dialog_response(GTK_DIALOG(oPluginManageDlg->window), STARDICT_RESPONSE_CONFIGURE);
		return true;
	} else {
		return false;
	}
}

static void add_order_list(std::list<std::string> &order_list, GtkTreeModel *now_tree_model, GtkTreeIter *parent)
{
	gboolean have_iter;
	GtkTreeIter iter;
	have_iter = gtk_tree_model_iter_children(now_tree_model, &iter, parent);
	gchar *filename;
	while (have_iter) {
		gtk_tree_model_get (now_tree_model, &iter, 4, &filename, -1);
		order_list.push_back(filename);
		g_free(filename);
		have_iter = gtk_tree_model_iter_next(now_tree_model, &iter);
	}
}

void PluginManageDlg::write_order_list()
{
	std::list<std::string> order_list;
	GtkTreeModel *now_tree_model = GTK_TREE_MODEL(plugin_tree_model);
	gboolean have_iter;
	GtkTreeIter iter;
	have_iter = gtk_tree_model_get_iter_first(now_tree_model, &iter);
	while (have_iter) {
		if (gtk_tree_model_iter_has_child(now_tree_model, &iter)) {
			add_order_list(order_list, now_tree_model, &iter);
		}
		have_iter = gtk_tree_model_iter_next(now_tree_model, &iter);
	}
#ifdef _WIN32
	{
		std::list<std::string> order_list_rel;
		rel_path_to_data_dir(order_list, order_list_rel);
		std::swap(order_list, order_list_rel);
	}
#endif
	conf->set_strlist("/apps/stardict/manage_plugins/plugin_order_list", order_list);
}

void PluginManageDlg::drag_data_get_cb(GtkWidget *widget, GdkDragContext *ctx, GtkSelectionData *data, guint info, guint time, PluginManageDlg *oPluginManageDlg)
{
	if (gtk_selection_data_get_target(data) == gdk_atom_intern("STARDICT_PLUGINMANAGE", FALSE)) {
		GtkTreeRowReference *ref;
		GtkTreePath *source_row;
		ref = (GtkTreeRowReference *)g_object_get_data(G_OBJECT(ctx), "gtk-tree-view-source-row");
		source_row = gtk_tree_row_reference_get_path(ref);
		if (source_row == NULL)
			return;
		GtkTreeIter iter;
		gtk_tree_model_get_iter(GTK_TREE_MODEL(oPluginManageDlg->plugin_tree_model), &iter, source_row);
		gtk_selection_data_set(data, gdk_atom_intern("STARDICT_PLUGINMANAGE", FALSE), 8, (const guchar *)&iter, sizeof(iter));
		gtk_tree_path_free(source_row);
	}
}

void PluginManageDlg::drag_data_received_cb(GtkWidget *widget, GdkDragContext *ctx, guint x, guint y, GtkSelectionData *sd, guint info, guint t, PluginManageDlg *oPluginManageDlg)
{
	if (gtk_selection_data_get_target(sd) == gdk_atom_intern("STARDICT_PLUGINMANAGE", FALSE) && gtk_selection_data_get_data(sd)) {
		GtkTreePath *path = NULL;
		GtkTreeViewDropPosition position;
		GtkTreeIter drag_iter;
		memcpy(&drag_iter, gtk_selection_data_get_data(sd), sizeof(drag_iter));
		if (gtk_tree_view_get_dest_row_at_pos(GTK_TREE_VIEW(widget), x, y, &path, &position)) {
			GtkTreeIter iter;
			GtkTreeModel *model = GTK_TREE_MODEL(oPluginManageDlg->plugin_tree_model);
			gtk_tree_model_get_iter(model, &iter, path);
			if (gtk_tree_model_iter_has_child(model, &iter)) {
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			if (gtk_tree_model_iter_has_child(model, &drag_iter)) {
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			GtkTreeIter parent_iter;
			if (!gtk_tree_model_iter_parent(model, &parent_iter, &iter)) {
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			GtkTreeIter drag_parent_iter;
			if (!gtk_tree_model_iter_parent(model, &drag_parent_iter, &drag_iter)) {
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			char *iter_str, *drag_iter_str;
			iter_str = gtk_tree_model_get_string_from_iter(model, &parent_iter);
			drag_iter_str = gtk_tree_model_get_string_from_iter(model, &drag_parent_iter);
			if (strcmp(iter_str, drag_iter_str) != 0) {
				g_free(iter_str);
				g_free(drag_iter_str);
				gtk_drag_finish (ctx, FALSE, FALSE, t);
				return;
			}
			g_free(iter_str);
			g_free(drag_iter_str);
			switch (position) {
				case GTK_TREE_VIEW_DROP_AFTER:
				case GTK_TREE_VIEW_DROP_INTO_OR_AFTER:
					gtk_tree_store_move_after(GTK_TREE_STORE(model), &drag_iter, &iter);
					break;
				case GTK_TREE_VIEW_DROP_BEFORE:
				case GTK_TREE_VIEW_DROP_INTO_OR_BEFORE:
					gtk_tree_store_move_before(GTK_TREE_STORE(model), &drag_iter, &iter);
					break;
				default: {
					gtk_drag_finish (ctx, FALSE, FALSE, t);
					return;
				}
			}
			oPluginManageDlg->write_order_list();
			oPluginManageDlg->order_changed_ = true;
			gtk_drag_finish (ctx, TRUE, FALSE, t);
		}
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
#if GTK_MAJOR_VERSION >= 3
#else
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
#endif
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

	GtkTargetEntry gte[] = {{(gchar *)"STARDICT_PLUGINMANAGE", GTK_TARGET_SAME_APP, 0}};
	gtk_tree_view_enable_model_drag_source(GTK_TREE_VIEW(treeview), GDK_BUTTON1_MASK, gte, 1, GDK_ACTION_COPY);
	gtk_tree_view_enable_model_drag_dest(GTK_TREE_VIEW(treeview), gte, 1, (GdkDragAction)(GDK_ACTION_COPY | GDK_ACTION_MOVE));
	g_signal_connect(G_OBJECT(treeview), "drag-data-received", G_CALLBACK(drag_data_received_cb), this);
	g_signal_connect(G_OBJECT(treeview), "drag-data-get", G_CALLBACK(drag_data_get_cb), this);

	gtk_tree_view_expand_all(GTK_TREE_VIEW (treeview));
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	return sw;
}

bool PluginManageDlg::ShowModal(GtkWindow *parent_win, bool &dict_changed, bool &order_changed)
{
	window = gtk_dialog_new();
	oStarDictPluginSystemInfo.pluginwin = window;
	gtk_window_set_transient_for(GTK_WINDOW(window), parent_win);
	//gtk_dialog_set_has_separator(GTK_DIALOG(window), false);
	gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	pref_button = gtk_dialog_add_button(GTK_DIALOG(window), _("Configure Pl_ug-in"), STARDICT_RESPONSE_CONFIGURE);
	gtk_widget_set_sensitive(pref_button, FALSE);
	gtk_dialog_add_button(GTK_DIALOG(window), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	gtk_dialog_set_default_response(GTK_DIALOG(window), GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(window), "response", G_CALLBACK(response_handler), this);
	GtkWidget *vbox;
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
#else
	vbox = gtk_vbox_new (FALSE, 5);
#endif
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
	GtkWidget *pluginlist = create_plugin_list();
	gtk_box_pack_start (GTK_BOX (vbox), pluginlist, true, true, 0);
	GtkWidget *expander = gtk_expander_new (_("<b>Plug-in Details</b>"));
	gtk_expander_set_use_markup(GTK_EXPANDER(expander), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), expander, false, false, 0);
	detail_label = gtk_label_new (NULL);
	gtk_label_set_line_wrap(GTK_LABEL(detail_label), TRUE);
	gtk_label_set_selectable(GTK_LABEL (detail_label), TRUE);
	gtk_container_add (GTK_CONTAINER (expander), detail_label);
	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG (window))), vbox, true, true, 0);
	gtk_widget_show_all (gtk_dialog_get_content_area(GTK_DIALOG (window)));
	gtk_window_set_title (GTK_WINDOW (window), _("Manage Plugins"));
	gtk_window_set_default_size(GTK_WINDOW(window), 250, 350);
	dict_changed_ = false;
	order_changed_ = false;
	gint result;
	while (true) {
		result = gtk_dialog_run(GTK_DIALOG(window));
		if (result ==GTK_RESPONSE_HELP || result == STARDICT_RESPONSE_CONFIGURE) {
		} else {
			break;
		}
	}
	/* When do we get GTK_RESPONSE_NONE response? Unable to reproduce. */
	if (result != GTK_RESPONSE_NONE) {
		dict_changed = dict_changed_;
		order_changed = order_changed_;
		gtk_widget_destroy(GTK_WIDGET(window));
	}
	window = NULL;
	treeview = NULL;
	detail_label = NULL;
	pref_button = NULL;
	plugin_tree_model = NULL;
	oStarDictPluginSystemInfo.pluginwin = NULL;

	return result == GTK_RESPONSE_NONE;
}
