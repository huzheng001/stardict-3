#ifndef __SD_MAINWIN_H__
#define __SD_MAINWIN_H__

#include <gtk/gtk.h>
#include <string>

#include "articleview.h"
#include "readword.h"

const guint MAX_HISTORY_WORD_ITEM_NUM=20;
const guint MAX_BACK_WORD_ITEM_NUM=20;


const int MIN_WINDOW_WIDTH=200;
const int MIN_WINDOW_HEIGHT=100;

enum TextWinQueryResult
{
	TEXT_WIN_FOUND,
	TEXT_WIN_NOT_FOUND,
	TEXT_WIN_SHOW_FIRST,
	TEXT_WIN_FUZZY_NOT_FOUND,
	TEXT_WIN_PATTERN_NOT_FOUND,
	TEXT_WIN_TIPS, // when the word entry is empty,it will show tip
	TEXT_WIN_INFO,
	TEXT_WIN_TREEDICT,
};

enum ListWinListWordType {
	LIST_WIN_NORMAL_LIST,
	LIST_WIN_FUZZY_LIST,
	LIST_WIN_DATA_LIST,
	LIST_WIN_PATTERN_LIST,
	LIST_WIN_EMPTY
};

struct BackListData {
	gchar *word;
	gdouble adjustment_value;
};

class TopWin {
private:
	gboolean enable_change_cb;
	GList *HisList;
	GSList *BackList;
	GtkWidget *HistoryMenu;
	GtkWidget *BackMenu;
	GtkWidget* WordCombo;

	static gint HisCompareFunc(gconstpointer a,gconstpointer b);
	static gint BackListDataCompareFunc(gconstpointer a,gconstpointer b);

	static void ClearCallback(GtkWidget *widget, TopWin *oTopWin);
	static void GoCallback(GtkWidget *widget, TopWin *oTopWin);
	static void BackCallback(GtkWidget *widget, TopWin *oTopWin);
	static void MenuCallback(GtkWidget *widget, TopWin *oTopWin);
	static gboolean on_back_button_press(GtkWidget * widget, GdkEventButton * event , TopWin *oTopWin);
	static void on_back_menu_item_activate(GtkMenuItem *menuitem, gint index);

	static void on_entry_changed(GtkEntry *entry, TopWin *oTopWin);
	static void on_entry_activate(GtkEntry *entry, TopWin *oTopWin);
	
	static void on_main_menu_preferences_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_dictmanage_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_newversion_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_help_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_about_activate(GtkMenuItem *menuitem, TopWin *oTopWin);
	static void on_main_menu_quit_activate(GtkMenuItem *menuitem, TopWin *oTopWin);

	void LoadHistory(void);
	void SaveHistory(void);
public:
	GtkWidget* MainMenu;

	TopWin();
	~TopWin();

	void Create(GtkWidget *vbox);
	void Destroy(void);
	void SetText(const gchar *word, bool notify=true);
	//void SetText_without_notify(const gchar *word);
	void TextSelectAll();	
	void InsertHisList(const gchar *word);
	void InsertBackList(const gchar *word = NULL);
	void do_back();
	void do_prev();
	void do_next();
	void do_menu();

	gboolean TextSelected();
	bool has_focus() {
		return GTK_WIDGET_HAS_FOCUS(GTK_COMBO(WordCombo)->entry);
	}
	static void ClipboardReceivedCallback(GtkClipboard *clipboard, const gchar *text, gpointer data);

	void set_position_in_text(gint pos) {
		gtk_editable_set_position(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry), pos);
	}
	void select_region_in_text(gint beg, gint end) {
		gtk_editable_select_region(GTK_EDITABLE(GTK_COMBO(WordCombo)->entry), beg, end);
	}
	const gchar *get_text() {
		if (!WordCombo)
			return "";
		return gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(WordCombo)->entry));
	}
	void grab_focus() {
		gtk_widget_grab_focus(GTK_COMBO(WordCombo)->entry);
	}
};

class ListWin {
private:
	GtkListStore *list_model;
	GtkTreeStore *tree_model;
	bool nowIsList;

	static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, ListWin *oListWin);
	static void on_selection_changed(GtkTreeSelection *selection, ListWin *oListWin);

public:
	ListWinListWordType list_word_type;
	GtkTreeView *treeview_;

	ListWin() {}
	void Create(GtkWidget *notebook);
	void SetModel(bool isListModel);
	void Destroy();
	void Clear();
	void ReScroll();
    void Prepend(const gchar *word);
	void InsertLast(const gchar *word);
	void SetTreeModel(std::vector<gchar *> *reslist);
	void SetTreeModel(std::list<STARDICT::LookupResponse::WordTreeElement *> *wordtree);
	bool treeview_has_focus() const {
		return GTK_WIDGET_HAS_FOCUS(GTK_WIDGET(treeview_));
	}
};

class TreeWin {
public:
	GtkWidget *treeview;

	TreeWin() {}
	bool Create(GtkWidget *notebook);
private:
	static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, TreeWin *oTreeWin);
	static void on_selection_changed(GtkTreeSelection *selection, TreeWin *oTreeWin);
};

class ResultWin {
public:
	GtkWidget *treeview;
	void Create(GtkWidget *notebook);
	void InsertLast(const gchar *word, const gchar *mark);
	void Clear();
private:
	static void on_selection_changed(GtkTreeSelection *selection, ResultWin *oResultWin);
};

class IndexWin {
public:
	GtkWidget *notebook;

	ListWin oListWin;
	ResultWin oResultWin;
	TreeWin oTreeWin;

	IndexWin();
	bool Create(GtkWidget *hpaned);
private:
};

class LeftWin {
public:
	GtkWidget *vbox;
	LeftWin();
	void Create(GtkWidget *hbox, bool has_treedict);
private:
	static void on_wazard_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin);
	static void on_appendix_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin);
	static void on_result_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin);
	static void on_translate_button_toggled(GtkToggleButton *button, LeftWin *oLeftWin);
	static void PreviousCallback(GtkWidget *widget, LeftWin *oLeftWin);
	static void NextCallback(GtkWidget *widget, LeftWin *oLeftWin);
};

class ToolWin {
private:

	static void ShowListCallback(GtkWidget *widget, gpointer data);
	static void HideListCallback(GtkWidget *widget, gpointer data);
	static void CopyCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void PlayCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void SaveCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void PrintCallback(GtkWidget *widget, ToolWin *oToolWin);
	static void SearchCallback(GtkWidget *widget, ToolWin *oToolWin);
public:
	GtkWidget* ShowListButton;
	GtkWidget* HideListButton;
	GtkWidget* PronounceWordButton;

	ToolWin() {}
	~ToolWin() {}
	void Create(GtkWidget *vbox);
	void do_search();
	void do_save();
};

class TextWin : public sigc::trackable {
public:
  std::string queryWord;
  std::string pronounceWord;
  ReadWordType readwordtype;
  TextWinQueryResult query_result;
  std::auto_ptr<ArticleView> view;
	gboolean search_from_beginning;
	std::string find_text;
	GtkEntry *eSearch;

  TextWin() {}
  ~TextWin() {}

  void Create(GtkWidget *vbox);
  void ShowTips();
  void ShowInfo();
  void ShowInitFailed();
  void Show(const gchar *str);
  void Show(const gchar *orig_word, gchar ***Word, gchar ****WordData);
  void ShowTreeDictData(gchar *data);
  void Show(const struct STARDICT::LookupResponse::DictResponse *dict_response);
  gboolean Find (const gchar *text, gboolean start);
	bool IsSearchPanelHasFocus() { return GTK_WIDGET_HAS_FOCUS(eSearch); } 

	void ShowSearchPanel();

private:
	GtkButton *btClose;
	GtkButton *btFind;
	GtkWidget *hbSearchPanel;
	

	void HideSearchPanel(void) 
		{
			gtk_widget_hide_all(hbSearchPanel);
		}


	static void SelectionCallback(GtkWidget* widget,GtkSelectionData *selection_data, guint time, TextWin *oTextWin);
	static gboolean on_button_press(GtkWidget * widget, GdkEventButton * event, TextWin *oTextWin);
	static bool find_first_tag(gchar *str, gchar * & beg, gchar * & end);
	static void OnCloseSearchPanel(GtkWidget *widget, TextWin *oTextWin);
	static gboolean OnSearchKeyPress(GtkWidget *widget, GdkEventKey *event,
					 TextWin *oTextWin);
	static void OnFindSearchPanel(GtkWidget *widget, TextWin *oTextWin);
};

class TransWin {
public:
	TransWin();
	void Create(GtkWidget *notebook);
	void SetText(const char *text, size_t len = -1);
	bool IsInputViewHasFocus() { return GTK_WIDGET_HAS_FOCUS(input_textview); }
private:
	GtkWidget *input_textview;
	GtkWidget *result_textview;
	GtkWidget *engine_combobox;
	GtkWidget *fromlang_combobox;
	GtkWidget *tolang_combobox;
	GtkWidget *trans_button;
	static void on_translate_button_clicked(GtkWidget *widget, TransWin *oTransWin);
	static void on_engine_combobox_changed(GtkWidget *widget, TransWin *oTransWin);
	static void on_fromlang_combobox_changed(GtkWidget *widget, TransWin *oTransWin);
	static void on_tolang_combobox_changed(GtkWidget *widget, TransWin *oTransWin);
	void SetComboBox(gint engine_index, gint fromlang_index, gint tolang_index);
	void GetHostFile(std::string &host, std::string &file, const char *text);
};

class MidWin {
public:
	GtkWidget* hpaned;
	GtkWidget* notebook;

	LeftWin oLeftWin;
	IndexWin oIndexWin;
	ToolWin oToolWin;
	TextWin oTextWin;
	TransWin oTransWin;

	MidWin() {}
	void Create(GtkWidget *vbox);
};

class BottomWin {
private:
	static void ScanCallback(GtkToggleButton *button, gpointer data);
	static void AboutCallback(GtkButton *button, gpointer data);
	static void QuitCallback(GtkButton *button, gpointer data);
	static void InternetSearchCallback(GtkButton *button, BottomWin *oBottomWin);
	static void NewVersionCallback(GtkButton *button, BottomWin *oBottomWin);
	static void DictManageCallback(GtkButton *button, BottomWin *oBottomWin);
	static void PreferenceCallback(GtkButton *button, BottomWin *oBottomWin);
	static gboolean on_internetsearch_button_press(GtkWidget * widget, GdkEventButton * event , BottomWin *oBottomWin);
	static void on_internetsearch_menu_item_activate(GtkMenuItem *menuitem, const gchar *website);

public:
	GtkWidget* ScanSelectionCheckButton;
	GtkWidget* SearchWebsiteMenu;

	BottomWin();
	void Create(GtkWidget *vbox);
};

#endif
