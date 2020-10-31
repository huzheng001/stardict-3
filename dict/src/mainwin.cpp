/*
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

#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <algorithm>

#ifdef _WIN32
#define VERSION "3.0.7"
#  include <gdk/gdkwin32.h>
#endif

#include "conf.h"
#include "desktop.h"
#include "stardict.h"
#include "lib/utils.h"
#include "lib/edit-distance.h"
#include "lib/stddict.h"
#include "lib/full_text_trans.h"

#include "mainwin.h"


/**************************************************/
TopWin::TopWin()
{
	WordCombo = NULL; //need by save_yourself_cb().
	BackList = NULL;
	BackList_index = 0;
	enable_change_cb = true;
	MainMenu = NULL;
	SearchMenu = NULL;
}

TopWin::~TopWin()
{
	GList *list = BackList;
	while (list) {
		g_free(((BackListData *)(list->data))->word);
		g_free(list->data);
		list = list->next;
	}
	g_list_free(BackList);
}

static void unfocus_combo_arrow(GtkWidget *widget, gpointer data)
{
	if (!GTK_IS_ENTRY(widget)) {
		gtk_widget_set_can_focus(widget, FALSE);
	}
}

void TopWin::Create(GtkWidget *vbox)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
#else
	GtkWidget *hbox = gtk_hbox_new(false,0);
#endif
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,3);
#endif

	back_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(back_button),gtk_image_new_from_stock(GTK_STOCK_GO_BACK, GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(back_button);
	gtk_button_set_relief (GTK_BUTTON (back_button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (back_button, FALSE);
	gtk_widget_set_sensitive(back_button, FALSE);
	g_signal_connect(G_OBJECT(back_button),"clicked", G_CALLBACK(BackCallback),this);
	g_signal_connect(G_OBJECT(back_button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), back_button,false,false,0);
	gtk_widget_set_tooltip_text(back_button,_("Go back (Alt+Left)"));

	forward_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(forward_button),gtk_image_new_from_stock(GTK_STOCK_GO_FORWARD,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(forward_button);
	gtk_button_set_relief (GTK_BUTTON (forward_button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (forward_button, FALSE);
	gtk_widget_set_sensitive(forward_button, FALSE);
	g_signal_connect(G_OBJECT(forward_button),"clicked", G_CALLBACK(ForwardCallback),this);
	g_signal_connect(G_OBJECT(forward_button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox), forward_button,false,false,0);
	gtk_widget_set_tooltip_text(forward_button,_("Go forward (Alt+Right)"));

	GtkListStore* list_store = gtk_list_store_new(1, G_TYPE_STRING);
	LoadHistory(list_store);
	WordCombo = gtk_combo_box_new_with_model_and_entry(GTK_TREE_MODEL(list_store));
	gtk_combo_box_set_entry_text_column(GTK_COMBO_BOX(WordCombo), 0);
	g_object_unref (G_OBJECT(list_store));
	gtk_widget_set_focus_on_click(GTK_WIDGET(WordCombo), FALSE);
	gtk_container_forall(GTK_CONTAINER(WordCombo), unfocus_combo_arrow, this);
	gtk_widget_set_size_request(WordCombo,60,-1);
	gtk_widget_show(WordCombo);
	GtkWidget *entry = gtk_bin_get_child(GTK_BIN(WordCombo));
	gtk_entry_set_max_length(GTK_ENTRY(entry), MAX_INDEX_KEY_SIZE-1);
	gtk_entry_set_icon_from_stock (GTK_ENTRY(entry), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_from_stock (GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY (entry), GTK_ENTRY_ICON_SECONDARY, _("Clear the search box"));
	g_signal_connect (G_OBJECT (entry), "icon-press", G_CALLBACK (on_entry_icon_press), this);
	g_signal_connect (G_OBJECT (entry), "changed", G_CALLBACK (on_entry_changed), this);
	g_signal_connect (G_OBJECT (entry), "activate", G_CALLBACK (on_entry_activate), this);
	g_signal_connect (G_OBJECT (entry), "populate-popup", G_CALLBACK (on_entry_populate_popup), this);
	gtk_box_pack_start(GTK_BOX(hbox),WordCombo,true,true,3);

#ifndef CONFIG_GPE
	GtkWidget *button;
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(GoCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Fuzzy query"));
#endif

	GtkWidget *label;
	label = gtk_label_new("\t");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox),label,false,false,0);

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_HOME,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(MenuCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Show the main menu (Alt+M)"));
}

void TopWin::Destroy(void)
{
	InsertHisList(get_text());
	SaveHistory();
	if (MainMenu)
		gtk_widget_destroy(MainMenu);
	if (SearchMenu)
		gtk_widget_destroy(SearchMenu);
}

void TopWin::on_entry_changed(GtkEntry *entry, TopWin *oTopWin)
{
	if (!(oTopWin->enable_change_cb))
		return;
	if (!conf->get_bool_at("main_window/search_while_typing"))
		return;
	const gchar *sWord = gtk_entry_get_text(entry);
	if(sWord[0]!='\0') {
		gpAppFrame->TopWinWordChange(sWord);
	} else {
		gpAppFrame->oMidWin.oTextWin.queryWord.clear();
		gpAppFrame->oMidWin.oIndexWin.oResultWin.Clear();
		gpAppFrame->oMidWin.oTextWin.ShowTips();
	}
}

void TopWin::do_search_by_fuzzyquery (GtkWidget *item, TopWin *oTopWin)
{
	const gchar *text = oTopWin->get_text();
	if (text[0]=='\0')
		return;
	gpAppFrame->LookupWithFuzzyToMainWin(text);
	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (enable_netdict) {
		std::string word;
		word = "/";
		word += text;
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, word.c_str());
		if (!gpAppFrame->oStarDictClient.try_cache(c)) {
			gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
			gpAppFrame->oStarDictClient.send_commands(1, c);
		}
	}
	oTopWin->TextSelectAll();
	oTopWin->grab_focus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}

void TopWin::do_search_by_patternmatch (GtkWidget *item, TopWin *oTopWin)
{
	const gchar *text = oTopWin->get_text();
	if (text[0]=='\0')
		return;
	gpAppFrame->LookupWithRuleToMainWin(text);
	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (enable_netdict) {
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, text);
		if (!gpAppFrame->oStarDictClient.try_cache(c)) {
			gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
			gpAppFrame->oStarDictClient.send_commands(1, c);
		}
	}
	oTopWin->TextSelectAll();
	oTopWin->grab_focus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}

void TopWin::do_search_by_regularmatch (GtkWidget *item, TopWin *oTopWin)
{
	const gchar *text = oTopWin->get_text();
	if (text[0]=='\0')
		return;
	gpAppFrame->LookupWithRegexToMainWin(text);
	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (enable_netdict) {
		std::string word;
		word = ":";
		word += text;
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, word.c_str());
		if (!gpAppFrame->oStarDictClient.try_cache(c)) {
			gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
			gpAppFrame->oStarDictClient.send_commands(1, c);
		}
	}
	oTopWin->TextSelectAll();
	oTopWin->grab_focus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}

void TopWin::do_search_by_fulltextsearch (GtkWidget *item, TopWin *oTopWin)
{
	const gchar *text = oTopWin->get_text();
	if (text[0]=='\0')
		return;
	gpAppFrame->LookupDataToMainWin(text);
	oTopWin->TextSelectAll();
	oTopWin->grab_focus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}

void TopWin::on_entry_icon_press(GtkEntry *entry, gint position, GdkEventButton *event, TopWin *oTopWin)
{
	if (position == GTK_ENTRY_ICON_PRIMARY) {
		if (oTopWin->SearchMenu == NULL) {
			oTopWin->SearchMenu = gtk_menu_new ();
			GtkWidget *menuitem;
			GtkWidget *image;

			menuitem = gtk_image_menu_item_new_with_mnemonic (_("_Fuzzy query"));
			image = gtk_image_new_from_stock (GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			g_signal_connect (menuitem, "activate", G_CALLBACK (do_search_by_fuzzyquery), oTopWin);
			gtk_menu_shell_append (GTK_MENU_SHELL (oTopWin->SearchMenu), menuitem);

			menuitem = gtk_image_menu_item_new_with_mnemonic (_("_Glob-style pattern matching"));
			image = gtk_image_new_from_stock (GTK_STOCK_EDIT, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			g_signal_connect (menuitem, "activate", G_CALLBACK (do_search_by_patternmatch), oTopWin);
			gtk_menu_shell_append (GTK_MENU_SHELL (oTopWin->SearchMenu), menuitem);

			menuitem = gtk_image_menu_item_new_with_mnemonic (_("_Regular expressions matching"));
			image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			g_signal_connect (menuitem, "activate", G_CALLBACK (do_search_by_regularmatch), oTopWin);
			gtk_menu_shell_append (GTK_MENU_SHELL (oTopWin->SearchMenu), menuitem);

			menuitem = gtk_image_menu_item_new_with_mnemonic (_("Full-_text search"));
			image = gtk_image_new_from_stock (GTK_STOCK_OPEN, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			g_signal_connect (menuitem, "activate", G_CALLBACK (do_search_by_fulltextsearch), oTopWin);
			gtk_menu_shell_append (GTK_MENU_SHELL (oTopWin->SearchMenu), menuitem);

			gtk_widget_show_all (oTopWin->SearchMenu);
		}
		gtk_menu_popup (GTK_MENU (oTopWin->SearchMenu), NULL, NULL, NULL, NULL, event->button, event->time);
	} else {
		play_sound_on_event("buttonactive");
		oTopWin->clear_entry();
	}
}

/* Clear the Search field and focus it. Be ready for new query. */
void TopWin::clear_entry()
{
	if (get_text()[0]) {
		InsertHisList(get_text());
		InsertBackList();
		SetText("");
	}
	grab_focus();
}

void TopWin::on_entry_activate(GtkEntry *entry, TopWin *oTopWin)
{
	gpAppFrame->TopWinEnterWord();
}

void TopWin::on_clear_history_menu_item_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	GtkListStore* list_store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(oTopWin->WordCombo)));
	gtk_list_store_clear(list_store);
}

void TopWin::on_entry_populate_popup(GtkEntry *entry, GtkMenu  *menu, TopWin *oTopWin)
{
	GtkWidget *menuitem;
	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("Clear _history"));
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_clear_history_menu_item_activate), oTopWin);
	GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_CLEAR, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
}

#ifndef CONFIG_GPE
void TopWin::GoCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");

	const gchar *text = oTopWin->get_text();
	if (text[0]=='\0')
		return;
	std::string res;
	query_t qt = analyse_query(text, res);
	switch (qt) {
	case qtFUZZY:
		gpAppFrame->LookupWithFuzzyToMainWin(res.c_str());
		break;
	case qtPATTERN:
		gpAppFrame->LookupWithRuleToMainWin(res.c_str());
		break;
	case qtREGEX:
		gpAppFrame->LookupWithRegexToMainWin(res.c_str());
		break;
	case qtFULLTEXT:
		gpAppFrame->LookupDataToMainWin(res.c_str());
		break;
	default:
		gpAppFrame->LookupWithFuzzyToMainWin(res.c_str());
	}
	if (qt != qtFULLTEXT) {
		bool enable_netdict = conf->get_bool_at("network/enable_netdict");
		if (enable_netdict) {
			std::string word;
			if (qt == qtSIMPLE) {
				word = "/";
				word += res;
			} else {
				word = text;
			}
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, word.c_str());
			if (!gpAppFrame->oStarDictClient.try_cache(c)) {
				gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
				gpAppFrame->oStarDictClient.send_commands(1, c);
			}
		}
	}

	oTopWin->TextSelectAll();
	oTopWin->grab_focus();
	oTopWin->InsertHisList(text);
	oTopWin->InsertBackList(text);
}
#endif

void TopWin::do_back()
{
	if (!BackList)
		return;
	GList *list = g_list_nth(BackList, BackList_index);
	if (!list)
		return;
	list = g_list_next(list);
	if (!list) {
		gtk_widget_set_sensitive(back_button, FALSE);
		return;
	}
	if (list->next == NULL)
		gtk_widget_set_sensitive(back_button, FALSE);
	BackList_index++;
	gtk_widget_set_sensitive(forward_button, TRUE);
	InsertHisList(get_text());
	SetText(((BackListData *)(list->data))->word);
	if (gtk_widget_has_focus(gtk_bin_get_child(GTK_BIN(WordCombo))))
		gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(WordCombo))), 0, -1);
	if (((BackListData *)(list->data))->adjustment_value != -1) {
		ProcessGtkEvent(); // so all the definition text have been inserted.
		gpAppFrame->oMidWin.oTextWin.view->scroll_to(((BackListData *)(list->data))->adjustment_value);
	}
}

void TopWin::do_forward()
{
	if (!BackList)
		return;
	GList *list = g_list_nth(BackList, BackList_index);
	if (!list)
		return;
	list = g_list_previous(list);
	if (!list) {
		gtk_widget_set_sensitive(forward_button, FALSE);
		return;
	}
	if (list->prev == NULL)
		gtk_widget_set_sensitive(forward_button, FALSE);
	BackList_index--;
	gtk_widget_set_sensitive(back_button, TRUE);
	InsertHisList(get_text());
	SetText(((BackListData *)(list->data))->word);
	if (gtk_widget_has_focus(GTK_WIDGET(gtk_bin_get_child(GTK_BIN(WordCombo)))))
		gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(WordCombo))), 0, -1);
	if (((BackListData *)(list->data))->adjustment_value != -1) {
		ProcessGtkEvent(); // so all the definition text have been inserted.
		gpAppFrame->oMidWin.oTextWin.view->scroll_to(((BackListData *)(list->data))->adjustment_value);
	}
}

void TopWin::BackCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");

	oTopWin->do_back();
}

void TopWin::ForwardCallback(GtkWidget *widget, TopWin *oTopWin)
{
	play_sound_on_event("buttonactive");

	oTopWin->do_forward();
}

void TopWin::do_prev()
{
	MidWin &midwin = gpAppFrame->oMidWin;
	if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_EMPTY)
		return;
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(midwin.oIndexWin.oListWin.treeview_);
	GtkTreeModel *model;
	GtkTreeIter iter;

	gboolean selected = gtk_tree_selection_get_selected(selection,&model,&iter);
	if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_NORMAL_LIST) {
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return;
		}
		GtkTreePath* path = gtk_tree_model_get_path(model,&iter);
		if (gtk_tree_path_prev(path)) {
			gtk_tree_model_get_iter(model,&iter,path);
			gtk_tree_selection_select_iter(selection,&iter);
			gtk_tree_view_scroll_to_cell(
				midwin.oIndexWin.oListWin.treeview_,
				path, NULL, FALSE, 0, 0);
		} else {
			// user have selected the first row.
			gchar *word;
			gtk_tree_model_get (model, &iter, 0, &word, -1);
			gpAppFrame->ListPreWords(word);
			if (conf->get_bool_at("network/enable_netdict")) {
				STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_PREVIOUS, word);
				if (!gpAppFrame->oStarDictClient.try_cache(c))
					gpAppFrame->oStarDictClient.send_commands(1, c);
			}
			g_free(word);
		}
		gtk_tree_path_free(path);
	} else if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_FUZZY_LIST ||
		   midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_PATTERN_LIST||
		   midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_DATA_LIST) {
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return;
		}
		GtkTreePath* path = gtk_tree_model_get_path(model,&iter);
		if (gtk_tree_path_prev(path)) {
			gtk_tree_model_get_iter(model, &iter, path);
			gtk_tree_selection_select_iter(selection, &iter);
			gtk_tree_view_scroll_to_cell(
				midwin.oIndexWin.oListWin.treeview_,
				path, NULL, FALSE, 0, 0);
		} //else  user have selected the first row,no action is need.		
		gtk_tree_path_free(path);
	}
}

void TopWin::do_next()
{
	MidWin &midwin = gpAppFrame->oMidWin;
	if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_EMPTY)
		return;
	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(midwin.oIndexWin.oListWin.treeview_);
	GtkTreeModel *model;
	GtkTreeIter iter;
	//make sure this will run,so model is set.
	gboolean selected = gtk_tree_selection_get_selected(selection, &model, &iter); 
	if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_NORMAL_LIST) {		
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return;
		}
		//if gtk_tree_model_iter_next fail,iter will be invalid,so save it.
		GtkTreeIter new_iter = iter; 
		gchar *word;
		if (gtk_tree_model_iter_next(model, &iter)) {
				gtk_tree_selection_select_iter(selection, &iter);
				GtkTreePath* path = gtk_tree_model_get_path(model, &iter);
				gtk_tree_view_scroll_to_cell(
					midwin.oIndexWin.oListWin.treeview_,
					path, NULL, FALSE, 0, 0);
				gtk_tree_path_free(path);			
		} else {
			// user have selected the last row.
			gtk_tree_model_get(model, &new_iter, 0, &word, -1);
			gpAppFrame->ListNextWords(word);
			if (conf->get_bool_at("network/enable_netdict")) {
				STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_NEXT, word);
				if (!gpAppFrame->oStarDictClient.try_cache(c))
					gpAppFrame->oStarDictClient.send_commands(1, c);
			}
			g_free(word);
		}
	} else if (midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_FUZZY_LIST ||
		   midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_PATTERN_LIST ||
		   midwin.oIndexWin.oListWin.list_word_type == LIST_WIN_DATA_LIST) {
		if (!selected) {
			if (!gtk_tree_model_get_iter_first(model,&iter))
				return;
		}
		if (gtk_tree_model_iter_next(model,&iter)) {
			gtk_tree_selection_select_iter(selection, &iter);
			GtkTreePath* path = gtk_tree_model_get_path(model,&iter);
			gtk_tree_view_scroll_to_cell(
				midwin.oIndexWin.oListWin.treeview_,
				path, NULL, FALSE, 0, 0);
			gtk_tree_path_free(path);
		}//else  user have selected the last row,no action is need.		
	}
}

void TopWin::on_main_menu_preferences_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->PopupPrefsDlg();
}

void TopWin::on_main_menu_dictmanage_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->PopupDictManageDlg();
}

void TopWin::on_main_menu_pluginmanage_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->PopupPluginManageDlg();
}

void TopWin::on_main_menu_keepabove_toggled(GtkCheckMenuItem *menuitem, TopWin *oTopWin)
{
	bool keepabove = gtk_check_menu_item_get_active(menuitem);
	conf->set_bool_at("main_window/keep_above", keepabove);
}

void TopWin::on_main_menu_downloaddict_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_url("http://stardict.huzheng.org");
}

void TopWin::on_main_menu_newversion_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_url("http://stardict-4.sourceforge.net");
}

void TopWin::on_main_menu_donate_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_url("http://www.huzheng.org/donate.php");
}

void TopWin::on_main_menu_help_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
  show_help(NULL);
}

void TopWin::on_main_menu_about_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	const gchar *authors[] = {
		"Hu Zheng <huzheng001@gmail.com>",
		"Sergey <kubtek@gmail.com>",
		"Evgeniy <dushistov@mail.ru>",
		"Alex Murygin <murygin@aitoc.com>",
		"Tao Wang <dancefire@gmail.com>",
		"Opera Wang <wangvisual@sohu.com>",
		"Ma Su'an <msa@wri.com.cn>",
		NULL
	};
	const gchar *documenters[] = {
		"Hu Zheng <huzheng001@gmail.com>",
		"Will Robinson <wsr23@stanford.edu>",
		"Anthony Fok <foka@debian.org>",
		NULL
	};
	gchar *translator_credits = _("translator_credits");

	gtk_show_about_dialog(GTK_WINDOW (gpAppFrame->window),
			      "name", _("StarDict"),
			      "version", VERSION,
			      "website", "http://stardict-4.sourceforge.net",
			      "comments", _("StarDict is an international dictionary for GNOME."),
			      "copyright", "Copyright \xc2\xa9 1999 by Ma Su'an\n" "Copyright \xc2\xa9 2002 by Opera Wang\n" "Copyright \xc2\xa9 2003-2004 by Hu Zheng\n" "Copyright \xc2\xa9 2005-2006 by Hu Zheng, Evgeniy\n" "Copyright \xc2\xa9 2007-2011 by Hu Zheng, Sergey\n" "Copyright \xc2\xa9 2012-2020 by Hu Zheng",
			      "authors", (const char **)authors,
			      "documenters", (const char **)documenters,
			      "translator-credits", strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
			      "logo", get_impl(gpAppFrame->oAppSkin.icon),
			      NULL);
}

void TopWin::on_main_menu_quit_activate(GtkMenuItem *menuitem, TopWin *oTopWin)
{
	gpAppFrame->Quit();
}

void TopWin::ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
	if (text) {
		std::string estr;
		stardict_input_escape(text, estr);
		gpAppFrame->Query(estr.c_str());
	}
}

void TopWin::do_menu()
{
	play_sound_on_event("menushow");

	if (!MainMenu) {
		MainMenu = gtk_menu_new();

		GtkWidget *menuitem;
		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Pr_eferences"));
		GtkWidget *image;
		image = gtk_image_new_from_stock(GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_preferences_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Manage _dictionaries"));
		image = gtk_image_new_from_stock(GTK_STOCK_PROPERTIES, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_dictmanage_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Manage _plugins"));
		image = gtk_image_new_from_stock(GTK_STOCK_DISCONNECT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_pluginmanage_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_check_menu_item_new_with_mnemonic(_("_Keep window above"));
		bool keepabove = conf->get_bool_at("main_window/keep_above");
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), keepabove);
		g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(on_main_menu_keepabove_toggled), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("Download dict_ionaries"));
		image = gtk_image_new_from_stock(GTK_STOCK_NETWORK, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_downloaddict_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_New version"));
		image = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_newversion_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("D_onate"));
		image = gtk_image_new_from_stock(GTK_STOCK_CDROM, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_donate_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Help"));
		image = gtk_image_new_from_stock(GTK_STOCK_HELP, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_help_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_About"));
		image = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_BUTTON);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_about_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);
		menuitem = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Quit"));
		image = gtk_image_new_from_stock(GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_main_menu_quit_activate), NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(MainMenu), menuitem);

		gtk_widget_show_all(MainMenu);

	}
	gtk_menu_popup(GTK_MENU(MainMenu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
}

void TopWin::MenuCallback(GtkWidget *widget, TopWin *oTopWin)
{
	oTopWin->do_menu();
}

void TopWin::SetText(const gchar *word, bool notify)
{
	if (strcmp(word, get_text()) == 0)
		return;
	enable_change_cb = false;
// this will emit "changed" signal twice, one for "", one for word. so disable it.
	gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(WordCombo))), word); 
	enable_change_cb = true;

	if (!notify)
		return;
	if(word[0]!='\0') {
		gpAppFrame->TopWinWordChange(word);
	} else {
		gpAppFrame->oMidWin.oTextWin.queryWord.clear();
		gpAppFrame->oMidWin.oIndexWin.oResultWin.Clear();
		gpAppFrame->oMidWin.oTextWin.ShowTips();
	}
}

/*void TopWin::SetText_without_notify(const gchar *word)
{
	enable_change_cb = false;
	gtk_entry_set_text(GTK_ENTRY(GTK_BIN(WordCombo)->child),word);
	enable_change_cb = true;
}*/

void TopWin::TextSelectAll()
{
	gtk_editable_select_region(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(WordCombo))),0,-1);
}

gboolean TopWin::TextSelected()
{
    return (gtk_editable_get_selection_bounds(GTK_EDITABLE(gtk_bin_get_child(GTK_BIN(WordCombo))),NULL,NULL));
}

/*
gint TopWin::BackListDataCompareFunc(gconstpointer a,gconstpointer b)
{
	return strcmp(((const BackListData *)a)->word, ((const BackListData *)b)->word);
}
*/

void TopWin::InsertHisList(const gchar *word)
{
	if (!word || word[0]=='\0')
		return;

	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(WordCombo));
	GtkTreeIter iter;
	gboolean have_item = gtk_tree_model_get_iter_first(model, &iter);
	gchar *str;
	size_t count = 0;
	GtkTreeIter iter_last;
	while (have_item) {
		count++;
		iter_last = iter;
		gtk_tree_model_get(model, &iter, 0, &str, -1);
		if (strcmp(str, word)==0) {
			gtk_list_store_move_after(GTK_LIST_STORE(model), &iter, NULL);
			g_free(str);
			return;
		}
		g_free(str);
		have_item = gtk_tree_model_iter_next(model, &iter);
	}
	if (count >= MAX_HISTORY_WORD_ITEM_NUM) {
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter_last);
	}
	gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
	gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, word, -1);
}

void TopWin::SaveHistory()
{
	const std::string& filePath = conf->get_string_at("dictionary/history");
#ifdef _WIN32
	FILE *f=g_fopen(abs_path_to_data_dir(filePath).c_str(), "w");
#else
	FILE *f=g_fopen(filePath.c_str(), "w");
#endif
	if (!f)
		return;
	GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(WordCombo));
	GtkTreeIter iter;
	gboolean have_item = gtk_tree_model_get_iter_first(model, &iter);
	gchar *word;
	while (have_item) {
		gtk_tree_model_get(model, &iter, 0, &word, -1);
#ifdef _MSC_VER
		fprintf_s(f, "%s\n", word);
#else
		fprintf(f, "%s\n", word);
#endif
		g_free(word);
		have_item = gtk_tree_model_iter_next(model, &iter);
	}
	fclose(f);
}

void TopWin::LoadHistory(GtkListStore* list_store)
{
	const std::string& filePath = conf->get_string_at("dictionary/history");
	gchar *buffer;
#ifdef _WIN32
	if(!g_file_get_contents(abs_path_to_data_dir(filePath).c_str(), &buffer, NULL, NULL))
#else
	if(!g_file_get_contents(filePath.c_str(), &buffer, NULL, NULL))
#endif
		return;
	gchar *p,*p1;
	p=buffer;
	GtkTreeIter iter;
	while (*p) {
		p1=strchr(p, '\n');
		if (p1) {
			*p1='\0';
			gtk_list_store_append(list_store, &iter);
			gtk_list_store_set(list_store, &iter, 0, p, -1);
			p = p1+1;
		} else {
			break;
		}
	}
	g_free(buffer);
}

void TopWin::InsertBackList(const gchar *word)
{
	BackListData *backlistdata;
	if (word) {
		if (word[0] == '\0')
			return;
		if (BackList && (strcmp(((BackListData *)(BackList->data))->word, word) == 0))
			return;
		backlistdata = (BackListData *)g_malloc(sizeof(BackListData));
		backlistdata->word = g_strdup(word);
		backlistdata->adjustment_value = -1;
	} else {
		word = gtk_entry_get_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(WordCombo))));
		if (word[0] == '\0')
			return;
		if (BackList && (strcmp(((BackListData *)(BackList->data))->word, word) == 0))
			return;
		backlistdata = (BackListData *)g_malloc(sizeof(BackListData));
		backlistdata->word = g_strdup(word);
		backlistdata->adjustment_value = gpAppFrame->oMidWin.oTextWin.view->scroll_pos();
	}
	for (guint i=0; i< BackList_index; i++) {
		g_free(((BackListData *)((BackList)->data))->word);
		g_free(BackList->data);
		BackList = g_list_delete_link(BackList, BackList);
	}
	if (BackList != NULL)
		gtk_widget_set_sensitive(back_button, TRUE);
	gtk_widget_set_sensitive(forward_button, FALSE);
	BackList = g_list_prepend(BackList, backlistdata);
	BackList_index = 0;
	if (g_list_length(BackList)> MAX_BACK_WORD_ITEM_NUM)	{
		GList *list = g_list_last(BackList);
		g_free(((BackListData *)(list->data))->word);
		g_free(list->data);
		BackList = g_list_delete_link(BackList,list);
	}
}

/**************************************************/
void ListWin::Create(GtkWidget *notebook)
{
	list_word_type = LIST_WIN_EMPTY;

	list_model = gtk_list_store_new (1,G_TYPE_STRING);
	tree_model = gtk_tree_store_new (1,G_TYPE_STRING);
	treeview_ = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_model)));
	nowIsList = true;
	gtk_widget_show(GTK_WIDGET(treeview_));
	//gtk_tree_view_set_rules_hint(treeview_, TRUE);
	gtk_tree_view_set_headers_visible(treeview_, FALSE);

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column(treeview_, column);

	GtkTreeSelection *selection =
		gtk_tree_view_get_selection(treeview_);
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);

	g_signal_connect(G_OBJECT(treeview_), "button_press_event",
			 G_CALLBACK(on_button_press), this);

	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolledwindow), GTK_WIDGET(treeview_));

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
}

void ListWin::SetModel(bool isListModel)
{
	if (isListModel) {
		if (!nowIsList) {
			gtk_tree_view_set_model(treeview_, GTK_TREE_MODEL(list_model));
			nowIsList = true;
		}
	} else {
		if (nowIsList) {
			gtk_tree_view_set_model(treeview_, GTK_TREE_MODEL(tree_model));
			nowIsList = false;
		}
	}
}

void ListWin::Destroy()
{
	g_object_unref (list_model);
	g_object_unref (tree_model);
}

void ListWin::Clear()
{
	gtk_list_store_clear(list_model);
	gtk_tree_store_clear(tree_model);
}

/*
void ListWin::RemoveLast()
{
	//note,this function is not to remove the list row,but the LIST_WIN_ROW_NUM row.
	gchar lastrow[5];
	sprintf(lastrow,"%d", LIST_WIN_ROW_NUM); // not LIST_WIN_ROW_NUM -1,as Prepend is done first.
	GtkTreePath *path = gtk_tree_path_new_from_string (lastrow);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL(list_model), &iter, path)) {
		gtk_list_store_remove(list_model, &iter);
	}
	gtk_tree_path_free(path);
}

void ListWin::UnSelectAll()
{
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview)));
}
*/

void ListWin::Prepend(const gchar *word)
{
	GtkTreeIter iter;
	gtk_list_store_prepend (list_model, &iter);
	gtk_list_store_set (list_model, &iter, 0, word, -1);
}

void ListWin::ReScroll()
{
	GtkTreePath *path = gtk_tree_path_new_from_string ("0");
	gtk_tree_view_scroll_to_cell(treeview_, path, NULL, FALSE, 0, 0);
	gtk_tree_path_free(path);
	/*if (gtk_widget_get_realized(GTK_WIDGET(treeview)))
		gtk_tree_view_scroll_to_point(GTK_TREE_VIEW(treeview),0,0);*/
}

void ListWin::InsertLast(const gchar *word)
{
	GtkTreeIter iter;
	gtk_list_store_append (list_model, &iter);
	gtk_list_store_set (list_model, &iter, 0, word, -1);
}

void ListWin::SetTreeModel(std::vector<gchar *> *reslist, std::vector<InstantDictIndex> &dictmask)
{
	GtkTreeIter parent;
	GtkTreeIter iter;
	const char *bookname = "";
	for (size_t i=0; i<dictmask.size(); i++) {
		if (!reslist[i].empty()) {
			gtk_tree_store_append(tree_model, &parent, NULL);
			if (dictmask[i].type == InstantDictType_LOCAL)
				bookname = gpAppFrame->oLibs.dict_name(dictmask[i].index).c_str();
			else if (dictmask[i].type == InstantDictType_VIRTUAL)
				bookname = gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(dictmask[i].index);
			else if (dictmask[i].type == InstantDictType_NET)
				bookname = gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(dictmask[i].index);
			gtk_tree_store_set(tree_model, &parent, 0, bookname, -1);
			for (std::vector<gchar *>::iterator p=reslist[i].begin();
			     p != reslist[i].end(); ++p) {
				gtk_tree_store_append(tree_model, &iter, &parent);
				gtk_tree_store_set (tree_model, &iter, 0, *p, -1);
				g_free(*p);
			}
		}
	}
	gtk_tree_view_expand_all(treeview_);
}

void ListWin::SetTreeModel(std::list<STARDICT::LookupResponse::WordTreeElement *> *wordtree)
{
    GtkTreeIter parent;
    GtkTreeIter iter;
    for (std::list<STARDICT::LookupResponse::WordTreeElement *>::iterator i = wordtree->begin(); i != wordtree->end(); ++i) {
        gtk_tree_store_append(tree_model, &parent, NULL);
        gtk_tree_store_set(tree_model, &parent, 0, (*i)->bookname, -1);
        for (std::list<char *>::iterator j = (*i)->wordlist.begin(); j != (*i)->wordlist.end(); ++j) {
            gtk_tree_store_append(tree_model, &iter, &parent);
            gtk_tree_store_set (tree_model, &iter, 0, *j, -1);
        }
    }
    gtk_tree_view_expand_all(treeview_);
}

static inline bool less_for_word_compare(const char *lh, const char *rh) {
        return stardict_server_collate(lh, rh, gpAppFrame->oLibs.get_CollationLevel(), gpAppFrame->oLibs.get_CollateFunction(), 0)<0;
}

void ListWin::MergeWordList(std::list<char *> *wordlist)
{
	std::list<char *> current_list;
	gboolean have_iter;
	GtkTreeIter iter;
	gchar *word;
	GtkTreeModel *model = GTK_TREE_MODEL(list_model);
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &word, -1);
		current_list.push_back(word);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	std::vector<char *> merge_list;
	for (std::list<char *>::iterator i = current_list.begin(); i != current_list.end(); ++i) {
		merge_list.push_back(*i);
	}
	bool duplicated;
	for (std::list<char *>::iterator i = wordlist->begin(); i != wordlist->end(); ++i) {
		duplicated = false;
		for (std::list<char *>::iterator j = current_list.begin(); j != current_list.end(); ++j) {
			if (strcmp(*i, *j)==0) {
				duplicated = true;
				break;
			}
		}
		if (duplicated)
			continue;
		merge_list.push_back(*i);
	}
	std::sort(merge_list.begin(), merge_list.end(), less_for_word_compare);
	gtk_list_store_clear(list_model);
	for (std::vector<char *>::iterator i = merge_list.begin(); i != merge_list.end(); ++i) {
		gtk_list_store_append (list_model, &iter);
		gtk_list_store_set (list_model, &iter, 0, *i, -1);
	}
	for (std::list<char *>::iterator i = current_list.begin(); i != current_list.end(); ++i) {
		g_free(*i);
	}
}

struct FuzzyWordstruct {
	char * pMatchWord;
	int iMatchWordDistance;
};

static inline bool less_for_fuzzy_compare(const FuzzyWordstruct & lh, const FuzzyWordstruct & rh) {
	if (lh.iMatchWordDistance!=rh.iMatchWordDistance)
		return lh.iMatchWordDistance<rh.iMatchWordDistance;
	return stardict_server_collate(lh.pMatchWord, rh.pMatchWord, gpAppFrame->oLibs.get_CollationLevel(), gpAppFrame->oLibs.get_CollateFunction(), 0)<0;
}

static inline void unicode_strdown(gunichar *str) {
	while (*str) {
		*str=g_unichar_tolower(*str);
		++str;
	}
}

void ListWin::MergeFuzzyList(std::list<char *> *wordlist)
{
	std::list<char *> current_list;
	gboolean have_iter;
	GtkTreeIter iter;
	gchar *word;
	GtkTreeModel *model = GTK_TREE_MODEL(list_model);
	have_iter = gtk_tree_model_get_iter_first(model, &iter);
	while (have_iter) {
		gtk_tree_model_get (model, &iter, 0, &word, -1);
		current_list.push_back(word);
		have_iter = gtk_tree_model_iter_next(model, &iter);
	}
	std::vector<FuzzyWordstruct> merge_list;
	FuzzyWordstruct fuzzyitem;
	EditDistance oEditDistance;
	gunichar *ucs4_str1, *ucs4_str2;
	glong ucs4_str1_len, ucs4_str2_len;
	ucs4_str2 = g_utf8_to_ucs4_fast(fuzzyWord.c_str(), -1, &ucs4_str2_len);
	unicode_strdown(ucs4_str2);
	for (std::list<char *>::iterator i = current_list.begin(); i != current_list.end(); ++i) {
		fuzzyitem.pMatchWord = *i;
		ucs4_str1 = g_utf8_to_ucs4_fast(*i, -1, &ucs4_str1_len);
		if (ucs4_str1_len > ucs4_str2_len)
			ucs4_str1[ucs4_str2_len]=0;
		unicode_strdown(ucs4_str1);
		fuzzyitem.iMatchWordDistance = oEditDistance.CalEditDistance(ucs4_str1, ucs4_str2, MAX_FUZZY_DISTANCE);
		g_free(ucs4_str1);
		merge_list.push_back(fuzzyitem);
	}
	bool duplicated;
	for (std::list<char *>::iterator i = wordlist->begin(); i != wordlist->end(); ++i) {
		duplicated = false;
		for (std::list<char *>::iterator j = current_list.begin(); j != current_list.end(); ++j) {
			if (strcmp(*i, *j)==0) {
				duplicated = true;
				break;
			}
		}
		if (duplicated)
			continue;
		fuzzyitem.pMatchWord = *i;
		ucs4_str1 = g_utf8_to_ucs4_fast(*i, -1, &ucs4_str1_len);
		if (ucs4_str1_len > ucs4_str2_len)
			ucs4_str1[ucs4_str2_len]=0;
		unicode_strdown(ucs4_str1);
		fuzzyitem.iMatchWordDistance = oEditDistance.CalEditDistance(ucs4_str1, ucs4_str2, MAX_FUZZY_DISTANCE);
		g_free(ucs4_str1);
		merge_list.push_back(fuzzyitem);
	}
	g_free(ucs4_str2);
	std::sort(merge_list.begin(), merge_list.end(), less_for_fuzzy_compare);
	gtk_list_store_clear(list_model);
	for (std::vector<FuzzyWordstruct>::iterator i = merge_list.begin(); i != merge_list.end(); ++i) {
		gtk_list_store_append (list_model, &iter);
		gtk_list_store_set (list_model, &iter, 0, i->pMatchWord, -1);
	}
	for (std::list<char *>::iterator i = current_list.begin(); i != current_list.end(); ++i) {
		g_free(*i);
	}
}

gboolean ListWin::on_button_press(GtkWidget * widget, GdkEventButton * event, ListWin *oListWin)
{
	if (event->type==GDK_2BUTTON_PRESS) {
		GtkTreeModel *model;
		GtkTreeIter iter;

		GtkTreeSelection *selection =
			gtk_tree_view_get_selection(oListWin->treeview_);
		if (gtk_tree_selection_get_selected (selection, &model, &iter))	{
			if (!oListWin->nowIsList) {
				if (gtk_tree_model_iter_has_child(model, &iter)) {
                                	GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
                                	if (gtk_tree_view_row_expanded(
						    oListWin->treeview_, path))
                                        	gtk_tree_view_collapse_row(				
							oListWin->treeview_, path);
                                	else
                                        	gtk_tree_view_expand_row(
							oListWin->treeview_, path, FALSE);
                                	gtk_tree_path_free(path);
					return TRUE;
                        	}
			}
			gchar *word;
			gtk_tree_model_get(model, &iter, 0, &word, -1);
			gpAppFrame->ListClick(word);
			g_free(word);
		}
		return TRUE;
	} else
		return FALSE;	
}

void ListWin::on_selection_changed(GtkTreeSelection *selection, ListWin *oListWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		if (!oListWin->nowIsList) {
			gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
			if (!strchr(path_str, ':')) { // bookname entry.
				g_free(path_str);
				return;
			}
			g_free(path_str);
		}
		gchar *word;
		gtk_tree_model_get (model, &iter, 0, &word, -1);
		gpAppFrame->SimpleLookupToTextWin(word, NULL);
		bool enable_netdict = conf->get_bool_at("network/enable_netdict");
		if (enable_netdict) {
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_DEFINE, word);
			if (!gpAppFrame->oStarDictClient.try_cache(c)) {
				gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
				gpAppFrame->oStarDictClient.send_commands(1, c);
			}
		}
		gpAppFrame->LookupNetDict(word, true);
		g_free(word);
	}
}

/**************************************************/
bool TreeWin::Create(GtkWidget *notebook)
{
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
	GtkTreeStore *model =
		gpAppFrame->oTreeDicts.Load(treedict_dirs_list_abs,
					treedict_order_list_abs,
					treedict_disable_list_abs);
#else
	GtkTreeStore *model =
		gpAppFrame->oTreeDicts.Load(treedict_dirs_list,
					treedict_order_list,
					treedict_disable_list);
#endif
	if (!model)
		return false;
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	gtk_widget_show(treeview);
	g_object_unref (model);
	//gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);

	g_signal_connect (G_OBJECT (treeview), "button_press_event", G_CALLBACK (on_button_press), this);
	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
	return true;
}

gboolean TreeWin::on_button_press(GtkWidget * widget, GdkEventButton * event, TreeWin *oTreeWin)
{
	if (event->type==GDK_2BUTTON_PRESS)
	{
		GtkTreeModel *model;
		GtkTreeIter iter;

		GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (oTreeWin->treeview));
		if (gtk_tree_selection_get_selected (selection, &model, &iter))
		{
			if (gtk_tree_model_iter_has_child(model, &iter)) {
				GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
				if (gtk_tree_view_row_expanded(GTK_TREE_VIEW (oTreeWin->treeview), path))
					gtk_tree_view_collapse_row(GTK_TREE_VIEW (oTreeWin->treeview), path);
				else
					gtk_tree_view_expand_row(GTK_TREE_VIEW (oTreeWin->treeview), path, false);
				gtk_tree_path_free(path);
			}
			else {
				guint32 offset, size;
				gtk_tree_model_get (model, &iter, 1, &offset, 2, &size, -1);
				if (size != 0) {
					gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
					gint iTreeDict;
					gchar *p = strchr(path_str, ':');
					if (p)
						*p = '\0';
					iTreeDict = atoi(path_str);
					gpAppFrame->ShowTreeDictDataToTextWin(offset, size, iTreeDict);
					g_free(path_str);
				}
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

void TreeWin::on_selection_changed(GtkTreeSelection *selection, TreeWin *oTreeWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		guint32 offset, size;
		gtk_tree_model_get (model, &iter, 1, &offset, 2, &size, -1);
		if (size != 0) {
			gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
			gint iTreeDict;
			gchar *p = strchr(path_str, ':');
			if (p)
				*p = '\0';
			iTreeDict = atoi(path_str);
			gpAppFrame->ShowTreeDictDataToTextWin(offset, size, iTreeDict);
			g_free(path_str);
		}
	}
}


/**************************************************/
void ResultWin::Create(GtkWidget *notebook)
{
	GtkListStore *model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	gtk_widget_show(treeview);
	g_object_unref (model);
	//gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes ("word", renderer,
						     "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed", G_CALLBACK (on_selection_changed), this);

	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
				       GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
}

void ResultWin::InsertLast(const gchar *word, const gchar *mark)
{
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	GtkTreeIter iter;
	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, word, 1, mark, -1);
}

void ResultWin::Clear()
{
	GtkListStore *model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)));
	gtk_list_store_clear(model);
}

void ResultWin::on_selection_changed(GtkTreeSelection *selection, ResultWin *oResultWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter))
	{
		gchar *markstr;
		gtk_tree_model_get(model, &iter, 1, &markstr, -1);
		GtkTextView *textview = GTK_TEXT_VIEW(gpAppFrame->oMidWin.oTextWin.view->widget());
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
		GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, markstr);
		g_free(markstr);
		if (mark) {
			gtk_text_view_scroll_to_mark(textview, mark, 0.0, TRUE, 0.0, 0.0);
		}
	}
}

/**************************************************/
void HistoryWin::Create(GtkWidget *notebook)
{

	GtkListStore *model = gpAppFrame->oTopWin.get_wordcombo_model();

	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(model));
	gtk_widget_show(treeview);
	g_object_unref (model);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (
		"word", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
	g_signal_connect (G_OBJECT (treeview), "key_press_event",
		G_CALLBACK (on_key_pressed), this);

	GtkTreeSelection *selection = gtk_tree_view_get_selection (
		GTK_TREE_VIEW (treeview));
	g_signal_connect (G_OBJECT (selection), "changed",
		G_CALLBACK (on_selection_changed), this);

	GtkWidget *scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_shadow_type (
		GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	gtk_container_add(GTK_CONTAINER(scrolledwindow),treeview);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolledwindow, NULL);
}

gboolean HistoryWin::on_key_pressed(GtkWidget *widget, GdkEventKey *event,
		HistoryWin *oHistoryWin)
{
	if (event->type==GDK_KEY_PRESS &&
			(event->keyval==GDK_KEY_Delete || event->keyval==GDK_KEY_KP_Delete)) {
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreeSelection *selection = gtk_tree_view_get_selection (
				GTK_TREE_VIEW (oHistoryWin->treeview));
		if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			gtk_tree_selection_select_iter(selection, &iter);
			// Select last row
			if (!gtk_tree_selection_get_selected(selection, &model, &iter)) {
				gint total = gtk_tree_model_iter_n_children(model, NULL);
				gtk_tree_model_iter_nth_child (model, &iter, NULL, total-1);
				gtk_tree_selection_select_iter(selection, &iter);
			}
			return TRUE;
		}
	}
	return FALSE;
}

void HistoryWin::on_selection_changed(GtkTreeSelection *selection, HistoryWin *oHistoryWin)
{
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gchar *word;
		gtk_tree_model_get (model, &iter, 0, &word, -1);
		gpAppFrame->SimpleLookupToTextWin(word, NULL);
		bool enable_netdict = conf->get_bool_at(
			"network/enable_netdict");
		if (enable_netdict) {
			STARDICT::Cmd *c = new STARDICT::Cmd(
				STARDICT::CMD_DEFINE, word);
			if (!gpAppFrame->oStarDictClient.try_cache(c)) {
				gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
				gpAppFrame->oStarDictClient.send_commands(1, c);
			}
		}
		gpAppFrame->LookupNetDict(word, true);
		g_free(word);
	}
}

/**************************************************/
LeftWin::LeftWin()
{
	choosegroup_menu = NULL;
}

LeftWin::~LeftWin()
{
	if (choosegroup_menu)
		gtk_widget_destroy(choosegroup_menu);
}

void LeftWin::Create(GtkWidget *hbox, bool has_treedict)
{
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
#else
	vbox = gtk_vbox_new(FALSE, 3);
#endif
	if (!conf->get_bool_at("main_window/hide_list"))
		gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox),vbox, false, false, 0);

	GtkWidget *wazard_button = gtk_radio_button_new(NULL);
	gtk_widget_set_can_focus (wazard_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(wazard_button), false);
	gtk_box_pack_start(GTK_BOX(vbox),wazard_button, false, false, 0);
	GtkWidget *image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_wazard));
	gtk_container_add (GTK_CONTAINER (wazard_button), image);
	gtk_widget_show_all(wazard_button);
	gtk_widget_set_tooltip_text(wazard_button,_("List"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wazard_button), true);
	g_signal_connect(G_OBJECT(wazard_button),"toggled", G_CALLBACK(on_wazard_button_toggled), this);

	GtkWidget *result_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
	gtk_widget_set_can_focus (result_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(result_button), false);
	gtk_box_pack_start(GTK_BOX(vbox),result_button, false, false, 0);
	image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_dictlist));
	gtk_container_add (GTK_CONTAINER (result_button), image);
	gtk_widget_show_all(result_button);
	gtk_widget_set_tooltip_text(result_button,_("Result"));
	g_signal_connect(G_OBJECT(result_button),"toggled", G_CALLBACK(on_result_button_toggled), this);
	
	GtkWidget *translate_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(result_button));
	gtk_widget_set_can_focus (translate_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(translate_button), false);
	gtk_box_pack_start(GTK_BOX(vbox),translate_button, false, false, 0);
	image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_translate));
	gtk_container_add (GTK_CONTAINER (translate_button), image);
	gtk_widget_show_all(translate_button);
	gtk_widget_set_tooltip_text(translate_button,_("Full-Text Translation"));
	g_signal_connect(G_OBJECT(translate_button),"toggled", G_CALLBACK(on_translate_button_toggled), this);

	GtkWidget *history_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(wazard_button));
	gtk_widget_set_can_focus (history_button, FALSE);
	gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(history_button), false);
	gtk_box_pack_start(GTK_BOX(vbox),history_button, false, false, 0);
	image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_history));
	gtk_container_add (GTK_CONTAINER (history_button), image);
	gtk_widget_show_all(history_button);
	gtk_widget_set_tooltip_text(history_button,_("History"));
	g_signal_connect(G_OBJECT(history_button),"toggled", G_CALLBACK(on_history_button_toggled), this);
	
	if (has_treedict) {
		GtkWidget *appendix_button = gtk_radio_button_new_from_widget(GTK_RADIO_BUTTON(translate_button));
		gtk_widget_set_can_focus (appendix_button, FALSE);
		gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(appendix_button), false);
		gtk_box_pack_start(GTK_BOX(vbox),appendix_button, false, false, 0);
		image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_appendix));
		gtk_container_add (GTK_CONTAINER (appendix_button), image);
		gtk_widget_show_all(appendix_button);
		gtk_widget_set_tooltip_text(appendix_button,_("Tree"));
		g_signal_connect(G_OBJECT(appendix_button),"toggled", G_CALLBACK(on_appendix_button_toggled), this);
	}

	GtkWidget *choosegroup_button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(choosegroup_button),gtk_image_new_from_stock(GTK_STOCK_CONVERT,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(choosegroup_button);
	gtk_button_set_relief (GTK_BUTTON (choosegroup_button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (choosegroup_button, FALSE);
	g_signal_connect(G_OBJECT(choosegroup_button),"clicked", G_CALLBACK(on_choose_group_button_clicked),this);
	gtk_box_pack_start(GTK_BOX(vbox),choosegroup_button,false,false,0);
	gtk_widget_set_tooltip_text(choosegroup_button, _("Choose dict group"));

	GtkWidget *button;
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_GO_DOWN,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(NextCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(vbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Next word (Alt+Down)"));

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_GO_UP,GTK_ICON_SIZE_BUTTON));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PreviousCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(vbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Previous word (Alt+Up)"));
}

void LeftWin::on_wazard_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.oIndexWin.notebook), 0);
	}
}

void LeftWin::on_result_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.oIndexWin.notebook), 1);
	}
}

void LeftWin::on_history_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.oIndexWin.notebook), 2);
	}
}

void LeftWin::on_translate_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin)
{
	if (gtk_toggle_button_get_active(button))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.notebook), 1);
}

void LeftWin::on_appendix_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin)
{
	if (gtk_toggle_button_get_active(button)) {
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.notebook), 0);
		gtk_notebook_set_current_page(GTK_NOTEBOOK(gpAppFrame->oMidWin.oIndexWin.notebook), 2);
	}
}

void LeftWin::PreviousCallback(GtkWidget *widget, LeftWin *oLeftWin)
{
	play_sound_on_event("buttonactive");

	gpAppFrame->oTopWin.do_prev();
}

void LeftWin::NextCallback(GtkWidget *widget, LeftWin *oLeftWin)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->oTopWin.do_next();
}

void LeftWin::UpdateChooseGroup()
{
	if (choosegroup_menu)
		gtk_widget_destroy(choosegroup_menu);
	choosegroup_menu = gtk_menu_new();
	GtkWidget *menuitem;
	menuitem = gtk_check_menu_item_new_with_label(_("Enable Net Dict"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), conf->get_bool_at("network/enable_netdict"));
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(on_enable_netdict_menuitem_toggled), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(choosegroup_menu), menuitem);
	menuitem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(choosegroup_menu), menuitem);
	std::list<std::string> group_list;
	for (std::list<DictManageGroup>::iterator i = gpAppFrame->dictinfo.groups.begin(); i != gpAppFrame->dictinfo.groups.end(); ++i) {
		group_list.push_back(i->name);
	}
	GSList *group = NULL;
	for (std::list<std::string>::iterator i = group_list.begin(); i != group_list.end(); ++i) {
		menuitem = gtk_radio_menu_item_new_with_label(group, i->c_str());
		g_object_set_data_full(G_OBJECT(menuitem), "stardict_dict_group", g_strdup(i->c_str()), g_free);
		g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(on_choose_group_menuitem_toggled), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(choosegroup_menu), menuitem);
		group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
		if (*i == gpAppFrame->dictinfo.active_group) {
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
		}
	}
	gtk_widget_show_all(choosegroup_menu);
}

void LeftWin::on_enable_netdict_menuitem_toggled(GtkCheckMenuItem *menuitem, LeftWin *oLeftWin)
{
	bool enable_netdict = static_cast<bool>(gtk_check_menu_item_get_active(menuitem));
	if(enable_netdict) {
		if(!confirm_enable_network_dicts())
			enable_netdict = false;
	}
	conf->set_bool_at("network/enable_netdict", enable_netdict);
}

void LeftWin::on_choose_group_menuitem_toggled(GtkCheckMenuItem *menuitem, LeftWin *oLeftWin)
{
	if (gtk_check_menu_item_get_active(menuitem)) {
		const char *group = (const char *)g_object_get_data(G_OBJECT(menuitem), "stardict_dict_group");
		gpAppFrame->dictinfo.active_group = group;
		UpdateDictMask();

		const gchar *sWord = gpAppFrame->oTopWin.get_text();
		if (sWord && sWord[0])
			gpAppFrame->TopWinWordChange(sWord);
	}
}

void LeftWin::on_choose_group_button_clicked(GtkWidget *widget, LeftWin *oLeftWin)
{
	oLeftWin->UpdateChooseGroup();
	if (oLeftWin->choosegroup_menu) {
		play_sound_on_event("menushow");
		gtk_menu_popup(GTK_MENU(oLeftWin->choosegroup_menu), NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
	}
}

/**************************************************/
IndexWin::IndexWin()
{
}

bool IndexWin::Create(GtkWidget *hpaned)
{
	notebook = gtk_notebook_new();
	if (!conf->get_bool_at("main_window/hide_list"))
		gtk_widget_show(notebook);

	gtk_paned_pack1(GTK_PANED(hpaned),notebook,true,false);

	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook),FALSE);
	oListWin.Create(notebook);
	oResultWin.Create(notebook);
	oHistoryWin.Create(notebook);

	return oTreeWin.Create(notebook);
}

/************************************************/

ToolWin::ToolWin()
{
}

ToolWin::~ToolWin()
{
}

void ToolWin::on_pronounce_menu_item_activate(GtkMenuItem *menuitem, int engine_index)
{
	gpAppFrame->oReadWord.ReadByEngine(gpAppFrame->oMidWin.oTextWin.pronounceWord.c_str(), engine_index);
}

void ToolWin::UpdatePronounceMenu()
{
	GtkWidget *PronounceWordMenu;
	std::list<std::pair<std::string, int> > engine_list = gpAppFrame->oReadWord.GetEngineList();
	if (engine_list.empty()) {
		PronounceWordMenu = NULL;
	} else {
		PronounceWordMenu = gtk_menu_new();
		GtkWidget *menuitem;
		for (std::list<std::pair<std::string, int> >::iterator i = engine_list.begin(); i != engine_list.end(); ++i) {
			menuitem = gtk_menu_item_new_with_label(i->first.c_str());
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_pronounce_menu_item_activate), GINT_TO_POINTER(i->second));
			gtk_menu_shell_append(GTK_MENU_SHELL(PronounceWordMenu), menuitem);
		}
		gtk_widget_show_all(PronounceWordMenu);
	}
	gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(PronounceWordMenuButton), PronounceWordMenu);
}

static void unfocus_menu_button(GtkWidget *widget, gpointer data)
{
	gtk_widget_set_can_focus(widget, FALSE);
	if (GTK_IS_CONTAINER(widget))
		gtk_container_forall(GTK_CONTAINER(widget), unfocus_menu_button, data);
}

void ToolWin::Create(GtkWidget *vbox)
{
	GtkWidget *hbox;
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	hbox = gtk_hbox_new(false,0);
#endif
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(vbox),hbox,false,false,3);
#endif

	GtkWidget *image;
	ShowListButton=gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_LAST,GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(ShowListButton),image);
	gtk_button_set_relief (GTK_BUTTON (ShowListButton), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (ShowListButton, FALSE);
	g_signal_connect(G_OBJECT(ShowListButton),"clicked", G_CALLBACK(ShowListCallback),NULL);
	g_signal_connect(G_OBJECT(ShowListButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),ShowListButton,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),ShowListButton,false,false,5);
#endif
	gtk_widget_set_tooltip_text(ShowListButton,_("Show the word list"));

	HideListButton=gtk_button_new();
	image = gtk_image_new_from_stock(GTK_STOCK_GOTO_FIRST,GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(HideListButton),image);
	gtk_button_set_relief (GTK_BUTTON (HideListButton), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (HideListButton, FALSE);
	g_signal_connect(G_OBJECT(HideListButton),"clicked", G_CALLBACK(HideListCallback),NULL);
	g_signal_connect(G_OBJECT(HideListButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),HideListButton,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),HideListButton,false,false,5);
#endif
	gtk_widget_set_tooltip_text(HideListButton,_("Hide the word list"));


	if (conf->get_bool_at("main_window/hide_list"))
		gtk_widget_show(ShowListButton);
	else
		gtk_widget_show(HideListButton);


	GtkWidget *button;
#ifndef CONFIG_GPE
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_COPY,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(CopyCallback),this);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);
	gtk_widget_set_tooltip_text(button,_("Copy"));
#endif

	PronounceWordMenuButton = gtk_menu_tool_button_new_from_stock(GTK_STOCK_EXECUTE);
	UpdatePronounceMenu();
	gtk_widget_show(GTK_WIDGET(PronounceWordMenuButton));
	gtk_widget_set_can_focus (GTK_WIDGET(PronounceWordMenuButton), FALSE);
	gtk_container_forall(GTK_CONTAINER(PronounceWordMenuButton), unfocus_menu_button, this);
	g_signal_connect(G_OBJECT(PronounceWordMenuButton),"clicked", G_CALLBACK(PlayCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(PronounceWordMenuButton),false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),GTK_WIDGET(PronounceWordMenuButton),false,false,5);
#endif
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(PronounceWordMenuButton), _("Pronounce the word (Enter)"));
	gtk_widget_set_sensitive(GTK_WIDGET(PronounceWordMenuButton), false);

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_SAVE,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(SaveCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);
#endif
	gtk_widget_set_tooltip_text(button,_("Save to file (Alt+E)"));

#if 0
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PRINT,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PrintCallback),this);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);
	gtk_widget_set_tooltip_text(button,_("Print"));
	gtk_widget_set_sensitive(button, false);
#endif

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(SearchCallback),this);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,5);
#endif
	gtk_widget_set_tooltip_text(button,_("Search in the definition text (Ctrl+F)"));
}

void ToolWin::ShowListCallback(GtkWidget *widget, gpointer data)
{
  conf->set_bool_at("main_window/hide_list", false);
}

void ToolWin::HideListCallback(GtkWidget *widget, gpointer data)
{
  conf->set_bool_at("main_window/hide_list", true);
}

#ifndef CONFIG_GPE
void ToolWin::CopyCallback(GtkWidget *widget, ToolWin *oToolWin)
{
  std::string text = gpAppFrame->oMidWin.oTextWin.view->get_text();

  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_set_text(clipboard, text.c_str(), -1);
}
#endif

void ToolWin::PlayCallback(GtkWidget *widget, ToolWin *oToolWin)
{
	gpAppFrame->oTopWin.InsertHisList(gpAppFrame->oTopWin.get_text());
	gpAppFrame->oTopWin.InsertBackList();
	gpAppFrame->oReadWord.read(gpAppFrame->oMidWin.oTextWin.pronounceWord.c_str(), gpAppFrame->oMidWin.oTextWin.readwordtype);
}

void ToolWin::do_save()
{
	TextWin &oTextWin=gpAppFrame->oMidWin.oTextWin;

	if (conf->get_bool_at("dictionary/only_export_word")) {
		if (!oTextWin.queryWord.empty()) {
			const std::string& filePath = conf->get_string_at("dictionary/export_file");
#ifdef _WIN32
			FILE *fp = fopen(abs_path_to_data_dir(filePath).c_str(), "a+");
#else
			FILE *fp = fopen(filePath.c_str(), "a+");
#endif
			if(fp) {
				fputs(oTextWin.queryWord.c_str(),fp);
				fputs("\n",fp);
				fclose(fp);
			}
		}
	} else {
		// check for selections in Text Area
		GtkTextIter start, end;
		gchar *str = NULL;
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin.view->widget()));
		if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end))
			 str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		const std::string& filePath = conf->get_string_at("dictionary/export_file");
#ifdef _WIN32
		FILE *fp = fopen(abs_path_to_data_dir(filePath).c_str(), "a+");
#else
		FILE *fp = fopen(filePath.c_str(), "a+");
#endif
		if(fp) {
			if(str) {
				fputs(oTextWin.queryWord.c_str(),fp);
				fputs("\n",fp);
				fputs(str,fp);
				fputs("\n",fp);
			} else {
				fputs(oTextWin.view->get_text().c_str(),fp);
			}
			//separat line
			fputs("\n",fp);
			fclose(fp);
		}
		if (str)
			g_free(str);
	}
	play_sound_on_event("buttonactive");
}

void ToolWin::SaveCallback(GtkWidget *widget, ToolWin *oToolWin)
{
	oToolWin->do_save();
}

void ToolWin::do_search()
{
	TextWin &oTextWin=gpAppFrame->oMidWin.oTextWin;
	oTextWin.ShowSearchPanel();
	oTextWin.search_from_beginning = TRUE;

	GtkTextIter start,end;
	GtkTextBuffer *buffer =
		gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin.view->widget()));
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		if (gtk_text_iter_get_offset(&end) - gtk_text_iter_get_offset(&start) < 80) {
			gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
			oTextWin.find_text = str;
      g_free(str);
		}
	}

	if (!oTextWin.find_text.empty())
		gtk_entry_set_text(oTextWin.eSearch, oTextWin.find_text.c_str());
}

void ToolWin::SearchCallback(GtkWidget *widget, ToolWin *oToolWin)
{
	oToolWin->do_search();
}


/**********************************************/

void TextWin::OnCloseSearchPanel(GtkWidget *widget, TextWin *oTextWin)
{
	oTextWin->HideSearchPanel();
}

gboolean TextWin::OnSearchKeyPress(GtkWidget *widget, GdkEventKey *event,
																	 TextWin *oTextWin)
{
	if (event->type==GDK_KEY_PRESS && event->keyval==GDK_KEY_Return)
		gtk_button_clicked(oTextWin->btFind);

	return FALSE;
}

void TextWin::OnFindSearchPanel(GtkWidget *widget, TextWin *oTextWin)
{
  const gchar *text=gtk_entry_get_text(oTextWin->eSearch);
  if (text && *text) {
    if (oTextWin->Find(text, oTextWin->search_from_beginning)) {
      oTextWin->search_from_beginning = FALSE;

      oTextWin->find_text = text;
    } else
      oTextWin->search_from_beginning = TRUE;
  }
}

void TextWin::ShowSearchPanel()
{
  gtk_widget_show(hbSearchPanel);
  gtk_widget_grab_focus(GTK_WIDGET(eSearch));
}

void TextWin::HideSearchPanel()
{
	gtk_widget_hide(hbSearchPanel);
}

void TextWin::set_bookname_style(BookNameStyle style)
{
	if (view.get()) {
		view->set_bookname_style(style);
	}
}

void TextWin::Create(GtkWidget *vbox)
{
	view.reset(new ArticleView(GTK_BOX(vbox), (BookNameStyle)conf->get_int_at("dictionary/bookname_style")));

	view->connect_on_link(sigc::mem_fun(gpAppFrame, &AppCore::on_link_click));
	g_signal_connect(G_OBJECT(view->widget()), "button_press_event",
		G_CALLBACK(on_button_press), this);
	g_signal_connect(G_OBJECT(view->widget()), "selection_received",
		G_CALLBACK(SelectionCallback), this);
	g_signal_connect(G_OBJECT(view->widget()), "populate-popup",
		G_CALLBACK(on_populate_popup), this);

#if GTK_MAJOR_VERSION >= 3
	hbSearchPanel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	hbSearchPanel = gtk_hbox_new(FALSE, 0);
#endif
	btClose = GTK_BUTTON(gtk_button_new());
	gtk_widget_show(GTK_WIDGET(btClose));
	GtkWidget *image =
	gtk_image_new_from_stock(GTK_STOCK_CANCEL,
		GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(btClose), image);
	gtk_button_set_relief(btClose, GTK_RELIEF_NONE);
	gtk_widget_set_can_focus(GTK_WIDGET(btClose), FALSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(btClose), _("Close"));
	g_signal_connect(G_OBJECT(btClose), "clicked",
		G_CALLBACK(OnCloseSearchPanel), this);
	gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(btClose), FALSE, FALSE, 3);
	eSearch = GTK_ENTRY(gtk_entry_new());
	gtk_widget_show(GTK_WIDGET(eSearch));
	gtk_widget_set_size_request(GTK_WIDGET(eSearch), 20, -1);
	g_signal_connect(G_OBJECT(eSearch), "key_press_event",
		G_CALLBACK(OnSearchKeyPress), this);

	gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(eSearch), TRUE, TRUE, 0);
	btFind = GTK_BUTTON(gtk_button_new_from_stock(GTK_STOCK_FIND));
	gtk_widget_show(GTK_WIDGET(btFind));
	gtk_button_set_relief(btFind, GTK_RELIEF_NONE);
	gtk_widget_set_can_focus(GTK_WIDGET(btFind), FALSE);
	gtk_widget_set_tooltip_text(GTK_WIDGET(btFind), _("Find"));
	g_signal_connect(G_OBJECT(btFind), "clicked",
		G_CALLBACK(OnFindSearchPanel), this);

	gtk_box_pack_start(GTK_BOX(hbSearchPanel), GTK_WIDGET(btFind), FALSE, FALSE, 5);

	//gtk_widget_show_all(hbSearchPanel);
	gtk_box_pack_start(GTK_BOX(vbox), hbSearchPanel, FALSE, FALSE, 0);
}

void TextWin::ShowInitFailed()
{
	char *fmt = _("Warning! No dictionary is loaded.\n"
		      "Please go to StarDict's website, download some dictionaries:\n"
		      "%s%s%s and put them in %s.");
	const char *link_pos = strstr(fmt, "%s%s%s");
	LinksPosList links;
	links.push_back(LinkDesc(g_utf8_strlen(fmt, link_pos - fmt),
				 sizeof("http://stardict-4.sourceforge.net") - 1, "http://stardict-4.sourceforge.net"));
	glib::CharStr esc_fmt(g_markup_escape_text(fmt, -1));
	std::string dirs_str;
#ifdef _WIN32
	std::list<std::string> dirs;
	{
		const std::list<std::string>& dirs_rel = conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
		abs_path_to_data_dir(dirs_rel, dirs);
	}
#else
	const std::list<std::string>& dirs = conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list");
#endif
	for(std::list<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it) {
		if(!dirs_str.empty())
			dirs_str += ", ";
		dirs_str += *it;
	}
	glib::CharStr mes(
		g_strdup_printf(get_impl(esc_fmt),
				"<span foreground=\"blue\" underline=\"single\">",
				"http://stardict-4.sourceforge.net",
				"</span>",
				dirs_str.c_str()));
	view->clear();
	view->append_pango_text_with_links(get_impl(mes), links);
	view->scroll_to(0);
}

void TextWin::ShowTips()
{
  query_result = TEXT_WIN_TIPS;
  view->set_text(
	  _("        Welcome to StarDict!\n\n"
	    "   Press Ctrl+Q to quit. Press Alt+Z to iconify (minimize) the window or Alt+X to minimize to tray.\n"
	    "   Press Alt+C or ESC to clear the input entry's text.\n"
	    "   Press Space key to move focus to the input entry.\n"
	    "   If the query word was not found, you can press Tab key to select the first word in the word list.\n"
	    "   After selected some text, clicking the middle mouse button on the main window's Definition area or on the notification area icon will look up that word.\n"
	    "   StarDict can match strings against patterns containing '*' (wildcard) and '?' (joker).\n"
	    "   Input a word beginning with \'/\' to do a Fuzzy query.\n"
	    "   When the floating window reports that a word was not found, double clicking will perform a fuzzy query.\n")
	  );
  view->scroll_to(0);
}

void TextWin::ShowInfo()
{
	query_result = TEXT_WIN_INFO;
	view->set_text(
		_("       Welcome to StarDict\n"
		  "StarDict is a Cross-Platform and international dictionary written in Gtk. "
		  "It has powerful features, such as \"Glob-style pattern matching,\" \"Scan selected word,\" \"Fuzzy query\" and \"Full-text search\".\n\n"
		  "       Here is an introduction to using StarDict:\n\n"
		  "       1. Glob-style pattern matching\n"
		  "       You can input strings containing \'*\' (wildcard) and \'?\' (joker) as the pattern. "
		  "\'*\' matches an arbitrary, possibly empty, string, and \'?\' matches an arbitrary character. "
		  "After pressing Enter, the words that match this pattern will be shown in the list.\n"
		  "       2. Regular expressions matching\n"
		  "       You can input strings as Perl-compatible regular expressions with a beginning \":\" character as the identifier. "
		  "After pressing Enter, the words that match this regex will be shown in the list.\n"
		  "       3. Fuzzy query\n"
		  "       When you can't remember how to spell a word exactly, you can try StarDict's Fuzzy query. "
		  "It uses \"Levenshtein Edit Distance\" to compute the similarity between two words, and gives the match results which are most "
		  "similar to the word that you input. "
		  "To create a fuzzy query, just input the word with a beginning \"/\", and then press Enter.\n"
		  "       4. Full-text search\n"
		  "       Full-text search searches for a word in the data. To create a Full-text search, just input the word with a beginning \"|\", and then press Enter. For example, \"|ab cd\" searches data which contain both \"ab\" and \"cd\". If the words contain Space character, you can use \"\\ \", such as \"|apple\\ pie\", other escaping characters are \"\\\\\" for \'\\\', \"\\t\" for Tab and \"\\n\" for new line.\n"
		  "       5. Special character search\n"
		  "       If your words contain special characters, you can use \'\\\' to escape it, for example, \"a\\*b\\?\" searches \"a*b?\", \"\\/abc\" searches \"/abc\".\n"
		  "       6. Scan the selected word\n"
		  "       Turn on the check button at the bottom-left corner of the StarDict window to activate this feature. "
		  "When this feature is on, StarDict will automatically look up words, phrases, and Chinese characters in other applications. "
		  "Just highlight a word or phrase with your mouse, and a floating window will pop up showing the definition of the "
		  "selected word.\n"
		  "       7. Dictionary management\n"
		  "       Click the \"Manage dictionaries\" button at the bottom-right corner of the window to access the dictionary management "
		  "dialog. From here, you can disable some dictionaries that you don't need, and set the dictionaries\' querying order.\n"
		  "\n\n"
		  "       Statement: This program is distributed in the hope that it will be useful, "
		  "but WITHOUT ANY WARRANTY; without the warranty "
		  "that the word spelling, definition, and phonetic information are correct.\n"
			)
		);

	view->scroll_to(0);
}

void TextWin::Show(const gchar *str)
{
  view->set_text(str);
  view->scroll_to(0);
}

void TextWin::Show(const gchar *orig_word, gchar ***Word, gchar ****WordData)
{
	view->begin_update();
	view->clear();
	view->goto_begin();

	int j,k;
	for (size_t i=0; i<gpAppFrame->query_dictmask.size(); i++) {
		if (Word[i]) {
			view->SetDictIndex(gpAppFrame->query_dictmask[i]);
			if (gpAppFrame->query_dictmask[i].type == InstantDictType_LOCAL) {
				const std::string &dicttype = gpAppFrame->oLibs.dict_type(gpAppFrame->query_dictmask[i].index);
				if (!dicttype.empty()) {
					size_t nPlugins = gpAppFrame->oStarDictPlugins->SpecialDictPlugins.nplugins();
					GtkWidget *widget = NULL;
					for (size_t iPlugin = 0; iPlugin < nPlugins; iPlugin++) {
						if (dicttype == gpAppFrame->oStarDictPlugins->SpecialDictPlugins.dict_type(iPlugin)) {
							gpAppFrame->oStarDictPlugins->SpecialDictPlugins.render_widget(iPlugin, true, gpAppFrame->query_dictmask[i].index, orig_word, Word[i], WordData[i], &widget);
							break;
						}
					}
					if (widget) {
						view->AppendHeaderMark();
						view->append_widget(widget);
						view->AppendNewline();
						continue;
					}
				}
				view->AppendHeader(gpAppFrame->oLibs.dict_name(gpAppFrame->query_dictmask[i].index).c_str());
			} else if (gpAppFrame->query_dictmask[i].type == InstantDictType_VIRTUAL) {
				view->AppendHeader(gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(gpAppFrame->query_dictmask[i].index));
			} else if (gpAppFrame->query_dictmask[i].type == InstantDictType_NET) {
				view->AppendHeader(gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(gpAppFrame->query_dictmask[i].index), gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_link(gpAppFrame->query_dictmask[i].index));
			}
			j=0;
			do {
				view->AppendWord(Word[i][j]);
				view->AppendData(WordData[i][j][0], Word[i][j],
						 orig_word);
				view->AppendNewline();
				k=1;
				while (WordData[i][j][k]) {
					view->AppendDataSeparate();
					view->AppendData(WordData[i][j][k],
							 Word[i][j], orig_word);
					view->AppendNewline();
					k++;
				}
				j++;
			} while (Word[i][j]);
		}
	}
	view->end_update();
}

void TextWin::Show(NetDictResponse *resp)
{
	view->begin_update();
	bool do_append;
	if (query_result == TEXT_WIN_FOUND || query_result == TEXT_WIN_SHOW_FIRST || query_result == TEXT_WIN_NET_FOUND || query_result == TEXT_WIN_NET_SHOW_FIRST) {
		do_append = true;
		view->goto_end();
	} else {
		do_append = false;
		view->clear();
		view->goto_begin();
	}
	if (resp->data) {
		query_result = TEXT_WIN_NET_FOUND;
		InstantDictIndex dict_index;
		dict_index.type = InstantDictType_UNKNOWN;
		view->SetDictIndex(dict_index);
		gchar *mark = g_strdup_printf("%d", view->get_bookindex());
		gpAppFrame->oMidWin.oIndexWin.oResultWin.InsertLast(resp->bookname, mark);
		g_free(mark);
		view->AppendHeader(resp->bookname, resp->booklink);
		view->AppendWord(resp->word);
		view->AppendData(resp->data, resp->word, resp->word);
		view->AppendNewline();
	} else {
		if (!do_append) {
			query_result = TEXT_WIN_NET_NOT_FOUND;
			Show(_("<Not Found!>"));
		}
	}
	view->end_update();
}

void TextWin::Show(const struct STARDICT::LookupResponse::DictResponse *dict_response, STARDICT::LookupResponse::ListType list_type)
{
	view->begin_update();
	bool do_append;
	if (query_result == TEXT_WIN_FOUND || query_result == TEXT_WIN_SHOW_FIRST || query_result == TEXT_WIN_NET_FOUND || query_result == TEXT_WIN_NET_SHOW_FIRST) {
		do_append = true;
		view->goto_end();
	} else {
		do_append = false;
		view->clear();
		view->goto_begin();
	}
	if (dict_response->dict_result_list.empty()) {
		if (!do_append) {
			query_result = TEXT_WIN_NET_NOT_FOUND;
			if (list_type == STARDICT::LookupResponse::ListType_Rule_List) {
				Show(_("Found no words matching this pattern!"));
			} else if (list_type == STARDICT::LookupResponse::ListType_Regex_List) {
				Show(_("Found no words matching this regular expression!"));
			} else if (list_type == STARDICT::LookupResponse::ListType_Fuzzy_List) {
				Show(_("There are too many spelling errors :-("));
			} else if (list_type == STARDICT::LookupResponse::ListType_Tree) {
				Show(_("There are no dictionary articles containing this word. :-("));
			} else {
				Show(_("<Not Found!>"));
			}
		}
	} else {
		if (dict_response->oword == queryWord) {
			query_result = TEXT_WIN_NET_FOUND;
		} else {
			if (query_result == TEXT_WIN_FOUND)
				query_result = TEXT_WIN_NET_FOUND;
			else if (query_result == TEXT_WIN_SHOW_FIRST)
				query_result = TEXT_WIN_NET_SHOW_FIRST;
		}
		InstantDictIndex dict_index;
		dict_index.type = InstantDictType_UNKNOWN;
		view->SetDictIndex(dict_index);
		for (std::list<struct STARDICT::LookupResponse::DictResponse::DictResult *>::const_iterator i = dict_response->dict_result_list.begin(); i != dict_response->dict_result_list.end(); ++i) {
			gchar *mark = g_strdup_printf("%d", view->get_bookindex());
			gpAppFrame->oMidWin.oIndexWin.oResultWin.InsertLast((*i)->bookname, mark);
			g_free(mark);
			view->AppendHeader((*i)->bookname);
			for (std::list<struct STARDICT::LookupResponse::DictResponse::DictResult::WordResult *>::iterator j = (*i)->word_result_list.begin(); j != (*i)->word_result_list.end(); ++j) {
				view->AppendWord((*j)->word);
				std::list<char *>::iterator k = (*j)->datalist.begin();
				view->AppendData(*k, (*j)->word, dict_response->oword);
				view->AppendNewline();
				for (++k; k != (*j)->datalist.end(); ++k) {
					view->AppendDataSeparate();
					view->AppendData(*k, (*j)->word, dict_response->oword);
					view->AppendNewline();
				}
			}
		}
	}
	view->end_update();
}

void TextWin::ShowTreeDictData(gchar *data)
{
	view->begin_update();
	view->clear();
	view->goto_begin();
	if (data) {
		view->AppendData(data, "", NULL);
		view->AppendNewline();
	}
	view->end_update();
}

gboolean TextWin::Find (const gchar *text, gboolean start)
{
  GtkTextBuffer *buffer =
    gtk_text_view_get_buffer(GTK_TEXT_VIEW(view->widget()));

  GtkTextIter iter;
  if (start)
    gtk_text_buffer_get_start_iter(buffer, &iter);
  else {
    GtkTextMark *mark = gtk_text_buffer_get_mark(buffer, "last_search");

    if (mark)
      gtk_text_buffer_get_iter_at_mark(buffer, &iter, mark);
    else
      gtk_text_buffer_get_start_iter(buffer, &iter);
  }

  GtkTextIter match_start, match_end;
  if (gtk_text_iter_forward_search(&iter, text,
				   (GtkTextSearchFlags) (GTK_TEXT_SEARCH_VISIBLE_ONLY | GTK_TEXT_SEARCH_TEXT_ONLY),
				   &match_start, &match_end,
				   NULL)) {
    gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view->widget()), &match_start,
				 0.0, TRUE, 0.5, 0.5);
    gtk_text_buffer_place_cursor(buffer, &match_end);
    gtk_text_buffer_move_mark(buffer,
			      gtk_text_buffer_get_mark(buffer, "selection_bound"),
			      &match_start);
    gtk_text_buffer_create_mark(buffer, "last_search", &match_end, FALSE);

    return TRUE;
  } else {
    GtkWidget *message_dlg =
      gtk_message_dialog_new(
			      GTK_WINDOW(gpAppFrame->window),
			      (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			      GTK_MESSAGE_INFO,
			      GTK_BUTTONS_OK,
			      _("The text \"%s\" was not found."), text);

    gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

    gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

    if (gtk_dialog_run(GTK_DIALOG(message_dlg))!=GTK_RESPONSE_NONE)
      gtk_widget_destroy(message_dlg);
    return FALSE;
  }
}

gboolean TextWin::on_button_press(GtkWidget * widget, GdkEventButton * event, TextWin *oTextWin)
{
	if (event->button == 2) {
#ifdef _WIN32
		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin->view->widget()));
		if (gtk_text_buffer_get_selection_bounds(buffer, NULL, NULL)) {
			gtk_selection_convert(widget, GDK_SELECTION_PRIMARY,
					      gpAppFrame->oSelection.UTF8_STRING_Atom,
					      GDK_CURRENT_TIME);
		} else {
			gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD),
				gpAppFrame->oTopWin.ClipboardReceivedCallback, &(gpAppFrame->oTopWin));
		}
#else
		gtk_selection_convert(widget, GDK_SELECTION_PRIMARY,
				      gpAppFrame->oSelection.UTF8_STRING_Atom,
				      GDK_CURRENT_TIME);
#endif
		return TRUE;
	}

	return FALSE;
}

void TextWin::on_pronounce_menu_item_activate(GtkMenuItem *menuitem, TextWin *oTextWin)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin->view->widget()));
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		gpAppFrame->oReadWord.read(str, oTextWin->selection_readwordtype);
		g_free(str);
	}
}

void TextWin::on_query_menu_item_activate(GtkMenuItem *menuitem, TextWin *oTextWin)
{
	GtkTextIter start, end;
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTextWin->view->widget()));
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		std::string estr;
		stardict_input_escape(str, estr);
		g_free(str);
		gpAppFrame->Query(estr.c_str());
	}
}

void TextWin::on_populate_popup(GtkTextView *textview, GtkMenu *menu, TextWin *oTextWin)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	GtkTextIter start, end;
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		GtkWidget *menuitem;
		menuitem = gtk_separator_menu_item_new();
		gtk_widget_show(menuitem);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
		gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		oTextWin->selection_readwordtype = gpAppFrame->oReadWord.canRead(str);
		if (oTextWin->selection_readwordtype != READWORD_CANNOT) {
			menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Pronounce"));
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_pronounce_menu_item_activate), oTextWin);
			GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			gtk_widget_show(menuitem);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
		}
		g_free(str);
		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Query"));
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_query_menu_item_activate), oTextWin);
		GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		gtk_widget_show(menuitem);
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	}
}

void TextWin::SelectionCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, TextWin *oTextWin)
{
	gchar *result;
	result = (gchar *)gtk_selection_data_get_text (selection_data);
	if (!result) {
		/* If we asked for UTF8 and didn't get it, try compound_text;
		* if we asked for compound_text and didn't get it, try string;
		* If we asked for anything else and didn't get it, give up.
		*/
		if (gtk_selection_data_get_target(selection_data) == gpAppFrame->oSelection.UTF8_STRING_Atom) {
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, gpAppFrame->oSelection.COMPOUND_TEXT_Atom, GDK_CURRENT_TIME);
		}
		else if (gtk_selection_data_get_target(selection_data) == gpAppFrame->oSelection.COMPOUND_TEXT_Atom)
		{
			gtk_selection_convert (widget, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING, GDK_CURRENT_TIME);
		}
		return;
	}
	std::string estr;
	stardict_input_escape(result, estr);
	g_free(result);
	gpAppFrame->Query(estr.c_str());
}

/*********************************************/
TransWin::TransWin()
{
}

void TransWin::SetEngine(gint index)
{
	if(index < 0 || index >= TranslateEngine_Size)
		index = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(engine_combobox), index);
}

void TransWin::SetFromLang(bool load, gint index)
{
	if(load) {
		const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(engine_combobox));
		const TransEngine& engine = gpAppFrame->oFullTextTrans.get_engine(engine_index);
		const size_t fromlangcnt = engine.get_source_lang_cnt();
		GtkListStore* list_store = gtk_list_store_new(1, G_TYPE_STRING);
		GtkTreeIter iter;
		for(size_t i = 0; i<fromlangcnt; ++i) {
			gtk_list_store_append(list_store, &iter);
			gtk_list_store_set(list_store, &iter,
				0, engine.get_source_lang(i).c_str(),
			-1);
		}
		gtk_combo_box_set_model(GTK_COMBO_BOX(fromlang_combobox), GTK_TREE_MODEL(list_store));
		g_object_unref (G_OBJECT(list_store));
	}
	if(index < 0)
		index = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(fromlang_combobox), index);
}

void TransWin::SetToLang(bool load, gint index)
{
	if(load) {
		const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(engine_combobox));
		const TransEngine& engine = gpAppFrame->oFullTextTrans.get_engine(engine_index);
		const gint fromlang_index = gtk_combo_box_get_active(GTK_COMBO_BOX(fromlang_combobox));
		const size_t tolangcnt = engine.get_target_lang_cnt(fromlang_index);
		GtkListStore* list_store = gtk_list_store_new(1, G_TYPE_STRING);
		GtkTreeIter iter;
		for(size_t i = 0; i<tolangcnt; ++i) {
			gtk_list_store_append(list_store, &iter);
			gtk_list_store_set(list_store, &iter,
				0, engine.get_target_lang(fromlang_index, i).c_str(),
				-1
			);
		}
		gtk_combo_box_set_model(GTK_COMBO_BOX(tolang_combobox), GTK_TREE_MODEL(list_store));
		g_object_unref (G_OBJECT(list_store));
	}
	if(index < 0)
		index = 0;
	gtk_combo_box_set_active(GTK_COMBO_BOX(tolang_combobox), index);
}

void TransWin::on_pronounce_menu_item_activate(GtkMenuItem *menuitem, TransWin *oTransWin)
{
	gpAppFrame->oReadWord.read(oTransWin->pronounceWord.c_str(), oTransWin->selection_readwordtype);
}

void TransWin::on_populate_popup(GtkTextView *textview, GtkMenu *menu, TransWin *oTransWin)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(textview);
	GtkTextIter start, end;
	if (gtk_text_buffer_get_selection_bounds(buffer, &start, &end)) {
		gchar *str = gtk_text_buffer_get_text(buffer, &start, &end, false);
		oTransWin->selection_readwordtype = gpAppFrame->oReadWord.canRead(str);
		if (oTransWin->selection_readwordtype != READWORD_CANNOT) {
			oTransWin->pronounceWord = str;
			GtkWidget *menuitem;
			menuitem = gtk_separator_menu_item_new();
			gtk_widget_show(menuitem);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
			menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Pronounce"));
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_pronounce_menu_item_activate), oTransWin);
			GtkWidget *image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
			gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
			gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
			gtk_widget_show(menuitem);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
		}
		g_free(str);
	}
}

void TransWin::Create(GtkWidget *notebook)
{
	GtkWidget *frame;
	frame = gtk_frame_new(NULL);

	GtkWidget *vbox;
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
#else
	vbox = gtk_vbox_new(false, 3);
#endif
	gtk_container_set_border_width(GTK_CONTAINER(vbox),8);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	GtkWidget *label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Full-Text Translation</b>"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);

	GtkWidget *hbox;
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);

	GtkListStore* list_store = gtk_list_store_new(1, G_TYPE_STRING);
	GtkTreeIter iter;
	for (size_t i = 0; i<TranslateEngine_Size; i++) {
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
			0, gpAppFrame->oFullTextTrans.get_engine(i).get_name().c_str(),
			-1
		);
	}
	engine_combobox = gtk_combo_box_new_with_model(GTK_TREE_MODEL(list_store));
	g_object_unref (G_OBJECT(list_store));
	GtkCellRenderer *renderer;
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (engine_combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (engine_combobox), renderer, "text", 0, NULL);
	gtk_widget_set_focus_on_click(GTK_WIDGET(engine_combobox), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), engine_combobox, false, false, 0);
	label = gtk_label_new(":");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	fromlang_combobox = gtk_combo_box_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (fromlang_combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (fromlang_combobox), renderer, "text", 0, NULL);
	gtk_widget_set_focus_on_click(GTK_WIDGET(fromlang_combobox), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), fromlang_combobox, false, false, 0);
	label = gtk_label_new(_("To"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	tolang_combobox = gtk_combo_box_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (tolang_combobox), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (tolang_combobox), renderer, "text", 0, NULL);
	gtk_widget_set_focus_on_click(GTK_WIDGET(tolang_combobox), FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), tolang_combobox, false, false, 0);
	SetEngine(conf->get_int_at("translate/engine"));
	SetFromLang(true, conf->get_int_at("translate/fromlang"));
	SetToLang(true, conf->get_int_at("translate/tolang"));
	g_signal_connect(G_OBJECT(engine_combobox),"changed", G_CALLBACK(on_engine_combobox_changed), this);
	g_signal_connect(G_OBJECT(fromlang_combobox),"changed", G_CALLBACK(on_fromlang_combobox_changed), this);
	g_signal_connect(G_OBJECT(tolang_combobox),"changed", G_CALLBACK(on_tolang_combobox_changed), this);

	input_textview = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(input_textview), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(input_textview), 5);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(input_textview), 5);
	g_signal_connect(G_OBJECT(input_textview), "populate-popup", G_CALLBACK(on_populate_popup), this);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), input_textview);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
#else
	hbox = gtk_hbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	trans_button = gtk_button_new();
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	GtkWidget *hbox1 = gtk_hbox_new(false, 2);
#endif
	gtk_container_add (GTK_CONTAINER (trans_button), hbox1);
	GtkWidget *image = gtk_image_new_from_pixbuf(get_impl(gpAppFrame->oAppSkin.index_translate));
	gtk_box_pack_start (GTK_BOX (hbox1), image, FALSE, FALSE, 0);
	label = gtk_label_new_with_mnemonic(_("_Translate"));
	gtk_box_pack_start (GTK_BOX (hbox1), label, FALSE, FALSE, 0);
	gtk_label_set_mnemonic_widget(GTK_LABEL(label), trans_button);
	g_signal_connect(G_OBJECT(trans_button),"clicked", G_CALLBACK(on_translate_button_clicked), this);
	gtk_box_pack_start(GTK_BOX(hbox), trans_button, false, false, 0);

	GtkWidget *clear_button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
	g_signal_connect(G_OBJECT(clear_button),"clicked", G_CALLBACK(on_clear_button_clicked), this);
	gtk_box_pack_start(GTK_BOX(hbox), clear_button, false, false, 5);

	result_textview = gtk_text_view_new();
	gtk_text_view_set_editable(GTK_TEXT_VIEW(result_textview), FALSE);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(result_textview), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(result_textview), 3);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(result_textview), 3);
	g_signal_connect(G_OBJECT(result_textview), "populate-popup", G_CALLBACK(on_populate_popup), this);
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), result_textview);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);

#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
#else
	hbox = gtk_hbox_new(false, 8);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), label, true, true, 0);
	label = gtk_label_new(_("Powered by -"));
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	link_label = gtk_label_new(NULL);
	GtkWidget *link_eventbox = gtk_event_box_new();
	g_signal_connect(G_OBJECT(link_eventbox),"button-release-event", G_CALLBACK(on_link_eventbox_clicked), this);
	gtk_container_add(GTK_CONTAINER(link_eventbox), link_label);
	gtk_box_pack_start (GTK_BOX (hbox), link_eventbox, FALSE, FALSE, 0);
	label = gtk_label_new(NULL);
	gtk_box_pack_end(GTK_BOX(hbox), label, true, true, 0);
	const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(engine_combobox));
	SetLink(engine_index);

	g_signal_connect(G_OBJECT(frame), "destroy", G_CALLBACK(on_destroy), this);
	gtk_widget_show_all(frame);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), frame, NULL);
	gtk_widget_realize(link_eventbox);
	GdkCursor* cursor = gdk_cursor_new(GDK_HAND2);
	gdk_window_set_cursor(gtk_widget_get_window(link_eventbox), cursor);
#if GTK_MAJOR_VERSION >= 3
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif

	gpAppFrame->oFullTextTrans.on_error_.connect(sigc::mem_fun(this, &TransWin::on_translate_error));
	gpAppFrame->oFullTextTrans.on_response_.connect(sigc::mem_fun(this, &TransWin::on_translate_response));
}

void TransWin::SetLink(gint engine_index)
{
	SetLink(gpAppFrame->oFullTextTrans.get_engine(engine_index).get_website_url().c_str());
}

void TransWin::SetLink(const char *linkname)
{
	gchar *markup = g_markup_printf_escaped("<span foreground=\"blue\" underline=\"single\">%s</span>", linkname);
	gtk_label_set_markup(GTK_LABEL(link_label), markup);
	g_free(markup);
}


void TransWin::on_link_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, TransWin *oTransWin)
{
	gint index = gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->engine_combobox));
	show_url(gpAppFrame->oFullTextTrans.get_engine(index).get_website_name().c_str());
}

void TransWin::on_engine_combobox_changed(GtkWidget *widget, TransWin *oTransWin)
{
	const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	oTransWin->SetLink(engine_index);
	oTransWin->SetFromLang(true, -1);
	oTransWin->SetToLang(true, -1);
}

void TransWin::on_fromlang_combobox_changed(GtkWidget *widget, TransWin *oTransWin)
{
	const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->engine_combobox));
	if(!gpAppFrame->oFullTextTrans.get_engine(engine_index).independent_target_lang())
		oTransWin->SetToLang(true, -1);
}

void TransWin::on_tolang_combobox_changed(GtkWidget *widget, TransWin *oTransWin)
{
}

void TransWin::on_translate_button_clicked(GtkWidget *widget, TransWin *oTransWin)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTransWin->input_textview));
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, false);
	if (text[0] == '\0') {
		g_free(text);
		GtkWidget *message_dlg = 
			gtk_message_dialog_new(
				GTK_WINDOW(gpAppFrame->window),
				(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
				GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
				_("Please input some words to translate."));
		gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
		gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
		g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
		gtk_widget_show(message_dlg);
		return;
	}
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTransWin->result_textview));
	gtk_text_buffer_set_text(buffer, _("Connecting..."), -1);
	gpAppFrame->oFullTextTrans.Translate(
		gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->engine_combobox)),
		gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->fromlang_combobox)),
		gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->tolang_combobox)),
		text
	);
	g_free(text);
}

void TransWin::on_clear_button_clicked(GtkWidget *widget, TransWin *oTransWin)
{
	GtkTextBuffer* buffer;
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTransWin->input_textview));
	gtk_text_buffer_set_text(buffer, "", -1);
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(oTransWin->result_textview));
	gtk_text_buffer_set_text(buffer, "", -1);
	gtk_widget_grab_focus(oTransWin->input_textview);
}

void TransWin::on_destroy(GtkWidget *object, TransWin* oTransWin)
{
	const gint engine_index = gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->engine_combobox));
	const gint fromlang_index = gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->fromlang_combobox));
	const gint tolang_index = gtk_combo_box_get_active(GTK_COMBO_BOX(oTransWin->tolang_combobox));
	conf->set_int_at("translate/engine", engine_index);
	conf->set_int_at("translate/fromlang", fromlang_index);
	conf->set_int_at("translate/tolang", tolang_index);
}

void TransWin::on_translate_error(const char * error_msg)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(result_textview));
	gtk_text_buffer_set_text(buffer, "", -1);
	GtkWindow *parent = GTK_WINDOW(gpAppFrame->window);
	GtkWidget *message_dlg =
		gtk_message_dialog_new(
			parent,
			(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
			GTK_MESSAGE_ERROR,  GTK_BUTTONS_OK,
			"%s", error_msg);
	gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
	gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
	g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
	gtk_widget_show(message_dlg);
}

void TransWin::on_translate_response(const char * text)
{
	GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(result_textview));
	gtk_text_buffer_set_text(buffer, text, -1);
}

/*********************************************/
void MidWin::Create(GtkWidget *vbox)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 2);
#endif
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 2);

	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook),FALSE);

#if GTK_MAJOR_VERSION >= 3
	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hpaned = gtk_hpaned_new();
#endif
	gtk_widget_show(hpaned);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), hpaned, NULL);

	bool has_treedict = oIndexWin.Create(hpaned);
	oLeftWin.Create(hbox, has_treedict);
	gtk_box_pack_start(GTK_BOX(hbox),notebook, true, true, 0);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	GtkWidget *vbox1 = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox1);
	oToolWin.Create(vbox1);
	oTextWin.Create(vbox1);
	gtk_paned_pack2(GTK_PANED(hpaned), vbox1, TRUE, FALSE);

	oTransWin.Create(notebook);
}

/*********************************************/
BottomWin::BottomWin()
{
	SearchWebsiteMenu = NULL;
	news_timeout_id = 0;
	link_timeout_id = 0;
	need_resume_news = false;
}

void BottomWin::Destroy()
{
	if (news_timeout_id != 0) {
		g_source_remove(news_timeout_id);
		news_timeout_id = 0;
	}
	if (link_timeout_id != 0) {
		g_source_remove(link_timeout_id);
		link_timeout_id = 0;
	}
	if (SearchWebsiteMenu)
		gtk_widget_destroy(SearchWebsiteMenu);
}

void BottomWin::Create(GtkWidget *vbox)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
#endif
	gtk_widget_show(hbox);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
#else
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
#endif

	ScanSelectionCheckButton = gtk_check_button_new_with_mnemonic(_("_Scan"));
	gtk_widget_show(ScanSelectionCheckButton);
	gtk_widget_set_can_focus (ScanSelectionCheckButton, FALSE);
	bool scan=conf->get_bool_at("dictionary/scan_selection");

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ScanSelectionCheckButton), scan);
	g_signal_connect(G_OBJECT(ScanSelectionCheckButton), "toggled",
									 G_CALLBACK(ScanCallback), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),ScanSelectionCheckButton,false,false,0);
	gtk_widget_set_tooltip_text(ScanSelectionCheckButton,_("Scan the selection"));

	GtkWidget *button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(AboutCallback), NULL);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
#ifdef CONFIG_GPE
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,2);
#else
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,8);
#endif
	gtk_widget_set_tooltip_text(button,_("Show info"));

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(QuitCallback), NULL);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Quit"));

	movenews_event_box = gtk_event_box_new();
	g_signal_connect (G_OBJECT (movenews_event_box), "enter_notify_event", G_CALLBACK (vEnterNotifyCallback), this);
	g_signal_connect (G_OBJECT (movenews_event_box), "leave_notify_event", G_CALLBACK (vLeaveNotifyCallback), this);
	gtk_widget_show(movenews_event_box);
	gtk_box_pack_start(GTK_BOX(hbox),movenews_event_box,false,false,0);
	news_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(movenews_event_box), news_label);
	gtk_misc_set_alignment (GTK_MISC (news_label), 0.0, 0.5);
	gtk_widget_show(news_label);

#if GTK_MAJOR_VERSION >= 3
	link_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	link_hbox = gtk_hbox_new(false, 0);
#endif
	gtk_box_pack_start(GTK_BOX(hbox),link_hbox,true,true,0);
	GtkWidget *label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(link_hbox), label, true, true, 0);
	GtkWidget *event_box = gtk_event_box_new();
	gtk_box_pack_start(GTK_BOX(link_hbox),event_box,false,false,0);
	g_signal_connect (G_OBJECT (event_box), "button-release-event", G_CALLBACK(on_link_eventbox_clicked), this);
	gtk_widget_show(event_box);
	gtk_widget_realize(event_box);
	GdkCursor* cursor = gdk_cursor_new(GDK_HAND2);
	gdk_window_set_cursor(gtk_widget_get_window(event_box), cursor);
#if GTK_MAJOR_VERSION >= 3
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
	link_label = gtk_label_new(NULL);
	gtk_container_add(GTK_CONTAINER(event_box), link_label);
	gtk_widget_show(link_label);
	label = gtk_label_new(NULL);
	gtk_widget_show(label);
	gtk_box_pack_end(GTK_BOX(link_hbox), label, true, true, 0);

	// the next buttons will be pack from right to left.
#ifndef CONFIG_GPE
	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PREFERENCES,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(PreferenceCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Preferences"));

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_PROPERTIES,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(DictManageCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Manage dictionaries"));

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_INDEX,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(NewVersionCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Go to the StarDict website"));
#endif

	button=gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_JUMP_TO,GTK_ICON_SIZE_SMALL_TOOLBAR));
	gtk_widget_show_all(button);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_widget_set_can_focus (button, FALSE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(InternetSearchCallback),this);
	g_signal_connect(G_OBJECT(button),"button_press_event", G_CALLBACK(on_internetsearch_button_press),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Search an Internet dictionary - Right button: website list"));
}

void BottomWin::on_link_eventbox_clicked(GtkWidget *widget, GdkEventButton *event, BottomWin *oBottomWin)
{
	gpAppFrame->on_link_click(oBottomWin->linklist[oBottomWin->link_index].second);
}

gboolean BottomWin::vEnterNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, BottomWin *oBottomWin)
{
	if (oBottomWin->news_timeout_id != 0) {
		g_source_remove(oBottomWin->news_timeout_id);
		oBottomWin->news_timeout_id = 0;
		oBottomWin->need_resume_news = true;
	}
	return TRUE;
}

gboolean BottomWin::vLeaveNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, BottomWin *oBottomWin)
{
	if (oBottomWin->need_resume_news) {
		oBottomWin->news_timeout_id = g_timeout_add(300, move_news, oBottomWin);
		oBottomWin->need_resume_news = false;
	}
	return TRUE;
}

gboolean BottomWin::change_link(gpointer data)
{
	BottomWin *oBottomWin = static_cast<BottomWin *>(data);
	oBottomWin->link_index++;
	if (oBottomWin->link_index == oBottomWin->linklist.size()) {
		oBottomWin->news_timeout_id = g_timeout_add(300, move_news, data);
		gtk_widget_hide(oBottomWin->link_hbox);
		gtk_widget_show(oBottomWin->movenews_event_box);
		oBottomWin->link_timeout_id = 0;
		return FALSE;
	} else {
		char *markup = g_markup_printf_escaped ("<span foreground=\"blue\" underline=\"single\">%s</span>", oBottomWin->linklist[oBottomWin->link_index].first.c_str());
		gtk_label_set_markup(GTK_LABEL(oBottomWin->link_label), markup);
		g_free(markup);
		return TRUE;
	}
}

gboolean BottomWin::move_news(gpointer data)
{
	BottomWin *oBottomWin = static_cast<BottomWin *>(data);
	const char *p = oBottomWin->news_text.c_str();
	size_t i;
	for (i = 0; i < oBottomWin->news_move_index; i++) {
		if (*p) {
			p = g_utf8_next_char(p);
		} else {
			break;
		}
	}
	if (*p) {
		oBottomWin->news_move_index++;
	} else {
		oBottomWin->news_move_index = 0;
		if (oBottomWin->linklist.empty()) {
			p = oBottomWin->news_text.c_str();
		} else {
			oBottomWin->link_index = 0;
			gtk_label_set_text(GTK_LABEL(oBottomWin->news_label), "");
			gtk_widget_hide(oBottomWin->movenews_event_box);
			char *markup = g_markup_printf_escaped ("<span foreground=\"blue\" underline=\"single\">%s</span>", oBottomWin->linklist[0].first.c_str());
			gtk_label_set_markup(GTK_LABEL(oBottomWin->link_label), markup);
			g_free(markup);
			gtk_widget_show(oBottomWin->link_hbox);
			oBottomWin->link_timeout_id = g_timeout_add(4000, change_link, data);
			oBottomWin->news_timeout_id = 0;
			return FALSE;
		}
	}
	const char *p1 = p;
	for (i=0; i < oBottomWin->news_move_len; i++) {
		if (*p1) {
			p1 = g_utf8_next_char(p1);
		} else {
			break;
		}
	}
	std::string text;
	text = "\t";
	text.append(p, p1-p);
	gtk_label_set_text(GTK_LABEL(oBottomWin->news_label), text.c_str());
	return TRUE;
}

void BottomWin::set_news(const char *news, const char *links)
{
	if (news) {
		news_text = news;
		if (IsASCII(news)) {
			news_move_len = 30;
		} else {
			news_move_len = 20;
		}
		news_move_index = 0;
		if (news_timeout_id == 0) {
			news_timeout_id = g_timeout_add(300, move_news, this);
		}
	} else {
		news_text.clear();
		news_move_index = 0;
		if (news_timeout_id != 0) {
			g_source_remove(news_timeout_id);
			news_timeout_id = 0;
		}
		gtk_label_set_text(GTK_LABEL(news_label), "");
	}
	if (links) {
		linklist.clear();
		std::list<std::string> lines;
		const char *p, *p1;
		p = links;
		do {
			p1 = strchr(p, '\n');
			if (p1) {
				lines.push_back(std::string(p, p1-p));
				p = p1 + 1;
			}
		} while (p1);
		lines.push_back(p);
		for (std::list<std::string>::iterator i = lines.begin(); i != lines.end(); ++i) {
			p = i->c_str();
			p1 = strchr(p, '\t');
			if (p1) {
				linklist.push_back(std::pair<std::string, std::string>(std::string(p, p1-p), std::string(p1+1)));
			}
		}
	} else {
		linklist.clear();
	}
}

void BottomWin::ScanCallback(GtkToggleButton *button, gpointer data)
{
	bool scan_selection=gtk_toggle_button_get_active(button);
	conf->set_bool_at("dictionary/scan_selection", scan_selection);
}

void BottomWin::AboutCallback(GtkButton *button, gpointer data)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->oMidWin.oTextWin.ShowInfo();
}

void BottomWin::QuitCallback(GtkButton *button, gpointer data)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->Quit();
}

gboolean BottomWin::on_internetsearch_button_press(GtkWidget * widget, GdkEventButton * event , BottomWin *oBottomWin)
{
	if (event->button == 3) {
		const std::list<std::string> &list=
			conf->get_strlist_at("main_window/search_website_list");
		if (list.empty())
			return true;

		if (oBottomWin->SearchWebsiteMenu)
			gtk_widget_destroy(oBottomWin->SearchWebsiteMenu);

		oBottomWin->SearchWebsiteMenu = gtk_menu_new();

		GtkWidget *menuitem;

		for (std::list<std::string>::const_iterator ci=list.begin(); ci!=list.end(); ++ci) {
			std::vector<std::string> web_list = split(*ci, '\t');
			if (web_list.size()==3 && web_list[2].find("%s")!=std::string::npos) {
				menuitem = gtk_image_menu_item_new_with_label(web_list[0].c_str());
				g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_internetsearch_menu_item_activate), const_cast<char *>(ci->c_str()));
				gtk_menu_shell_append(GTK_MENU_SHELL(oBottomWin->SearchWebsiteMenu), menuitem);
			}
		}

		gtk_widget_show_all(oBottomWin->SearchWebsiteMenu);
		gtk_menu_popup(GTK_MENU(oBottomWin->SearchWebsiteMenu), NULL, NULL, NULL, NULL, event->button, event->time);
		return true;
	}
	return false;
}

void BottomWin::on_internetsearch_menu_item_activate(GtkMenuItem *menuitem, const gchar *website)
{
	InternetSearch(website);
}

void BottomWin::InternetSearchCallback(GtkButton *button, BottomWin *oBottomWin)
{
	const std::list<std::string> &search_website_list=
		conf->get_strlist_at("main_window/search_website_list");
	if (search_website_list.empty())
		return;

	InternetSearch(search_website_list.front());
}

#ifndef CONFIG_GPE
void BottomWin::NewVersionCallback(GtkButton *button, BottomWin *oBottomWin)
{
  show_url("http://stardict-4.sourceforge.net");
}

void BottomWin::DictManageCallback(GtkButton *button, BottomWin *oBottomWin)
{
	gpAppFrame->PopupDictManageDlg();
}

void BottomWin::PreferenceCallback(GtkButton *button, BottomWin *oBottomWin)
{
	gpAppFrame->PopupPrefsDlg();
}
#endif

void BottomWin::InternetSearch(const std::string& website)
{
	std::vector<std::string> weblist = split(website.c_str(), '\t');

	if (weblist.size()!=3)
		return;

	const gchar *text = gpAppFrame->oTopWin.get_text();
	if (text[0]) {
		const gchar *str, *p;
		str = weblist[2].c_str();
		p = strstr(str, "%s");
		if (p) {
			const gchar *p1;
			p1 = p +2;
			p = strchr(p1, '%');
			if (p) {
				g_print(_("Wrong InternetSearch URL! More than 1 \'%%\' in the string!\n"));
				return;
			}
		} else {
			g_print(_("Wrong InternetSearch URL! No \'%%s\' in the string!\n"));
			return;
		}

		glib::CharStr esc_text(g_uri_escape_string(text, NULL, FALSE));
		glib::CharStr url(g_strdup_printf(str, get_impl(esc_text)));
		show_url(get_impl(url));
	} else {
		show_url(weblist[1].c_str());
	}
}
