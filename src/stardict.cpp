/* StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2003-2006 HuZheng <huzheng_001@163.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the StarDict Team and others 2003-2006.  See the AUTHORS
 * file for a list of people on the StarDict Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * StarDict at http://stardict.sourceforge.net.
 */


#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#  include <libgnomeui/libgnomeui.h>
#endif

#ifdef CONFIG_GNOME
#  include "stardict-application-server.h"
#  include "GNOME_Stardict.h"
#endif

#ifdef CONFIG_GPE
#  include <gpe/init.h>
#endif

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#  include <windows.h>
#  include "win32/intl.h"

HINSTANCE stardictexe_hInstance;
#endif

#include "desktop.hpp"
#include "splash.h"
#include "conf.h"
#include "utils.h"
#include "prefsdlg.h"
#include "iskeyspressed.hpp"
#include "class_factory.hpp"
#include "progresswin.hpp"

#include "stardict.h"

AppCore *gpAppFrame;

static gboolean hide_option = FALSE;

#ifdef CONFIG_GNOME
static gint debug = 0;
static gboolean quit_option = FALSE;

static const struct poptOption options [] =
{
	{ "debug", 'g', POPT_ARG_NONE, &debug, 0,
	  N_("Turn on all debugging messages"), NULL },

	{ "hide", 'h', POPT_ARG_NONE, &hide_option, 1,
	  N_("Hide the main window"), NULL },

	{ "quit", 'q', POPT_ARG_NONE, &quit_option, 1,
	  N_("Quit an existing instance of stardict"), NULL },

	{NULL, '\0', 0, NULL, 0}
};
#endif


/********************************************************************/
class change_cursor {
public:
	change_cursor(GdkWindow *win_,GdkCursor *busy, GdkCursor *norm_):
		win(win_), norm(norm_) { gdk_window_set_cursor(win, busy); }
	~change_cursor() { gdk_window_set_cursor(win, norm); }
private:
	GdkWindow *win;
	GdkCursor *norm;
};
/********************************************************************/
class gtk_show_progress_t : public show_progress_t {
public:
	void notify_about_work() { ProcessGtkEvent(); }
} gtk_show_progress;
/********************************************************************/
AppCore::AppCore() :
	oLibs(&gtk_show_progress,
	      conf->get_bool_at("dictionary/create_cache_file"),
	      conf->get_bool_at("dictionary/enable_collation"),
	      conf->get_int_at("dictionary/collate_function"))
{
	word_change_timeout_ = 0;
	window = NULL; //need by save_yourself_cb().
	dict_manage_dlg = NULL;
	prefs_dlg = NULL;
#ifdef CONFIG_GNOME
    gnome_sound_init(NULL);
#endif
}

AppCore::~AppCore()
{
#ifdef CONFIG_GNOME
	gnome_sound_shutdown();
#endif
	delete dict_manage_dlg;
	delete prefs_dlg;
	g_free(iCurrentIndex);
}

class load_show_progress_t : public show_progress_t {
public:
	void notify_about_start(const std::string& title) {
		stardict_splash.display_action(title);
	}
} load_show_progress;

void AppCore::on_change_scan(bool val)
{
	conf->set_bool_at("dictionary/scan_selection", val);
}

void AppCore::on_maximize()
{       
	if (oTopWin.get_text()[0]) {
//so user can input word directly.
		gtk_widget_grab_focus(oMidWin.oTextWin.view->widget()); 
	} else {
		//this won't change selection text.
		oTopWin.grab_focus();
	}
}

void AppCore::on_middle_button_click()
{
	if (conf->get_bool_at("notification_area_icon/query_in_floatwin")) {
		oSelection.LastClipWord.clear();
		gtk_selection_convert(oSelection.selection_widget,
				      GDK_SELECTION_PRIMARY,
				      oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	} else {
		oDockLet->maximize_from_tray();
		gtk_selection_convert(oMidWin.oTextWin.view->widget(),
				      GDK_SELECTION_PRIMARY,
				      oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	}
}

void AppCore::on_link_click(const char *link)
{
	if (link && *link) {
		oTopWin.InsertHisList(oTopWin.get_text());
		oTopWin.InsertBackList();
		oTopWin.SetText(link);
	}
}

void AppCore::Create(gchar *queryword)
{
	oLibs.set_show_progress(&load_show_progress);
	oLibs.load(conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list"),
		   conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list"),
		   conf->get_strlist("/apps/stardict/manage_dictionaries/dict_disable_list")
		);
    oLibs.SetDictMask(dictmask, NULL, -1, -1);
	oLibs.set_show_progress(&gtk_show_progress);
    
    oStarDictClient.set_server(conf->get_string_at("network/server").c_str(), conf->get_int_at("network/port"));
    const std::string &user = conf->get_string_at("network/user");
    const std::string &md5passwd = conf->get_string_at("network/md5passwd");
    if (!user.empty() && !md5passwd.empty()) {
        oStarDictClient.set_auth(user.c_str(), md5passwd.c_str());
    }
    oStarDictClient.on_error_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_error));
    oStarDictClient.on_lookup_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_lookup_end));
    oStarDictClient.on_define_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_define_end));
    oStarDictClient.on_register_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_register_end));
    oStarDictClient.on_getdictmask_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_getdictmask_end));
    oStarDictClient.on_getdirinfo_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_getdirinfo_end));

	iCurrentIndex=(CurrentIndex *)g_malloc0(oLibs.ndicts()*sizeof(CurrentIndex));

	if (conf->get_bool_at("dictionary/use_custom_font")) {
		const std::string &custom_font(conf->get_string_at("dictionary/custom_font"));

		if (!custom_font.empty())	{
			gchar *aa =
				g_strdup_printf("style \"custom-font\" { font_name= \"%s\" }\n"
						"class \"GtkWidget\" style \"custom-font\"\n",
						custom_font.c_str());
			gtk_rc_parse_string(aa);
			g_free(aa);
		}
	}

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window),2);
	bool maximized=conf->get_bool_at("main_window/maximized");

	int width=conf->get_int_at("main_window/window_width");
	int height=conf->get_int_at("main_window/window_height");

	if (width < MIN_WINDOW_WIDTH)
		width = MIN_WINDOW_WIDTH;
	if (height < MIN_WINDOW_HEIGHT)
		height = MIN_WINDOW_HEIGHT;
	gtk_window_set_default_size (GTK_WINDOW(window), width, height);
	if (maximized)
		gtk_window_maximize(GTK_WINDOW(window));
	gtk_window_set_title (GTK_WINDOW (window), _("StarDict"));
	gtk_window_set_icon(GTK_WINDOW(window),
			    get_impl(oAppSkin.icon));
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (on_delete_event), this);
	g_signal_connect (G_OBJECT (window), "window_state_event", G_CALLBACK (on_window_state_event), this);
	g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (vKeyPressReleaseCallback), this);
	g_signal_connect (G_OBJECT (window), "key_release_event", G_CALLBACK (vKeyPressReleaseCallback), this);

	tooltips = gtk_tooltips_new ();

	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window),vbox);
	oTopWin.Create(vbox);
	oMidWin.Create(vbox);
	oBottomWin.Create(vbox);
	unlock_keys.reset(static_cast<hotkeys *>(PlatformFactory::create_class_by_name("hotkeys",
										       GTK_WINDOW(window))));
	unlock_keys->set_comb(combnum2str(conf->get_int_at("dictionary/scan_modifier_key")));
	oFloatWin.Create();
	bool scan=conf->get_bool_at("dictionary/scan_selection");
	oDockLet.reset(PlatformFactory::create_tray_icon(window, scan, tooltips,
							 oAppSkin));
	oDockLet->on_quit_.connect(sigc::mem_fun(this, &AppCore::Quit));
	oDockLet->on_change_scan_.connect(
		sigc::mem_fun(this, &AppCore::on_change_scan));
	oDockLet->on_maximize_.connect(
		sigc::mem_fun(this, &AppCore::on_maximize));
	oDockLet->on_middle_btn_click_.connect(
		sigc::mem_fun(this, &AppCore::on_middle_button_click));
	oSelection.Init();
#ifdef _WIN32
	oClipboard.Init();
	oMouseover.Init();
	oHotkey.Init();
#endif

	if (scan) {
		oSelection.start();
#ifdef _WIN32
		if (conf->get_bool_at("dictionary/scan_clipboard")) {
			oClipboard.start();
		}
		oMouseover.start();
#endif
	}
#ifdef _WIN32
	if (conf->get_bool_at("dictionary/use_scan_hotkey"))
		oHotkey.start_scan();
	if (conf->get_bool_at("dictionary/use_mainwindow_hotkey"))
		oHotkey.start_mainwindow();
#endif

	bool hide=conf->get_bool_at("main_window/hide_on_startup");

	//NOTICE: when docklet embedded failed,it should always show the window,but,how to detect the failure?
	// As stardict is FOR GNOME,so i don't want to consider the case that haven't the Notification area applet.
	if (!hide_option && (queryword || !hide)) {
		oDockLet->hide_state();
		gtk_widget_show(window);
	} else {
// This may be needed, so gtk_window_get_screen() in gtk_iskeyspressed.cpp can always work.
		gtk_widget_realize(window);
		gdk_notify_startup_complete();
	}

	if (oLibs.ndicts()) {
		if (queryword) {
			Query(queryword);
			g_free(queryword);
			//don't set queryword to NULL here,need by DockLet::EmbeddedCallback().
		}	else
			oMidWin.oTextWin.ShowTips();
	} else
		oMidWin.oTextWin.ShowInitFailed();
}

gboolean AppCore::on_delete_event(GtkWidget * window, GdkEvent *event , AppCore *app)
{
	app->oDockLet->minimize_to_tray();
	return TRUE;
}

gboolean AppCore::on_window_state_event(GtkWidget *window,
					GdkEventWindowState *event, AppCore *app)
{
	switch (event->changed_mask) {
	case GDK_WINDOW_STATE_WITHDRAWN:
		if (conf->get_bool_at("dictionary/scan_selection"))
			app->oDockLet->set_scan_mode(true);
		else
			app->oDockLet->set_scan_mode(false);
		if (!(event->new_window_state & GDK_WINDOW_STATE_WITHDRAWN)) {
			app->oDockLet->hide_state();
			if (app->oTopWin.get_text()[0])
				gtk_widget_grab_focus(app->oMidWin.oTextWin.view->widget());
		}
		break;
	case GDK_WINDOW_STATE_ICONIFIED:
		if (!(event->new_window_state & GDK_WINDOW_STATE_ICONIFIED)) {
			if (app->oTopWin.get_text()[0]) {
				//this is better than the next two line because it don't change selection.
				gtk_widget_grab_focus(app->oMidWin.oTextWin.view->widget());
			} else {
				app->oTopWin.grab_focus();
			}
		}
		break;
	case GDK_WINDOW_STATE_MAXIMIZED:
		conf->set_bool_at("main_window/maximized",
				  (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED));
		break;
	default:
		/* nothing */break;
	}

	return FALSE;
}

gboolean AppCore::vKeyPressReleaseCallback(GtkWidget * window, GdkEventKey *event , AppCore *oAppCore)
{
	gboolean return_val=true;  //if return TRUE,the widget which in the main window will not receive any keyboard event.

	gboolean only_ctrl_pressed = ((event->state & GDK_CONTROL_MASK)&&(!(event->state & GDK_MOD1_MASK))&&(!(event->state & GDK_SHIFT_MASK)));
	gboolean only_mod1_pressed = ((event->state & GDK_MOD1_MASK)&&(!(event->state & GDK_CONTROL_MASK))&&(!(event->state & GDK_SHIFT_MASK)));
	if ((event->keyval==GDK_q || event->keyval==GDK_Q) && only_ctrl_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->Quit();
	}
	else if ((event->keyval==GDK_x || event->keyval==GDK_X) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			oAppCore->oDockLet->minimize_to_tray();
		}
	}
	else if ((event->keyval==GDK_z || event->keyval==GDK_Z) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			gtk_window_iconify(GTK_WINDOW(window));
		}
	}
	else if ((event->keyval==GDK_e || event->keyval==GDK_E) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			oAppCore->oMidWin.oToolWin.do_save();
		}
	}
	else if (event->keyval==GDK_F1 && (!(event->state & GDK_CONTROL_MASK)) && (!(event->state & GDK_MOD1_MASK)) && (!(event->state & GDK_SHIFT_MASK))) {
		if (event->type==GDK_KEY_PRESS)
		  show_help(NULL);
	}
	else if ((event->keyval==GDK_f || event->keyval==GDK_F) && only_ctrl_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oMidWin.oToolWin.do_search();
	}
	else if ((event->keyval==GDK_Left) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_back();
	}
	else if ((event->keyval==GDK_Up) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_prev();
	}
	else if ((event->keyval==GDK_Down) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_next();
	}
	else if ((event->keyval==GDK_m || event->keyval==GDK_M) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_menu();
	}
	else if ((event->keyval==GDK_v || event->keyval==GDK_V) && only_ctrl_pressed &&
			!oAppCore->oTopWin.has_focus() &&
			!oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		if (event->type==GDK_KEY_PRESS) {
			gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), oAppCore->oTopWin.ClipboardReceivedCallback, &(oAppCore->oTopWin));
		}
	}
	else if (event->type==GDK_KEY_PRESS &&
						 event->keyval >= 0x21 && event->keyval <= 0x7E &&
						 !(event->state & GDK_CONTROL_MASK) &&
						 !(event->state & GDK_MOD1_MASK) &&
						 !oAppCore->oTopWin.has_focus() &&
						 !oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
		oAppCore->oTopWin.InsertBackList();
		gchar str[2] = { event->keyval, '\0' };
		oAppCore->oTopWin.grab_focus();
		oAppCore->oTopWin.SetText(str, conf->get_bool_at("main_window/search_while_typing"));
		oAppCore->oTopWin.set_position_in_text(1);
	} else if (event->type==GDK_KEY_PRESS && event->keyval == GDK_BackSpace &&
			!oAppCore->oTopWin.has_focus() &&
			!oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		if (oAppCore->oTopWin.get_text()[0]) {
			oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
                        oAppCore->oTopWin.InsertBackList();
                        oAppCore->oTopWin.SetText("");
		}
		oAppCore->oTopWin.grab_focus();
	} else if (event->type == GDK_KEY_PRESS &&
		   event->keyval == GDK_Return &&
		   !oAppCore->oTopWin.has_focus() &&
		   !oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		if (oAppCore->oMidWin.oIndexWin.oListWin.treeview_has_focus()) {
			GtkTreeModel *model;
			GtkTreeIter iter;

			GtkTreeSelection *selection = 
				gtk_tree_view_get_selection(oAppCore->oMidWin.oIndexWin.oListWin.treeview_);
			if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
				if (gtk_tree_model_iter_has_child(model, &iter)) {
					GtkTreePath *path = gtk_tree_model_get_path(model, &iter);
					if (gtk_tree_view_row_expanded(
						    oAppCore->oMidWin.oIndexWin.oListWin.treeview_,
						    path))
						gtk_tree_view_collapse_row(
							oAppCore->oMidWin.oIndexWin.oListWin.treeview_, path);
					else
						gtk_tree_view_expand_row(
							oAppCore->oMidWin.oIndexWin.oListWin.treeview_,
							path, FALSE);
					gtk_tree_path_free(path);
				} else {
					gchar *word;
					gtk_tree_model_get(model, &iter, 0, &word, -1);
					oAppCore->ListClick(word);
					g_free(word);
				}
			}
		}
		else {
			oAppCore->oTopWin.grab_focus();
			oAppCore->TopWinEnterWord(oAppCore->oTopWin.get_text());
		}
	}	else if (event->type==GDK_KEY_PRESS &&
						 event->keyval == 0x20 &&
						 !oAppCore->oTopWin.has_focus() &&
						 !oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
		oAppCore->oTopWin.InsertBackList();
		oAppCore->oTopWin.grab_focus();
	} else {
		switch (event->keyval) {
		case GDK_Escape:
			if (event->type==GDK_KEY_PRESS) {
				if (oAppCore->oTopWin.get_text()[0]) {
					oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
					oAppCore->oTopWin.InsertBackList();
					oAppCore->oTopWin.SetText("");
					oAppCore->oTopWin.grab_focus();
				} else {
					if (!oAppCore->oTopWin.has_focus())
						oAppCore->oTopWin.grab_focus();
				}
			}
			break;
		default:
			return_val=false;
			break;
		}
	}
	return return_val;
}

bool AppCore::SimpleLookupToFloat(const char* sWord, bool bShowIfNotFound)
{
	if (sWord==NULL || sWord[0]=='\0')
		return true;
	char *SearchWord = (char *)g_malloc(strlen(sWord)+1);
	const char *P1;
	char *P2, *EndPointer;
	P1=sWord;
	P2=SearchWord;
	// delete chinese space at the begining
	while (*P1 && g_unichar_isspace(g_utf8_get_char(P1)))
		P1 = g_utf8_next_char(P1);
	//format word, delete any spilth blanks.
	while(*P1) {
		if (g_unichar_isspace(g_utf8_get_char(P1))) {
			*P2++=' ';
			P1 = g_utf8_next_char(P1);
			while(g_unichar_isspace(g_utf8_get_char(P1)))
				P1 = g_utf8_next_char(P1);
		} else {
			g_utf8_strncpy(P2,P1,1);
			P1 = g_utf8_next_char(P1);
			P2 = g_utf8_next_char(P2);
		}
	}
	*P2='\0';
	EndPointer=SearchWord+strlen(SearchWord);

	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * oLibs.ndicts());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * oLibs.ndicts());
	CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * oLibs.ndicts());

	//find the word use most biggest length
	while (EndPointer>SearchWord) {
		// delete end spaces
		while (EndPointer>SearchWord && *EndPointer==' ')
			*EndPointer--='\0';

		bool bFound = false;
		for (size_t iLib=0;iLib<oLibs.ndicts();iLib++)
			BuildResultData(SearchWord, iIndex, false, iLib, pppWord, ppppWordData, bFound, 2);
		if (bFound) {
			oFloatWin.ShowText(pppWord, ppppWordData, SearchWord);
			oTopWin.InsertHisList(SearchWord);
			FreeResultData(pppWord, ppppWordData);
			g_free(iIndex);
			g_free(SearchWord);
			return true;
		}
		// delete last word
		if (bIsPureEnglish(SearchWord)) {
			while (EndPointer>=SearchWord && *EndPointer!=' ')
				EndPointer--;
			if (EndPointer>=SearchWord)
				*EndPointer='\0';
		}	else {// delete one character per time
			EndPointer = g_utf8_find_prev_char(SearchWord,EndPointer);
			if (EndPointer)
				*EndPointer='\0';
			else
				EndPointer = SearchWord-1; // so < SearchWord
		}
	}
	FreeResultData(pppWord, ppppWordData);
	g_free(iIndex);

	// not found
	if (bShowIfNotFound) {
		ShowNotFoundToFloatWin(sWord,_("<Not Found!>"), false);
		oTopWin.InsertHisList(sWord); //really need?
	}
	g_free(SearchWord);
	return false;
}

#ifdef _WIN32
bool AppCore::SmartLookupToFloat(const gchar* sWord, int BeginPos, bool bShowIfNotFound)
{
	if (sWord==NULL || sWord[0]=='\0')
		return true;
	char *SearchWord = g_strdup(sWord);
	char *P1 = SearchWord + BeginPos;
	P1 = g_utf8_next_char(P1);
	while (*P1 && !g_unichar_isspace(g_utf8_get_char(P1)))
		P1 = g_utf8_next_char(P1);
	*P1='\0';
	P1 = SearchWord + BeginPos;
	if (BeginPos) {
		if (g_unichar_isspace(g_utf8_get_char(P1)))
			P1 = g_utf8_prev_char(P1);
		while (P1>SearchWord && !g_unichar_isspace(g_utf8_get_char(g_utf8_prev_char(P1))))
			P1 = g_utf8_prev_char(P1);
	}

	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * oLibs.ndicts());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * oLibs.ndicts());
	CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * oLibs.ndicts());

	int SearchTimes = 2;
	while (SearchTimes) {
		bool bFound = false;
		for (size_t iLib=0;iLib<oLibs.ndicts();iLib++)
			BuildResultData(P1, iIndex, false, iLib, pppWord, ppppWordData, bFound, 2);
		if (bFound) {
			oFloatWin.ShowText(pppWord, ppppWordData, P1);
			oTopWin.InsertHisList(P1);
			FreeResultData(pppWord, ppppWordData);
			g_free(iIndex);
			g_free(SearchWord);
			return true;
		}
		SearchTimes--;
		if (!SearchTimes)
			break;
		if (bIsPureEnglish(P1)) {
			char *P2 = SearchWord + BeginPos;
			if (g_ascii_isupper(*P2)) {
				char *P3 = SearchWord + BeginPos;
				P2++;
				if (*P2) {
					if (g_ascii_isupper(*P2)) {
						P2++;
						while (*P1 && g_ascii_isupper(*P2))
							P2++;
						while (P3>SearchWord && g_ascii_isupper(*(P3-1)))
							P3--;
					} else if (g_ascii_islower(*P2)){
						P2++;
						while (*P2 && g_ascii_islower(*P2))
							P2++;
					}
					if (*P2) {
						*P2='\0';
					} else {
						if (P3==P1)
							break;
					}
					P1=P3;
				} else {
					while (P3>SearchWord && g_ascii_isupper(*(P3-1)))
						P3--;
					if (P3==P1)
						break;
					P1=P3;
				}
			} else if (g_ascii_islower(*P2)) {
				char *P3 = SearchWord + BeginPos;
				while (P3>SearchWord && g_ascii_islower(*(P3-1)))
					P3--;
				if (P3>SearchWord && g_ascii_isupper(*(P3-1)))
					P3--;
				P2++;
				while (*P2 && g_ascii_islower(*P2))
					P2++;
				if (*P2) {
					*P2='\0';
				} else {
					if (P3==P1)
						break;
				}
				P1=P3;
			} else if (*P2 == '-') {
				*P2=' ';
				SearchTimes = 2;
			} else {
				char *P3 = SearchWord + BeginPos;
				while (P3>SearchWord && g_ascii_isalpha(*(P3-1)))
					P3--;
				if (P3!=P2) {
					*P2='\0';
					P1=P3;
				}
				else
					break;
			}
		} else {
			if (P1==SearchWord + BeginPos) {
				char *EndPointer=P1+strlen(P1);
				EndPointer = g_utf8_prev_char(EndPointer);
				if (EndPointer!=P1) {
					*EndPointer='\0';
					SearchTimes = 2;
				}
				else {
					break;
				}
			} else {
				P1 = SearchWord + BeginPos;
				SearchTimes = 2;
			}
		}
	}
	FreeResultData(pppWord, ppppWordData);
	g_free(iIndex);

	// not found
	if (bShowIfNotFound) {
		ShowNotFoundToFloatWin(P1,_("<Not Found!>"), false);
		oTopWin.InsertHisList(P1); //really need?
	}
	g_free(SearchWord);
	return false;
}
#endif

void AppCore::BuildResultData(const char* sWord, CurrentIndex *iIndex, const gchar *piIndexValidStr, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound, gint Method)
{
    int iRealLib = dictmask[iLib];
	gint i, j;
	gint count=0, syncount;
	bool bLookupWord, bLookupSynonymWord;
	gint nWord;
	glong iWordIdx;
	if (piIndexValidStr) {
		if (iIndex[iLib].idx != INVALID_INDEX) {
            bLookupWord = !strcmp(oLibs.poGetWord(iIndex[iLib].idx, iRealLib, 0), piIndexValidStr);
		} else {
			bLookupWord = false;
		}
		if (iIndex[iLib].synidx != UNSET_INDEX && iIndex[iLib].synidx != INVALID_INDEX) {
            bLookupSynonymWord = !strcmp(oLibs.poGetSynonymWord(iIndex[iLib].synidx, iRealLib, 0), piIndexValidStr);
		} else {
			bLookupSynonymWord = false;
		}
	} else {
		if (Method==0) {
            bLookupWord = oLibs.LookupWord(sWord, iIndex[iLib].idx, iRealLib, 0);
            bLookupSynonymWord = oLibs.LookupSynonymWord(sWord, iIndex[iLib].synidx, iRealLib, 0);
		} else if (Method==1) {
			bLookupWord = oLibs.LookupSimilarWord(sWord, iIndex[iLib].idx, iRealLib, 0);
			bLookupSynonymWord = oLibs.LookupSynonymSimilarWord(sWord, iIndex[iLib].synidx, iRealLib, 0);
		} else {
			bLookupWord = oLibs.SimpleLookupWord(sWord, iIndex[iLib].idx, iRealLib, 0);
			bLookupSynonymWord = oLibs.SimpleLookupSynonymWord(sWord, iIndex[iLib].synidx, iRealLib, 0);
		}
	}
	if (bLookupWord || bLookupSynonymWord) {
		glong orig_idx, orig_synidx;
        orig_idx = oLibs.CltIndexToOrig(iIndex[iLib].idx, iRealLib, 0);
        orig_synidx = oLibs.CltSynIndexToOrig(iIndex[iLib].synidx, iRealLib, 0);
		nWord=0;
		if (bLookupWord)
			nWord++;
		if (bLookupSynonymWord) {
			syncount = oLibs.GetOrigWordCount(orig_synidx, iRealLib, false);
			nWord+=syncount;
		}
		pppWord[iLib] = (gchar **)g_malloc(sizeof(gchar *)*(nWord+1));
		ppppWordData[iLib] = (gchar ***)g_malloc(sizeof(gchar **)*(nWord));
		if (bLookupWord) {
			pppWord[iLib][0] = g_strdup(oLibs.poGetOrigWord(orig_idx, iRealLib));
			count = oLibs.GetOrigWordCount(orig_idx, iRealLib, true);
			ppppWordData[iLib][0] = (gchar **)g_malloc(sizeof(gchar *)*(count+1));
			for (i=0;i<count;i++) {
				ppppWordData[iLib][0][i] = stardict_datadup(oLibs.poGetOrigWordData(orig_idx+i, iRealLib));
			}
			ppppWordData[iLib][0][count] = NULL;
			i=1;
		} else {
			i=0;
		}
		for (j=0;i<nWord;i++,j++) {
			iWordIdx = oLibs.poGetOrigSynonymWordIdx(orig_synidx+j, iRealLib);
			if (bLookupWord) {
				if (iWordIdx>=orig_idx && (iWordIdx<orig_idx+count)) {
					nWord--;
					i--;
					continue;
				}
			}
			pppWord[iLib][i] = g_strdup(oLibs.poGetOrigWord(iWordIdx, iRealLib));
			ppppWordData[iLib][i] = (gchar **)g_malloc(sizeof(gchar *)*2);
			ppppWordData[iLib][i][0] = stardict_datadup(oLibs.poGetOrigWordData(iWordIdx, iRealLib));
			ppppWordData[iLib][i][1] = NULL;
		}
		pppWord[iLib][nWord] = NULL;
		bFound = true;
	} else {
		pppWord[iLib] = NULL;
	}
}

void AppCore::FreeResultData(gchar ***pppWord, gchar ****ppppWordData)
{
	if (!pppWord)
		return;
	int j, k;
	size_t i;
	for (i=0; i<oLibs.ndicts(); i++) {
		if (pppWord[i]) {
			j=0;
			while (pppWord[i][j]) {
				k=0;
				while (ppppWordData[i][j][k]) {
					g_free(ppppWordData[i][j][k]);
					k++;
				}
				g_free(pppWord[i][j]);
				g_free(ppppWordData[i][j]);
				j++;
			}
			g_free(pppWord[i]);
			g_free(ppppWordData[i]);
		}
	}
	g_free(pppWord);
	g_free(ppppWordData);
}

/* the input can be:
 * (sWord,NULL,false) look up the sWord.
 * (sWord,piIndex,false),look up the sWord,and set piIndex
 * to the new indexes that found.
 * (sWord,piIndex,true), show word by piIndex's information.
 * it will always found, so bTryMoreIfNotFound is useless.
 */
bool AppCore::SimpleLookupToTextWin(const char* sWord, CurrentIndex *piIndex, const gchar *piIndexValidStr, bool bTryMoreIfNotFound, bool bShowNotfound, bool isShowFirst)
{
	bool bFound = false;
	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * oLibs.ndicts());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * oLibs.ndicts());
	CurrentIndex *iIndex;
	if (!piIndex)
		iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * oLibs.ndicts());
	else
		iIndex = piIndex;

	for (size_t iLib=0; iLib<oLibs.ndicts(); iLib++)
		BuildResultData(sWord, iIndex, piIndexValidStr, iLib, pppWord, ppppWordData, bFound, 0);
	if (!bFound && !piIndexValidStr) {
		for (size_t iLib=0; iLib<oLibs.ndicts(); iLib++)
			BuildResultData(sWord, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 1);
	}
	if (bFound) {
		ShowDataToTextWin(pppWord, ppppWordData, sWord, isShowFirst);
	} else {
		if (bTryMoreIfNotFound) {
			gchar *word = g_strdup(sWord);
			gchar *hword;
			hword = GetHeadWord(word);
			if (*hword) {
				if (!strcmp(hword,sWord)) {
					if (bShowNotfound)
						ShowNotFoundToTextWin(sWord,_("<Not Found!>"), TEXT_WIN_NOT_FOUND);
				} else {
					for (size_t iLib=0;iLib<oLibs.ndicts();iLib++)
						BuildResultData(hword, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 0);
					if (!bFound) {
						for (size_t iLib=0; iLib<oLibs.ndicts(); iLib++)
							BuildResultData(hword, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 1);
					}
					if (bFound) {
						ShowDataToTextWin(pppWord, ppppWordData, sWord, isShowFirst);
					} else {
						if (bShowNotfound)
							ShowNotFoundToTextWin(sWord,_("<Not Found!>"), TEXT_WIN_NOT_FOUND);
					}
				}
			} else {
				if (bShowNotfound)
					ShowNotFoundToTextWin(sWord,_("<Not Found!>"), TEXT_WIN_NOT_FOUND);
			}
			g_free(word);
		} else {
			if (bShowNotfound)
				ShowNotFoundToTextWin(sWord,_("<Not Found!>"), TEXT_WIN_NOT_FOUND);
		}
	}

	if (!piIndex)
		g_free(iIndex);

	FreeResultData(pppWord, ppppWordData);
	return bFound;
}

struct FullTextSearchDialog {
	GtkWidget *progress_bar;
};

static void updateSearchDialog(gpointer data, gdouble fraction)
{
	FullTextSearchDialog *Dialog = (FullTextSearchDialog *)data;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(Dialog->progress_bar), fraction);
	while (gtk_events_pending())
		gtk_main_iteration();
}

static void on_fulltext_search_cancel_clicked(GtkButton *button, bool *cancel)
{
	*cancel = true;
}

static gboolean on_fulltext_search_window_delete_event(GtkWidget * window, GdkEvent *event , bool *cancel)
{
	*cancel = true;
	return true;
}

void AppCore::LookupDataToMainWin(const gchar *sWord)
{
	if (!sWord || !*sWord)
		return;
	change_cursor busy(window->window,
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(false);

	bool cancel = false;
	FullTextSearchDialog Dialog;
	GtkWidget *search_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(search_window), _("Full-text search..."));
	gtk_window_set_transient_for(GTK_WINDOW(search_window), GTK_WINDOW(window));
	gtk_window_set_position(GTK_WINDOW(search_window), GTK_WIN_POS_CENTER_ON_PARENT);
	GtkWidget *vbox = gtk_vbox_new(false, 6);
	gtk_container_add(GTK_CONTAINER(search_window),vbox);
	Dialog.progress_bar = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(vbox),Dialog.progress_bar,false,false,0);
	GtkWidget *button = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	gtk_box_pack_start(GTK_BOX(vbox),button,false,false,0);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_fulltext_search_cancel_clicked), &cancel);
	g_signal_connect (G_OBJECT (search_window), "delete_event", G_CALLBACK (on_fulltext_search_window_delete_event), &cancel);
	gtk_widget_show_all(search_window);

	//clock_t t=clock();
	std::vector< std::vector<gchar *> > reslist(dictmask.size());
	if (oLibs.LookupData(sWord, &reslist[0], updateSearchDialog, &Dialog, &cancel, dictmask)) {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_DATA_LIST;
		for (size_t i=0; i<oLibs.ndicts(); i++) {
			if (!reslist[i].empty()) {
				SimpleLookupToTextWin(reslist[i][0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.
				break;
			}
		}
		oMidWin.oIndexWin.oListWin.SetTreeModel(&reslist[0]);
		oMidWin.oIndexWin.oListWin.ReScroll();
	} else {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_EMPTY;
		ShowNotFoundToTextWin(sWord, _("There are no dictionary's article with such word :-("), TEXT_WIN_FUZZY_NOT_FOUND);
	}
	//t=clock()-t;
	//g_message("Time: %.3lf sec\n", double(t)/CLOCKS_PER_SEC);
	gtk_widget_destroy(search_window);
}

void AppCore::LookupWithFuzzyToMainWin(const gchar *sWord)
{
	if (sWord[0] == '\0')
		return;
	change_cursor busy(window->window,
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	gchar *fuzzy_reslist[MAX_FUZZY_MATCH_ITEM];
	bool Found=
		oLibs.LookupWithFuzzy(sWord, fuzzy_reslist, MAX_FUZZY_MATCH_ITEM, dictmask);

	// show
	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	if (Found) {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_FUZZY_LIST;

		//SimpleLookupToTextWin(oFuzzystruct[0].pMatchWord,NULL);
		SimpleLookupToTextWin(fuzzy_reslist[0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.

		for (int i=0; i<MAX_FUZZY_MATCH_ITEM && fuzzy_reslist[i]; i++) {
			oMidWin.oIndexWin.oListWin.InsertLast(fuzzy_reslist[i]);
			g_free(fuzzy_reslist[i]);
			//printf("fuzzy %s,%d\n",oFuzzystruct[i].pMatchWord,oFuzzystruct[i].iMatchWordDistance);
		}
		oMidWin.oIndexWin.oListWin.ReScroll();
	} else {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_EMPTY;
		ShowNotFoundToTextWin(sWord,_("There are too many spelling errors :-("), TEXT_WIN_FUZZY_NOT_FOUND);
	}
}

void AppCore::LookupWithFuzzyToFloatWin(const gchar *sWord)
{
	if (sWord[0] == '\0')
		return;
	change_cursor busy(oFloatWin.FloatWindow->window,
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));
	gchar *fuzzy_reslist[MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM];
	bool Found = oLibs.LookupWithFuzzy(sWord, fuzzy_reslist, MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM, dictmask);
	if (Found) {
		int i, count=0;
		for (i=0; i<MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM; i++) {
			if (fuzzy_reslist[i])
				count++;
			else
				break;
		}
		gchar ****ppppWord = (gchar ****)g_malloc(sizeof(gchar ***) * count);
		gchar *****pppppWordData = (gchar *****)g_malloc(sizeof(gchar ****) * count);
		const gchar **ppOriginWord = (const gchar **)g_malloc(sizeof(gchar *) * count);
		CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * oLibs.ndicts());

		gchar ***pppWord;
		gchar ****ppppWordData;
		for (i=0;i<count;i++) {
			bool bFound = false;
			pppWord = (gchar ***)g_malloc(sizeof(gchar **) * oLibs.ndicts());
			ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * oLibs.ndicts());

			ppOriginWord[i] = fuzzy_reslist[i];
			for (size_t iLib=0; iLib<oLibs.ndicts(); iLib++)
				BuildResultData(fuzzy_reslist[i], iIndex, false, iLib, pppWord, ppppWordData, bFound, 2);
			if (bFound) {// it is certainly be true.
				ppppWord[i]=pppWord;
				pppppWordData[i]=ppppWordData;
			} else {
				FreeResultData(pppWord, ppppWordData);
				ppppWord[i]=NULL;
			}
		}
		oFloatWin.ShowText(ppppWord, pppppWordData, ppOriginWord, count, sWord);
		for (i=0; i<count; i++) {
			if (ppppWord[i])
				FreeResultData(ppppWord[i], pppppWordData[i]);
		}
		g_free(ppppWord);
		g_free(pppppWordData);
		g_free(ppOriginWord);
		g_free(iIndex);

		for (i=0;i<count;i++)
			g_free(fuzzy_reslist[i]);
	} else
		ShowNotFoundToFloatWin(sWord,_("Fuzzy query failed, too :-("), true);
}

void AppCore::LookupWithRuleToMainWin(const gchar *word)
{
	change_cursor busy(window->window,
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	gchar **ppMatchWord = (gchar **)g_malloc(sizeof(gchar *) * (MAX_MATCH_ITEM_PER_LIB) * oLibs.ndicts());
	gint iMatchCount=oLibs.LookupWithRule(word, ppMatchWord, dictmask);
	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	if (iMatchCount) {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_PATTERN_LIST;

		for (gint i=0; i<iMatchCount; i++)
			oMidWin.oIndexWin.oListWin.InsertLast(ppMatchWord[i]);
		//memset(iCurrentIndex,'\0',sizeof(iCurrentIndex));    // iCurrentIndex is ineffective now.

		// show the first word.
		//SimpleLookupToTextWin(ppMatchWord[0],NULL);
		SimpleLookupToTextWin(ppMatchWord[0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.
		oMidWin.oIndexWin.oListWin.ReScroll();
		for(gint i=0; i<iMatchCount; i++)
			g_free(ppMatchWord[i]);
	} else {
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_EMPTY;
		ShowNotFoundToTextWin(word,_("Found no words matching this pattern!"), TEXT_WIN_PATTERN_NOT_FOUND);
	}
	g_free(ppMatchWord);
}

void AppCore::ShowDataToTextWin(gchar ***pppWord, gchar ****ppppWordData,
				const gchar *sOriginWord, bool isShowFirst)
{
	oMidWin.oTextWin.Show(sOriginWord, pppWord, ppppWordData);
	if (isShowFirst)
		oMidWin.oTextWin.query_result = TEXT_WIN_SHOW_FIRST;
	else
		oMidWin.oTextWin.query_result = TEXT_WIN_FOUND;
	oMidWin.oTextWin.queryWord = sOriginWord;

	oMidWin.oIndexWin.oResultWin.Clear();
	for (size_t i=0; i < oLibs.ndicts(); i++) {
		if (pppWord[i]) {
			gchar *mark = g_strdup_printf("%d", i);
			oMidWin.oIndexWin.oResultWin.InsertLast(oLibs.dict_name(i).c_str(), mark);
			g_free(mark);
		}
	}

	gboolean canRead = oReadWord.canRead(sOriginWord);
	if (canRead) {
		oMidWin.oTextWin.pronounceWord = sOriginWord;
	}
	else {
		for (size_t i=0;i< oLibs.ndicts(); i++) {
			if (pppWord[i] && strcmp(pppWord[i][0], sOriginWord)) {
				if (oReadWord.canRead(pppWord[i][0])) {
					canRead = true;
					oMidWin.oTextWin.pronounceWord = pppWord[i][0];
				}
				break;
			}
		}
	}
	gtk_widget_set_sensitive(oMidWin.oToolWin.PronounceWordButton, canRead);
}

void AppCore::ShowTreeDictDataToTextWin(guint32 offset, guint32 size, gint iTreeDict)
{
	oMidWin.oTextWin.ShowTreeDictData(oTreeDicts.poGetWordData(offset, size, iTreeDict));
	oMidWin.oTextWin.query_result = TEXT_WIN_TREEDICT;

	oMidWin.oIndexWin.oResultWin.Clear();
}

void AppCore::ShowNotFoundToTextWin(const char* sWord,const char* sReason, TextWinQueryResult query_result)
{
	oMidWin.oTextWin.Show(sReason);
	oMidWin.oTextWin.query_result = query_result;
	oMidWin.oTextWin.queryWord = sWord;

	oMidWin.oIndexWin.oResultWin.Clear();

	gboolean canRead = oReadWord.canRead(sWord);
	if (canRead)
		oMidWin.oTextWin.pronounceWord = sWord;
	gtk_widget_set_sensitive(oMidWin.oToolWin.PronounceWordButton, canRead);
}

void AppCore::ShowNotFoundToFloatWin(const char* sWord,const char* sReason, gboolean fuzzy)
{
	oFloatWin.ShowNotFound(sWord, sReason, fuzzy);
}

void AppCore::TopWinEnterWord(const gchar *text)
{
	if (text[0]=='\0')
		return;
	std::string res;
	switch (analyse_query(text, res)) {
	case qtFUZZY:
		LookupWithFuzzyToMainWin(res.c_str());
		break;
	case qtREGEXP:
		LookupWithRuleToMainWin(res.c_str());
		break;
	case qtDATA:
		LookupDataToMainWin(res.c_str());
		break;
	default:
		if (!conf->get_bool_at("main_window/search_while_typing")) {
			if (oMidWin.oTextWin.queryWord != res) {
				bool showfirst = conf->get_bool_at("main_window/showfirst_when_notfound");
				bool find = SimpleLookupToTextWin(res.c_str(), iCurrentIndex, NULL, true, !showfirst);
				ListWords(res.c_str(), iCurrentIndex, !find && showfirst);
				oTopWin.select_region_in_text(0, -1);
				oTopWin.InsertHisList(text);
				oTopWin.InsertBackList();
				return;
			}
		}
		switch (oMidWin.oTextWin.query_result) {
		case TEXT_WIN_NOT_FOUND:
		case TEXT_WIN_SHOW_FIRST:
			LookupWithFuzzyToMainWin(res.c_str());
			break;
		case TEXT_WIN_INFO:
		case TEXT_WIN_TREEDICT:
		{
			GtkTreeSelection *selection =
				gtk_tree_view_get_selection(oMidWin.oIndexWin.oListWin.treeview_);
			GtkTreeModel *model;
			GtkTreeIter iter;
			gboolean selected = gtk_tree_selection_get_selected(selection,&model,&iter);
			bool not_first_row=false;
			if (selected) {
				gchar *path_str = gtk_tree_model_get_string_from_iter(model,&iter);
				if (!strcmp(path_str,"0"))
					not_first_row = false;
				else
					not_first_row = true;
				g_free(path_str);
			}
			if (!selected || not_first_row) {
				// now select the first row.
				GtkTreePath* path = gtk_tree_path_new_first();
				gtk_tree_model_get_iter(model,&iter,path);
				gtk_tree_selection_select_iter(selection,&iter);
				gtk_tree_view_scroll_to_cell(oMidWin.oIndexWin.oListWin.treeview_,
							     path, NULL, FALSE, 0, 0);
				gtk_tree_path_free(path);
			} else
				SimpleLookupToTextWin(res.c_str(), iCurrentIndex, res.c_str(), false); //text 's index is already cached.
			break;
		}
		case TEXT_WIN_FOUND:
			if (oMidWin.oTextWin.queryWord != res) {
				//user have selected some other word in the list,now select the first word again.
				GtkTreePath* path = gtk_tree_path_new_first();
				GtkTreeModel *model =
					gtk_tree_view_get_model(oMidWin.oIndexWin.oListWin.treeview_);
				GtkTreeIter iter;
				gtk_tree_model_get_iter(model,&iter,path);
				GtkTreeSelection *selection =
					gtk_tree_view_get_selection(oMidWin.oIndexWin.oListWin.treeview_);
				gtk_tree_selection_select_iter(selection,&iter);
				gtk_tree_view_scroll_to_cell(
					oMidWin.oIndexWin.oListWin.treeview_, path, NULL, FALSE, 0, 0);
				gtk_tree_path_free(path);
			}
			break;
		default:
			/*nothing*/break;
		}//switch (oMidWin.oTextWin.query_result) {
	}
    if (conf->get_bool_at("network/enable_netdict")) {
        STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, text);
        oStarDictClient.send_commands(1, c);
    }

	//when TEXT_WIN_TIPS,the text[0]=='\0',already returned.
	oTopWin.select_region_in_text(0, -1);
	oTopWin.InsertHisList(text);
	oTopWin.InsertBackList();
	if (GTK_WIDGET_SENSITIVE(oMidWin.oToolWin.PronounceWordButton))
		oReadWord.read(oMidWin.oTextWin.pronounceWord.c_str());
}

void AppCore::TopWinWordChange(const gchar* sWord)
{
	std::string res;
	switch (analyse_query(sWord, res)) {
	case qtREGEXP:
		oMidWin.oTextWin.Show(_("Press Enter to list the words that match the pattern."));
		break;
	case qtFUZZY:
		if (strlen(sWord)==1)
			oMidWin.oTextWin.Show(_("Fuzzy query..."));
		break;
	case qtDATA:
		if (strlen(sWord)==1)
			oMidWin.oTextWin.Show(_("Full-text search..."));
		break;
	default:
		stop_word_change_timer();
		delayed_word_ = res;
		word_change_timeout_ = g_timeout_add(100, on_word_change_timeout, this);
	}
}

gboolean AppCore::on_word_change_timeout(gpointer data)
{
	AppCore *app = static_cast<AppCore *>(data);
	bool showfirst = conf->get_bool_at("main_window/showfirst_when_notfound");
	bool find = app->SimpleLookupToTextWin(app->delayed_word_.c_str(),
					       app->iCurrentIndex, NULL, true,
					       !showfirst);
	app->ListWords(app->delayed_word_.c_str(), app->iCurrentIndex,
		       !find && showfirst);

    if (conf->get_bool_at("network/enable_netdict")) {
        STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, app->delayed_word_.c_str());
        app->oStarDictClient.send_commands(1, c);
    }

    app->word_change_timeout_ = 0;//next line destroy timer
	return FALSE;
}

void AppCore::ListWords(const gchar *sWord, CurrentIndex* iIndex, bool showfirst)
{
	CurrentIndex *iCurrent = (CurrentIndex*)g_memdup(iIndex, sizeof(CurrentIndex)*oLibs.ndicts());

	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);

	int iWordCount=0;
	const gchar * poCurrentWord=oLibs.poGetCurrentWord(iCurrent, dictmask, 0);
	if (poCurrentWord) {
		if (showfirst) {
			gchar *cword = g_strdup(poCurrentWord);
			SimpleLookupToTextWin(sWord, iCurrent, cword, false, true, true);
			oMidWin.oIndexWin.oListWin.InsertLast(cword);
			g_free(cword);
		} else {
			oMidWin.oIndexWin.oListWin.InsertLast(poCurrentWord);
		}
		iWordCount++;

		while (iWordCount<LIST_WIN_ROW_NUM &&
					 (poCurrentWord=oLibs.poGetNextWord(NULL,iCurrent, dictmask, 0))) {
			oMidWin.oIndexWin.oListWin.InsertLast(poCurrentWord);
			iWordCount++;
		}
	} else {
		if (showfirst)
			ShowNotFoundToTextWin(sWord,_("<Not Found!>"), TEXT_WIN_NOT_FOUND);
	}

	if (iWordCount) {
		oMidWin.oIndexWin.oListWin.ReScroll();
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_NORMAL_LIST;
	} else
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_EMPTY;

	g_free(iCurrent);
}

void AppCore::Query(const gchar *word)
{
	oTopWin.InsertHisList(oTopWin.get_text());
	oTopWin.InsertBackList();
	oTopWin.SetText(word);
	std::string res;
	switch (analyse_query(word, res)) {
	case qtFUZZY:
		LookupWithFuzzyToMainWin(res.c_str());
		break;
	case qtREGEXP:
		LookupWithRuleToMainWin(res.c_str());
		break;
	case qtDATA:
                LookupDataToMainWin(res.c_str());
                break;
	default:
		/*nothing?*/;
	}

	oTopWin.TextSelectAll();
	oTopWin.grab_focus();
	oTopWin.InsertHisList(word);
	oTopWin.InsertBackList(word);
}

void AppCore::ListClick(const gchar *word)
{
	oTopWin.InsertHisList(oTopWin.get_text());
	oTopWin.InsertBackList();
	oTopWin.SetText(word);
	oTopWin.InsertHisList(word);
	oTopWin.InsertBackList(word);
	oTopWin.grab_focus();
}

void AppCore::on_stardict_client_error(const char *error_msg)
{
    GtkWidget *message_dlg =
        gtk_message_dialog_new(
                GTK_WINDOW(window),
                (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                error_msg);
    gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
    gtk_dialog_run(GTK_DIALOG(message_dlg));
    gtk_widget_destroy(message_dlg);
}

void AppCore::on_stardict_client_register_end(const char *msg)
{
    GtkWidget *message_dlg =
        gtk_message_dialog_new(
                GTK_WINDOW(window),
                (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                msg);
    gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
    gtk_dialog_run(GTK_DIALOG(message_dlg));
    gtk_widget_destroy(message_dlg);
}

void AppCore::on_stardict_client_getdictmask_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_getdictmask(msg);
}

void AppCore::on_stardict_client_getdirinfo_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_getdirinfo(msg);
}

void AppCore::on_stardict_client_lookup_end(const struct STARDICT::LookupResponse *lookup_response)
{
    oMidWin.oTextWin.Show(&(lookup_response->dict_response));
    oMidWin.oIndexWin.oListWin.Clear();
    oMidWin.oIndexWin.oListWin.SetModel(true);
    if (!lookup_response->wordlist.empty()) {
        for (std::list<char *>::const_iterator i = lookup_response->wordlist.begin(); i != lookup_response->wordlist.end(); ++i) {
            oMidWin.oIndexWin.oListWin.InsertLast(*i);
        }
        oMidWin.oIndexWin.oListWin.ReScroll();
        oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_NORMAL_LIST;
    } else {
        oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_EMPTY;
    }
}

void AppCore::on_stardict_client_define_end(const struct STARDICT::DictResponse *dict_response)
{
    oMidWin.oTextWin.Show(dict_response);
}

class reload_show_progress_t : public show_progress_t {
public:
	reload_show_progress_t(progress_win &pw_) : pw(pw_) {}
	void notify_about_start(const std::string& title) {
		pw.display_action(title);
	}
private:
	progress_win &pw;
};

void AppCore::PopupPrefsDlg()
{
	if (!prefs_dlg) {
		prefs_dlg = new PrefsDlg(GTK_WINDOW(window),
					 get_impl(oAppSkin.icon),
					 unlock_keys->possible_combs());
		bool enbcol =
			conf->get_bool_at("dictionary/enable_collation");
		int colf =
			conf->get_int_at("dictionary/collate_function");
		bool exiting = prefs_dlg->ShowModal();
		delete prefs_dlg;
		prefs_dlg = NULL;
		if (exiting)
			return;
		if (enbcol == conf->get_bool_at("dictionary/enable_collation") &&
		    colf == conf->get_int_at("dictionary/collate_function"))
			return;
		progress_win pw;
		reload_show_progress_t rsp(pw);
		oLibs.set_show_progress(&rsp);
		reload_dicts();
		oLibs.set_show_progress(&gtk_show_progress);
	}
}

void AppCore::reload_dicts()
{
	oLibs.reload(conf->get_strlist("/apps/stardict/manage_dictionaries/dict_dirs_list"),
		     conf->get_strlist("/apps/stardict/manage_dictionaries/dict_order_list"),
		     conf->get_strlist("/apps/stardict/manage_dictionaries/dict_disable_list"),
		     conf->get_bool_at("dictionary/enable_collation"),
		     conf->get_int_at("dictionary/collate_function"));
    oLibs.SetDictMask(dictmask, NULL, -1, -1);
	g_free(iCurrentIndex);
	iCurrentIndex = (CurrentIndex*)g_malloc0(sizeof(CurrentIndex) * oLibs.ndicts());

	const gchar *sWord = oTopWin.get_text();

	if (sWord && sWord[0])
		TopWinWordChange(sWord);
}

void AppCore::PopupDictManageDlg()
{

	if (!dict_manage_dlg)
		dict_manage_dlg =
			new DictManageDlg(GTK_WINDOW(window),
					  get_impl(oAppSkin.index_wazard),
					  get_impl(oAppSkin.index_appendix));
	if (dict_manage_dlg->Show())
		return;
	progress_win pw;
	reload_show_progress_t rsp(pw);
	oLibs.set_show_progress(&rsp);
	reload_dicts();
	oLibs.set_show_progress(&gtk_show_progress);
}

void AppCore::stop_word_change_timer()
{
	if (word_change_timeout_) {
		g_source_remove(word_change_timeout_);
		word_change_timeout_ = 0;
	}

}

void AppCore::End()
{
	stop_word_change_timer();
	oSelection.End();
#ifdef _WIN32
	oClipboard.End();
	oMouseover.End();
	oHotkey.End();
#endif
	oFloatWin.End();

	oDockLet.reset(0);

	if (dict_manage_dlg)
		dict_manage_dlg->Close();
	if (prefs_dlg)
		prefs_dlg->Close(); // After user open the preferences dialog, then choose quit in the notification icon, this dialog can be closed.
	oTopWin.Destroy();
	oMidWin.oIndexWin.oListWin.Destroy();

	if (oBottomWin.SearchWebsiteMenu)
		gtk_widget_destroy(oBottomWin.SearchWebsiteMenu);
	gtk_widget_destroy(window);
}

void AppCore::Init(gchar *queryword)
{
	conf->notify_add("/apps/stardict/preferences/main_window/hide_list",
			 sigc::mem_fun(this, &AppCore::on_main_win_hide_list_changed));
	conf->notify_add("/apps/stardict/preferences/dictionary/scan_selection",
			 sigc::mem_fun(this, &AppCore::on_dict_scan_select_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/lock",
			 sigc::mem_fun(this, &AppCore::on_floatwin_lock_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/lock_x",
			 sigc::mem_fun(this, &AppCore::on_floatwin_lock_x_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/lock_y",
			 sigc::mem_fun(this, &AppCore::on_floatwin_lock_y_changed));
	conf->notify_add("/apps/stardict/preferences/dictionary/scan_modifier_key",
			 sigc::mem_fun(this, &AppCore::on_scan_modifier_key_changed));
	if (!hide_option)
		stardict_splash.show();

	oAppSkin.load();

	Create(queryword);

#ifdef CONFIG_GNOME
  stardict_app_server =
    stardict_application_server_new(gdk_screen_get_default());
#endif

	gtk_main();
}

void AppCore::Quit()
{

	if (!conf->get_bool_at("main_window/maximized")) {
		gint width, height;
		gtk_window_get_size(GTK_WINDOW(window), &width, &height);
    conf->set_int_at("main_window/window_width", width);
    conf->set_int_at("main_window/window_height", height);
	}
	gint pos = gtk_paned_get_position(GTK_PANED(oMidWin.hpaned));
  conf->set_int_at("main_window/hpaned_pos", pos);

	if (conf->get_bool_at("floating_window/lock")) {
		gint x, y;
		gtk_window_get_position(GTK_WINDOW(oFloatWin.FloatWindow), &x, &y);
    conf->set_int_at("floating_window/lock_x", x);
    conf->set_int_at("floating_window/lock_y", y);
	}

	End();

#ifdef CONFIG_GNOME
	bonobo_object_unref (stardict_app_server);
#endif
   unlock_keys.reset(0);
	 conf.reset(0);
	 gtk_main_quit();
}

void AppCore::on_main_win_hide_list_changed(const baseconfval* hideval)
{
	bool hide = static_cast<const confval<bool> *>(hideval)->val_;

	if (hide) {
		gtk_widget_hide(oMidWin.oToolWin.HideListButton);
		gtk_widget_show(oMidWin.oToolWin.ShowListButton);
		gtk_widget_hide(oMidWin.oIndexWin.vbox);
	} else {
		gtk_widget_hide(oMidWin.oToolWin.ShowListButton);
		gtk_widget_show(oMidWin.oToolWin.HideListButton);
		gtk_widget_show(oMidWin.oIndexWin.vbox);
	}
}

void AppCore::on_dict_scan_select_changed(const baseconfval* scanval)
{
	bool scan = static_cast<const confval<bool> *>(scanval)->val_;

	gtk_widget_set_sensitive(oFloatWin.StopButton, scan);
	if (scan != gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oBottomWin.ScanSelectionCheckButton)))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(oBottomWin.ScanSelectionCheckButton), scan);

	oDockLet->set_scan_mode(scan);
	if (GTK_WIDGET_VISIBLE(window))
		oDockLet->hide_state();
	if (scan) {
		bool lock=conf->get_bool_at("floating_window/lock");
		if (lock && !oFloatWin.QueryingWord.empty())
			oFloatWin.Show();
		oSelection.start();
#ifdef _WIN32
		if (conf->get_bool_at("dictionary/scan_clipboard")) {
			oClipboard.start();
		}
		oMouseover.start();
#endif
	} else {
		oFloatWin.Hide();
		oSelection.stop();
#ifdef _WIN32
		if (conf->get_bool_at("dictionary/scan_clipboard")) {
			oClipboard.stop();
		}
		oMouseover.stop();
#endif
	}
}

void AppCore::on_floatwin_lock_changed(const baseconfval* lockval)
{
	bool lock = static_cast<const confval<bool> *>(lockval)->val_;
	if (lock)
		gtk_image_set_from_stock(GTK_IMAGE(oFloatWin.lock_image),
					 GTK_STOCK_GOTO_LAST, GTK_ICON_SIZE_MENU);
	else
		gtk_image_set_from_stock(GTK_IMAGE(oFloatWin.lock_image),
					 GTK_STOCK_GO_FORWARD, GTK_ICON_SIZE_MENU);
}

void AppCore::on_floatwin_lock_x_changed(const baseconfval* lock_x_val)
{
	int lock_x=static_cast<const confval<int> *>(lock_x_val)->val_;
	if (conf->get_bool_at("floating_window/lock")) {
		gint old_x, old_y;
		gtk_window_get_position(GTK_WINDOW(oFloatWin.FloatWindow), &old_x, &old_y);
		if (lock_x!=old_x)
			gtk_window_move(GTK_WINDOW(oFloatWin.FloatWindow), lock_x, old_y);
	}
}

void AppCore::on_floatwin_lock_y_changed(const baseconfval* lock_y_val)
{
	int lock_y=static_cast<const confval<int> *>(lock_y_val)->val_;

	if (conf->get_bool_at("floating_window/lock")) {
		gint old_x,old_y;
		gtk_window_get_position(GTK_WINDOW(oFloatWin.FloatWindow), &old_x, &old_y);
		if (lock_y!=old_y)
			gtk_window_move(GTK_WINDOW(oFloatWin.FloatWindow), old_x, lock_y);
	}
}

void AppCore::on_scan_modifier_key_changed(const baseconfval* keyval)
{
	int key = static_cast<const confval<int> *>(keyval)->val_;
	unlock_keys->set_comb(combnum2str(key));
}

gchar* GetPureEnglishAlpha(gchar *str)
{
	while (*str && (!((*str >= 'a' && *str <='z')||(*str >= 'A' && *str <='Z'))))
		str++;
	gchar *p = str;
	while (*p && ((*p >= 'a' && *p <='z')||(*p >= 'A' && *p <='Z')||(*p==' ')))
		p++;
	*p='\0';
	return str;
}

gchar* GetHeadWord(gchar *str)
{
	while (g_ascii_isspace(*str))
		str++;
	glong len = g_utf8_strlen(str, -1);
	if (len) {
		gchar *last_str = g_utf8_offset_to_pointer(str, len-1);
		gunichar last = g_utf8_get_char(last_str);
		while ((g_unichar_isspace(last) || g_unichar_ispunct(last)) || g_unichar_isdigit(last)) {
			*last_str = '\0';
			last_str = g_utf8_find_prev_char(str, last_str);
			if (!last_str)
				break;
			last = g_utf8_get_char(last_str);
		}
	}
	return str;
}

gboolean stardict_on_enter_notify(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	play_sound_on_event("buttonactive");

	return FALSE;
}

#ifdef CONFIG_GNOME
static void
stardict_handle_automation_cmdline (gchar *queryword)
{
	CORBA_Environment env;
	GNOME_Stardict_Application server;

	CORBA_exception_init (&env);

	server = bonobo_activation_activate_from_id ("OAFIID:GNOME_Stardict_Application",
                                                     0, NULL, &env);
	if (!server) {
		g_free(queryword);
		gdk_notify_startup_complete ();
		return;
	}
	//g_return_if_fail (server != NULL);

	if (quit_option) {
		GNOME_Stardict_Application_quit (server, &env);
	}
	else {
		if (queryword) {
			GNOME_Stardict_Application_queryWord (server, queryword, &env);
		}
		if (hide_option) {
			GNOME_Stardict_Application_hide (server, &env);
		}
		else {
			GNOME_Stardict_Application_grabFocus (server, &env);
			g_message(_("StarDict is already running. Using the running process."));
		}
	}


	bonobo_object_release_unref (server, &env);
	CORBA_exception_free (&env);

	g_free(queryword);

	/* we never popup a window, so tell startup-notification that
	 * we're done */
	gdk_notify_startup_complete ();
}

static void client_die_cb (GnomeClient *client, gpointer client_data)
{
	gpAppFrame->Quit();
}

static gboolean save_yourself_cb (GnomeClient       *client,
                              gint               phase,
                              GnomeRestartStyle  save_style,
                              gint               shutdown,
                              GnomeInteractStyle interact_style,
                              gint               fast,
                              gpointer           client_data)
{
    gchar *argv[] = {NULL, NULL, NULL};
	gchar *word = NULL;
    gint argc = 1;

    argv[0] = (gchar *)client_data;

	if (gpAppFrame->window) {
		if (!GTK_WIDGET_VISIBLE(gpAppFrame->window))
			argv[argc++] = "-h";
	}

	const gchar *text = gpAppFrame->oTopWin.get_text();
    	if (text[0]) {
		word = g_strdup(text);
        	argv[argc++] = word;
	}	

    gnome_client_set_restart_command(client, argc, argv);
    gnome_client_set_clone_command(client, argc, argv);
	if (word)
		g_free(word);

    return true;
}
#endif

#ifdef _WIN32
static void stardict_dummy_print(const gchar*)
{
}

static void stardict_dummy_log_handler(const gchar *,
				       GLogLevelFlags,
				       const gchar *,
				       gpointer)
{
}
#endif

static void set_data_dir()
{
	//set gStarDictDataDir;
#ifdef _WIN32
	HMODULE hmod;

	if ((hmod = GetModuleHandle(NULL))==0)
		exit(EXIT_FAILURE);
	char tmp_buf[256];
	if (GetModuleFileName(hmod, tmp_buf, sizeof(tmp_buf))==0)
		exit(EXIT_FAILURE);

	gchar* buf = g_path_get_dirname(tmp_buf);
	gStarDictDataDir=buf;
	g_free(buf);
#else
	gStarDictDataDir = STARDICT_DATA_DIR;
#endif
}

#ifdef _WIN32
int stardict_main(int argc,char **argv)
#else
int main(int argc,char **argv)
#endif
{
	set_data_dir();
#ifdef _WIN32
	bindtextdomain (GETTEXT_PACKAGE, (gStarDictDataDir + G_DIR_SEPARATOR_S "locale").c_str());
#else
	bindtextdomain (GETTEXT_PACKAGE, STARDICT_LOCALEDIR);
#endif
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	std::string userdir(get_user_config_dir());
	if (!g_file_test(userdir.c_str(), GFileTest(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))) {
		if (g_mkdir(userdir.c_str(), S_IRWXU)==-1)
			g_warning("Cannot create directory %s.", userdir.c_str());
	}
#ifdef _WIN32
	gchar *title=g_locale_from_utf8(_("StarDict"), -1, NULL, NULL, NULL);
	HWND ll_winhandle = FindWindowA(0, title);
	g_free(title);
	if (ll_winhandle > 0) {
		if (IsIconic(ll_winhandle))
                        ShowWindow(ll_winhandle,SW_RESTORE);
                else
                        SetForegroundWindow(ll_winhandle);
                return EXIT_SUCCESS;
	}
#endif
#if defined(_WIN32) || defined(CONFIG_GTK)
	gtk_set_locale();
	gtk_init(&argc, &argv);
#endif
#ifdef CONFIG_GPE
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);
#endif
#if defined(_WIN32)
	g_log_set_handler(NULL, (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("Gdk", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("Gtk", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("GLib", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("GModule", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("GLib-GObject", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_log_set_handler("GThread", (GLogLevelFlags)(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION),
			  stardict_dummy_log_handler, NULL);
	g_set_print_handler(stardict_dummy_print);
#endif
#ifndef CONFIG_GNOME
	static GOptionEntry entries[] = {
		{ "hide", 'h', 0, G_OPTION_ARG_NONE, &hide_option,
		  _("Do not show splash screen"), NULL },
		{ NULL },
	};

	glib::OptionContext opt_cnt(g_option_context_new(_("word")));
	g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
	g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
	glib::Error err;
	if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
		g_warning(_("Options parsing failed: %s\n"), err->message);
		return EXIT_FAILURE;
	}
	gchar *queryword = NULL;

	if (argc > 1)		
		if (g_utf8_validate(argv[1], -1, NULL))
			queryword= g_strdup(argv[1]);
		else
			queryword = g_locale_to_utf8(argv[1], -1, NULL, NULL,
						     NULL);
			
			
	
#else
	GnomeProgram *program;
	program = gnome_program_init ("stardict", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("Dictionary"),
			    GNOME_PARAM_APP_DATADIR, DATADIR,
			    NULL);



	GValue        value = { 0 };
	poptContext   pctx;

	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT,
			       g_value_init (&value, G_TYPE_POINTER));
	pctx = (poptContext) g_value_get_pointer (&value);
	g_value_unset (&value);

	char **args;
	args = (char**) poptGetArgs(pctx);

	gchar *queryword = NULL;
  if (args && args[0]) {
		//only look up the first word should OK.
    if (g_utf8_validate (args[0], -1, NULL))
			queryword= g_strdup(args[0]);
		else
			queryword = g_locale_to_utf8(args[0],-1,NULL,NULL,NULL);
	}
	poptFreeContext (pctx);

	CORBA_Object factory;
	factory = bonobo_activation_activate_from_id
		("OAFIID:GNOME_Stardict_Factory",
		Bonobo_ACTIVATION_FLAG_EXISTING_ONLY,
		NULL, NULL);

  if (factory != NULL) {
		/* there is an instance already running, so send
		 * commands to it if needed
		 */
                stardict_handle_automation_cmdline (queryword);
                /* and we're done */
    return EXIT_SUCCESS;
	}

	GnomeClient *client;
	if ((client = gnome_master_client()) != NULL) {
		g_signal_connect (client, "save_yourself", G_CALLBACK (save_yourself_cb), (gpointer) argv[0]);
		g_signal_connect (client, "die", G_CALLBACK (client_die_cb), NULL);
	}
#endif
	conf.reset(new AppConf);
	AppCore oAppCore;
	gpAppFrame = &oAppCore;
	oAppCore.Init(queryword);

	return EXIT_SUCCESS;
}

#ifdef _WIN32

#ifdef __GNUC__
#  ifndef _stdcall
#    define _stdcall  __attribute__((stdcall))
#  endif
#endif

int _stdcall
WinMain (struct HINSTANCE__ *hInstance,
	 struct HINSTANCE__ *hPrevInstance,
	 char               *lpszCmdLine,
	 int                 nCmdShow)
{
	stardictexe_hInstance = hInstance;
	return stardict_main (__argc, __argv);
}
#endif
