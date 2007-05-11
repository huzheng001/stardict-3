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
#include "lib/lib.h"
#include "lib/stardict_client.hpp"
#include "lib/pluginmanager.h"
#include "lib/httpmanager.h"
#include "skin.h"
#include "mainwin.h"
#ifdef _WIN32
#  include "win32/clipboard.h"
#  include "win32/mouseover.h"
#  include "win32/hotkey.h"
#endif

#include "tray.hpp"
#include "floatwin.h"
#include "selection.h"
#include "dictmanagedlg.h"
#include "prefsdlg.h"
#include "readword.h"
#include "iskeyspressed.hpp"

extern AppCore *gpAppFrame;

//notice!!! when you change these DEFAULT const value,remember that you'd better change data/stardict.schemas.in too!

const int MAX_FUZZY_MATCH_ITEM=100;
const int MAX_FLOAT_WINDOW_FUZZY_MATCH_ITEM=5;

const int LIST_WIN_ROW_NUM = 30; //how many words show in the list win.


class AppCore : public sigc::trackable {
private:
	DictManageDlg *dict_manage_dlg;
	PrefsDlg *prefs_dlg;
	guint word_change_timeout_;
	std::string delayed_word_;

	static int MatchWordCompare(const void * s1, const void * s2);
	static gboolean on_delete_event(GtkWidget * window, GdkEvent *event , AppCore *oAppCore);
	static gboolean on_window_state_event(GtkWidget * window, GdkEventWindowState *event , AppCore *oAppCore);
	static gboolean vKeyPressReleaseCallback(GtkWidget * window, GdkEventKey *event , AppCore *oAppCore);
	void reload_dicts();
	void on_main_win_hide_list_changed(const baseconfval*);
	void on_dict_scan_select_changed(const baseconfval*);
	void on_floatwin_lock_changed(const baseconfval*);
	void on_floatwin_lock_x_changed(const baseconfval*);
	void on_floatwin_lock_y_changed(const baseconfval*);
	void on_scan_modifier_key_changed(const baseconfval*);
	static gboolean on_word_change_timeout(gpointer data);
	void stop_word_change_timer();
	void on_change_scan(bool val);
	void on_maximize();
	void on_middle_button_click();
public:
	CurrentIndex *iCurrentIndex;
	unsigned int waiting_mainwin_lookupcmd_seq;
	unsigned int waiting_floatwin_lookupcmd_seq;
	GtkWidget *window;
	GtkTooltips *tooltips;

	TopWin oTopWin;
	MidWin oMidWin;
	BottomWin oBottomWin;

	Selection oSelection;
#ifdef _WIN32
	Clipboard oClipboard;
	Mouseover oMouseover;
	Hotkey oHotkey;
#endif
	FloatWin oFloatWin;
	std::auto_ptr<TrayBase> oDockLet;

	Libs oLibs;
	TreeDicts oTreeDicts;
	StarDictClient oStarDictClient;
	StarDictPlugins *oStarDictPlugins;
	HttpManager oHttpManager;
	std::auto_ptr<hotkeys> unlock_keys;
	AppSkin oAppSkin;
	ReadWord oReadWord;

#ifdef CONFIG_GNOME
	BonoboObject *stardict_app_server;
#endif

	std::vector<InstantDictIndex> dictmask;

	AppCore();
	~AppCore();
	void Init(gchar *queryword);
	void Quit();
	void Create(gchar *queryword);
	void End();
	void Query(const gchar *word);
	void BuildResultData(const char* sWord, CurrentIndex *iIndex, const gchar *piIndexValidStr, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound, gint Method);
	void BuildVirtualDictData(const char* sWord, int iLib, gchar ***pppWord, gchar ****ppppWordData, bool &bFound);
	void FreeResultData(gchar ***pppWord, gchar ****ppppWordData);
	bool SimpleLookupToFloat(const gchar* sWord, bool bShowIfNotFound);
#ifdef _WIN32
	bool SmartLookupToFloat(const gchar* sWord, int BeginPos, bool bShowIfNotFound);
#endif
	bool SimpleLookupToTextWin(const gchar* sWord, CurrentIndex* piIndex, const gchar *piIndexValidStr = NULL, bool bTryMoreIfNotFound = false, bool bShowNotfound = true, bool isShowFirst = false);
	void LookupDataToMainWin(const gchar *sWord);
	void LookupWithFuzzyToMainWin(const gchar* word);
	void LookupWithFuzzyToFloatWin(const gchar * word);
	void LookupWithRuleToMainWin(const gchar* word);
	void ShowDataToTextWin(gchar ***pppWord, gchar ****ppppWordData,const gchar * sOriginWord, bool isShowFirst);
	void ShowTreeDictDataToTextWin(guint32 offset, guint32 size, gint iTreeDict);
	void ShowNotFoundToTextWin(const char* sWord,const char* sReason, TextWinQueryResult query_result);
	void ShowNotFoundToFloatWin(const char* sWord,const char* sReason, gboolean fuzzy);

	void TopWinEnterWord(const gchar *text);
	void TopWinWordChange(const gchar* sWord);
	void ListWords(const gchar *sWord, CurrentIndex* iStartIndex, bool showfirst);
	
	void ListClick(const gchar *word);
	void PopupPrefsDlg();
	void PopupDictManageDlg();
	void on_link_click(const std::string &link);

	void on_stardict_client_error(const char *);
	void on_stardict_client_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq);
	void on_stardict_client_floatwin_lookup_end(const struct STARDICT::LookupResponse *lookup_response, unsigned int seq);
	void on_stardict_client_register_end(const char *);
	void on_stardict_client_getdictmask_end(const char *);
	void on_stardict_client_dirinfo_end(const char *);
	void on_stardict_client_dictinfo_end(const char *);
	void on_stardict_client_maxdictcount_end(int);
	void on_stardict_client_previous_end(std::list<char *> *wordlist_response);
	void on_stardict_client_next_end(std::list<char *> *wordlist_response);

	void on_http_client_error(HttpClient*, const char *);
	void on_http_client_response(HttpClient*);

	static void on_stardict_virtual_dict_plugin_lookup_end(const struct VirtualDictLookupResponse *);
};

#ifdef _WIN32
extern HINSTANCE stardictexe_hInstance;
#endif

extern gchar* GetPureEnglishAlpha(gchar *str);
extern gchar* GetHeadWord(gchar *str);
extern gboolean stardict_on_enter_notify (GtkWidget * widget, GdkEventCrossing * event, gpointer data);

#endif
