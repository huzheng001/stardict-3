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

#ifndef __SD_PREFS_DLG_H__
#define __SD_PREFS_DLG_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <list>

class PrefsDlg {
public:
	struct SkinEntry {
		std::string path;
		std::string name;
	};

private:
  GtkWidget *notebook;
#ifndef CONFIG_GPE
  GtkWidget *categories_tree;
  GtkTreeModel *categories_tree_model;
#endif
  GtkWidget *custom_font_hbox;
  GtkWidget *scan_modifier_key_vbox;
  GtkWidget *collation_hbox;
  GtkWidget *tts_textview;
  GtkWidget *searchwebsite_treeview;
  GtkWidget *use_tts_program_hbox;

  GtkWindow *parent_window;
#ifndef CONFIG_GPE
  GdkPixbuf *stardict_logo;
#endif
  GtkEntry *eExportFile;
  GtkEntry *eStarDictServer;
  GtkEntry *eStarDictServerPort;
  GtkButton *bAccount;
  std::string register_user;
  std::string register_hex_md5saltpassword;
  std::string changepassword_user;
  std::string new_password;
  GtkEntry *eSoundPlayCommand;
  GtkEntry *eVideoPlayCommand;
  GtkEntry *eURLOpenCommand;
#ifndef CONFIG_GPE
  GtkWidget *categories_window;
#endif
#ifndef _WIN32
  GtkEntry *eTTSCommandline;
#endif
  GtkWidget *scan_hotkey_editor, *mainwindow_hotkey_editor;
  const std::list<std::string>& key_combs;

  std::vector<SkinEntry> skins;
  bool skin_changed;

#ifndef CONFIG_GPE
  GtkTreeModel* create_categories_tree_model ();
  void create_categories_tree(void);
  void setup_logo_page();
#endif
  void setup_dictionary_scan_page();
  void setup_dictionary_font_page();
  void setup_dictionary_cache_page();
  void setup_dictionary_export_page();
  void setup_dictionary_sound_page();
  void setup_dictionary_video_page();
  void setup_dictionary_article_rendering();
  void setup_dictionary_dict_management();
  void setup_network_netdict();
  void setup_network_web_browser();
  void setup_mainwin_input_page();
  void setup_mainwin_options_page();
  void setup_mainwin_searchwebsite_page();
  void setup_NotificationAreaIcon_options_page();
  void setup_floatwin_options_page();
#ifndef CONFIG_GPE
  void setup_floatwin_size_page();
#endif
  GtkWidget* create_notebook ();

  void write_mainwin_searchwebsite_list();

  static void response_handler (GtkDialog *dialog, gint res_id, PrefsDlg *oPrefsDlg);
#ifndef CONFIG_GPE
  static void categories_tree_selection_cb (GtkTreeSelection *selection, PrefsDlg *oPrefsDlg);
  static gboolean selection_init (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, PrefsDlg *oPrefsDlg);
  static void categories_tree_realize (GtkWidget *widget, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_dictionary_font_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_font_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_scan_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifdef _WIN32
  static void on_setup_dictionary_scan_clipboard_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
#ifndef CONFIG_DARWIN
  static void on_setup_dictionary_use_scan_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_dictionary_scan_combobox_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_scan_hide_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_CreateCacheFile_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_EnableCollation_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_collation_combobox_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_cache_cleanbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_export_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_export_browse_button_clicked(GtkButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_sound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_do_not_add_bad_dict_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  //static void on_setup_dictionary_add_new_dict_in_active_group_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  //static void on_setup_dictionary_add_new_plugin_in_active_group_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifndef _WIN32
  static void on_setup_dictionary_always_sound_cmd_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_dictionary_use_tts_program_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_dictionary_tts_defaultbutton_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_network_netdict_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_network_account_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
#if defined(_WIN32) || defined(CONFIG_GNOME)
  static void on_setup_dictionary_always_url_cmd_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_network_changepassword_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_network_register_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchWhileTyping_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_input_timeout_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_showfirstWhenNotfound_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_startup_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifdef _WIN32
  static void on_setup_mainwin_autorun_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
#ifndef CONFIG_DARWIN
  static void on_setup_mainwin_use_mainwindow_hotkey_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#endif
  static void on_setup_mainwin_skin_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_transparent_scale_changed(GtkRange *range, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_moveup_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_movedown_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_add_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_mainwin_searchwebsite_remove_button_clicked(GtkWidget *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_NotificationAreaIcon_MiddleButtonClickAction_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_pronounce_ckbutton_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_show_float_if_not_found(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
#ifndef CONFIG_GPE
  static void on_setup_floatwin_size_max_width_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_size_max_height_spinbutton_changed(GtkSpinButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_use_custom_bg_toggled(GtkToggleButton *button, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_color_set(GtkColorButton *widget, PrefsDlg *oPrefsDlg);
  static void on_setup_floatwin_transparent_scale_changed(GtkRange *range, PrefsDlg *oPrefsDlg);
  static void on_setup_bookname_style_changed(GtkComboBox *combobox, PrefsDlg *oPrefsDlg);
  static void on_markup_search_word(GtkToggleButton *, PrefsDlg *);

  void resize_categories_tree(void);
  void find_skins();
#endif

  void change_font_for_all_widgets(const std::string& fontname);
public:
  GtkWidget *window;
  PrefsDlg(GtkWindow *parent, GdkPixbuf *logo, const std::list<std::string>& key_combs);
  bool ShowModal();
  void Close();
  void on_register_end(const char *msg);
  void on_changepassword_end(const char *msg);
};

#endif
