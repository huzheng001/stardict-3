/* StarDict - A international dictionary for GNOME.
 * Copyright (C) 2003-2007 Hu Zheng <huzheng001@gmail.com>
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

/*
 * Modified by the StarDict Team and others 2003-2006.  See the AUTHORS
 * file for a list of people on the StarDict Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * StarDict at <http://stardict-4.sourceforge.net>.
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
#include <iostream>
#include <sstream>

#ifdef CONFIG_GNOME
#  include <libgnome/libgnome.h>
#endif

#ifdef CONFIG_GNOME
#  include <bonobo/bonobo-main.h>
#  include "stardict-application-server.h"
#  include "GNOME_Stardict.h"
#endif

#ifdef CONFIG_GPE
#  include <gpe/init.h>
#endif

#ifdef CONFIG_MAEMO
#include "hildon-widgets/hildon-program.h"
#include "hildon-widgets/hildon-window.h"
#endif

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#  include "win32/intl.h"
#endif

#include "desktop.h"
#include "splash.h"
#include "conf.h"
#include "lib/utils.h"
#include "prefsdlg.h"
#include "iskeyspressed.h"
#include "class_factory.h"
#include "progresswin.h"
#include "dictmanagedlg.h"
#include "pluginmanagedlg.h"
#include "prefsdlg.h"
#include "lib/netdictcache.h"
#include "lib/full_text_trans.h"
#include "log.h"
#include "cmdlineopts.h"

#include "stardict.h"

AppCore *gpAppFrame;

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
class reload_show_progress_t : public show_progress_t {
public:
	reload_show_progress_t(progress_win &pw_) : pw(pw_) {}
	void notify_about_start(const std::string& title) {
		pw.display_action(title);
	}
	void notify_about_work() {
		ProcessGtkEvent();
	}
private:
	progress_win &pw;
};

/********************************************************************/
class load_show_progress_t : public show_progress_t {
public:
	void notify_about_start(const std::string& title) {
		stardict_splash.display_action(title);
	}
} load_show_progress;

/********************************************************************/
AppCore::AppCore() :
	oLibs(&gtk_show_progress,
	      conf->get_bool_at("dictionary/create_cache_file"),
	      conf->get_bool_at("dictionary/enable_collation") ? CollationLevel_SINGLE : CollationLevel_NONE,
	      int_to_colate_func(conf->get_int_at("dictionary/collate_function")))
{
	iCurrentIndex = NULL;
	word_change_timeout_id = 0;
	window = NULL; //need by save_yourself_cb().
	dict_manage_dlg = NULL;
	plugin_manage_dlg = NULL;
	prefs_dlg = NULL;
	oStarDictPlugins = NULL;
}

AppCore::~AppCore()
{
	stop_word_change_timer();
	delete dict_manage_dlg;
	delete plugin_manage_dlg;
	delete prefs_dlg;
	g_free(iCurrentIndex);
	delete oStarDictPlugins;
	// window?
}

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

void AppCore::on_docklet_middle_button_click()
{
	TNotifAreaMiddleClickAction action = TNotifAreaMiddleClickAction(
		conf->get_int_at("notification_area_icon/middle_click_action"));
	if (action == namclaQueryFloatWindow) {
		oSelection.LastClipWord.clear();
		gtk_selection_convert(oSelection.selection_widget,
				      GDK_SELECTION_PRIMARY,
				      oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	} else if(action == namclaQueryMainWindow){
		oDockLet->maximize_from_tray();
		gtk_selection_convert(oMidWin.oTextWin.view->widget(),
				      GDK_SELECTION_PRIMARY,
				      oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	}
}

void AppCore::on_link_click(const std::string &link)
{
	if (g_str_has_prefix(link.c_str(), "query://")) {
		oTopWin.InsertHisList(oTopWin.get_text());
		oTopWin.InsertBackList();
		oTopWin.SetText(link.c_str() + sizeof("query://") -1);
	} else if (g_str_has_prefix(link.c_str(), "bword://")) {
		oTopWin.InsertHisList(oTopWin.get_text());
		oTopWin.InsertBackList();
		oTopWin.SetText(link.c_str() + sizeof("bword://") -1);
	} else {
		show_url(link.c_str());
	}
}

void AppCore::do_send_http_request(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer userdata)
{
	HttpClient *client = new HttpClient();
	client->on_error_.connect(sigc::mem_fun(gpAppFrame, &AppCore::on_http_client_error));
	client->on_response_.connect(sigc::mem_fun(gpAppFrame, &AppCore::on_http_client_response));
	gpAppFrame->oHttpManager.Add(client);
	client->SendHttpGetRequestWithCallback(shost, sfile, callback_func, userdata);
}

void AppCore::set_news(const char *news, const char *links)
{
	gpAppFrame->oBottomWin.set_news(news, links);
}

void AppCore::show_netdict_resp(const char *dict, NetDictResponse *resp, bool ismainwin)
{
	if (ismainwin)
		gpAppFrame->oMidWin.oTextWin.Show(resp);
	else {
		if(gpAppFrame->composite_lookup_float_win.got_net_dict_responce(dict, resp->word))
			gpAppFrame->oFloatWin.AppendTextNetDict(resp);
		if(gpAppFrame->composite_lookup_float_win.is_got_all_responses())
			gpAppFrame->oFloatWin.EndLookup();
	}
}

void AppCore::lookup_dict(size_t dictid, const char *sWord, char ****Word, char *****WordData)
{
	InstantDictIndex instance_dict_index;
	instance_dict_index.type = InstantDictType_LOCAL;
	instance_dict_index.index = dictid;
	std::vector<InstantDictIndex> dictmask;
	dictmask.push_back(instance_dict_index);
	bool bFound = false;
	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * dictmask.size());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * dictmask.size());
	CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * dictmask.size());
	gpAppFrame->BuildResultData(dictmask, sWord, iIndex, NULL, 0, pppWord, ppppWordData, bFound, 2);
	*Word = pppWord;
	*WordData = ppppWordData;
	g_free(iIndex);
}

void AppCore::ShowPangoTips(const char *word, const char *text)
{
	gpAppFrame->oFloatWin.ShowPangoTips(word, text);
}

void AppCore::Create(const gchar *queryword)
{
	if (conf->get_bool_at("dictionary/use_custom_font")) {
		const std::string &custom_font(conf->get_string_at("dictionary/custom_font"));

		if (!custom_font.empty())	{
			gchar *aa =
				g_strdup_printf("style \"custom-font\" { font_name= \"%s\" }\n"
						"class \"GtkWidget\" style \"custom-font\"\n",
						custom_font.c_str());
#if GTK_MAJOR_VERSION >= 3
			GtkCssProvider *css_provider = gtk_css_provider_get_default();
			gtk_css_provider_load_from_data(css_provider, aa, -1, NULL);
#else
			gtk_rc_parse_string(aa);
#endif
			g_free(aa);
		}
	}

#ifdef CONFIG_MAEMO
	HildonProgram *program;
	program = HILDON_PROGRAM(hildon_program_get_instance());
	g_set_application_name(_("StarDict"));
	window = hildon_window_new();
	hildon_program_add_window(program, HILDON_WINDOW(window));
#else
#if defined(CONFIG_GNOME)
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
#endif
#endif

// Init oStarDictPlugins after we get window.
	oStarDictPluginSystemInfo.datadir = conf_dirs->get_data_dir();
	oStarDictPluginSystemInfo.mainwin = window;
	oStarDictPluginSystemService.send_http_request = do_send_http_request;
	oStarDictPluginSystemService.show_url = show_url;
	oStarDictPluginSystemService.set_news = set_news;
	oStarDictPluginSystemService.encode_uri_string = common_encode_uri_string;
	oStarDictPluginSystemService.build_dictdata = common_build_dictdata;
	oStarDictPluginSystemService.netdict_save_cache_resp = netdict_save_cache_resp;
	oStarDictPluginSystemService.show_netdict_resp = show_netdict_resp;
	oStarDictPluginSystemService.lookup_dict = lookup_dict;
	oStarDictPluginSystemService.FreeResultData = FreeResultData;
	oStarDictPluginSystemService.ShowPangoTips = ShowPangoTips;
	std::list<DictItemId> plugin_new_install_list;
	UpdatePluginList(plugin_new_install_list);
#ifdef _WIN32
	std::list<std::string> plugin_order_list;
	std::list<std::string> plugin_disable_list;
	{
		const std::list<std::string>& plugin_order_list_rel
			= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
		const std::list<std::string>& plugin_disable_list_rel
			= conf->get_strlist("/apps/stardict/manage_plugins/plugin_disable_list");
		abs_path_to_data_dir(plugin_order_list_rel, plugin_order_list);
		abs_path_to_data_dir(plugin_disable_list_rel, plugin_disable_list);
	}
#else
	const std::list<std::string>& plugin_order_list
		= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
	const std::list<std::string>& plugin_disable_list
		= conf->get_strlist("/apps/stardict/manage_plugins/plugin_disable_list");
#endif
	oStarDictPlugins = new StarDictPlugins(conf_dirs->get_plugin_dir(),
		plugin_order_list,
		plugin_disable_list);

	oLibs.set_show_progress(&load_show_progress);
	std::list<DictItemId> dict_new_install_list;
	bool verify_dict = conf->get_bool_at("dictionary/do_not_load_bad_dict");
	UpdateDictList(dict_new_install_list, &load_show_progress, verify_dict);
	UpdateConfigXML(dict_new_install_list, plugin_new_install_list, oStarDictPlugins);
	LoadDictInfo();
	{
		std::list<DictItemId> load_list;
		GetUsedDictList(load_list);
		std::list<std::string> s_load_list;
		DictItemId::convert(s_load_list, load_list);
		oLibs.load(s_load_list);
	}
	oLibs.set_show_progress(&gtk_show_progress);

	oStarDictClient.set_server(conf->get_string_at("network/server").c_str(), conf->get_int_at("network/port"));
	const std::string &user = conf->get_string_at("network/user");
	const std::string &md5saltpasswd = conf->get_string_at("network/md5saltpasswd");
	if (!user.empty() && !md5saltpasswd.empty()) {
		oStarDictClient.set_auth(user.c_str(), md5saltpasswd.c_str());
	}
	oStarDictClient.on_error_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_error));
	oStarDictClient.on_lookup_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_lookup_end));
	oStarDictClient.on_floatwin_lookup_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_floatwin_lookup_end));
	oStarDictClient.on_register_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_register_end));
	oStarDictClient.on_changepassword_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_changepassword_end));
	oStarDictClient.on_getdictmask_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_getdictmask_end));
	oStarDictClient.on_getadinfo_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_getadinfo_end));
	oStarDictClient.on_dirinfo_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_dirinfo_end));
	oStarDictClient.on_dictinfo_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_dictinfo_end));
	oStarDictClient.on_maxdictcount_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_maxdictcount_end));
	oStarDictClient.on_previous_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_previous_end));
	oStarDictClient.on_next_end_.connect(sigc::mem_fun(this, &AppCore::on_stardict_client_next_end));

	UpdateDictMask();

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
	int transparent=conf->get_int_at("main_window/transparent");
	if (transparent != 0)
		gtk_widget_set_opacity(window, (100-transparent)/100.0);
	gtk_window_set_title (GTK_WINDOW (window), _("StarDict"));
	gtk_window_set_icon(GTK_WINDOW(window),
			    get_impl(oAppSkin.icon));
	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	g_signal_connect (G_OBJECT (window), "show", G_CALLBACK (on_mainwin_show_event), this);
	g_signal_connect (G_OBJECT (window), "delete_event", G_CALLBACK (on_delete_event), this);
	g_signal_connect (G_OBJECT (window), "window_state_event", G_CALLBACK (on_window_state_event), this);
	g_signal_connect (G_OBJECT (window), "key_press_event", G_CALLBACK (vKeyPressReleaseCallback), this);
	g_signal_connect (G_OBJECT (window), "key_release_event", G_CALLBACK (vKeyPressReleaseCallback), this);

#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
#endif
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
	oDockLet.reset(PlatformFactory::create_tray_icon(window, scan,
							 oAppSkin));
	oDockLet->on_quit_.connect(sigc::mem_fun(this, &AppCore::Quit));
	oDockLet->on_change_scan_.connect(
		sigc::mem_fun(this, &AppCore::on_change_scan));
	oDockLet->on_maximize_.connect(
		sigc::mem_fun(this, &AppCore::on_maximize));
	oDockLet->on_middle_btn_click_.connect(
		sigc::mem_fun(this, &AppCore::on_docklet_middle_button_click));
	oSelection.Init();
#ifdef _WIN32
	oClipboard.Init();
	oMouseover.Init();
#endif
	oHotkey.Init();
#ifdef ENABLE_LOG_WINDOW
	gLogWindow.Init();
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
	if (conf->get_bool_at("dictionary/use_scan_hotkey")) {
		const std::string &hotkey = conf->get_string_at(
		  "dictionary/scan_hotkey");
		oHotkey.start_scan(hotkey.c_str());
	}
	if (conf->get_bool_at("dictionary/use_mainwindow_hotkey")) {
		const std::string &hotkey = conf->get_string_at(
		  "dictionary/mainwindow_hotkey");
		oHotkey.start_mainwindow(hotkey.c_str());
	}

	bool hide=conf->get_bool_at("main_window/hide_on_startup");

	//NOTICE: when docklet embedded failed,it should always show the window,but,how to detect the failure?
	// As stardict is FOR GNOME,so i don't want to consider the case that haven't the Notification area applet.
	if (!CmdLineOptions::get_hide() && (queryword || !hide)) {
		oDockLet->hide_state();
		gtk_widget_show(window);
	} else {
// This may be needed, so gtk_window_get_screen() in gtk_iskeyspressed.cpp can always work.
		gtk_widget_realize(window);
		gdk_notify_startup_complete();
	}

	bool have_netdict = false;
	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++) {
		if (query_dictmask[iLib].type == InstantDictType_NET) {
			have_netdict = true;
			break;
		}
	}
	if (oLibs.has_dict() || conf->get_bool_at("network/enable_netdict") || have_netdict) {
		if (queryword) {
			Query(queryword);
		} else {
			oMidWin.oTextWin.ShowTips();
		}
	} else {
		oMidWin.oTextWin.ShowInitFailed();
	}

	bool keepabove = conf->get_bool_at("main_window/keep_above");
	gtk_window_set_keep_above(GTK_WINDOW(window), keepabove);
}

void AppCore::on_mainwin_show_event(GtkWidget * window, AppCore *app)
{
	static bool loaded = false;
	if (!loaded) {
		loaded = true;
		// We need to set the hpaned position after the window showed, or it will become incorrect.
		int pos=conf->get_int_at("main_window/hpaned_pos");
		gtk_paned_set_position(GTK_PANED(app->oMidWin.hpaned), pos);
	}
}

gboolean AppCore::on_delete_event(GtkWidget * window, GdkEvent *event , AppCore *app)
{
#ifdef CONFIG_DARWIN
	gtk_window_iconify(GTK_WINDOW(window));
#else
	app->oDockLet->minimize_to_tray();
#endif
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
	if ((event->keyval==GDK_KEY_q || event->keyval==GDK_KEY_Q) && only_ctrl_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->Quit();
	}
	else if ((event->keyval==GDK_KEY_x || event->keyval==GDK_KEY_X) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			oAppCore->oDockLet->minimize_to_tray();
		}
	}
	else if ((event->keyval==GDK_KEY_z || event->keyval==GDK_KEY_Z) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			gtk_window_iconify(GTK_WINDOW(window));
		}
	}
	else if ((event->keyval==GDK_KEY_e || event->keyval==GDK_KEY_E) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS) {
			oAppCore->oMidWin.oToolWin.do_save();
		}
	}
	else if (event->keyval==GDK_KEY_F1 && (!(event->state & GDK_CONTROL_MASK)) && (!(event->state & GDK_MOD1_MASK)) && (!(event->state & GDK_SHIFT_MASK))) {
		if (event->type==GDK_KEY_PRESS)
		  show_help(NULL);
	}
	else if ((event->keyval==GDK_KEY_f || event->keyval==GDK_KEY_F) && only_ctrl_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oMidWin.oToolWin.do_search();
	}
	else if ((event->keyval==GDK_KEY_Left) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_back();
	}
	else if ((event->keyval==GDK_KEY_Right) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_forward();
	}
	else if ((event->keyval==GDK_KEY_Up) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_prev();
	}
	else if ((event->keyval==GDK_KEY_Down) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_next();
	}
	else if ((event->keyval==GDK_KEY_c || event->keyval==GDK_KEY_C) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.clear_entry();
	}
	else if ((event->keyval==GDK_KEY_m || event->keyval==GDK_KEY_M) && only_mod1_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.do_menu();
	}
	else if ((event->keyval==GDK_KEY_u || event->keyval==GDK_KEY_U) && only_ctrl_pressed) {
		if (event->type==GDK_KEY_PRESS)
			oAppCore->oTopWin.clear_entry();
	}
	else if ((event->keyval==GDK_KEY_v || event->keyval==GDK_KEY_V) && only_ctrl_pressed &&
			!oAppCore->oTopWin.has_focus() &&
			!oAppCore->oMidWin.oTransWin.IsInputViewHasFocus() &&
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
						 !oAppCore->oMidWin.oTransWin.IsInputViewHasFocus() &&
						 !oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
		oAppCore->oTopWin.InsertBackList();
		gchar str[2] = { (gchar)(event->keyval), '\0' };
		oAppCore->oTopWin.grab_focus();
		oAppCore->oTopWin.SetText(str, conf->get_bool_at("main_window/search_while_typing"));
		oAppCore->oTopWin.set_position_in_text(1);
	} else if (event->type==GDK_KEY_PRESS && event->keyval == GDK_KEY_BackSpace &&
		!oAppCore->oTopWin.has_focus() &&
		!oAppCore->oMidWin.oTransWin.IsInputViewHasFocus() &&
		!oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		oAppCore->oTopWin.clear_entry();
	} else if (event->type == GDK_KEY_PRESS &&
		   event->keyval == GDK_KEY_Return &&
		   !oAppCore->oTopWin.has_focus() &&
		   !oAppCore->oMidWin.oTransWin.IsInputViewHasFocus() &&
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
			oAppCore->TopWinEnterWord();
		}
	} else if (event->type==GDK_KEY_PRESS &&
						 event->keyval == 0x20 &&
						 !oAppCore->oTopWin.has_focus() &&
						 !oAppCore->oMidWin.oTransWin.IsInputViewHasFocus() &&
						 !oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus()) {
		oAppCore->oTopWin.InsertHisList(oAppCore->oTopWin.get_text());
		oAppCore->oTopWin.InsertBackList();
		oAppCore->oTopWin.grab_focus();
	} else if(event->type==GDK_KEY_PRESS && event->keyval == GDK_KEY_Escape) {
		if(oAppCore->oMidWin.oTextWin.IsSearchPanelHasFocus())
			oAppCore->oMidWin.oTextWin.HideSearchPanel();
		else
			oAppCore->oTopWin.clear_entry();
	} else {
		return_val=false;
	}
	return return_val;
}

void AppCore::SimpleLookupToFloat(const char* sWord, bool IgnoreScanModifierKey)
{
	oFloatWin.StartLookup(sWord, IgnoreScanModifierKey);
	composite_lookup_float_win.new_lookup();
	if (IsASCII(sWord)) {
		if (SimpleLookupToFloatLocal(sWord)) {
			//found
		} else {
			gchar *sWord2 = g_strdup(sWord);
			gchar *a = GetPureEnglishAlpha(sWord2);
			if (*a) {
				if (strcmp(sWord, a) == 0) {
					// Not found.
					oTopWin.InsertHisList(a); //really need?
				} else {
					SimpleLookupToFloatLocal(a);
				}
			} else {
				// The string is too strange, don't show any thing.
			}
			g_free(sWord2);
		}
	} else {
		SimpleLookupToFloatLocal(sWord);
	}
	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (enable_netdict) {
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_SELECT_QUERY, sWord);
		if (!oStarDictClient.try_cache(c)) {
			composite_lookup_float_win.send_StarDict_net_request(c->seq);
			oStarDictClient.send_commands(1, c);
		}
	}
	LookupNetDict(sWord, false);
	composite_lookup_float_win.done_lookup();
	if(composite_lookup_float_win.is_got_all_responses())
		oFloatWin.EndLookup();
}

bool AppCore::SimpleLookupToFloatLocal(const char* sWord)
{
	if (sWord==NULL || sWord[0]=='\0')
		return true;
	char *SearchWord = (char *)g_malloc(strlen(sWord)+1);
	copy_normalize_trim_spaces(SearchWord, sWord);
	if (SearchWord[0]=='\0') {
		strcpy(SearchWord, sWord);
	}
	char *EndPointer = SearchWord+strlen(SearchWord);

	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * scan_dictmask.size());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * scan_dictmask.size());
	CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * scan_dictmask.size());

	//find the word use most biggest length
	while (EndPointer>SearchWord) {
		EndPointer = delete_trailing_spaces_ASCII(SearchWord, EndPointer);

		bool bFound = false;
		for (size_t iLib=0;iLib<scan_dictmask.size();iLib++)
			BuildResultData(scan_dictmask, SearchWord, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 2);
		for (size_t iLib=0; iLib<scan_dictmask.size(); iLib++)
			BuildVirtualDictData(scan_dictmask, SearchWord, iLib, pppWord, ppppWordData, bFound);
		if (bFound) {
			oFloatWin.AppendTextLocalDict(pppWord, ppppWordData, SearchWord);
			oTopWin.InsertHisList(SearchWord);
			FreeResultData(scan_dictmask.size(), pppWord, ppppWordData);
			g_free(iIndex);
			g_free(SearchWord);
			return true;
		}
		if (IsASCII(SearchWord)) {
			EndPointer = delete_trailing_word_ASCII(SearchWord, EndPointer);
		} else {
			EndPointer = delete_trailing_char(SearchWord, EndPointer);
		}
	}
	FreeResultData(scan_dictmask.size(), pppWord, ppppWordData);
	g_free(iIndex);
	g_free(SearchWord);
	return false;
}

#ifdef _WIN32
void AppCore::SmartLookupToFloat(const gchar* sWord, int BeginPos, bool IgnoreScanModifierKey)
{
	oFloatWin.StartLookup(sWord, IgnoreScanModifierKey);
	composite_lookup_float_win.new_lookup();
	LocalSmartLookupToFloat(sWord, BeginPos);
	/* sWord is not a candidate to search in net dictionaries */
	composite_lookup_float_win.done_lookup();
	if(composite_lookup_float_win.is_got_all_responses())
		oFloatWin.EndLookup();
}

bool AppCore::LocalSmartLookupToFloat(const gchar* sWord, int BeginPos)
{
	if (sWord==NULL || sWord[0]=='\0')
		return false;
	char *SearchWord = (char *)g_malloc(strlen(sWord)+1);
	std::string TriedSearchWord;

	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * scan_dictmask.size());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * scan_dictmask.size());
	CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * scan_dictmask.size());

	int try_cnt = 0;
	while(true) {
		bool skip_try = false;
		switch(try_cnt)
		{
			case 0:
				extract_word(SearchWord, sWord, BeginPos, is_space_or_punct);
				TriedSearchWord = SearchWord;
				++try_cnt;
				break;
			case 1:
				extract_word(SearchWord, sWord, BeginPos, is_not_alpha);
				if(SearchWord[0])
					TriedSearchWord = SearchWord;
				++try_cnt;
				break;
			case 2:
				{
					gunichar c = g_utf8_get_char(sWord + BeginPos);
					if(g_unichar_islower(c))
						extract_word(SearchWord, sWord, BeginPos, is_not_lower);
					else if(g_unichar_isupper(c))
						extract_word(SearchWord, sWord, BeginPos, is_not_upper);
					else
						skip_try = true;
					if(SearchWord[0])
						TriedSearchWord = SearchWord;
					++try_cnt;
					break;
				}
			case 3:
				extract_capitalized_word(SearchWord, sWord, BeginPos,
					g_unichar_isupper, g_unichar_islower);
				if(SearchWord[0])
					TriedSearchWord = SearchWord;
				++try_cnt;
				break;
			case 4:
				strcpy(SearchWord, TriedSearchWord.c_str());
				++try_cnt;
				skip_try = true;
			case 5:
				{ // cut last char
					char *end = SearchWord + strlen(SearchWord);
					end = g_utf8_prev_char(end);
					*end = '\0';
					if(!SearchWord[0]) {
						++try_cnt;
						skip_try = true;
					}
					break;
				}
			default:
				goto loop_end;
		}
		if(!SearchWord[0] || skip_try)
			continue;
		
		bool bFound = false;
		for (size_t iLib=0;iLib<scan_dictmask.size();iLib++)
			BuildResultData(scan_dictmask, SearchWord, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 2);
		for (size_t iLib=0; iLib<scan_dictmask.size(); iLib++)
			BuildVirtualDictData(scan_dictmask, SearchWord, iLib, pppWord, ppppWordData, bFound);
		
		if (bFound) {
			oFloatWin.AppendTextLocalDict(pppWord, ppppWordData, SearchWord);
			oTopWin.InsertHisList(SearchWord);
			FreeResultData(scan_dictmask.size(), pppWord, ppppWordData);
			g_free(iIndex);
			g_free(SearchWord);
			return true;
		}
	}
loop_end:
	FreeResultData(scan_dictmask.size(), pppWord, ppppWordData);
	g_free(iIndex);
	g_free(SearchWord);
	return false;
}
#endif

void AppCore::BuildVirtualDictData(std::vector<InstantDictIndex> &dictmask, const char* sWord, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound)
{
	if (dictmask[iLib].type == InstantDictType_NET) {
		const char *dict_cacheid = oStarDictPlugins->NetDictPlugins.dict_cacheid(dictmask[iLib].index);
		NetDictResponse *resp = netdict_get_cache_resp(dict_cacheid, sWord);
		if (resp && resp->data) {
			pppWord[iLib] = (gchar **)g_malloc(sizeof(gchar *)*2);
			pppWord[iLib][0] = g_strdup(resp->word);
			pppWord[iLib][1] = NULL;
			ppppWordData[iLib] = (gchar ***)g_malloc(sizeof(gchar **)*(1));
			ppppWordData[iLib][0] = (gchar **)g_malloc(sizeof(gchar *)*2);
			ppppWordData[iLib][0][0] =  stardict_datadup(resp->data);
			ppppWordData[iLib][0][1] = NULL;
			bFound = true;
		} else {
			pppWord[iLib] = NULL;
		}
	} else if (dictmask[iLib].type == InstantDictType_VIRTUAL) {
		oStarDictPlugins->VirtualDictPlugins.lookup(dictmask[iLib].index, sWord, &(pppWord[iLib]), &(ppppWordData[iLib]));
		if (pppWord[iLib])
			bFound = true;
	}
}

void AppCore::BuildResultData(std::vector<InstantDictIndex> &dictmask, const char* sWord, CurrentIndex *iIndex, const gchar *piIndexValidStr, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound, gint Method)
{
	if (dictmask[iLib].type != InstantDictType_LOCAL)
		return;

	int iRealLib = dictmask[iLib].index;
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
			bLookupWord = oLibs.LookupWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);
			bLookupSynonymWord = oLibs.LookupSynonymWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);
		} else if (Method==1) {
			bLookupWord = oLibs.LookupSimilarWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);
			bLookupSynonymWord = oLibs.LookupSynonymSimilarWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);
		} else {
			bLookupWord = oLibs.SimpleLookupWord(sWord, iIndex[iLib].idx, iIndex[iLib].idx_suggest, iRealLib, 0);
			bLookupSynonymWord = oLibs.SimpleLookupSynonymWord(sWord, iIndex[iLib].synidx, iIndex[iLib].synidx_suggest, iRealLib, 0);
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

void AppCore::FreeResultData(size_t dictmask_size, gchar ***pppWord, gchar ****ppppWordData)
{
	if (!pppWord)
		return;
	int j, k;
	size_t i;
	for (i=0; i<dictmask_size; i++) {
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

/* The input can be:
 * (sWord, NULL, NULL). Look up the sWord.
 * (sWord, piIndex, NULL). Look up the sWord, and set piIndex to the new indexes that found.
 * (sWord, piIndex, "word"), show sWord by piIndex's information, while the index point to "word".
 */
bool AppCore::SimpleLookupToTextWin(const char* sWord, CurrentIndex *piIndex, const gchar *piIndexValidStr, bool bTryMoreIfNotFound, bool bShowNotfound, bool isShowFirst)
{
	bool bFound = false;
	gchar ***pppWord = (gchar ***)g_malloc(sizeof(gchar **) * query_dictmask.size());
	gchar ****ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * query_dictmask.size());
	CurrentIndex *iIndex;
	if (!piIndex)
		iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * query_dictmask.size());
	else
		iIndex = piIndex;

	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
		BuildResultData(query_dictmask, sWord, iIndex, piIndexValidStr, iLib, pppWord, ppppWordData, bFound, 0);
	if (!bFound && !piIndexValidStr) {
		for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
			BuildResultData(query_dictmask, sWord, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 1);
	}
	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
		BuildVirtualDictData(query_dictmask, piIndexValidStr?piIndexValidStr:sWord, iLib, pppWord, ppppWordData, bFound);
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
					for (size_t iLib=0;iLib<query_dictmask.size();iLib++)
						BuildResultData(query_dictmask, hword, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 0);
					if (!bFound) {
						for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
							BuildResultData(query_dictmask, hword, iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 1);
					}
					for (size_t iLib=0; iLib<query_dictmask.size(); iLib++)
						BuildVirtualDictData(query_dictmask, hword, iLib, pppWord, ppppWordData, bFound);
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

	FreeResultData(query_dictmask.size(), pppWord, ppppWordData);

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

class LookupDataDialog {
public:
	LookupDataDialog(const char *sWord);
	void show();
private:
	std::string word;
	GtkListStore *now_tree_model;
	static void on_fulltext_search_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, LookupDataDialog *oLookupDataDialog);
};

LookupDataDialog::LookupDataDialog(const char *sWord)
{
	word = sWord;
}

void LookupDataDialog::on_fulltext_search_dict_enable_toggled (GtkCellRendererToggle *cell, gchar *path_str, LookupDataDialog *oLookupDataDialog)
{
	GtkTreeModel *model = GTK_TREE_MODEL(oLookupDataDialog->now_tree_model);
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeIter iter;
	gtk_tree_model_get_iter (model, &iter, path);
	gboolean enable;
	gtk_tree_model_get (model, &iter, 0, &enable, -1);
	enable = !enable;
	gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, enable, -1);
	gtk_tree_path_free (path);
}

void LookupDataDialog::show()
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(_("Full-text Search"), GTK_WINDOW(gpAppFrame->window), GTK_DIALOG_MODAL, NULL, NULL, NULL);
	GtkWidget *button = gtk_dialog_add_button(GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_button(GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
	gtk_widget_grab_focus(button);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 5);
#endif
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),vbox,true,true,0);
	GtkWidget *sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start(GTK_BOX(vbox),sw,true,true,0);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (sw, 300, 200);
	now_tree_model = gtk_list_store_new(3, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_LONG);
	for (std::vector<InstantDictIndex>::iterator i = gpAppFrame->query_dictmask.begin(); i != gpAppFrame->query_dictmask.end(); ++i) {
		if (i->type == InstantDictType_LOCAL) {
			GtkTreeIter new_iter;
			gtk_list_store_append(now_tree_model, &new_iter);
			gtk_list_store_set(now_tree_model, &new_iter, 0, TRUE, 1, gpAppFrame->oLibs.dict_name(i->index).c_str(), 2, i->index, -1);
		}
	}
	GtkWidget *now_treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(now_tree_model));
	g_object_unref (G_OBJECT (now_tree_model));
	//gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (now_treeview), TRUE);
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_toggle_new ();
	g_signal_connect (renderer, "toggled", G_CALLBACK (on_fulltext_search_dict_enable_toggled), this);
	column = gtk_tree_view_column_new_with_attributes (_("Search"), renderer, "active", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new_with_attributes (_("Dictionary Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(now_treeview), column);
	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);
	gtk_container_add (GTK_CONTAINER (sw), now_treeview);
	gtk_widget_show_all(vbox);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_ACCEPT) {
		std::vector<InstantDictIndex> dictmask;
		GtkTreeIter iter;
		GtkTreeModel *model = GTK_TREE_MODEL(now_tree_model);
		gboolean have_next = gtk_tree_model_get_iter_first(model, &iter);
		while (have_next) {
			gboolean enable;
			gtk_tree_model_get (model, &iter, 0, &enable, -1);
			if (enable) {
				glong index;
				gtk_tree_model_get (model, &iter, 2, &index, -1);
				InstantDictIndex dictindex;
				dictindex.type = InstantDictType_LOCAL;
				dictindex.index = index;
				dictmask.push_back(dictindex);
			}
			have_next = gtk_tree_model_iter_next(model, &iter);
		}
		gtk_widget_destroy(dialog);
		gpAppFrame->oMidWin.oIndexWin.oListWin.Clear();
		gpAppFrame->oMidWin.oIndexWin.oListWin.SetModel(false);
		gpAppFrame->oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_DATA_LIST;
		bool enable_netdict = conf->get_bool_at("network/enable_netdict");
		if (enable_netdict) {
			std::string lookupword = "|";
			lookupword += word;
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, lookupword.c_str());
			if (!gpAppFrame->oStarDictClient.try_cache(c)) {
				gpAppFrame->waiting_mainwin_lookupcmd_seq = c->seq;
				gpAppFrame->oStarDictClient.send_commands(1, c);
			}
		}
		gpAppFrame->LookupDataWithDictMask(word.c_str(), dictmask);
	} else {
		gtk_widget_destroy(dialog);
	}
}

void AppCore::LookupDataToMainWin(const gchar *sWord)
{
	if (query_dictmask.empty()) {
		oMidWin.oIndexWin.oListWin.Clear();
		oMidWin.oIndexWin.oListWin.SetModel(false);
		oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_DATA_LIST;
		bool enable_netdict = conf->get_bool_at("network/enable_netdict");
		if (enable_netdict) {
			std::string lookupword = "|";
			lookupword += sWord;
			STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, lookupword.c_str());
			if (!oStarDictClient.try_cache(c)) {
				waiting_mainwin_lookupcmd_seq = c->seq;
				oStarDictClient.send_commands(1, c);
			}
		} else {
			ShowNotFoundToTextWin(sWord, _("There are no dictionary articles containing this word. :-("), TEXT_WIN_FUZZY_NOT_FOUND);
		}
	} else {
		LookupDataDialog *dialog = new LookupDataDialog(sWord);
		dialog->show();
		delete dialog;
	}
}

void AppCore::LookupDataWithDictMask(const gchar *sWord, std::vector<InstantDictIndex> &dictmask)
{
	if (!sWord || !*sWord)
		return;
	change_cursor busy(gtk_widget_get_window(window),
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	bool cancel = false;
	FullTextSearchDialog Dialog;
	GtkWidget *search_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(search_window), _("Full-text search..."));
	gtk_window_set_transient_for(GTK_WINDOW(search_window), GTK_WINDOW(window));
	gtk_window_set_position(GTK_WINDOW(search_window), GTK_WIN_POS_CENTER_ON_PARENT);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 6);
#endif
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
		for (size_t i=0; i<dictmask.size(); i++) {
			if (!reslist[i].empty()) {
				SimpleLookupToTextWin(reslist[i][0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.
				break;
			}
		}
		oMidWin.oIndexWin.oListWin.SetTreeModel(&reslist[0], dictmask);
		oMidWin.oIndexWin.oListWin.ReScroll();
	} else {
		ShowNotFoundToTextWin(sWord, _("There are no dictionary articles containing this word. :-("), TEXT_WIN_FUZZY_NOT_FOUND);
	}
	//t=clock()-t;
	//g_message("Time: %.3lf sec\n", double(t)/CLOCKS_PER_SEC);
	gtk_widget_destroy(search_window);
}

void AppCore::LookupWithFuzzyToMainWin(const gchar *sWord)
{
	if (sWord[0] == '\0')
		return;
	change_cursor busy(gtk_widget_get_window(window),
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	gchar *fuzzy_reslist[MAX_FUZZY_MATCH_ITEM];
	bool Found=
		oLibs.LookupWithFuzzy(sWord, fuzzy_reslist, MAX_FUZZY_MATCH_ITEM, query_dictmask);

	// show
	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	oMidWin.oIndexWin.oListWin.fuzzyWord = sWord;
	oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_FUZZY_LIST;

	if (Found) {
		//SimpleLookupToTextWin(oFuzzystruct[0].pMatchWord,NULL);
		SimpleLookupToTextWin(fuzzy_reslist[0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.

		for (int i=0; i<MAX_FUZZY_MATCH_ITEM && fuzzy_reslist[i]; i++) {
			oMidWin.oIndexWin.oListWin.InsertLast(fuzzy_reslist[i]);
			g_free(fuzzy_reslist[i]);
			//g_print("fuzzy %s,%d\n",oFuzzystruct[i].pMatchWord,oFuzzystruct[i].iMatchWordDistance);
		}
		oMidWin.oIndexWin.oListWin.ReScroll();
	} else {
		ShowNotFoundToTextWin(sWord,_("There are too many spelling errors :-("), TEXT_WIN_FUZZY_NOT_FOUND);
	}
}

void AppCore::LookupWithFuzzyToFloatWin(const gchar *sWord)
{
	if (sWord[0] == '\0')
		return;

	oFloatWin.StartLookup(sWord);
	composite_lookup_float_win.new_lookup();
	gchar *fuzzy_reslist[MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM];
	bool Found = oLibs.LookupWithFuzzy(sWord, fuzzy_reslist, MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM, scan_dictmask);
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
		CurrentIndex *iIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * scan_dictmask.size());

		gchar ***pppWord;
		gchar ****ppppWordData;
		for (i=0;i<count;i++) {
			bool bFound = false;
			pppWord = (gchar ***)g_malloc(sizeof(gchar **) * scan_dictmask.size());
			ppppWordData = (gchar ****)g_malloc(sizeof(gchar ***) * scan_dictmask.size());

			ppOriginWord[i] = fuzzy_reslist[i];
			for (size_t iLib=0; iLib<scan_dictmask.size(); iLib++)
				BuildResultData(scan_dictmask, fuzzy_reslist[i], iIndex, NULL, iLib, pppWord, ppppWordData, bFound, 2);
			for (size_t iLib=0; iLib<scan_dictmask.size(); iLib++)
				BuildVirtualDictData(scan_dictmask, fuzzy_reslist[i], iLib, pppWord, ppppWordData, bFound);
			if (bFound) {// it is certainly be true.
				ppppWord[i]=pppWord;
				pppppWordData[i]=ppppWordData;
			} else {
				FreeResultData(scan_dictmask.size(), pppWord, ppppWordData);
				ppppWord[i]=NULL;
			}
		}
		oFloatWin.AppendTextFuzzy(ppppWord, pppppWordData, ppOriginWord, count, sWord);
		for (i=0; i<count; i++) {
			if (ppppWord[i])
				FreeResultData(scan_dictmask.size(), ppppWord[i], pppppWordData[i]);
		}
		g_free(ppppWord);
		g_free(pppppWordData);
		g_free(ppOriginWord);
		g_free(iIndex);

		for (i=0;i<count;i++)
			g_free(fuzzy_reslist[i]);
	}
	composite_lookup_float_win.done_lookup();
	if(composite_lookup_float_win.is_got_all_responses())
		oFloatWin.EndLookup();
}

void AppCore::LookupWithRuleToMainWin(const gchar *word)
{
	change_cursor busy(gtk_widget_get_window(window),
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	gchar **ppMatchWord = (gchar **)g_malloc(sizeof(gchar *) * (MAX_MATCH_ITEM_PER_LIB*2) * query_dictmask.size()); //Need to be MAX_MATCH_ITEM_PER_LIB*2 as oLibs.LookupWithRule will call LookupWithRule() and LookupWithRuleSynonym().
	gint iMatchCount=oLibs.LookupWithRule(word, ppMatchWord, query_dictmask);
	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_PATTERN_LIST;

	if (iMatchCount) {
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
		ShowNotFoundToTextWin(word,_("Found no words matching this pattern!"), TEXT_WIN_PATTERN_NOT_FOUND);
	}
	g_free(ppMatchWord);
}

void AppCore::LookupWithRegexToMainWin(const gchar *word)
{
	change_cursor busy(gtk_widget_get_window(window),
			   get_impl(oAppSkin.watch_cursor),
			   get_impl(oAppSkin.normal_cursor));

	gchar **ppMatchWord = (gchar **)g_malloc(sizeof(gchar *) * (MAX_MATCH_ITEM_PER_LIB*2) * query_dictmask.size()); //Need to be MAX_MATCH_ITEM_PER_LIB*2 as oLibs.LookupWithRegex will call LookupWithRegex() and LookupWithRegexSynonym().
	gint iMatchCount=oLibs.LookupWithRegex(word, ppMatchWord, query_dictmask);
	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_PATTERN_LIST;

	if (iMatchCount) {
		for (gint i=0; i<iMatchCount; i++)
			oMidWin.oIndexWin.oListWin.InsertLast(ppMatchWord[i]);
		SimpleLookupToTextWin(ppMatchWord[0], iCurrentIndex, NULL); // so iCurrentIndex is refreshed.
		oMidWin.oIndexWin.oListWin.ReScroll();
		for(gint i=0; i<iMatchCount; i++)
			g_free(ppMatchWord[i]);
	} else {
		ShowNotFoundToTextWin(word,_("Found no words matching this regular expression!"), TEXT_WIN_PATTERN_NOT_FOUND);
	}
	g_free(ppMatchWord);
}

void AppCore::LookupNetDict(const char *sWord, bool ismainwin)
{
	std::vector<InstantDictIndex> *dictmask;
	if (ismainwin) {
		dictmask = &query_dictmask;
	} else {
		dictmask = &scan_dictmask;
	}
	for (size_t iLib=0; iLib<dictmask->size(); iLib++) {
		if ((*dictmask)[iLib].type == InstantDictType_NET) {
			const char *dict_cacheid = oStarDictPlugins->NetDictPlugins.dict_cacheid((*dictmask)[iLib].index);
			NetDictResponse *resp = netdict_get_cache_resp(dict_cacheid, sWord);
			if(!ismainwin)
				composite_lookup_float_win.send_net_dict_request(dict_cacheid, sWord);
			if (!resp) {
				oStarDictPlugins->NetDictPlugins.lookup((*dictmask)[iLib].index, sWord, ismainwin);
			} else {
				show_netdict_resp(dict_cacheid, resp, ismainwin);
			}
		}
	}
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
	int bookindex = 0;
	for (size_t i=0; i < query_dictmask.size(); i++) {
		if (pppWord[i]) {
			gchar *mark = g_strdup_printf("%d", bookindex);
			bookindex++;
			if (query_dictmask[i].type == InstantDictType_LOCAL)
				oMidWin.oIndexWin.oResultWin.InsertLast(oLibs.dict_name(query_dictmask[i].index).c_str(), mark);
			else if (query_dictmask[i].type == InstantDictType_VIRTUAL)
				oMidWin.oIndexWin.oResultWin.InsertLast(oStarDictPlugins->VirtualDictPlugins.dict_name(query_dictmask[i].index), mark);
			else if (query_dictmask[i].type == InstantDictType_NET)
				oMidWin.oIndexWin.oResultWin.InsertLast(oStarDictPlugins->NetDictPlugins.dict_name(query_dictmask[i].index), mark);
			g_free(mark);
		}
	}

	oMidWin.oTextWin.readwordtype = oReadWord.canRead(sOriginWord);
	if (oMidWin.oTextWin.readwordtype != READWORD_CANNOT) {
		oMidWin.oTextWin.pronounceWord = sOriginWord;
	}
	else {
		for (size_t i=0;i< query_dictmask.size(); i++) {
			if (pppWord[i] && strcmp(pppWord[i][0], sOriginWord)) {
				oMidWin.oTextWin.readwordtype = oReadWord.canRead(pppWord[i][0]);
				if (oMidWin.oTextWin.readwordtype != READWORD_CANNOT) {
					oMidWin.oTextWin.pronounceWord = pppWord[i][0];
				}
				break;
			}
		}
	}
	gtk_widget_set_sensitive(GTK_WIDGET(oMidWin.oToolWin.PronounceWordMenuButton), (oMidWin.oTextWin.readwordtype != READWORD_CANNOT));
}

void AppCore::ShowTreeDictDataToTextWin(guint32 offset, guint32 size, gint iTreeDict)
{
	oMidWin.oTextWin.ShowTreeDictData(oTreeDicts.poGetWordData(offset, size, iTreeDict));
	oMidWin.oTextWin.query_result = TEXT_WIN_TREEDICT;

	oMidWin.oIndexWin.oResultWin.Clear();
}

void AppCore::ShowNotFoundToTextWin(const char* sWord,const char* sReason, TextWinQueryResult query_result)
{
	bool have_netdict = false;
	for (size_t iLib=0; iLib<query_dictmask.size(); iLib++) {
		if (query_dictmask[iLib].type == InstantDictType_NET) {
			have_netdict = true;
			break;
		}
	}
	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (!enable_netdict && !have_netdict) {
		oMidWin.oTextWin.Show(sReason);
	}
	oMidWin.oTextWin.query_result = query_result;
	oMidWin.oTextWin.queryWord = sWord;

	oMidWin.oIndexWin.oResultWin.Clear();

	oMidWin.oTextWin.readwordtype = oReadWord.canRead(sWord);
	if (oMidWin.oTextWin.readwordtype != READWORD_CANNOT)
		oMidWin.oTextWin.pronounceWord = sWord;
	gtk_widget_set_sensitive(GTK_WIDGET(oMidWin.oToolWin.PronounceWordMenuButton), oMidWin.oTextWin.readwordtype != READWORD_CANNOT);
}

void AppCore::TopWinEnterWord()
{
	const gchar *text = oTopWin.get_text();
	if (text[0]=='\0')
		return;
	oTopWin.select_region_in_text(0, -1);
	oTopWin.InsertHisList(text);
	oTopWin.InsertBackList();
	std::string res;
	switch (analyse_query(text, res)) {
	case qtFUZZY:
		LookupWithFuzzyToMainWin(res.c_str());
		break;
	case qtPATTERN:
		LookupWithRuleToMainWin(res.c_str());
		break;
	case qtREGEX:
		LookupWithRegexToMainWin(res.c_str());
		break;
	case qtFULLTEXT:
		LookupDataToMainWin(res.c_str());
		return;
	default:
		if (!conf->get_bool_at("main_window/search_while_typing")) {
			if (oMidWin.oTextWin.queryWord != res) {
				bool showfirst = conf->get_bool_at("main_window/showfirst_when_notfound");
				bool find = SimpleLookupToTextWin(res.c_str(), iCurrentIndex, NULL, true, !showfirst);
				if (!find && showfirst) {
					const gchar *sug_word = oLibs.GetSuggestWord(res.c_str(), iCurrentIndex, query_dictmask, 0);
					if (sug_word) {
						gchar *suit_word = g_strdup(sug_word);
						SimpleLookupToTextWin(suit_word, iCurrentIndex, NULL, false, true, true);
						g_free(suit_word);
					} else {
						ShowNotFoundToTextWin(res.c_str(), _("<Not Found!>"), TEXT_WIN_NOT_FOUND);
					}
				}
				ListWords(iCurrentIndex);
				break;
			}
		}
		switch (oMidWin.oTextWin.query_result) {
		case TEXT_WIN_NOT_FOUND:
		case TEXT_WIN_SHOW_FIRST:
		case TEXT_WIN_NET_NOT_FOUND:
		case TEXT_WIN_NET_SHOW_FIRST:
			LookupWithFuzzyToMainWin(res.c_str());
			if (conf->get_bool_at("network/enable_netdict")) {
				std::string word = "/";
				word += res;
				STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, word.c_str());
				if (!oStarDictClient.try_cache(c)) {
					waiting_mainwin_lookupcmd_seq = c->seq;
					oStarDictClient.send_commands(1, c);
				}
			}
			return;
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
				return;
			} else {
				SimpleLookupToTextWin(res.c_str(), iCurrentIndex, res.c_str(), false); //text 's index is already cached.
			}
			break;
		}
		case TEXT_WIN_FOUND:
		case TEXT_WIN_NET_FOUND:
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
			} else {
				if (gtk_widget_get_sensitive(GTK_WIDGET(oMidWin.oToolWin.PronounceWordMenuButton)))
					oReadWord.read(oMidWin.oTextWin.pronounceWord.c_str(), oMidWin.oTextWin.readwordtype);
			}
			return;
		default:
			/*nothing*/break;
		}//switch (oMidWin.oTextWin.query_result) {
	}
	if (conf->get_bool_at("network/enable_netdict")) {
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, text);
		if (!oStarDictClient.try_cache(c)) {
			waiting_mainwin_lookupcmd_seq = c->seq;
			oStarDictClient.send_commands(1, c);
		}
	}
	LookupNetDict(text, true);
}

void AppCore::TopWinWordChange(const gchar* sWord)
{
	std::string res;
	switch (analyse_query(sWord, res)) {
	case qtPATTERN:
		oMidWin.oTextWin.Show(_("Press Enter to list the words that match the pattern."));
		break;
	case qtREGEX:
		oMidWin.oTextWin.Show(_("Press Enter to list the words that match this regular expression."));
		break;
	case qtFUZZY:
		if (strlen(sWord)==1)
			oMidWin.oTextWin.Show(_("Fuzzy query..."));
		break;
	case qtFULLTEXT:
		if (strlen(sWord)==1)
			oMidWin.oTextWin.Show(_("Full-text search..."));
		break;
	default:
		stop_word_change_timer();
		delayed_word_ = res;
		int word_change_timeout = conf->get_int_at("main_window/word_change_timeout");
		if(word_change_timeout > 0) {
			word_change_timeout_id = g_timeout_add(word_change_timeout, on_word_change_timeout, this);
		} else {
			//Allow word_change_timeout to be 0, so do an immediate search.
			on_word_change_timeout(this);
		}
	}
}

gboolean AppCore::on_word_change_timeout(gpointer data)
{
	AppCore *app = static_cast<AppCore *>(data);
	bool showfirst = conf->get_bool_at("main_window/showfirst_when_notfound");
	bool find = app->SimpleLookupToTextWin(app->delayed_word_.c_str(),
					       app->iCurrentIndex, NULL, true,
					       !showfirst);
	if (!find && showfirst) {
		const gchar *sug_word = app->oLibs.GetSuggestWord(app->delayed_word_.c_str(), app->iCurrentIndex, app->query_dictmask, 0);
		if (sug_word) {
			gchar *suit_word = g_strdup(sug_word);
			app->SimpleLookupToTextWin(suit_word, app->iCurrentIndex, NULL, false, true, true);
			g_free(suit_word);
		} else {
			app->ShowNotFoundToTextWin(app->delayed_word_.c_str(), _("<Not Found!>"), TEXT_WIN_NOT_FOUND);
		}
	}
	app->ListWords(app->iCurrentIndex);

	bool enable_netdict = conf->get_bool_at("network/enable_netdict");
	if (enable_netdict) {
		STARDICT::Cmd *c = new STARDICT::Cmd(STARDICT::CMD_LOOKUP, app->delayed_word_.c_str());
		if (!app->oStarDictClient.try_cache(c)) {
			app->waiting_mainwin_lookupcmd_seq = c->seq;
			app->oStarDictClient.send_commands(1, c);
		}
	}
	app->LookupNetDict(app->delayed_word_.c_str(), true);

	app->word_change_timeout_id = 0;//next line destroy timer
	return FALSE;
}

void AppCore::ListWords(CurrentIndex* iIndex)
{
	CurrentIndex *iCurrent = (CurrentIndex*)g_memdup(iIndex, sizeof(CurrentIndex)*query_dictmask.size());

	oMidWin.oIndexWin.oListWin.Clear();
	oMidWin.oIndexWin.oListWin.SetModel(true);
	oMidWin.oIndexWin.oListWin.list_word_type = LIST_WIN_NORMAL_LIST;

	int iWordCount=0;
	const gchar * poCurrentWord=oLibs.poGetCurrentWord(iCurrent, query_dictmask, 0);
	if (poCurrentWord) {
		oMidWin.oIndexWin.oListWin.InsertLast(poCurrentWord);
		iWordCount++;

		while (iWordCount<LIST_WIN_ROW_NUM &&
					 (poCurrentWord=oLibs.poGetNextWord(NULL,iCurrent, query_dictmask, 0))) {
			oMidWin.oIndexWin.oListWin.InsertLast(poCurrentWord);
			iWordCount++;
		}
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
	g_free(iCurrent);
}

void AppCore::ListPreWords(const char*sWord)
{
	oMidWin.oIndexWin.oListWin.Clear();
	CurrentIndex *iPreIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * query_dictmask.size());
	const gchar *preword = oLibs.poGetPreWord(sWord, iPreIndex, query_dictmask, 0);
	if (preword) {
		int iWordCount=1;
		oMidWin.oIndexWin.oListWin.Prepend(preword);
		while (iWordCount<15 && (preword=oLibs.poGetPreWord(NULL,iPreIndex, query_dictmask, 0))) {
			oMidWin.oIndexWin.oListWin.Prepend(preword);
			iWordCount++;
		}
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
	g_free(iPreIndex);
}

void AppCore::ListNextWords(const char*sWord)
{
	oMidWin.oIndexWin.oListWin.Clear();
	CurrentIndex *iNextIndex = (CurrentIndex *)g_malloc(sizeof(CurrentIndex) * query_dictmask.size());
	const gchar *nextword = oLibs.poGetNextWord(sWord, iNextIndex, query_dictmask, 0);
	if (nextword) {
		int iWordCount=1;
		oMidWin.oIndexWin.oListWin.InsertLast(nextword);
		while (iWordCount<30 && (nextword = oLibs.poGetNextWord(NULL, iNextIndex, query_dictmask, 0))) {
			oMidWin.oIndexWin.oListWin.InsertLast(nextword);
			iWordCount++;
		}
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
	g_free(iNextIndex);
}

void AppCore::Query(const gchar *word)
{
	oTopWin.InsertHisList(oTopWin.get_text());
	oTopWin.InsertBackList();
	oTopWin.SetText(word);
	std::string res;
	switch (analyse_query(word, res)) {
	case qtSIMPLE:
		break;
	default:
		TopWinEnterWord();
		break;
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
	GtkWindow *parent;
	if (dict_manage_dlg && dict_manage_dlg->window && gtk_widget_get_visible(GTK_WIDGET(dict_manage_dlg->window))) {
		parent = GTK_WINDOW(dict_manage_dlg->window);
	} else if (prefs_dlg && prefs_dlg->window) {
		parent = GTK_WINDOW(prefs_dlg->window);
	} else {
		parent = GTK_WINDOW(window);
	}
    GtkWidget *message_dlg =
        gtk_message_dialog_new(
                parent,
                (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
                "%s", error_msg);
    gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
    gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
    g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
    gtk_widget_show(message_dlg);
}

void AppCore::on_stardict_client_register_end(const char *msg)
{
	if (prefs_dlg) {
		prefs_dlg->on_register_end(msg);
	}
}

void AppCore::on_stardict_client_changepassword_end(const char *msg)
{
	if (prefs_dlg) {
		prefs_dlg->on_changepassword_end(msg);
	}
}

void AppCore::on_stardict_client_getdictmask_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_getdictmask(msg);
}

void AppCore::on_stardict_client_getadinfo_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_getadinfo(msg);
}

void AppCore::on_stardict_client_dirinfo_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_dirinfo(msg);
}

void AppCore::on_stardict_client_dictinfo_end(const char *msg)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_dictinfo(msg);
}

void AppCore::on_stardict_client_maxdictcount_end(int count)
{
	if (dict_manage_dlg)
		dict_manage_dlg->network_maxdictcount(count);
}

void AppCore::on_stardict_client_previous_end(std::list<char *> *wordlist_response)
{
	if (!wordlist_response->empty()) {
		oMidWin.oIndexWin.oListWin.MergeWordList(wordlist_response);
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
}

void AppCore::on_stardict_client_next_end(std::list<char *> *wordlist_response)
{
	if (!wordlist_response->empty()) {
		oMidWin.oIndexWin.oListWin.MergeWordList(wordlist_response);
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
}

void AppCore::on_stardict_client_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq)
{
    if (seq != 0 && waiting_mainwin_lookupcmd_seq != seq)
        return;
    oMidWin.oTextWin.Show(&(lookup_response->dict_response), lookup_response->listtype);
    if (lookup_response->listtype == STARDICT::LookupResponse::ListType_List || lookup_response->listtype == STARDICT::LookupResponse::ListType_Rule_List || lookup_response->listtype == STARDICT::LookupResponse::ListType_Regex_List) {
	if (!lookup_response->wordlist->empty()) {
		oMidWin.oIndexWin.oListWin.MergeWordList(lookup_response->wordlist);
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
    } else if (lookup_response->listtype == STARDICT::LookupResponse::ListType_Fuzzy_List) {
	if (!lookup_response->wordlist->empty()) {
		oMidWin.oIndexWin.oListWin.MergeFuzzyList(lookup_response->wordlist);
		oMidWin.oIndexWin.oListWin.ReScroll();
	}
    } else if (lookup_response->listtype == STARDICT::LookupResponse::ListType_Tree) {
        if (!lookup_response->wordtree->empty()) {
            oMidWin.oIndexWin.oListWin.SetTreeModel(lookup_response->wordtree);
            oMidWin.oIndexWin.oListWin.ReScroll();
        }
    }
}

void AppCore::on_stardict_client_floatwin_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq)
{
	if(seq == 0 || composite_lookup_float_win.got_StarDict_net_responce(seq)) {
		oFloatWin.AppendTextStarDictNet(&(lookup_response->dict_response));
		if(composite_lookup_float_win.is_got_all_responses())
			oFloatWin.EndLookup();
	}
}

void AppCore::on_http_client_error(HttpClient *http_client, const char *error_msg)
{
	if (http_client->callback_func_) {
		http_client->callback_func_(NULL, 0, http_client->userdata);
	} else {
		GtkWindow *parent;
		if (dict_manage_dlg && dict_manage_dlg->window && gtk_widget_get_visible(GTK_WIDGET(dict_manage_dlg->window))) {
			parent = GTK_WINDOW(dict_manage_dlg->window);
		} else if (prefs_dlg && prefs_dlg->window) {
			parent = GTK_WINDOW(prefs_dlg->window);
		} else {
			parent = GTK_WINDOW(window);
		}
		GtkWidget *message_dlg =
			gtk_message_dialog_new(
				parent,
				(GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
				GTK_MESSAGE_INFO,  GTK_BUTTONS_OK,
				"%s", error_msg);
		gtk_dialog_set_default_response(GTK_DIALOG(message_dlg), GTK_RESPONSE_OK);
		gtk_window_set_resizable(GTK_WINDOW(message_dlg), FALSE);
		g_signal_connect_swapped (message_dlg, "response", G_CALLBACK (gtk_widget_destroy), message_dlg);
		gtk_widget_show(message_dlg);
	}
	oHttpManager.Remove(http_client);
}

void AppCore::on_http_client_response(HttpClient *http_client)
{
	if (http_client->callback_func_) {
		http_client->callback_func_(http_client->buffer, http_client->buffer_len, http_client->userdata);
		oHttpManager.Remove(http_client);
		return;
	}
	oHttpManager.Remove(http_client);
}

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
		progress_win pw(GTK_WINDOW(gpAppFrame->window));
		reload_show_progress_t rsp(pw);
		show_progress_t *old_progress = oLibs.get_show_progress();
		oLibs.set_show_progress(&rsp);
		reload_dicts();
		oLibs.set_show_progress(old_progress);
	}
}

void AppCore::reload_dicts()
{
	std::list<DictItemId> load_list;
	GetUsedDictList(load_list);
	std::list<std::string> s_load_list;
	DictItemId::convert(s_load_list, load_list);
	oLibs.reload(s_load_list,
		conf->get_bool_at("dictionary/enable_collation") ? CollationLevel_SINGLE : CollationLevel_NONE,
		int_to_colate_func(conf->get_int_at("dictionary/collate_function")));
	UpdateDictMask();

	const gchar *sWord = oTopWin.get_text();

	if (sWord && sWord[0])
		TopWinWordChange(sWord);
}

void AppCore::PopupDictManageDlg()
{

	if (!dict_manage_dlg) {
		dict_manage_dlg = new DictManageDlg(GTK_WINDOW(window), get_impl(oAppSkin.index_wazard), get_impl(oAppSkin.index_appendix));
		bool dictmanage_config_changed;
		bool exiting = dict_manage_dlg->ShowModal(dictmanage_config_changed);
		delete dict_manage_dlg;
		dict_manage_dlg = NULL;
		if (exiting)
			return;
		if (dictmanage_config_changed) {
			progress_win pw(GTK_WINDOW(gpAppFrame->window));
			reload_show_progress_t rsp(pw);
			show_progress_t *old_progress = oLibs.get_show_progress();
			oLibs.set_show_progress(&rsp);
			reload_dicts();
			oLibs.set_show_progress(old_progress);
		}
	}
}

void AppCore::PopupPluginManageDlg()
{
	if (!plugin_manage_dlg) {
		plugin_manage_dlg = new PluginManageDlg();
		bool dict_changed;
		bool order_changed;
		bool exiting = plugin_manage_dlg->ShowModal(GTK_WINDOW(window), dict_changed, order_changed);
		delete plugin_manage_dlg;
		plugin_manage_dlg = NULL;
		if (exiting)
			return;
		if (order_changed) {
#ifdef _WIN32
			std::list<std::string> plugin_order_list;
			{
				const std::list<std::string>& plugin_order_list_rel
					= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
				abs_path_to_data_dir(plugin_order_list_rel, plugin_order_list);
			}
#else
			const std::list<std::string>& plugin_order_list
				= conf->get_strlist("/apps/stardict/manage_plugins/plugin_order_list");
#endif
			oStarDictPlugins->VirtualDictPlugins.reorder(plugin_order_list);
			oStarDictPlugins->NetDictPlugins.reorder(plugin_order_list);
			oMidWin.oToolWin.UpdatePronounceMenu();
		}
		if (dict_changed) {
			UpdateDictMask();

			const gchar *sWord = oTopWin.get_text();
			if (sWord && sWord[0])
				TopWinWordChange(sWord);
		}
	}
}

void AppCore::stop_word_change_timer()
{
	if (word_change_timeout_id) {
		g_source_remove(word_change_timeout_id);
		word_change_timeout_id = 0;
	}
}

void AppCore::End()
{
	stop_word_change_timer();
	oSelection.End();
#ifdef _WIN32
	oClipboard.End();
	oMouseover.End();
#endif
#ifdef ENABLE_LOG_WINDOW
	gLogWindow.End();
#endif
	oHotkey.End();
	oFloatWin.End();

	oDockLet.reset(0);

	if (dict_manage_dlg)
		dict_manage_dlg->Close();
	if (prefs_dlg)
		prefs_dlg->Close(); // After user open the preferences dialog, then choose quit in the notification icon, this dialog can be closed.
	oTopWin.Destroy();
	oMidWin.oIndexWin.oListWin.Destroy();
	oBottomWin.Destroy();

	gtk_widget_destroy(window);
}

void AppCore::Init(const gchar *queryword)
{
	conf->notify_add("/apps/stardict/preferences/main_window/hide_list",
			 sigc::mem_fun(this, &AppCore::on_main_win_hide_list_changed));
	conf->notify_add("/apps/stardict/preferences/main_window/keep_above",
			 sigc::mem_fun(this, &AppCore::on_main_win_keep_above_changed));
	conf->notify_add("/apps/stardict/preferences/dictionary/scan_selection",
			 sigc::mem_fun(this, &AppCore::on_dict_scan_select_changed));
	conf->notify_add("/apps/stardict/preferences/dictionary/scan_modifier_key",
			 sigc::mem_fun(this, &AppCore::on_scan_modifier_key_changed));

	g_debug(_("Loading skin..."));
#ifdef _WIN32
	oAppSkin.load(abs_path_to_data_dir(conf->get_string_at("main_window/skin")));
#else
	oAppSkin.load(conf->get_string_at("main_window/skin"));
#endif
	g_debug(_("Skin loaded."));

	if (!CmdLineOptions::get_hide())
		stardict_splash.show();

	Create(queryword);

#ifdef CONFIG_GNOME
  stardict_app_server =
    stardict_application_server_new(NULL);
#endif

	stardict_splash.on_mainwin_finish();
	oStarDictPlugins->MiscPlugins.on_mainwin_finish();
#ifdef CONFIG_GNOME
	gtk_main();
#endif
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

	End();

#ifdef CONFIG_GNOME
	bonobo_object_unref (stardict_app_server);
#endif
	unlock_keys.reset(0);
	conf.reset(0);
#ifdef CONFIG_GNOME
	gtk_main_quit();
#endif
}

void AppCore::on_main_win_hide_list_changed(const baseconfval* hideval)
{
	bool hide = static_cast<const confval<bool> *>(hideval)->val_;

	if (hide) {
		gtk_widget_hide(oMidWin.oToolWin.HideListButton);
		gtk_widget_show(oMidWin.oToolWin.ShowListButton);
		gtk_widget_hide(oMidWin.oLeftWin.vbox);
		gtk_widget_hide(oMidWin.oIndexWin.notebook);
	} else {
		gtk_widget_hide(oMidWin.oToolWin.ShowListButton);
		gtk_widget_show(oMidWin.oToolWin.HideListButton);
		gtk_widget_show(oMidWin.oLeftWin.vbox);
		gtk_widget_show(oMidWin.oIndexWin.notebook);
	}
}

void AppCore::on_main_win_keep_above_changed(const baseconfval* hideval)
{
	bool keepabove = static_cast<const confval<bool> *>(hideval)->val_;

	gtk_window_set_keep_above(GTK_WINDOW(window), keepabove);
}

void AppCore::on_dict_scan_select_changed(const baseconfval* scanval)
{
	bool scan = static_cast<const confval<bool> *>(scanval)->val_;

	if (scan != static_cast<bool>(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(oBottomWin.ScanSelectionCheckButton))))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(oBottomWin.ScanSelectionCheckButton), scan);

	oDockLet->set_scan_mode(scan);
	if (gtk_widget_get_visible(GTK_WIDGET(window)))
		oDockLet->hide_state();
	if (scan) {
		bool lock=conf->get_bool_at("floating_window/lock");
		if (lock && !oFloatWin.getQueryingWord().empty())
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
stardict_handle_automation_cmdline (const gchar *queryword)
{
	CORBA_Environment env;
	GNOME_Stardict_Application server;

	CORBA_exception_init (&env);
	CORBA_char id[] = "OAFIID:GNOME_Stardict_Application";

	server = bonobo_activation_activate_from_id (id, 0, NULL, &env);
	if (!server) {
		gdk_notify_startup_complete ();
		return;
	}
	//g_return_if_fail (server != NULL);

	if (CmdLineOptions::get_quit()) {
		GNOME_Stardict_Application_quit (server, &env);
	}
	else {
		if (queryword) {
			GNOME_Stardict_Application_queryWord (server, queryword, &env);
		}
		if (CmdLineOptions::get_hide()) {
			GNOME_Stardict_Application_hide (server, &env);
		} else {
			GNOME_Stardict_Application_grabFocus (server, &env);
			g_message(_("StarDict is already running. Using the running process."));
		}
	}


	bonobo_object_release_unref (server, &env);
	CORBA_exception_free (&env);


	/* we never popup a window, so tell startup-notification that
	 * we're done */
	gdk_notify_startup_complete ();
}

/*
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
		if (!gtk_widget_get_visible(GTK_WIDGET(gpAppFrame->window)))
			argv[argc++] = (gchar *)"-h";
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
*/
#endif

#if defined(_WIN32) && defined(_MSC_VER)
/* Synchronize environment variables in CRTs.
See section "Two copies of CRT" in doc/README_windows.txt for more details. */
void synchronize_crt_enviroment(void)
{
	const char* varname = "LANG";
	size_t size;
	if(getenv_s(&size, NULL, 0, varname)) {
		g_warning("Unable to get the value of the %s environment variable", varname);
		return;
	}
	if(size == 0)
		return; // variable is not found
	std::vector<char> buf(size);
	if(getenv_s(&size, &buf[0], size, varname)) {
		g_warning("Unable to get the value of the %s environment variable", varname);
		return;
	}
	if(!g_setenv(varname, &buf[0], TRUE)) {
		g_warning("Unable to set the %s environment variable.", varname);
		return;
	}
}
#endif

#if defined(CONFIG_GNOME)
#else
static void activateMe(GtkApplication *app, const char *query_word) {
    GtkWidget *app_window = gtk_application_window_new(app);
    gpAppFrame->window = app_window;
    gpAppFrame->Init(query_word);
}
#endif

#ifdef _WIN32
DLLIMPORT int stardict_main(HINSTANCE hInstance, int argc, char **argv)
#else
int main(int argc,char **argv)
#endif
{
#if defined(_WIN32)
	stardictexe_hInstance = hInstance;
#endif // #if defined(_WIN32)
	CmdLineOptions::pre_parse_arguments(argc, argv);
	const char* const dirs_config_file = CmdLineOptions::get_dirs_config_pre();
	conf_dirs.reset(new AppDirs(dirs_config_file ? dirs_config_file : ""));
	app_dirs = conf_dirs.get();
	bindtextdomain (GETTEXT_PACKAGE, conf_dirs->get_locale_dir().c_str());
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#if defined(_WIN32) && defined(_MSC_VER)
	synchronize_crt_enviroment();
#endif
#if defined(CONFIG_GNOME)
	gtk_init(&argc, &argv);
#endif
	/* Register an interim logger.
	On Windows, without the logger all output produced by g_option_context_parse will be lost.
	For example, g_option_context_parse prints usage message when stardict is invoked with --help option.
	The problem is g_option_context_parse uses g_print function to print messages, 
	but this function produce no output on windows console. See test_windows_console
	function to check what works with windows console.
	*/
	logger.reset(new Logger(MessageLevel_MESSAGE, MessageLevel_NONE));
#if defined(_WIN32) && defined(_DEBUG)
	//test_windows_console();
#endif
#ifdef CONFIG_GPE
	if (gpe_application_init (&argc, &argv) == FALSE)
		exit (1);
#endif
	GOptionContext *context;
	context = g_option_context_new(_("- Lookup words"));
	g_option_context_add_main_entries(context, CmdLineOptions::get_options(), GETTEXT_PACKAGE);
#ifndef CONFIG_GNOME
	glib::Error err;
	if (!g_option_context_parse(context, &argc, &argv, get_addr(err))) {
		g_warning(_("Options parsing failed: %s\n"), err->message);
		g_option_context_free(context);
		return EXIT_FAILURE;
	}
	g_option_context_free(context);
#else // #ifndef CONFIG_GNOME
	//GnomeProgram *program;
	gnome_program_init ("stardict", VERSION,
			    LIBGNOME_MODULE, argc, argv,
			    GNOME_PARAM_GOPTION_CONTEXT, context,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
			    _("Dictionary"),
			    GNOME_PARAM_APP_DATADIR, conf_dirs->get_system_data_dir().c_str(),
			    NULL);
	bonobo_init(&argc, argv);
#endif // #ifndef CONFIG_GNOME

	const char *query_word = NULL;
	if(CmdLineOptions::get_query_words())
		query_word = CmdLineOptions::get_query_words()[0];

	logger->set_console_message_level(CmdLineOptions::get_console_message_level());
	logger->set_log_message_level(CmdLineOptions::get_log_message_level());

#ifndef CONFIG_GNOME
#ifdef _WIN32
	if (CmdLineOptions::get_newinstance() == FALSE) {
		gchar *title=g_locale_from_utf8(_("StarDict"), -1, NULL, NULL, NULL);
		HWND ll_winhandle = FindWindowA("gdkWindowToplevel", title);
		g_free(title);
		if (ll_winhandle > 0) {
			if (IsIconic(ll_winhandle))
				ShowWindow(ll_winhandle,SW_RESTORE);
			else
				SetForegroundWindow(ll_winhandle);
			g_message("Stardict is already running.");
			return EXIT_SUCCESS;
		}
	}
#endif // #ifdef _WIN32
#else // #ifndef CONFIG_GNOME
	if (CmdLineOptions::get_newinstance() == FALSE) {
		CORBA_Object factory;
		CORBA_char id[] = "OAFIID:GNOME_Stardict_Factory";
		factory = bonobo_activation_activate_from_id(id,
			Bonobo_ACTIVATION_FLAG_EXISTING_ONLY,
			NULL, NULL);

		if (factory != NULL) {
			/* there is an instance already running, so send
			 * commands to it if needed
			 */
			stardict_handle_automation_cmdline (query_word);
			/* and we're done */
			return EXIT_SUCCESS;
		}
	}

	/*
	GnomeClient *client;
	if ((client = gnome_master_client()) != NULL) {
		g_signal_connect (client, "save_yourself", G_CALLBACK (save_yourself_cb), (gpointer) argv[0]);
		g_signal_connect (client, "die", G_CALLBACK (client_die_cb), NULL);
	}
	*/
#endif // #ifndef CONFIG_GNOME
	g_debug(_("Loading StarDict configuration..."));
	conf.reset(new AppConf);
	g_debug(_("StarDict configuration loaded."));
	AppCore oAppCore;
	gpAppFrame = &oAppCore;
#if defined(CONFIG_GNOME)
	oAppCore.Init(query_word);
	return EXIT_SUCCESS;
#else
    GtkApplication *app;
    app = gtk_application_new("com.app.stardict", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(activateMe), (gpointer) query_word);
    int status;
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
#endif
}
