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

#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include "stardict.h"
#include "conf.h"
#include "desktop.h"
#include "lib/utils.h"
#include "iskeyspressed.h"
#include "lib/md5.h"
#include "lib/file-utils.h"

#include "prefsdlg.h"
#include "hotkeyeditor.h"
#include "cmdlineopts.h"

#ifndef CONFIG_GPE
enum {
	LOGO = 0,
	DICTIONARY_SCAN_SETTINGS,
	DICTIONARY_FONT_SETTINGS,
	DICTIONARY_CACHE_SETTINGS,
	DICTIONARY_EXPORT_SETTINGS,
	DICTIONARY_SOUND_SETTINGS,
	DICTIONARY_VIDEO_SETTINGS,
	DICIONARY_ARTICLE_RENDERING,
	DICIONARY_DICT_MANAGEMENT,
	NETWORK_NETDICT,
	NETWORK_WEB_BROWSER,
	MAINWIN_INPUT_SETTINGS,
	MAINWIN_OPTIONS_SETTINGS,
	MAINWIN_SEARCH_WEBSITE_SETTINGS,
	NOTIFICATION_AREA_ICON_OPITIONS_SETTINGS,
	FLOATWIN_OPTIONS_SETTINGS,
	FLOATWIN_SIZE_SETTINGS,
};

enum
{
	CATEGORY_COLUMN = 0,
	PAGE_NUM_COLUMN,
	NUM_COLUMNS
};


struct CategoriesTreeItem {
  const gchar			*category;

  CategoriesTreeItem 	*children;

  gint			notebook_page;
};

static CategoriesTreeItem dictionary_behavior [] = {
	{N_("Scan Selection"), NULL, DICTIONARY_SCAN_SETTINGS},
	{N_("Font"), NULL, DICTIONARY_FONT_SETTINGS},
	{N_("Cache"), NULL, DICTIONARY_CACHE_SETTINGS},
	{N_("Export"), NULL, DICTIONARY_EXPORT_SETTINGS},
	{N_("Sound"), NULL, DICTIONARY_SOUND_SETTINGS},
	{N_("Video"), NULL, DICTIONARY_VIDEO_SETTINGS},
	{N_("Article rendering"), NULL, DICIONARY_ARTICLE_RENDERING},
	{N_("Dict management"), NULL, DICIONARY_DICT_MANAGEMENT},
	{ NULL }
};

static CategoriesTreeItem network_behavior [] = {
    {N_("Net Dict"), NULL, NETWORK_NETDICT},
    {N_("Web browser"), NULL, NETWORK_WEB_BROWSER},
    { NULL }
};

static CategoriesTreeItem mainwin_behavior [] =
{
	{N_("Input"), NULL, MAINWIN_INPUT_SETTINGS},
	{N_("Options"), NULL, MAINWIN_OPTIONS_SETTINGS},
	{N_("Search website"), NULL, MAINWIN_SEARCH_WEBSITE_SETTINGS},

	{ NULL }
};

static CategoriesTreeItem NotificationAreaIcon_behavior [] =
{
	{N_("Options"), NULL, NOTIFICATION_AREA_ICON_OPITIONS_SETTINGS},

	{ NULL }
};

static CategoriesTreeItem floatwin_behavior [] =
{
	{N_("Options"), NULL, FLOATWIN_OPTIONS_SETTINGS},
	{N_("Appearance"), NULL, FLOATWIN_SIZE_SETTINGS},

	{ NULL }
};

static CategoriesTreeItem toplevel [] =
{
	{N_("Dictionary"), dictionary_behavior, LOGO},
	{N_("Network"), network_behavior, LOGO},
	{N_("Main window"), mainwin_behavior, LOGO},
	{N_("Notification area icon"), NotificationAreaIcon_behavior, LOGO},
	{N_("Floating window"), floatwin_behavior, LOGO},

	{ NULL }
};

static gint last_selected_page_num = DICTIONARY_SCAN_SETTINGS;
#endif

void PrefsDlg::response_handler (GtkDialog *dialog, gint res_id, PrefsDlg *oPrefsDlg)
{
  if (res_id==GTK_RESPONSE_HELP)
    show_help("stardict-prefs");
}

#ifndef CONFIG_GPE
GtkTreeModel* PrefsDlg::create_categories_tree_model ()
{
	GtkTreeStore *model;
	GtkTreeIter iter;
	CategoriesTreeItem *category = toplevel;

	model = gtk_tree_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	while (category->category) {
		CategoriesTreeItem *sub_category = category->children;
		gtk_tree_store_append (model, &iter, NULL);
		gtk_tree_store_set (model, &iter,
			CATEGORY_COLUMN, gettext (category->category),
			PAGE_NUM_COLUMN, category->notebook_page,
			-1);

		while (sub_category->category) {
			GtkTreeIter child_iter;
			gtk_tree_store_append (model, &child_iter, &iter);
			gtk_tree_store_set (model, &child_iter,
				CATEGORY_COLUMN, gettext (sub_category->category),
				PAGE_NUM_COLUMN, sub_category->notebook_page,
				-1);
			sub_category++;
		}
		category++;
	}
	return GTK_TREE_MODEL (model);
}

void PrefsDlg::categories_tree_selection_cb (GtkTreeSelection *selection, PrefsDlg *oPrefsDlg)
{
 	GtkTreeIter iter;
	GValue value = {0, };

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get_value (oPrefsDlg->categories_tree_model, &iter,
			    PAGE_NUM_COLUMN,
			    &value);

	last_selected_page_num = g_value_get_int (&value);

	if (oPrefsDlg->notebook != NULL)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (oPrefsDlg->notebook),
					       last_selected_page_num);
	g_value_unset (&value);
}

gboolean PrefsDlg::selection_init (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, PrefsDlg *oPrefsDlg)
{
	GValue value = {0, };
	gint page_num;

	gtk_tree_model_get_value (oPrefsDlg->categories_tree_model, iter,
			    PAGE_NUM_COLUMN,
			    &value);

	page_num = g_value_get_int (&value);

	g_value_unset (&value);

	if (page_num == last_selected_page_num)
	{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->categories_tree));

		gtk_tree_selection_select_iter (selection, iter);

		gtk_notebook_set_current_page (GTK_NOTEBOOK (oPrefsDlg->notebook), page_num);

		return TRUE;
	}
	return FALSE;
}

void PrefsDlg::categories_tree_realize (GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	gtk_tree_view_expand_all(GTK_TREE_VIEW(widget));

	gtk_tree_model_foreach(oPrefsDlg->categories_tree_model,
			       GtkTreeModelForeachFunc(selection_init),
			       oPrefsDlg);
}

void PrefsDlg::create_categories_tree(void)
{
  GtkWidget *sw;
  GtkTreeModel *model;
  GtkWidget *treeview;
  GtkCellRenderer *renderer;
  GtkTreeSelection *selection;
  GtkTreeViewColumn *column;
  gint col_offset;

  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
				       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  gtk_widget_set_size_request (sw, 140, 240);

  model = create_categories_tree_model ();

  treeview = gtk_tree_view_new_with_model (model);
  g_object_unref (G_OBJECT (model));
  //gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);

  categories_tree = treeview;
  categories_tree_model = model;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

  gtk_tree_selection_set_mode (selection,
			       GTK_SELECTION_SINGLE);

  /* add column for category */
  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);

  col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
							    -1, _("Categories"),
							    renderer, "text",
							    CATEGORY_COLUMN,
							    NULL);

  column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

  g_signal_connect (selection, "changed",
		    G_CALLBACK (categories_tree_selection_cb),
		    this);

  gtk_container_add (GTK_CONTAINER (sw), treeview);

  g_signal_connect (G_OBJECT (treeview), "realize",
		    G_CALLBACK (categories_tree_realize),
		    this);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);

  categories_window=sw;
}

void PrefsDlg::setup_logo_page()
{
  GtkWidget *image = gtk_image_new_from_pixbuf(stardict_logo);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),image,NULL);
}
#endif

static GtkWidget *prepare_page(GtkNotebook *notebook, const gchar *caption,
			       const gchar *stock_id)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
#else
	GtkWidget *vbox = gtk_vbox_new(FALSE, 12);
#endif
#ifdef CONFIG_GPE
        gtk_container_set_border_width(GTK_CONTAINER (vbox), 5);
        GtkWidget *nb_label = gtk_label_new(caption);
        gtk_notebook_append_page(notebook, vbox, nb_label);
#else
	gtk_notebook_append_page(notebook, vbox, NULL);
#endif

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 6);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	GtkWidget *image =
		gtk_image_new_from_stock(stock_id,
					 GTK_ICON_SIZE_LARGE_TOOLBAR);
	gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, FALSE, 0);
	GtkWidget *label = gtk_label_new(NULL);
	glib::CharStr label_caption(
		g_markup_printf_escaped("<span weight=\"bold\" size=\"x-large\">%s</span>", caption));
	gtk_label_set_markup(GTK_LABEL(label), get_impl(label_caption));
	gtk_box_pack_start(GTK_BOX(hbox),label, FALSE, FALSE, 0);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hseparator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#else
	GtkWidget *hseparator = gtk_hseparator_new();
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hseparator,FALSE,FALSE,0);

	return vbox;
}

void PrefsDlg::on_setup_dictionary_scan_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(oPrefsDlg->scan_modifier_key_vbox,b);
	conf->set_bool_at("dictionary/only_scan_while_modifier_key", b);
}

#ifdef _WIN32
void PrefsDlg::on_setup_dictionary_scan_clipboard_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b) {
		if (conf->get_bool_at("dictionary/scan_selection"))
			gpAppFrame->oClipboard.start();
	} else {
		if (conf->get_bool_at("dictionary/scan_selection"))
			gpAppFrame->oClipboard.stop();
	}
	conf->set_bool_at("dictionary/scan_clipboard", b);
}
#endif
#ifndef CONFIG_DARWIN
void PrefsDlg::on_setup_dictionary_use_scan_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b) {
		gtk_widget_set_sensitive(oPrefsDlg->scan_hotkey_editor, true);
		const std::string &hotkey = conf->get_string_at(
		  "dictionary/scan_hotkey");
		gpAppFrame->oHotkey.start_scan(hotkey.c_str());
	} else {
		gtk_widget_set_sensitive(oPrefsDlg->scan_hotkey_editor, false);
		gpAppFrame->oHotkey.stop_scan();
	}
	conf->set_bool_at("dictionary/use_scan_hotkey", b);
}
#endif
void PrefsDlg::on_setup_dictionary_scan_combobox_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg)
{
  gint key = gtk_combo_box_get_active(combobox);
  conf->set_int_at("dictionary/scan_modifier_key", key);
}

void PrefsDlg::on_setup_dictionary_scan_hide_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean hide = gtk_toggle_button_get_active(button);
  conf->set_bool_at("dictionary/hide_floatwin_when_modifier_key_released", hide);
}

void scan_hotkey_changed(GtkWidget *widget, guint key, guint modifiers)
{
	bool active = conf->get_bool_at("dictionary/use_scan_hotkey");
	const char *s = gtk_accelerator_name(key, GdkModifierType(modifiers));
	conf->set_string_at("dictionary/scan_hotkey", std::string(s));
	if (active) {
		gpAppFrame->oHotkey.stop_scan();
		gpAppFrame->oHotkey.start_scan(s);
	}
}

void mainwindow_hotkey_changed(GtkWidget *widget, guint key, guint modifiers)
{
	bool active = conf->get_bool_at("dictionary/use_mainwindow_hotkey");
	const char *s = gtk_accelerator_name(key, GdkModifierType(modifiers));
	conf->set_string_at("dictionary/mainwindow_hotkey", std::string(s));
	if (active) {
		gpAppFrame->oHotkey.stop_mainwindow();
		gpAppFrame->oHotkey.start_mainwindow(s);
	}
}

void PrefsDlg::setup_dictionary_scan_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Scan Selection"), GTK_STOCK_CONVERT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 0);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Only scan while the modifier key is being pressed."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	bool only_scan_while_modifier_key=
	conf->get_bool_at("dictionary/only_scan_while_modifier_key");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 only_scan_while_modifier_key);
	g_signal_connect(G_OBJECT(check_button), "toggled",
									 G_CALLBACK(on_setup_dictionary_scan_ckbutton_toggled), this);

#if GTK_MAJOR_VERSION >= 3
	scan_modifier_key_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	scan_modifier_key_vbox = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1), scan_modifier_key_vbox,
										 FALSE, FALSE, 12);
	gtk_widget_set_sensitive(scan_modifier_key_vbox,
													 only_scan_while_modifier_key);

	check_button = gtk_check_button_new_with_mnemonic(_("H_ide floating window when modifier key released."));
	gtk_box_pack_start(GTK_BOX(scan_modifier_key_vbox),check_button,false,false,0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 conf->get_bool_at("dictionary/hide_floatwin_when_modifier_key_released"));
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_scan_hide_ckbutton_toggled), this);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 12);
#endif
	gtk_box_pack_start(GTK_BOX(scan_modifier_key_vbox), hbox,false,false,0);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Scan modifier _key:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	GtkWidget *combobox = gtk_combo_box_text_new();
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(combobox), FALSE);

	for (std::list<std::string>::const_iterator p=key_combs.begin();
	     p!=key_combs.end(); ++p) {
	  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), p->c_str());
	}

	int scan_modifier_key=
		conf->get_int_at("dictionary/scan_modifier_key");

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), scan_modifier_key);

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combobox);
	gtk_box_pack_start(GTK_BOX(hbox), combobox, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (combobox), "changed", G_CALLBACK (on_setup_dictionary_scan_combobox_changed), this);

#ifdef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("_Scan clipboard."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("dictionary/scan_clipboard"));
	g_signal_connect(G_OBJECT(check_button), "toggled",
									 G_CALLBACK(on_setup_dictionary_scan_clipboard_ckbutton_toggled), this);

#endif
#ifndef CONFIG_DARWIN
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	hbox = gtk_hbox_new(false, 12);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	check_button = gtk_check_button_new_with_mnemonic(_("_Use scan hotkey: Ctrl+Alt+X."));
	gtk_box_pack_start(GTK_BOX(hbox),check_button,false,false,0);
	StardictHotkeyEditor *hkeditor = stardict_hotkey_editor_new();
	scan_hotkey_editor = GTK_WIDGET(hkeditor);
	g_signal_connect(G_OBJECT(hkeditor), "hotkey-changed", G_CALLBACK(scan_hotkey_changed), NULL);
	const std::string &hotkey = conf->get_string_at("dictionary/scan_hotkey");


	gtk_entry_set_text(GTK_ENTRY(hkeditor), hotkey.c_str());
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(hkeditor),true,true,0);
	bool hk_active = conf->get_bool_at("dictionary/use_scan_hotkey");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), hk_active);
	gtk_widget_set_sensitive(scan_hotkey_editor, hk_active);
	g_signal_connect(G_OBJECT(check_button), "toggled",
									 G_CALLBACK(on_setup_dictionary_use_scan_hotkey_ckbutton_toggled), this);
#endif
}

void PrefsDlg::change_font_for_all_widgets(const std::string& fontname)
{
	gchar *aa =
		g_strdup_printf("style \"custom-font\" { font_name= \"%s\" }\n"
										"class \"GtkWidget\" style \"custom-font\"\n", fontname.c_str());
#if GTK_MAJOR_VERSION >= 3
	GtkCssProvider *css_provider = gtk_css_provider_get_default();
	gtk_css_provider_load_from_data(css_provider, aa, -1, NULL);
#else
	gtk_rc_parse_string(aa);
	GdkScreen *screen = gtk_window_get_screen(parent_window);
	GtkSettings *settings=gtk_settings_get_for_screen(screen);
	gtk_rc_reset_styles(settings);
#endif
	g_free(aa);
#ifndef CONFIG_GPE
	resize_categories_tree();
#endif
}

void PrefsDlg::on_setup_dictionary_font_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  gboolean b = gtk_toggle_button_get_active(button);
  gtk_widget_set_sensitive(oPrefsDlg->custom_font_hbox, b);
  conf->set_bool_at("dictionary/use_custom_font", b);
	if (b) {
		const std::string &custom_font=
			conf->get_string_at("dictionary/custom_font");
		oPrefsDlg->change_font_for_all_widgets(custom_font);
	} else
		oPrefsDlg->change_font_for_all_widgets("");
}

void PrefsDlg::on_setup_dictionary_font_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *dlg = gtk_font_chooser_dialog_new(_("Choose dictionary font"), GTK_WINDOW (oPrefsDlg->window));
#else
	GtkWidget *dlg = gtk_font_selection_dialog_new(_("Choose dictionary font"));
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (oPrefsDlg->window));
#endif
	const gchar *text = gtk_button_get_label(GTK_BUTTON(widget));
	if (strcmp(text,_("Choose"))) {
#if GTK_MAJOR_VERSION >= 3
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(dlg), text);
#else
		gtk_font_selection_dialog_set_font_name(GTK_FONT_SELECTION_DIALOG(dlg), text);
#endif
	}
#if GTK_MAJOR_VERSION >= 3
	gtk_font_chooser_set_preview_text(GTK_FONT_CHOOSER(dlg),_("Dictionary font"));
#else
	gtk_font_selection_dialog_set_preview_text(GTK_FONT_SELECTION_DIALOG(dlg),_("Dictionary font"));
#endif
  gint result = gtk_dialog_run (GTK_DIALOG (dlg));
  if (result==GTK_RESPONSE_OK) {
#if GTK_MAJOR_VERSION >= 3
	gchar *font_name = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(dlg));
#else
	gchar *font_name = gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(dlg));
#endif
    if (font_name) {
      gtk_button_set_label(GTK_BUTTON(widget),font_name);
      conf->set_string_at("dictionary/custom_font", std::string(font_name));
    }
    if (font_name && font_name[0]) {
			oPrefsDlg->change_font_for_all_widgets(font_name);
    }
  }
  gtk_widget_destroy (dlg);
}

void PrefsDlg::setup_dictionary_font_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Font"), GTK_STOCK_SELECT_FONT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false,6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Use custom font."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	bool use_custom_font=
		conf->get_bool_at("dictionary/use_custom_font");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 use_custom_font);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_font_ckbutton_toggled), this);
#if GTK_MAJOR_VERSION >= 3
	custom_font_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	custom_font_hbox = gtk_hbox_new(false, 12);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),custom_font_hbox,false,false,0);
	gtk_widget_set_sensitive(custom_font_hbox, use_custom_font);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Dictionary _font:"));
	gtk_box_pack_start(GTK_BOX(custom_font_hbox),label,false,false,0);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	GtkWidget *button;
	const std::string &custom_font=
		conf->get_string_at("dictionary/custom_font");

	if (!custom_font.empty())
		button = gtk_button_new_with_label(custom_font.c_str());
	else
		button=gtk_button_new_with_label(_("Choose"));

	gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
	gtk_box_pack_start(GTK_BOX(custom_font_hbox),button,false,false,0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_setup_dictionary_font_button_clicked), this);
}

void PrefsDlg::on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/create_cache_file",enable);
}

void PrefsDlg::on_setup_dictionary_cache_EnableCollation_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(oPrefsDlg->collation_hbox, enable);
	conf->set_bool_at("dictionary/enable_collation",enable);
}

void PrefsDlg::on_setup_dictionary_collation_combobox_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg)
{
	gint key = gtk_combo_box_get_active(combobox);
	conf->set_int_at("dictionary/collate_function", key);
}

void PrefsDlg::on_setup_dictionary_cache_cleanbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	RemoveCacheFiles();
}

void PrefsDlg::setup_dictionary_cache_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Cache"), GTK_STOCK_HARDDISK);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("Create c_ache files to speed up loading."));
	bool enable = conf->get_bool_at("dictionary/create_cache_file");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	check_button = gtk_check_button_new_with_mnemonic(_("_Sort word list by collation function."));
	enable = conf->get_bool_at("dictionary/enable_collation");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_cache_EnableCollation_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
#if GTK_MAJOR_VERSION >= 3
	collation_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	collation_hbox = gtk_hbox_new(false,6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),collation_hbox,false,false,0);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("\tCollation _function:"));
	gtk_box_pack_start(GTK_BOX(collation_hbox),label,false,false,0);
	GtkWidget *combobox = gtk_combo_box_text_new();
	gtk_combo_box_set_focus_on_click(GTK_COMBO_BOX(combobox), FALSE);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_general_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_unicode_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_bin");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_czech_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_danish_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_esperanto_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_estonian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_hungarian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_icelandic_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_latvian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_lithuanian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_persian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_polish_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_roman_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_romanian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_slovak_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_slovenian_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_spanish_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_spanish2_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_swedish_ci");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combobox), "utf8_turkish_ci");
	int collate_function = conf->get_int_at("dictionary/collate_function");
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), collate_function);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), combobox);
	gtk_box_pack_start(GTK_BOX(collation_hbox), combobox, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (combobox), "changed", G_CALLBACK (on_setup_dictionary_collation_combobox_changed), this);
	gtk_widget_set_sensitive(collation_hbox, enable);

	label = gtk_label_new(_("After enabled collation, when load the dictionaries for the first time, it will take some time for sorting, please wait for a moment."));
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox1),label,false,false,0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(false,6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,false,false,0);
	GtkWidget *button = gtk_button_new_with_mnemonic(_("C_lean all cache files"));
	gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON));
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_setup_dictionary_cache_cleanbutton_clicked), this);
}



void PrefsDlg::on_setup_dictionary_export_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/only_export_word", enable);
}

void PrefsDlg::on_setup_dictionary_export_browse_button_clicked(GtkButton *button, PrefsDlg *oPrefsDlg)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new (_("Open file..."),
			GTK_WINDOW(oPrefsDlg->window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (dialog), gtk_entry_get_text(oPrefsDlg->eExportFile));
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text(oPrefsDlg->eExportFile, filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

void PrefsDlg::setup_dictionary_export_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Export"), GTK_STOCK_SAVE);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("_Only export words."));
	bool enable= conf->get_bool_at("dictionary/only_export_word");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_export_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 6);
#endif
	GtkWidget *label=gtk_label_new(_("File name:"));
	gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, FALSE, 0);
	GtkWidget *e = gtk_entry_new();
	const std::string &exportfile = conf->get_string_at("dictionary/export_file");
#ifdef _WIN32
	gtk_entry_set_text(GTK_ENTRY(e), abs_path_to_data_dir(exportfile).c_str());
#else
	gtk_entry_set_text(GTK_ENTRY(e), exportfile.c_str());
#endif
	gtk_box_pack_start(GTK_BOX(hbox1), e, TRUE, TRUE, 0);
	eExportFile=GTK_ENTRY(e);

	GtkWidget *button;
	button = gtk_button_new_with_mnemonic(_("_Browse..."));
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_dictionary_export_browse_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox1, FALSE, FALSE, 0);
}

void PrefsDlg::on_setup_bookname_style_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg)
{
        int style = gtk_combo_box_get_active(combobox);
        conf->set_int_at("dictionary/bookname_style", style);
	gpAppFrame->oFloatWin.set_bookname_style((BookNameStyle)style);
	gpAppFrame->oMidWin.oTextWin.set_bookname_style((BookNameStyle)style);
}

void PrefsDlg::on_markup_search_word(GtkToggleButton *button, PrefsDlg *)
{
	conf->set_bool_at("dictionary/markup_search_word",
			  gtk_toggle_button_get_active(button));
}

void PrefsDlg::setup_dictionary_article_rendering()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Article rendering"),
				       GTK_STOCK_CONVERT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 0);

#if GTK_MAJOR_VERSION >= 3
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
        GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
#endif
        gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
        GtkWidget *label = gtk_label_new(_("Dictionary name showing style:"));
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        GtkWidget *cb = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("Default"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("One blank line"));
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("Two blank lines"));
        gtk_box_pack_start(GTK_BOX(hbox), cb, TRUE, TRUE, 0);
        int style = conf->get_int_at("dictionary/bookname_style");
        gtk_combo_box_set_active(GTK_COMBO_BOX(cb), style);
        g_signal_connect(G_OBJECT(cb), "changed", G_CALLBACK(on_setup_bookname_style_changed), this);

	GtkWidget *ck_btn =
		gtk_check_button_new_with_mnemonic(_("Highlight _search term"));
	gtk_box_pack_start(GTK_BOX(vbox1), ck_btn, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_btn),
				     conf->get_bool_at("dictionary/markup_search_word"));
	g_signal_connect(G_OBJECT(ck_btn), "toggled",
			 G_CALLBACK(on_markup_search_word), this);
}

void PrefsDlg::setup_dictionary_dict_management()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Dictionary management"),
				       GTK_STOCK_CONVERT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 0);

	GtkWidget *ck_btn =
		gtk_check_button_new_with_mnemonic(_("Don't load _bad dictionaries."));
	gtk_box_pack_start(GTK_BOX(vbox1), ck_btn, FALSE, FALSE, 6);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_btn),
		conf->get_bool_at("dictionary/do_not_load_bad_dict"));
	g_signal_connect(G_OBJECT(ck_btn), "toggled",
			 G_CALLBACK(on_setup_dictionary_do_not_add_bad_dict_ckbutton_toggled), this);

/*
	ck_btn =
		gtk_check_button_new_with_mnemonic(_("Add new _dictionaries to active dictionary group."));
	gtk_box_pack_start(GTK_BOX(vbox1), ck_btn, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_btn),
		conf->get_bool_at("dictionary/add_new_dict_in_active_group"));
	g_signal_connect(G_OBJECT(ck_btn), "toggled",
			 G_CALLBACK(on_setup_dictionary_add_new_dict_in_active_group_ckbutton_toggled), this);

	ck_btn = gtk_check_button_new_with_mnemonic(_("Add new _plug-ins to active dictionary group."));
	gtk_box_pack_start(GTK_BOX(vbox1), ck_btn, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_btn),
	conf->get_bool_at("dictionary/add_new_plugin_in_active_group"));
	g_signal_connect(G_OBJECT(ck_btn), "toggled",
		 G_CALLBACK(on_setup_dictionary_add_new_plugin_in_active_group_ckbutton_toggled), this);
*/
}

void PrefsDlg::on_setup_dictionary_do_not_add_bad_dict_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/do_not_load_bad_dict", enable);
}

/*
void PrefsDlg::on_setup_dictionary_add_new_dict_in_active_group_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/add_new_dict_in_active_group", enable);
}

void PrefsDlg::on_setup_dictionary_add_new_plugin_in_active_group_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/add_new_plugin_in_active_group", enable);
}
*/

void PrefsDlg::on_setup_dictionary_sound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  gboolean enable = gtk_toggle_button_get_active(button);
  conf->set_bool_at("dictionary/enable_sound_event",enable);
}

#if defined(_WIN32)
#else
void PrefsDlg::on_setup_dictionary_always_sound_cmd_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  gboolean enable = gtk_toggle_button_get_active(button);
  conf->set_bool_at("dictionary/always_use_sound_play_command",enable);
}
#endif

void PrefsDlg::on_setup_dictionary_tts_defaultbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oPrefsDlg->tts_textview));
#ifdef _WIN32
	gtk_text_buffer_set_text(text_view_buffer, "C:\\Program Files\\WyabdcRealPeopleTTS\nC:\\Program Files\\OtdRealPeopleTTS\nWyabdcRealPeopleTTS\nOtdRealPeopleTTS", -1);
#else
	gtk_text_buffer_set_text(text_view_buffer, "/usr/share/WyabdcRealPeopleTTS\n/usr/share/OtdRealPeopleTTS", -1);
#endif
}

#ifndef _WIN32
void PrefsDlg::on_setup_dictionary_use_tts_program_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean enable = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(oPrefsDlg->use_tts_program_hbox,enable);
	conf->set_bool("/apps/stardict/preferences/dictionary/use_tts_program", enable);
	gpAppFrame->oReadWord.use_command_tts = enable;
	gpAppFrame->oMidWin.oToolWin.UpdatePronounceMenu();
}
#endif

void PrefsDlg::setup_dictionary_sound_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Sound"), GTK_STOCK_YES);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

	GtkWidget *check_button;
	check_button = gtk_check_button_new_with_mnemonic(_("Enable _sound event."));
	bool enable=
		conf->get_bool_at("dictionary/enable_sound_event");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_sound_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
	GtkWidget *label, *hbox2;

#if defined(_WIN32)
#else

#if GTK_MAJOR_VERSION >= 3
	hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox2 = gtk_hbox_new(FALSE, 6);
#endif
	label=gtk_label_new(_("Command for playing sound files:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	GtkWidget *e = gtk_entry_new();
	gtk_widget_set_size_request(e, 50, -1);
	const std::string &playcmd=
		conf->get_string_at("dictionary/sound_play_command");
	gtk_entry_set_text(GTK_ENTRY(e), playcmd.c_str());
	gtk_box_pack_start(GTK_BOX(hbox2), e, TRUE, TRUE, 0);
	eSoundPlayCommand=GTK_ENTRY(e);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 0);

	check_button = gtk_check_button_new_with_mnemonic(_("Always use sound play command."));
	enable = conf->get_bool_at("dictionary/always_use_sound_play_command");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect(G_OBJECT (check_button), "toggled", G_CALLBACK(on_setup_dictionary_always_sound_cmd_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
#endif

	label = gtk_label_new(_("RealPeopleTTS search path:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
	gtk_box_pack_start(GTK_BOX(vbox1),label,false,false,0);
	tts_textview = gtk_text_view_new();
	gtk_widget_set_size_request(tts_textview, -1, 70);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tts_textview), GTK_WRAP_CHAR);
	std::string ttspath;
	{
#ifdef _WIN32
		/* Let's the user see paths as they are, that is relative paths. */
#endif
		const std::list<std::string> &ttspathlist = conf->get_strlist_at("dictionary/tts_path");
		for(std::list<std::string>::const_iterator it = ttspathlist.begin();
			it != ttspathlist.end(); ++it) {
			if(!ttspath.empty())
				ttspath += "\n";
			ttspath += *it;
		}
	}
	GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tts_textview));
	gtk_text_buffer_set_text(text_view_buffer, ttspath.c_str(), -1);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), tts_textview);

#if GTK_MAJOR_VERSION >= 3
	hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox2 = gtk_hbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(hbox2), scrolled_window, TRUE, TRUE, 0);

	GtkWidget *button = gtk_button_new_with_mnemonic(_("_Default"));
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (on_setup_dictionary_tts_defaultbutton_clicked), this);
	gtk_box_pack_start(GTK_BOX(hbox2), button, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox1),hbox2,false,false,0);

#ifndef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("Enable _TTS program."));
	enable = conf->get_bool("/apps/stardict/preferences/dictionary/use_tts_program");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect (G_OBJECT (check_button), "toggled", G_CALLBACK (on_setup_dictionary_use_tts_program_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
#if GTK_MAJOR_VERSION >= 3
	use_tts_program_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	use_tts_program_hbox = gtk_hbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),use_tts_program_hbox,false,false,0);
	gtk_widget_set_sensitive(use_tts_program_hbox,enable);
	label = gtk_label_new(_("Command line:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
	gtk_box_pack_start(GTK_BOX(use_tts_program_hbox),label,false,false,0);
	GtkWidget *comboboxentry = gtk_combo_box_text_new_with_entry();
	gtk_widget_set_size_request(comboboxentry, 30, -1);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "echo %s | festival --tts &");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "espeak %s &");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "flite -t %s &");
	eTTSCommandline = GTK_ENTRY(gtk_bin_get_child(GTK_BIN(comboboxentry)));
	const std::string &tts_program_cmdline = conf->get_string("/apps/stardict/preferences/dictionary/tts_program_cmdline");
	gtk_entry_set_text(eTTSCommandline, tts_program_cmdline.c_str());
	gtk_box_pack_start(GTK_BOX(use_tts_program_hbox),comboboxentry,true,true,0);
#endif
}

void PrefsDlg::setup_dictionary_video_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Video"), GTK_STOCK_YES);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox2 = gtk_hbox_new(FALSE, 6);
#endif
	GtkWidget *label = gtk_label_new(_("Command for playing video files:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_widget_set_size_request(entry, 50, -1);
	const std::string &playcmd=
		conf->get_string_at("dictionary/video_play_command");
	gtk_entry_set_text(GTK_ENTRY(entry), playcmd.c_str());
	gtk_box_pack_start(GTK_BOX(hbox2), entry, TRUE, TRUE, 0);
	eVideoPlayCommand=GTK_ENTRY(entry);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 0);
}

void PrefsDlg::on_setup_network_netdict_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	bool enable_netdict = gtk_toggle_button_get_active(button);
	if(enable_netdict) {
		if(!confirm_enable_network_dicts(oPrefsDlg->window)) {
			enable_netdict = false;
			// revert button state
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
		}
	}
	conf->set_bool_at("network/enable_netdict", enable_netdict);
}

static void on_account_passwd_entry_activated(GtkEntry *entry, GtkDialog *dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

void PrefsDlg::on_setup_network_account_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
    GtkWidget *account_dialog;
    account_dialog =
        gtk_dialog_new_with_buttons (_("Account"),
                GTK_WINDOW (oPrefsDlg->window),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_CANCEL,
                GTK_RESPONSE_CANCEL,
                GTK_STOCK_OK,
                GTK_RESPONSE_OK,
                NULL);
    GtkWidget *table = gtk_table_new(2, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(account_dialog))), table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_User Name:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
    GtkWidget *user_entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), user_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), user_entry, 1, 2, 0, 1, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("_Password:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
    GtkWidget *passwd_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(passwd_entry), FALSE);
    g_signal_connect(G_OBJECT(passwd_entry),"activate", G_CALLBACK(on_account_passwd_entry_activated), account_dialog);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), passwd_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), passwd_entry, 1, 2, 1, 2, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    gtk_dialog_set_default_response(GTK_DIALOG(account_dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(account_dialog), FALSE);
    gtk_widget_show_all(GTK_WIDGET(account_dialog));
    while (gtk_dialog_run(GTK_DIALOG(account_dialog))==GTK_RESPONSE_OK) {
        const gchar *user = gtk_entry_get_text(GTK_ENTRY(user_entry));
        if (!user[0]) {
            conf->set_string_at("network/user", "");
            conf->set_string_at("network/md5saltpasswd", "");
            gtk_button_set_label(oPrefsDlg->bAccount, "Guest");
            gpAppFrame->oStarDictClient.set_auth("", "");
            break;
        }
        gchar *error_msg = NULL;
        const gchar *passwd = gtk_entry_get_text(GTK_ENTRY(passwd_entry));
        if (!passwd[0])
            error_msg = _("Please input the password.");
        if (error_msg) {
            GtkWidget *message_dlg =
                gtk_message_dialog_new(
                        GTK_WINDOW(account_dialog),
                        (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                        GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                        "%s", error_msg);
            gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
            gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
            gtk_dialog_run(GTK_DIALOG(message_dlg));
            gtk_widget_destroy(message_dlg);
            continue;
        }
        conf->set_string_at("network/user", user);
        struct MD5Context ctx;
        unsigned char digest[16];
        MD5Init(&ctx);
        MD5Update(&ctx, (const unsigned char*)passwd, strlen(passwd));
        MD5Update(&ctx, (const unsigned char*)"StarDict", 8); //StarDict-Protocol 0.4, add md5 salt.
        MD5Final(digest, &ctx );
        char hex[33];
        for (int i = 0; i < 16; i++)
            sprintf( hex+2*i, "%02x", digest[i] );
        hex[32] = '\0';
        conf->set_string_at("network/md5saltpasswd", hex);
        gtk_button_set_label(oPrefsDlg->bAccount, user);
        gpAppFrame->oStarDictClient.set_auth(user, hex);
        break;
    }
    gtk_widget_destroy(account_dialog);
}

void PrefsDlg::on_register_end(const char *msg)
{
	gtk_button_set_label(bAccount, register_user.c_str());
	conf->set_string_at("network/user", register_user);
	conf->set_string_at("network/md5saltpasswd", register_hex_md5saltpassword);
	gpAppFrame->oStarDictClient.set_auth(register_user.c_str(), register_hex_md5saltpassword.c_str());

	GtkWidget *message_dlg = gtk_message_dialog_new(GTK_WINDOW(window),
			(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
	g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
	gtk_widget_show(message_dlg);
}

void PrefsDlg::on_changepassword_end(const char *msg)
{
	conf->set_string_at("network/md5saltpasswd", new_password);
	gpAppFrame->oStarDictClient.set_auth(changepassword_user.c_str(), new_password.c_str());

	GtkWidget *message_dlg = gtk_message_dialog_new(GTK_WINDOW(window),
			(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);
	gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
	g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
	gtk_widget_show(message_dlg);
}

static void on_changepassword_confirmnewpassword_entry_activated(GtkEntry *entry, GtkDialog *dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

void PrefsDlg::on_setup_network_changepassword_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	const std::string &user= conf->get_string_at("network/user");
	const std::string &old_md5saltpasswd= conf->get_string_at("network/md5saltpasswd");
	if (user.empty() || old_md5saltpasswd.empty()) {
		GtkWidget *message_dlg =
                gtk_message_dialog_new(
                        GTK_WINDOW(oPrefsDlg->window),
                        (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                        GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                        "%s", _("Please setup the user name and password first!"));
		gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
		gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
		gtk_dialog_run(GTK_DIALOG(message_dlg));
		gtk_widget_destroy(message_dlg);
		return;
       }

    GtkWidget *changepassword_dialog;
    changepassword_dialog =
        gtk_dialog_new_with_buttons (_("Change password"),
                GTK_WINDOW (oPrefsDlg->window),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_CANCEL,
                GTK_RESPONSE_CANCEL,
                GTK_STOCK_OK,
                GTK_RESPONSE_OK,
                NULL);
    GtkWidget *table = gtk_table_new(3, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(changepassword_dialog))), table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    GtkWidget *label = gtk_label_new_with_mnemonic(_("Current _password:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
    GtkWidget *current_password_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(current_password_entry), FALSE);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), current_password_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), current_password_entry, 1, 2, 0, 1, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("_New password:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
    GtkWidget *new_password_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(new_password_entry), FALSE);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), new_password_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), new_password_entry, 1, 2, 1, 2, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("Con_firm new password:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
    GtkWidget *confirm_new_password_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(confirm_new_password_entry), FALSE);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), confirm_new_password_entry);
    g_signal_connect(G_OBJECT(confirm_new_password_entry),"activate", G_CALLBACK(on_changepassword_confirmnewpassword_entry_activated), changepassword_dialog);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), confirm_new_password_entry, 1, 2, 2, 3, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    gtk_dialog_set_default_response(GTK_DIALOG(changepassword_dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(changepassword_dialog), FALSE);
    gtk_widget_show_all(GTK_WIDGET(changepassword_dialog));
    char hex[33];
    while (gtk_dialog_run(GTK_DIALOG(changepassword_dialog))==GTK_RESPONSE_OK) {
        const gchar *error_msg = NULL;
        const gchar *corrent_password = gtk_entry_get_text(GTK_ENTRY(current_password_entry));
        const gchar *new_password = gtk_entry_get_text(GTK_ENTRY(new_password_entry));
        const gchar *confirm_new_password = gtk_entry_get_text(GTK_ENTRY(confirm_new_password_entry));
        if (!corrent_password[0]) {
            error_msg = _("Please input the corrent password!");
        } else if (!new_password[0]) {
            error_msg = _("Please input the new password!");
        } else if (!confirm_new_password[0]) {
            error_msg = _("Please input the password again!");
        } else if (strcmp(new_password, confirm_new_password)!=0) {
            error_msg = _("Two passwords are not the same!");
        } else if (strcmp(corrent_password, new_password)==0) {
            error_msg = _("Password didn't change!");
	} else {
        	struct MD5Context ctx;
        	unsigned char digest[16];
        	MD5Init(&ctx);
        	MD5Update(&ctx, (const unsigned char*)corrent_password, strlen(corrent_password));
        	MD5Update(&ctx, (const unsigned char*)"StarDict", 8); //StarDict-Protocol 0.4, add md5 salt.
        	MD5Final(digest, &ctx );
        	for (int i = 0; i < 16; i++)
			sprintf( hex+2*i, "%02x", digest[i] );
	        hex[32] = '\0';
		if (old_md5saltpasswd != hex) {
			error_msg = _("Corrent password is wrong!");
		}
	}
        if (error_msg) {
            GtkWidget *message_dlg =
                gtk_message_dialog_new(
                        GTK_WINDOW(changepassword_dialog),
                        (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                        GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                        "%s", error_msg);
            gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
            gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
            gtk_dialog_run(GTK_DIALOG(message_dlg));
            gtk_widget_destroy(message_dlg);
            continue;
        }
        struct MD5Context ctx;
        unsigned char digest[16];
        MD5Init(&ctx);
        MD5Update(&ctx, (const unsigned char*)new_password, strlen(new_password));
        MD5Update(&ctx, (const unsigned char*)"StarDict", 8); //StarDict-Protocol 0.4, add md5 salt.
        MD5Final(digest, &ctx );
        char hex2[33];
        for (int i = 0; i < 16; i++)
            sprintf( hex2+2*i, "%02x", digest[i] );
        hex2[32] = '\0';
	const gchar *server = gtk_entry_get_text(oPrefsDlg->eStarDictServer);
	int port = atoi(gtk_entry_get_text(oPrefsDlg->eStarDictServerPort));
	gpAppFrame->oStarDictClient.set_server(server, port);
	gpAppFrame->oStarDictClient.set_auth("", "");
	oPrefsDlg->changepassword_user = user;
	oPrefsDlg->new_password = hex2;
        STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_CHANGE_PASSWD, user.c_str(), hex, hex2, gpAppFrame->oStarDictClient.RSA_Public_Key_e, gpAppFrame->oStarDictClient.RSA_Public_Key_n);
        gpAppFrame->oStarDictClient.send_commands(1, c);
        break;
    }
    gtk_widget_destroy(changepassword_dialog);
}

static void on_register_email_entry_activated(GtkEntry *entry, GtkDialog *dialog)
{
	gtk_dialog_response(dialog, GTK_RESPONSE_OK);
}

void PrefsDlg::on_setup_network_register_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
    GtkWidget *register_dialog;
    register_dialog =
        gtk_dialog_new_with_buttons (_("Register"),
                GTK_WINDOW (oPrefsDlg->window),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_STOCK_CANCEL,
                GTK_RESPONSE_CANCEL,
                GTK_STOCK_OK,
                GTK_RESPONSE_OK,
                NULL);
    GtkWidget *table = gtk_table_new(4, 2, FALSE);
    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(register_dialog))), table);
    gtk_container_set_border_width(GTK_CONTAINER(table), 6);
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_User Name:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
    GtkWidget *user_entry = gtk_entry_new ();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), user_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), user_entry, 1, 2, 0, 1, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("_Password:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
    GtkWidget *passwd_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(passwd_entry), FALSE);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), passwd_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), passwd_entry, 1, 2, 1, 2, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("Password _again:"));
    gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
    GtkWidget *passwd_again_entry = gtk_entry_new ();
    gtk_entry_set_visibility(GTK_ENTRY(passwd_again_entry), FALSE);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), passwd_again_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), passwd_again_entry, 1, 2, 2, 3, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    label = gtk_label_new_with_mnemonic(_("_Email:"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
    GtkWidget *email_entry = gtk_entry_new ();
    g_signal_connect(G_OBJECT(email_entry),"activate", G_CALLBACK(on_register_email_entry_activated), register_dialog);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), email_entry);
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4, GTK_FILL, (GtkAttachOptions)0, 6, 4);
    gtk_table_attach(GTK_TABLE(table), email_entry, 1, 2, 3, 4, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);
    gtk_dialog_set_default_response(GTK_DIALOG(register_dialog), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(register_dialog), FALSE);
    gtk_widget_show_all(GTK_WIDGET(register_dialog));
    while (gtk_dialog_run(GTK_DIALOG(register_dialog))==GTK_RESPONSE_OK) {
        gchar *error_msg = NULL;
        const gchar *user = gtk_entry_get_text(GTK_ENTRY(user_entry));
        const gchar *passwd = gtk_entry_get_text(GTK_ENTRY(passwd_entry));
        const gchar *passwd_again = gtk_entry_get_text(GTK_ENTRY(passwd_again_entry));
        const gchar *email = gtk_entry_get_text(GTK_ENTRY(email_entry));
        if (!user[0])
            error_msg = _("Please input the user name.");
        else if (!passwd[0])
            error_msg = _("Please input the password.");
        else if (!passwd_again[0])
            error_msg = _("Please input the password again.");
	else if (strcmp(passwd, passwd_again)!=0)
           error_msg = _("Two passwords are not the same!");
        else if (!email[0])
            error_msg = _("Please input the email.");
        else if (strchr(email, '@')==NULL)
            error_msg = _("Please input a valid email.");
        if (error_msg) {
            GtkWidget *message_dlg =
                gtk_message_dialog_new(
                        GTK_WINDOW(register_dialog),
                        (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                        GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                        "%s", error_msg);
            gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
            gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
            gtk_dialog_run(GTK_DIALOG(message_dlg));
            gtk_widget_destroy(message_dlg);
            continue;
        }
        struct MD5Context ctx;
        unsigned char digest[16];
        MD5Init(&ctx);
        MD5Update(&ctx, (const unsigned char*)passwd, strlen(passwd));
        MD5Update(&ctx, (const unsigned char*)"StarDict", 8); //StarDict-Protocol 0.4, add md5 salt.
        MD5Final(digest, &ctx );
        char hex[33];
        for (int i = 0; i < 16; i++)
            sprintf( hex+2*i, "%02x", digest[i] );
        hex[32] = '\0';
	const gchar *server = gtk_entry_get_text(oPrefsDlg->eStarDictServer);
	int port = atoi(gtk_entry_get_text(oPrefsDlg->eStarDictServerPort));
	gpAppFrame->oStarDictClient.set_server(server, port);
	gpAppFrame->oStarDictClient.set_auth("", "");
	oPrefsDlg->register_user = user;
	oPrefsDlg->register_hex_md5saltpassword = hex;
        STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_REGISTER, user, hex, email, gpAppFrame->oStarDictClient.RSA_Public_Key_e, gpAppFrame->oStarDictClient.RSA_Public_Key_n);
        gpAppFrame->oStarDictClient.send_commands(1, c);
        break;
    }
    gtk_widget_destroy(register_dialog);
}

void PrefsDlg::setup_network_netdict()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Net Dict"),
				       GTK_STOCK_NETWORK);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, FALSE, FALSE, 0);

	GtkWidget *ck_btn =
		gtk_check_button_new_with_mnemonic(_("Enable _network dictionaries."));
	gtk_box_pack_start(GTK_BOX(vbox1), ck_btn, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_btn),
				     conf->get_bool_at("network/enable_netdict"));
	g_signal_connect(G_OBJECT(ck_btn), "toggled",
			 G_CALLBACK(on_setup_network_netdict_ckbutton_toggled), this);

    GtkWidget *table;
    table = gtk_table_new(3, 2, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_box_pack_start(GTK_BOX(vbox1),table,false,false,0);
    GtkWidget *label=gtk_label_new(_("StarDict server:"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	GtkWidget *comboboxentry = gtk_combo_box_text_new_with_entry();
	gtk_table_attach(GTK_TABLE(table), comboboxentry, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict2.stardict.net");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict.stardict.me");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict.stardict.cc");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict.stardict.org");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict.stardict.cn");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(comboboxentry), "dict.stardict.site");
	eStarDictServer=GTK_ENTRY(gtk_bin_get_child(GTK_BIN(comboboxentry)));
	const std::string &server= conf->get_string_at("network/server");
	gtk_entry_set_text(eStarDictServer, server.c_str());
    label=gtk_label_new(_("Port:"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    GtkWidget *e = gtk_entry_new();
    int port = conf->get_int_at("network/port");
    gchar *str = g_strdup_printf("%d", port);
    gtk_entry_set_text(GTK_ENTRY(e), str);
    g_free(str);
    gtk_table_attach(GTK_TABLE(table), e, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
    eStarDictServerPort=GTK_ENTRY(e);
    label=gtk_label_new(_("Account:"));
    gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
    const std::string &user= conf->get_string_at("network/user");
    GtkWidget *button;
    if (user.empty())
        button = gtk_button_new_with_label("Guest");
    else
        button = gtk_button_new_with_label(user.c_str());
    g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_network_account_button_clicked), this);
    gtk_table_attach(GTK_TABLE(table), button, 1, 2, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
    bAccount = GTK_BUTTON(button);

    button = gtk_button_new_with_mnemonic(_("Change _password"));
    g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_network_changepassword_button_clicked), this);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 6);
#endif
    gtk_box_pack_start(GTK_BOX(hbox1),button,false,false,0);
    gtk_box_pack_start(GTK_BOX(vbox1),hbox1,false,false,0);

    button = gtk_button_new_with_mnemonic(_("_Register an account"));
    g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_network_register_button_clicked), this);
#if GTK_MAJOR_VERSION >= 3
	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox1 = gtk_hbox_new(FALSE, 6);
#endif
    gtk_box_pack_start(GTK_BOX(hbox1),button,false,false,0);
    gtk_box_pack_start(GTK_BOX(vbox1),hbox1,false,false,0);

    label = gtk_label_new(_("Warning: Requests to remote StarDict server are sent over the network in an unencrypted form. Do not enable this if you are translating sensitive documents."));
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox1),label,false,false,6);
}

#if defined(_WIN32) || defined(CONFIG_GNOME)
void PrefsDlg::on_setup_dictionary_always_url_cmd_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	conf->set_bool_at("dictionary/always_use_open_url_command",
		gtk_toggle_button_get_active(button));
}
#endif

void PrefsDlg::setup_network_web_browser()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Web browser"), GTK_STOCK_EXECUTE);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox2 = gtk_hbox_new(FALSE, 6);
#endif
	GtkWidget *label = gtk_label_new(_("Command for opening URLs:"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_widget_set_size_request(entry, 50, -1);
	const std::string &cmd=
		conf->get_string_at("dictionary/url_open_command");
	gtk_entry_set_text(GTK_ENTRY(entry), cmd.c_str());
	gtk_box_pack_start(GTK_BOX(hbox2), entry, TRUE, TRUE, 0);
	eURLOpenCommand=GTK_ENTRY(entry);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox2, FALSE, FALSE, 0);

#if defined(_WIN32) || defined(CONFIG_GNOME)
	GtkWidget *check_button 
		= gtk_check_button_new_with_mnemonic(_("_Always use this command for opening URLs."));
	gboolean enable
		= conf->get_bool_at("dictionary/always_use_open_url_command");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), enable);
	g_signal_connect(G_OBJECT (check_button), "toggled", 
		G_CALLBACK(on_setup_dictionary_always_url_cmd_ckbutton_toggled), (gpointer)this);
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,false,false,0);
#endif
}

void PrefsDlg::on_setup_mainwin_searchWhileTyping_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/search_while_typing",
		    gtk_toggle_button_get_active(button));
}

void PrefsDlg::on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/showfirst_when_notfound",
		    gtk_toggle_button_get_active(button));
}

void PrefsDlg::on_setup_mainwin_input_timeout_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg)
{
	gint timeout = gtk_spin_button_get_value_as_int(button);
	conf->set_int_at("main_window/word_change_timeout", timeout);
}

void PrefsDlg::setup_mainwin_input_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Input"),
				       GTK_STOCK_EDIT);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,FALSE,FALSE, 0);

	GtkWidget *check_button =
		gtk_check_button_new_with_mnemonic(_("_Search while typing."));
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
				     conf->get_bool_at("main_window/search_while_typing"));
	g_signal_connect(G_OBJECT(check_button), "toggled",
			 G_CALLBACK(on_setup_mainwin_searchWhileTyping_ckbutton_toggled), this);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,FALSE,FALSE, 0);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Word change _timeout:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE, 0);
	GtkWidget *spin_button;
	spin_button = gtk_spin_button_new_with_range(0,2000,50);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spin_button);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), conf->get_int_at("main_window/word_change_timeout"));
	g_signal_connect(G_OBJECT(spin_button), "value-changed", G_CALLBACK(on_setup_mainwin_input_timeout_spinbutton_changed), this);
	gtk_box_pack_start(GTK_BOX(hbox),spin_button,FALSE,FALSE, 0);
	label=gtk_label_new(_("(default:300 ms)"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE, 0);
	check_button = gtk_check_button_new_with_mnemonic(_("Show the _first word when not found."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,FALSE,FALSE,0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("main_window/showfirst_when_notfound"));
	g_signal_connect(G_OBJECT(check_button), "toggled",
			 G_CALLBACK(on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled), this);
}

void PrefsDlg::on_setup_mainwin_startup_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("main_window/hide_on_startup",
								 gtk_toggle_button_get_active(button));
}

#ifdef _WIN32
void PrefsDlg::on_setup_mainwin_autorun_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	if(CmdLineOptions::get_portable_mode())
		return;
	gboolean b = gtk_toggle_button_get_active(button);
	HKEY hKEY;
	LONG lRet;
	if (b) {
		lRet =RegOpenKeyEx(HKEY_CURRENT_USER,
			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
			KEY_ALL_ACCESS, &hKEY);
		if(lRet==ERROR_SUCCESS) {
			std::string path = build_path(conf_dirs->get_data_dir(), "stardict.exe");
			std::string path_utf8;
			std_win_string path_win;
			if(file_name_to_utf8(path, path_utf8) 
			&& utf8_to_windows(path_utf8, path_win)) {
				RegSetValueEx(hKEY, TEXT("StarDict"), 0, REG_SZ, 
					(const BYTE*)path_win.c_str(), sizeof(TCHAR)*(path_win.length()+1));
			}
			RegCloseKey(hKEY);
		}
	} else {
		lRet =RegOpenKeyEx(HKEY_CURRENT_USER,
			TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
			KEY_ALL_ACCESS, &hKEY);
		if(lRet==ERROR_SUCCESS) {
			RegDeleteValue(hKEY, TEXT("StarDict"));
			RegCloseKey(hKEY);
		}
	}
}
#endif
#ifndef CONFIG_DARWIN
void PrefsDlg::on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean b = gtk_toggle_button_get_active(button);
	if (b) {
		gtk_widget_set_sensitive(oPrefsDlg->mainwindow_hotkey_editor, true);
		const std::string &hotkey = conf->get_string_at(
		  "dictionary/mainwindow_hotkey");
		gpAppFrame->oHotkey.start_mainwindow(hotkey.c_str());
	} else {
		gtk_widget_set_sensitive(oPrefsDlg->mainwindow_hotkey_editor, false);
		gpAppFrame->oHotkey.stop_mainwindow();
	}
	conf->set_bool_at("dictionary/use_mainwindow_hotkey", b);
}
#endif
void PrefsDlg::on_setup_mainwin_transparent_scale_changed(GtkRange *range, PrefsDlg *oPrefsDlg)
{
	gint transparent = (gint)gtk_range_get_value(range);
	conf->set_int_at("main_window/transparent", transparent);
	gtk_widget_set_opacity(gpAppFrame->window, (100-transparent)/100.0);
}

void PrefsDlg::on_setup_mainwin_skin_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg)
{
	int index = gtk_combo_box_get_active(combobox);
	if ((index >= 0) && (index < int(oPrefsDlg->skins.size()))) {
		if (! oPrefsDlg->skin_changed) {
			GtkWidget *message_dlg = gtk_message_dialog_new(
				NULL,
				(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
				GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
				_("Skin change will take effect after application restart"));
			gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
			gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
			gtk_dialog_run(GTK_DIALOG(message_dlg));
			gtk_widget_destroy(message_dlg);
		}
		oPrefsDlg->skin_changed = true;
		const std::string skin_path(oPrefsDlg->skins[index].path);
#ifdef _WIN32
		conf->set_string_at("main_window/skin", rel_path_to_data_dir(skin_path));
#else
		conf->set_string_at("main_window/skin", skin_path);
#endif
	}
}

class SkinDetector {
private:
	std::vector<PrefsDlg::SkinEntry> *m_skins;
public:
	SkinDetector(std::vector<PrefsDlg::SkinEntry> &skins) : m_skins(&skins) {}
	void operator ()(const std::string &filename, bool disable);
};


void SkinDetector::operator ()(const std::string &filename, bool disable)
{
	if (! disable) {
		SkinStorage skin(filename.c_str());
		if (skin.is_valid()) {
			int n = m_skins->size();
			m_skins->resize(n+1);
			(*m_skins)[n].path = filename;
			(*m_skins)[n].name = skin.get_name();
		}
	}
}


void PrefsDlg::find_skins()
{
	skins.resize(1);
	skins[0].path = "";
	skins[0].name = _("Default"); // i.e. no skin applied
	std::list<std::string> dirs;
	dirs.push_back(std::string(g_get_home_dir())+"/.stardict/skins");
	dirs.push_back(build_path(conf_dirs->get_data_dir(), "skins"));
	for_each_dir(dirs, SkinDetector(skins));
}


void PrefsDlg::setup_mainwin_options_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Options"), GTK_STOCK_EXECUTE);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,FALSE,FALSE, 0);

	GtkWidget *check_button;
	GtkWidget *hbox;
#ifdef _WIN32
	check_button = gtk_check_button_new_with_mnemonic(_("_Auto run StarDict after boot."));
	gtk_box_pack_start(GTK_BOX(vbox1),check_button,FALSE,FALSE,0);
	gboolean autorun;

	HKEY hKEY;
	LONG lRet =RegOpenKeyEx(HKEY_CURRENT_USER,
		TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run"), 0,
		KEY_QUERY_VALUE, &hKEY);
	if(lRet!=ERROR_SUCCESS) {
		autorun = false;
	} else {
		const size_t buf_size_c=MAX_PATH;
		const size_t buf_size_b=buf_size_c*sizeof(TCHAR);
		TCHAR run_path_win[buf_size_c];
		DWORD cbData_1=buf_size_b;
		DWORD type_1=REG_SZ;
		lRet=RegQueryValueEx(hKEY, TEXT("StarDict"), NULL, &type_1,
			(LPBYTE)run_path_win, &cbData_1);
		RegCloseKey(hKEY);
		if((lRet!=ERROR_SUCCESS)||(cbData_1 > buf_size_b)) {
			autorun = false;
		} else {
			std::string path = build_path(conf_dirs->get_data_dir(), "stardict.exe");
			std::string run_path_utf8;
			std::string run_path;
			if(windows_to_utf8(run_path_win, run_path_utf8)
			&& utf8_to_file_name(run_path_utf8, run_path))
				autorun = path == run_path;
			else
				autorun = false;
		}
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), autorun);
	if(CmdLineOptions::get_portable_mode())
		gtk_widget_set_sensitive(check_button, FALSE);
	g_signal_connect(G_OBJECT(check_button), "toggled",
		G_CALLBACK(on_setup_mainwin_autorun_ckbutton_toggled), this);
#endif
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	hbox = gtk_hbox_new(false, 12);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, FALSE, FALSE, 0);
	GtkWidget *lbl = gtk_label_new(_("Skin:"));
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(lbl),FALSE,FALSE,0);
	GtkWidget *cb = gtk_combo_box_text_new();
#ifdef _WIN32
	std::string current_skin_path = abs_path_to_data_dir(conf->get_string_at("main_window/skin"));
#else
	std::string current_skin_path = conf->get_string_at("main_window/skin");
#endif
	find_skins();
	for (int i = 0; i < int(skins.size()); i++) {
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), skins[i].name.c_str());
		if (skins[i].path == current_skin_path)
			gtk_combo_box_set_active(GTK_COMBO_BOX(cb), i);
	}
	gtk_box_pack_start(GTK_BOX(hbox),cb,true,true,0);
	g_signal_connect(G_OBJECT(cb), "changed", G_CALLBACK(on_setup_mainwin_skin_changed), this);

	check_button = gtk_check_button_new_with_mnemonic(_("Hide main window when _starting StarDict."));
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
	bool hide=
		conf->get_bool_at("main_window/hide_on_startup");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), hide);
	g_signal_connect(G_OBJECT(check_button), "toggled",
									 G_CALLBACK(on_setup_mainwin_startup_ckbutton_toggled), this);

#ifndef CONFIG_DARWIN

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	hbox = gtk_hbox_new(false, 12);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1), hbox,false,false,0);
	check_button = gtk_check_button_new_with_mnemonic(_("_Use open main window hotkey: Ctrl+Alt+Z."));
	gtk_box_pack_start(GTK_BOX(hbox),check_button,false,false,0);
	StardictHotkeyEditor *hkeditor = stardict_hotkey_editor_new();
	mainwindow_hotkey_editor = GTK_WIDGET(hkeditor);
	g_signal_connect(G_OBJECT(hkeditor), "hotkey-changed", G_CALLBACK(mainwindow_hotkey_changed), NULL);
	const std::string &hotkey = conf->get_string_at("dictionary/mainwindow_hotkey");
	gtk_entry_set_text(GTK_ENTRY(hkeditor), hotkey.c_str());
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(hkeditor),true,true,0);
	bool hk_active = conf->get_bool_at("dictionary/use_mainwindow_hotkey");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), hk_active);
	gtk_widget_set_sensitive(mainwindow_hotkey_editor, hk_active);
	g_signal_connect(G_OBJECT(check_button), "toggled",
									 G_CALLBACK(on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled), this);
#endif

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,FALSE,FALSE, 0);
	GtkWidget *label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("_Transparency:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE, 0);
	GtkWidget *hscale;
#if GTK_MAJOR_VERSION >= 3
	hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 80, 1);
#else
	hscale = gtk_hscale_new_with_range(0,80,1);
#endif
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), hscale);
	int transparent=conf->get_int_at("main_window/transparent");
	gtk_range_set_value(GTK_RANGE(hscale), transparent);
	g_signal_connect(G_OBJECT(hscale), "value-changed", G_CALLBACK(on_setup_mainwin_transparent_scale_changed), this);
	gtk_box_pack_start(GTK_BOX(hbox),hscale,TRUE,TRUE, 0);
}

void PrefsDlg::write_mainwin_searchwebsite_list()
{
	GtkTreeIter iter;
	gboolean have_iter;
	gchar  *website_name, *website_link, *website_searchlink;
	std::list<std::string> searchwebsite_list;
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW (searchwebsite_treeview));

	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &website_name, 1, &website_link, 2, &website_searchlink, -1);
		std::string website(std::string(website_name)+'\t'+website_link+'\t'+website_searchlink);
		g_free(website_name);
		g_free(website_link);
		g_free(website_searchlink);
		searchwebsite_list.push_back(website);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	conf->set_strlist_at("main_window/search_website_list", searchwebsite_list);
}

void PrefsDlg::on_setup_mainwin_searchwebsite_moveup_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		if (gtk_tree_path_prev(path)) {
			GtkTreeIter prev;
			gtk_tree_model_get_iter(model, &prev, path);
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &prev);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}
		gtk_tree_path_free(path);
	}
}

void PrefsDlg::on_setup_mainwin_searchwebsite_movedown_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_path_next(path);
		GtkTreeIter next;
		if (gtk_tree_model_get_iter(model, &next, path)) {
			gtk_list_store_swap(GTK_LIST_STORE(model), &iter, &next);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}
		gtk_tree_path_free(path);
	}
}

void PrefsDlg::on_setup_mainwin_searchwebsite_add_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
  GtkWidget *searchwebsite_add_dialog;
  GtkWidget *searchwebsite_add_dialog_name_entry;
  GtkWidget *searchwebsite_add_dialog_link_entry;
  GtkWidget *searchwebsite_add_dialog_searchlink_entry;

	searchwebsite_add_dialog =
		gtk_dialog_new_with_buttons (_("Add"),
																 GTK_WINDOW (oPrefsDlg->window),
																 GTK_DIALOG_DESTROY_WITH_PARENT,
																 GTK_STOCK_CANCEL,
																 GTK_RESPONSE_CANCEL,
																 GTK_STOCK_OK,
																 GTK_RESPONSE_OK,
																 NULL);
	GtkWidget *table = gtk_table_new(3, 2, FALSE);
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(searchwebsite_add_dialog))), table);
#ifndef CONFIG_GPE
	gtk_container_set_border_width(GTK_CONTAINER(table), 6);
#endif
	GtkWidget *label = gtk_label_new_with_mnemonic(_("Website Name"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, .5);
	searchwebsite_add_dialog_name_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	gtk_widget_set_size_request(searchwebsite_add_dialog_name_entry, 100, -1);
#endif
	gtk_entry_set_activates_default(GTK_ENTRY(searchwebsite_add_dialog_name_entry), TRUE);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), searchwebsite_add_dialog_name_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_name_entry, 1, 2, 0, 1, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);


	label = gtk_label_new_with_mnemonic(_("Website link"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	searchwebsite_add_dialog_link_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	 gtk_widget_set_size_request(searchwebsite_add_dialog_link_entry, 100, -1);
#endif
	gtk_entry_set_activates_default (GTK_ENTRY (searchwebsite_add_dialog_link_entry), TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), searchwebsite_add_dialog_link_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_link_entry, 1, 2, 1, 2, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);

	label = gtk_label_new_with_mnemonic(_("Website search link"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	searchwebsite_add_dialog_searchlink_entry = gtk_entry_new ();
#ifdef CONFIG_GPE
	gtk_widget_set_size_request(searchwebsite_add_dialog_searchlink_entry, 100, -1);
#endif
	gtk_entry_set_activates_default (GTK_ENTRY (searchwebsite_add_dialog_searchlink_entry), TRUE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), searchwebsite_add_dialog_searchlink_entry);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 6, 4);
	gtk_table_attach(GTK_TABLE(table), searchwebsite_add_dialog_searchlink_entry, 1, 2, 2, 3, GTK_EXPAND, (GtkAttachOptions)0, 0, 4);

	gtk_dialog_set_default_response(GTK_DIALOG(searchwebsite_add_dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(searchwebsite_add_dialog), FALSE);

	gtk_widget_show_all(GTK_WIDGET(searchwebsite_add_dialog));
	while (gtk_dialog_run(GTK_DIALOG(searchwebsite_add_dialog))==GTK_RESPONSE_OK) {
		gchar *error_msg = NULL;
		const gchar *website_name = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_name_entry));
		const gchar *website_link = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_link_entry));
		const gchar *website_searchlink = gtk_entry_get_text(GTK_ENTRY(searchwebsite_add_dialog_searchlink_entry));
		if (!website_name[0]) {
			error_msg = _("Please input the website name.");
		} else if (!website_link[0]) {
			error_msg = _("Please input the website link.");
		} else if (!website_searchlink[0]) {
			error_msg = _("Please input the website search link.");
		} else {
			const gchar *p;
			p = strstr(website_searchlink, "%s");
			if (p) {
				if (strchr(p+2, '%')) {
					error_msg = _("The website search link contain more than 1 \"%\" characters!");
				}
			} else {
				error_msg = _("The website search link should contain a \"%s\" string for querying a word.");
			}
		}

		if (error_msg) {
			GtkWidget *message_dlg =
				gtk_message_dialog_new(
					GTK_WINDOW(searchwebsite_add_dialog),
					 (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_MESSAGE_INFO,	GTK_BUTTONS_OK,
					"%s", error_msg);

			gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
			gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);

			gtk_dialog_run(GTK_DIALOG(message_dlg));
			gtk_widget_destroy(message_dlg);
			continue;
		}
		GtkListStore *model =
			GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(oPrefsDlg->searchwebsite_treeview)));
		GtkTreeIter iter;
		gtk_list_store_prepend(model, &iter);
		gtk_list_store_set(model, &iter,
											 0, website_name,
											 1, website_link,
											 2, website_searchlink,
											 3, TRUE,
											 -1);
		oPrefsDlg->write_mainwin_searchwebsite_list();
		break;
	}
	gtk_widget_destroy(searchwebsite_add_dialog);
}

void PrefsDlg::on_setup_mainwin_searchwebsite_remove_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg)
{
	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreeModel *model;
	GtkTreeIter iter;
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		if (gtk_list_store_remove(GTK_LIST_STORE(model), &iter)) {
			GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
			gtk_tree_selection_select_path(selection, path);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview), path, NULL, false, 0, 0);
			gtk_tree_path_free(path);
		}
		oPrefsDlg->write_mainwin_searchwebsite_list();
	}
}

void PrefsDlg::on_setup_mainwin_searchwebsite_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, PrefsDlg *oPrefsDlg)
{
	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW (oPrefsDlg->searchwebsite_treeview));
	GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
	GtkTreeIter iter;

	glong column;
	column = (glong)(g_object_get_data (G_OBJECT (cell), "column"));
	gtk_tree_model_get_iter (model, &iter, path);

	switch (column) {
	case 0:
	case 1:
		if (new_text[0]) {
			gtk_list_store_set (GTK_LIST_STORE (model), &iter, column, new_text, -1);
			oPrefsDlg->write_mainwin_searchwebsite_list();
		}
		break;
	case 2:
		if (new_text[0]) {
			const gchar *p;
			const gchar *error_msg = NULL;
			p = strstr(new_text, "%s");
			if (p) {
				if (strchr(p+2, '%')) {
					error_msg = _("The website search link contain more than 1 \"%\" characters!");
				}
			} else {
				error_msg = _("The website search link should contain a \"%s\" string for querying a word.");
			}


			if (error_msg) {
				GtkWidget *message_dlg;

				message_dlg = gtk_message_dialog_new (
					GTK_WINDOW (oPrefsDlg->window),
					(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
					GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK, "%s",
					error_msg);

				gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

				gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

				gtk_dialog_run (GTK_DIALOG (message_dlg));
				gtk_widget_destroy (message_dlg);
			} else {
				gtk_list_store_set (GTK_LIST_STORE (model), &iter, 2, new_text, -1);
				oPrefsDlg->write_mainwin_searchwebsite_list();
			}
		}
		break;

    }

	gtk_tree_path_free (path);
}

void PrefsDlg::setup_mainwin_searchwebsite_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Search website"), GTK_STOCK_JUMP_TO);
	GtkWidget *vbox2;
#if GTK_MAJOR_VERSION >= 3
	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	vbox2 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), vbox2, true, true,0);

	GtkListStore *model;
	model = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_BOOLEAN);

	const std::list<std::string> &web_list=
		conf->get_strlist_at("main_window/search_website_list");

	GtkTreeIter iter;
	for (std::list<std::string>::const_iterator wit=web_list.begin();
			 wit!=web_list.end(); ++wit) {
		std::vector<std::string> l=split(*wit, '\t');
		if (l.size()==3) {
			gtk_list_store_append(model, &iter);
			gtk_list_store_set(model, &iter,
												 0, l[0].c_str(),
												 1, l[1].c_str(),
												 2, l[2].c_str(),
												 3, TRUE,
												 -1);
		}
	}

	GtkWidget *sw;
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
      	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);

	gtk_widget_set_size_request (sw, 300, 180);

	searchwebsite_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	g_object_unref (G_OBJECT (model));
	//gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (searchwebsite_treeview), TRUE);

	GtkTreeSelection *selection;
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (searchwebsite_treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(0));
	column = gtk_tree_view_column_new_with_attributes (_("Website Name"), renderer, "text", 0, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(1));
	column = gtk_tree_view_column_new_with_attributes (_("Website link"), renderer, "text", 1, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	renderer = gtk_cell_renderer_text_new ();
	g_signal_connect (renderer, "edited", G_CALLBACK (on_setup_mainwin_searchwebsite_cell_edited), this);
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER(2));
	column = gtk_tree_view_column_new_with_attributes (_("Website search link"), renderer, "text", 2, "editable", 3, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(searchwebsite_treeview), column);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	gtk_container_add (GTK_CONTAINER (sw), searchwebsite_treeview);
	gtk_box_pack_start (GTK_BOX (vbox2), sw, TRUE, TRUE, 0);

	GtkWidget *hbox1;
#if GTK_MAJOR_VERSION >= 3
	hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox1 = gtk_hbox_new(false,6);
#endif
	GtkWidget *button;
	button = gtk_button_new();
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_moveup_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	button = gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
	gtk_container_add(GTK_CONTAINER(button), image);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_movedown_button_clicked), this);
	gtk_box_pack_start (GTK_BOX (hbox1), button, FALSE, FALSE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_REMOVE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_remove_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);

/*	button = gtk_button_new();
	GtkWidget *align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (button), align);
	GtkWidget *hbox2 = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (align), hbox2);
	label = gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("_Modify"));
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
	image = gtk_image_new_from_stock (GTK_STOCK_CONVERT, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX (hbox2), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_edit_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);*/

	button = gtk_button_new_from_stock(GTK_STOCK_ADD);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_setup_mainwin_searchwebsite_add_button_clicked), this);
	gtk_box_pack_end (GTK_BOX (hbox1), button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (vbox2), hbox1, false, false, 0);
}

void PrefsDlg::on_setup_NotificationAreaIcon_MiddleButtonClickAction_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg)
{
	int index = gtk_combo_box_get_active(combobox);
	conf->set_int_at("notification_area_icon/middle_click_action", index);
}

void PrefsDlg::setup_NotificationAreaIcon_options_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Options"), GTK_STOCK_DND);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 12);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	GtkWidget *label = gtk_label_new(_("When middle mouse button is clicked:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	GtkWidget *cb = gtk_combo_box_text_new();
	/* order of items must match the TNotifAreaMiddleClickAction enum */
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("Query selection in floating window"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("Query selection in main window"));
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cb), _("Do nothing"));
	gtk_box_pack_start(GTK_BOX(hbox), cb, TRUE, TRUE, 0);
	int action = conf->get_int_at("notification_area_icon/middle_click_action");
	gtk_combo_box_set_active(GTK_COMBO_BOX(cb), action);
	g_signal_connect(G_OBJECT(cb), "changed", G_CALLBACK(on_setup_NotificationAreaIcon_MiddleButtonClickAction_changed), this);
}


void PrefsDlg::on_setup_floatwin_pronounce_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("floating_window/pronounce_when_popup",
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::on_setup_show_float_if_not_found(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
  conf->set_bool_at("floating_window/show_if_not_found",
								 gtk_toggle_button_get_active(button));
}

void PrefsDlg::setup_floatwin_options_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Options"), GTK_STOCK_DND);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Pronounce the word when it pops up."));
	bool pronounce_when_popup=
		conf->get_bool_at("floating_window/pronounce_when_popup");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button),
															 pronounce_when_popup);
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_setup_floatwin_pronounce_ckbutton_toggled), this);
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);

	check_button = gtk_check_button_new_with_mnemonic(_("_Show floating window if word not found."));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("floating_window/show_if_not_found"));
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_setup_show_float_if_not_found), this);
	gtk_box_pack_start(GTK_BOX(vbox1), check_button, FALSE, FALSE, 0);
}

#ifndef CONFIG_GPE
void PrefsDlg::on_setup_floatwin_size_max_width_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg)
{
	gint width = gtk_spin_button_get_value_as_int(button);
	conf->set_int_at("floating_window/max_window_width", width);
}

void PrefsDlg::on_setup_floatwin_size_max_height_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg)
{
	gint height = gtk_spin_button_get_value_as_int(button);
	conf->set_int_at("floating_window/max_window_height", height);
}

void PrefsDlg::on_setup_floatwin_use_custom_bg_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg)
{
	gboolean use = gtk_toggle_button_get_active(button);
	conf->set_bool_at("floating_window/use_custom_bg", use);
}

void PrefsDlg::on_setup_floatwin_color_set(GtkColorButton *widget, PrefsDlg *oPrefsDlg)
{
#if GTK_MAJOR_VERSION >= 3
	GdkRGBA color;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(widget), &color);
	conf->set_double_at("floating_window/bg_red", color.red);
	conf->set_double_at("floating_window/bg_green", color.green);
	conf->set_double_at("floating_window/bg_blue", color.blue);
#else
	GdkColor color;
	gtk_color_button_get_color(widget, &color);
	conf->set_int_at("floating_window/bg_red", color.red);
	conf->set_int_at("floating_window/bg_green", color.green);
	conf->set_int_at("floating_window/bg_blue", color.blue);
#endif
	gpAppFrame->oFloatWin.set_bg();
}

void PrefsDlg::on_setup_floatwin_transparent_scale_changed(GtkRange *range, PrefsDlg *oPrefsDlg)
{
	gint transparent = (gint)gtk_range_get_value(range);
	conf->set_int_at("floating_window/transparent", transparent);
}

void PrefsDlg::setup_floatwin_size_page()
{
	GtkWidget *vbox = prepare_page(GTK_NOTEBOOK(notebook), _("Appearance"), GTK_STOCK_ZOOM_FIT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox1 = gtk_vbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox),vbox1,false,false, 0);
	GtkWidget *table;
	table = gtk_table_new(3, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 6);
	gtk_table_set_col_spacings(GTK_TABLE(table), 6);
	gtk_box_pack_start(GTK_BOX(vbox1),table,false,false,0);

	int max_width=
		conf->get_int_at("floating_window/max_window_width");
	int max_height=
		conf->get_int_at("floating_window/max_window_height");

	GdkScreen *screen = gtk_window_get_screen(parent_window);
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);

	GtkWidget *label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Max window _width:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	GtkWidget *spin_button;
	spin_button = gtk_spin_button_new_with_range(MIN_MAX_FLOATWIN_WIDTH,screen_width,1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spin_button);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), max_width);
	g_signal_connect(G_OBJECT(spin_button), "value-changed",
				 G_CALLBACK(on_setup_floatwin_size_max_width_spinbutton_changed), this);
	gtk_table_attach(GTK_TABLE(table), spin_button, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	label=gtk_label_new(_("(default:320)"));
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 0, 1, GTK_FILL, GTK_FILL, 0, 0);

	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("Max window hei_ght:"));
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	spin_button = gtk_spin_button_new_with_range(MIN_MAX_FLOATWIN_HEIGHT,screen_height,1);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), spin_button);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON(spin_button), GTK_UPDATE_IF_VALID);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_button), max_height);
	g_signal_connect (G_OBJECT (spin_button), "value-changed", G_CALLBACK (on_setup_floatwin_size_max_height_spinbutton_changed), (gpointer)this);
	gtk_table_attach(GTK_TABLE(table), spin_button, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	label=gtk_label_new(_("(default:240)"));
	gtk_table_attach(GTK_TABLE(table), label, 2, 3, 1, 2, GTK_FILL, GTK_FILL, 0, 0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget*hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget*hbox1 = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox1,false,false,0);
	GtkWidget *check_button = gtk_check_button_new_with_mnemonic(_("_Use custom background color:"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check_button), conf->get_bool_at("floating_window/use_custom_bg"));
	g_signal_connect(G_OBJECT(check_button), "toggled", G_CALLBACK(on_setup_floatwin_use_custom_bg_toggled), this);
	gtk_box_pack_start(GTK_BOX(hbox1),check_button,false,false,0);
#if GTK_MAJOR_VERSION >= 3
	GdkRGBA color;
	color.red = conf->get_double_at("floating_window/bg_red");
	color.green = conf->get_double_at("floating_window/bg_green");
	color.blue = conf->get_double_at("floating_window/bg_blue");
	color.alpha = 1;
	GtkWidget *colorbutton = gtk_color_button_new_with_rgba(&color);
#else
	GdkColor color;
	color.red = conf->get_int_at("floating_window/bg_red");
	color.green = conf->get_int_at("floating_window/bg_green");
	color.blue = conf->get_int_at("floating_window/bg_blue");
	GtkWidget *colorbutton = gtk_color_button_new_with_color(&color);
#endif
	g_signal_connect(G_OBJECT(colorbutton), "color-set", G_CALLBACK(on_setup_floatwin_color_set), this);
	gtk_box_pack_start(GTK_BOX(hbox1),colorbutton,false,false,0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox1),hbox,FALSE,FALSE, 0);
	label=gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(label), _("_Transparency:"));
	gtk_box_pack_start(GTK_BOX(hbox),label,FALSE,FALSE, 0);
	GtkWidget *hscale;
#if GTK_MAJOR_VERSION >= 3
	hscale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 80, 1);
#else
	hscale = gtk_hscale_new_with_range(0,80,1);
#endif
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), hscale);
	int transparent=conf->get_int_at("floating_window/transparent");
	gtk_range_set_value(GTK_RANGE(hscale), transparent);
	g_signal_connect(G_OBJECT(hscale), "value-changed", G_CALLBACK(on_setup_floatwin_transparent_scale_changed), this);
	gtk_box_pack_start(GTK_BOX(hbox),hscale,TRUE,TRUE, 0);
}
#endif

GtkWidget* PrefsDlg::create_notebook ()
{
	notebook = gtk_notebook_new();
	GtkNotebook *nb = GTK_NOTEBOOK(notebook);
#ifdef CONFIG_GPE
	gtk_notebook_set_scrollable(nb, true);
#else
	gtk_notebook_set_show_tabs(nb,false);
	gtk_notebook_set_show_border(nb,false);
	setup_logo_page ();
#endif
	setup_dictionary_scan_page ();
	setup_dictionary_font_page ();
	setup_dictionary_cache_page ();
	setup_dictionary_export_page ();
	setup_dictionary_sound_page();
	setup_dictionary_video_page();
	setup_dictionary_article_rendering();
	setup_dictionary_dict_management();
	setup_network_netdict();
	setup_network_web_browser();
	setup_mainwin_input_page ();
	setup_mainwin_options_page ();
	setup_mainwin_searchwebsite_page();
	setup_NotificationAreaIcon_options_page();
	setup_floatwin_options_page ();
#ifdef CONFIG_GPE
	gtk_notebook_set_current_page (nb, 0);
#else
	setup_floatwin_size_page ();
	gtk_notebook_set_current_page (nb, LOGO);
#endif
	return notebook;
}


PrefsDlg::PrefsDlg(GtkWindow *parent, GdkPixbuf *logo, const std::list<std::string>& key_combs_) :
	key_combs(key_combs_)
{
  parent_window=parent;
#ifndef CONFIG_GPE
  stardict_logo=logo;
#endif

  window = NULL;
  skin_changed = false;
}

bool PrefsDlg::ShowModal()
{
  window = gtk_dialog_new();
  gtk_window_set_transient_for(GTK_WINDOW(window), parent_window);
  gtk_window_set_default_size(GTK_WINDOW(window), -1, 390);

  gtk_dialog_add_button(GTK_DIALOG(window),
			GTK_STOCK_HELP,
			GTK_RESPONSE_HELP);

  gtk_dialog_add_button(GTK_DIALOG(window),
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response(GTK_DIALOG(window),
				  GTK_RESPONSE_CLOSE);
  g_signal_connect(G_OBJECT(window), "response",
		   G_CALLBACK(response_handler), this);
#ifndef CONFIG_GPE
  GtkWidget *hbox;
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 18);
#else
	hbox = gtk_hbox_new (FALSE, 18);
#endif
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
  GtkWidget *r;
#if GTK_MAJOR_VERSION >= 3
	r = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
#else
	r = gtk_vbox_new (FALSE, 6);
#endif

  GtkWidget *label;
  label = gtk_label_new_with_mnemonic (_("Cat_egories:"));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);
  create_categories_tree();


  gtk_box_pack_start(GTK_BOX(r), label, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(r), categories_window, TRUE, TRUE, 0);
#endif

  GtkWidget *l = create_notebook ();

#ifdef CONFIG_GPE
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), l, true, true, 0);
#else
	gtk_box_pack_start (GTK_BOX (hbox), r, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area(GTK_DIALOG (window))), hbox, true, true, 0);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), categories_tree);
#endif

  gtk_widget_show_all (gtk_dialog_get_content_area(GTK_DIALOG (window)));
  gtk_window_set_title (GTK_WINDOW (window), _("Preferences"));

#ifndef CONFIG_GPE
  resize_categories_tree();
#endif
	gint result;
	while ((result = gtk_dialog_run(GTK_DIALOG(window)))==GTK_RESPONSE_HELP)
		;
	if (result != GTK_RESPONSE_NONE) {
		const gchar *ch;
		ch = gtk_entry_get_text(eExportFile);
		if (ch) {
#ifdef _WIN32
			conf->set_string_at("dictionary/export_file", rel_path_to_data_dir(ch));
#else
			conf->set_string_at("dictionary/export_file", ch);
#endif
		}
#ifndef _WIN32
		ch = gtk_entry_get_text(eTTSCommandline);
		if (ch) {
			conf->set_string("/apps/stardict/preferences/dictionary/tts_program_cmdline", ch);
			gpAppFrame->oReadWord.tts_program_cmdline = ch;
		}
#endif
		const gchar *server;
		ch = gtk_entry_get_text(eStarDictServer);
		if (ch && ch[0])
			server = ch;
		else
			server = _("dict2.stardict.net");
		conf->set_string_at("network/server", server);
		int port;
		ch = gtk_entry_get_text(eStarDictServerPort);
		if (ch && ch[0])
			port = atoi(ch);
		else
			port = 2629;
		conf->set_int_at("network/port", port);
		gpAppFrame->oStarDictClient.set_server(server, port);

#ifdef _WIN32
#else
		ch = gtk_entry_get_text(eSoundPlayCommand);
		if (ch)
			conf->set_string_at("dictionary/sound_play_command", ch);
#endif

		ch = gtk_entry_get_text(eVideoPlayCommand);
		if (ch)
			conf->set_string_at("dictionary/video_play_command", ch);
		ch = gtk_entry_get_text(eURLOpenCommand);
		if (ch)
			conf->set_string_at("dictionary/url_open_command", ch);
		GtkTextBuffer *text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tts_textview));
		GtkTextIter start_iter;
		GtkTextIter end_iter;
		gtk_text_buffer_get_start_iter(text_view_buffer, &start_iter);
		gtk_text_buffer_get_end_iter(text_view_buffer, &end_iter);
		gchar *text = gtk_text_buffer_get_text(text_view_buffer, &start_iter, &end_iter, FALSE);
		std::list<std::string> ttspathlist;
		{
			const gchar* p = text;
			const gchar* q;
			while(true)
			{
				q = strchr(p, '\n');
				if(!q)
					q = strchr(p, '\0');
				if(p<q)
					ttspathlist.push_back(std::string(p, q-p));
				if(!*q)
					break;
				p = q + 1;
			}
		}
#ifdef _WIN32
		/* Convert paths to relative paths.
		The text buffer was initialized with relative paths. 
		If the user has not changed them, convertion will not mangle paths, they'll be unchanged.
		When the user added a new path it'll be converted to a relative path if possible. */
		{
			std::list<std::string> paths;
			rel_path_to_data_dir(ttspathlist, paths);
			std::swap(paths, ttspathlist);
		}
#endif
		conf->set_strlist_at("dictionary/tts_path", ttspathlist);
		gpAppFrame->oReadWord.LoadRealTtsPath(ttspathlist);
		g_free(text);
		gtk_widget_destroy(GTK_WIDGET(window));
		window = NULL;
		return false;
	} else {
		return true;
	}
}

void PrefsDlg::Close()
{
	if (window) {
		gtk_widget_destroy (window);
		window = NULL;
	}
}

#ifndef CONFIG_GPE
void PrefsDlg::resize_categories_tree(void)
{
  //this is hack for prevet horizontaly scrolling
  //if you know how it make better, just write
  GtkRequisition rtv, rsw;
#if GTK_MAJOR_VERSION >= 3
  gtk_widget_get_preferred_size(categories_tree, NULL, &rtv);
  gtk_widget_get_preferred_size(gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(categories_window)), NULL, &rsw);
#else
	gtk_widget_size_request(categories_tree, &rtv);
	gtk_widget_size_request(GTK_SCROLLED_WINDOW(categories_window)->vscrollbar, &rsw);
#endif
  gtk_widget_set_size_request(categories_window, rtv.width+rsw.width+25, -1);
}
#endif
