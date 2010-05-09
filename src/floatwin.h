#ifndef __SD_FLOATWIN_H__
#define __SD_FLOATWIN_H__

#include <gtk/gtk.h>
#include <string>
#include "articleview.h"
#include "readword.h"

const int FLOAT_TIMEOUT=300;		        // interval used by floating window.
const int DISAPPEAR_DISTANCE=400; // it is the square of the distance.
const int FLOATWIN_BORDER_WIDTH=4;
const int FLOATWIN_OFFSET_X=8;
const int FLOATWIN_OFFSET_Y=8; //when the window is too right,the x will change,so make offset_y not equal to 0 will make button box not show all the same.

enum FloatWinQueryResult
{
	FLOAT_WIN_FOUND,
	FLOAT_WIN_FUZZY_FOUND,
	FLOAT_WIN_NOT_FOUND,
	FLOAT_WIN_FUZZY_NOT_FOUND,
	FLOAT_WIN_NET_FOUND,
	FLOAT_WIN_NET_NOT_FOUND,
};

class FloatWin {
public:
	FloatWin();
	void Create();
	void End();
	void ShowTextLocal(gchar ***pppWord, gchar **** ppppWordData, const gchar * sOriginWord);
	void ShowTextFuzzy(gchar ****ppppWord, gchar ***** pppppWordData, const gchar ** ppOriginWord, gint count, const gchar * sOriginWord);
	void ShowTextStarDictNet(const struct STARDICT::LookupResponse::DictResponse *dict_response);
	void ShowTextNetDict(NetDictResponse *resp);
	void ShowNotFound(const char* sWord, const char* sReason, gboolean fuzzy);
	void ShowPangoTips(const char *word, const char *text);
	void Show();
	void Hide();
	const std::string& getQueryingWord(void) const { return QueryingWord; }
	GtkWidget *getFloatWindow(void) const { return FloatWindow; }
private:
	gint press_x_root,press_y_root,press_window_x,press_window_y;
	gint popup_pointer_x, popup_pointer_y;
	gint timeout;
	gint now_window_width,now_window_height;
	gboolean button_box_once_shown;
	gboolean ismoving;

	GtkWidget *FloatWindow;
	GtkWidget *button_hbox;
	GtkWidget *LockImage, *PronounceWordButton, *StopButton;
	GtkWidget *menu;
	std::string QueryingWord;
	std::string PronounceWord;
	ReadWordType readwordtype;
	FloatWinQueryResult found_result;
	std::auto_ptr<ArticleView> view;
	
	static void on_query_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_save_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_play_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_stop_click(GtkWidget *widget, FloatWin *oFloatWin);
#ifndef CONFIG_GPE
	static void on_help_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_quit_click(GtkWidget *widget, FloatWin *oFloatWin);
#endif
	static void vLockCallback(GtkWidget *widget, FloatWin *oFloatWin);
	static gint vTimeOutCallback(gpointer data);
	static gboolean vEnterNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin);
	static gboolean vLeaveNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin);
	static gboolean vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin);
	static gboolean vButtonReleaseCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin);
	static gboolean vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , FloatWin *oFloatWin);
	static void on_menu_copy_activate(GtkWidget * widget, FloatWin *oFloatWin);
	static void on_menu_save_activate(GtkWidget * widget, FloatWin *oFloatWin);
	static void on_menu_query_activate(GtkWidget * widget, FloatWin *oFloatWin);
	static void on_menu_play_activate(GtkWidget * widget, FloatWin *oFloatWin);
	static void on_menu_fuzzyquery_activate(GtkWidget * widget, FloatWin *oFloatWin);
	void on_lock_changed(const baseconfval*);
	void on_dict_scan_select_changed(const baseconfval*);
	void on_lock_x_changed(const baseconfval*);
	void on_lock_y_changed(const baseconfval*);
	void on_transparent_changed(const baseconfval*);
	void on_bg_red_changed(const baseconfval*);
	void on_bg_green_changed(const baseconfval*);
	void on_bg_blue_changed(const baseconfval*);
	void on_use_custom_bg_changed(const baseconfval*);
	
	gint get_vscrollbar_width(void);
	static gint get_window_border_width(void);
	void float_window_size(gint& window_width, gint& window_height);
	void float_window_position(gboolean usePointerPosition,
		gint window_width, gint window_height, gint& x, gint& y);
	void remember_pointer_position(void);
	void Popup(gboolean changeposition);
	void create_button_hbox(void);
	int get_distance_pointer_to_window(void);
	void button_box_show_first_time(void);
	void show_popup_menu(GdkEventButton * event);
	void set_transparent(int transparent);
	void set_bg(void);
	static const gchar* get_lock_image_stock_id(void);
	void restore_locked_position(void);
	static std::string get_not_found_markup(const gchar* sWord, const gchar* sReason);
	static std::string get_head_word_markup(const gchar* sWord);
};

#endif
