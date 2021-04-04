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

#ifndef __STAR_DICT_H__
#define __STAR_DICT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#ifdef CONFIG_GNOME
# include <bonobo/bonobo-object.h>
# include <libgnome/libgnome.h>
#endif


class AppCore;

#include "conf.h"
#include "lib/utils.h"
#include "lib/stardict_client.h"
#include "lib/pluginmanager.h"
#include "lib/httpmanager.h"
#include "skin.h"
#include "mainwin.h"
#ifdef _WIN32
#  include "win32/clipboard.h"
#  include "win32/mouseover.h"
#endif

#include "tray.h"
#include "floatwin.h"
#include "selection.h"
#include "readword.h"
#include "iskeyspressed.h"
#include "dictmanage.h"
#include "globalhotkeys.h"
#include "lib/compositelookup.h"
#include "lib/full_text_trans.h"
#include "lib/stddict.h"
#include "lib/treedict.h"

extern AppCore *gpAppFrame;

//notice!!! when you change these DEFAULT const value,remember that you'd better change data/stardict.schemas.in too!

const int MAX_FUZZY_MATCH_ITEM=100;
const int MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM=5;

const int LIST_WIN_ROW_NUM = 30; //how many words show in the list win.

class DictManageDlg;
class PluginManageDlg;
class PrefsDlg;

class AppCore final : public sigc::trackable {
private:
	DictManageDlg *dict_manage_dlg;
	PluginManageDlg *plugin_manage_dlg;
	PrefsDlg *prefs_dlg;
	guint word_change_timeout_id;
	std::string delayed_word_;
	CompositeLookup composite_lookup_float_win;
	bool have_queryword;

	static int MatchWordCompare(const void * s1, const void * s2);
	static void on_mainwin_show_event(GtkWidget * window, AppCore *app);
	static gboolean on_delete_event(GtkWidget * window, GdkEvent *event , AppCore *oAppCore);
	static gboolean on_window_state_event(GtkWidget * window, GdkEventWindowState *event , AppCore *oAppCore);
	static gboolean vKeyPressReleaseCallback(GtkWidget * window, GdkEventKey *event , AppCore *oAppCore);
	void reload_dicts();
	void on_main_win_hide_list_changed(const baseconfval*);
	void on_main_win_keep_above_changed(const baseconfval*);
	void on_dict_scan_select_changed(const baseconfval*);
	void on_scan_modifier_key_changed(const baseconfval*);
	static gboolean on_word_change_timeout(gpointer data);
	void stop_word_change_timer();
	void on_change_scan(bool val);
	void on_maximize();
	void on_docklet_middle_button_click();
	bool SimpleLookupToFloatLocal(const gchar* sWord);
#ifdef _WIN32
	bool LocalSmartLookupToFloat(const gchar* sWord, int BeginPos);
#endif
public:
	CurrentIndex *iCurrentIndex;
	unsigned int waiting_mainwin_lookupcmd_seq;
	/* last directory selected in save/open file and similar dialogs */
	std::string last_selected_directory;
	GtkWidget *window;

	TopWin oTopWin;
	MidWin oMidWin;
	BottomWin oBottomWin;

	Selection oSelection;
#ifdef _WIN32
	Clipboard oClipboard;
	Mouseover oMouseover;
#endif
	GlobalHotkeys oHotkey;
	FloatWin oFloatWin;
	std::unique_ptr<TrayBase> oDockLet;

	Libs oLibs;
	TreeDicts oTreeDicts;
	StarDictClient oStarDictClient;
	StarDictPlugins *oStarDictPlugins;
	HttpManager oHttpManager;
	std::unique_ptr<hotkeys> unlock_keys;
	AppSkin oAppSkin;
	ReadWord oReadWord;
	FullTextTrans oFullTextTrans;

#ifdef CONFIG_GNOME
	BonoboObject *stardict_app_server;
#endif

	DictManageInfo dictinfo;
	std::vector<InstantDictIndex> query_dictmask;
	std::vector<InstantDictIndex> scan_dictmask;

	AppCore();
	~AppCore();
	void Init(const gchar *queryword);
	void Quit();
	void Create(const gchar *queryword);
	void End();
	void Query(const gchar *word);
	void BuildResultData(std::vector<InstantDictIndex> &dictmask, const char* sWord, CurrentIndex *iIndex, const gchar *piIndexValidStr, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound, gint Method);
	void BuildVirtualDictData(std::vector<InstantDictIndex> &dictmask, const char* sWord, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound);
	static void FreeResultData(size_t dictmask_size, gchar ***pppWord, gchar ****ppppWordData);
	void SimpleLookupToFloat(const char* sToken, bool IgnoreScanModifierKey = false);
#ifdef _WIN32
	void SmartLookupToFloat(const gchar* sWord, int BeginPos, bool IgnoreScanModifierKey);
#endif
	bool SimpleLookupToTextWin(const gchar* sWord, CurrentIndex* piIndex, const gchar *piIndexValidStr = NULL, bool bTryMoreIfNotFound = false, bool bShowNotfound = true, bool isShowFirst = false);
	void LookupDataToMainWin(const gchar *sWord);
	void LookupDataWithDictMask(const gchar *sWord, std::vector<InstantDictIndex> &dictmask);
	void LookupWithFuzzyToMainWin(const gchar* word);
	void LookupWithFuzzyToFloatWin(const gchar * word);
	void LookupWithRuleToMainWin(const gchar* word);
	void LookupWithRegexToMainWin(const gchar* word);
	void LookupNetDict(const char *word, bool ismainwin);
	void ShowDataToTextWin(gchar ***pppWord, gchar ****ppppWordData,const gchar * sOriginWord, bool isShowFirst);
	void ShowTreeDictDataToTextWin(guint32 offset, guint32 size, gint iTreeDict);
	void ShowNotFoundToTextWin(const char* sWord,const char* sReason, TextWinQueryResult query_result);

	void TopWinEnterWord();
	void TopWinWordChange(const gchar* sWord);
	void ListWords(CurrentIndex* iStartIndex);
	void ListPreWords(const char*sWord);
	void ListNextWords(const char*sWord);

	void ListClick(const gchar *word);
	void PopupPrefsDlg();
	void PopupDictManageDlg();
	void PopupPluginManageDlg();
	void on_link_click(const std::string &link);

	void on_stardict_client_error(const char *);
	void on_stardict_client_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq);
	void on_stardict_client_floatwin_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq);
	void on_stardict_client_register_end(const char *);
	void on_stardict_client_changepassword_end(const char *);
	void on_stardict_client_getdictmask_end(const char *);
	void on_stardict_client_getadinfo_end(const char *);
	void on_stardict_client_dirinfo_end(const char *);
	void on_stardict_client_dictinfo_end(const char *);
	void on_stardict_client_maxdictcount_end(int);
	void on_stardict_client_previous_end(std::list<char *> *wordlist_response);
	void on_stardict_client_next_end(std::list<char *> *wordlist_response);

	void on_http_client_error(HttpClient*, const char *);
	void on_http_client_response(HttpClient*);
	static void do_send_http_request(const char* shost, const char* sfile, get_http_response_func_t callback_func, gpointer userdata);
	static void set_news(const char *news, const char *links);
	static void show_netdict_resp(const char *dict, NetDictResponse *resp, bool ismainwin);
	static void lookup_dict(size_t dictid, const char *sWord, char ****Word, char *****WordData);
	static void ShowPangoTips(const char *word, const char *text);
};

extern gchar* GetPureEnglishAlpha(gchar *str);
extern gchar* GetHeadWord(gchar *str);
extern gboolean stardict_on_enter_notify (GtkWidget * widget, GdkEventCrossing * event, gpointer data);

#ifdef _WIN32
#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

extern "C" {
	DLLIMPORT extern int stardict_main(HINSTANCE hInstance, int argc, char **argv);
}
#endif

#endif
