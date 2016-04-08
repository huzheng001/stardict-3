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

#ifndef __SD_FLOATWIN_H__
#define __SD_FLOATWIN_H__

#include <gtk/gtk.h>
#include <string>
#include "articleview.h"
#include "readword.h"

const int FLOAT_TIMEOUT=300;		        // interval used by floating window.
/* If lookup results do not come promptly, we show a message that the request is being processed.
This variable specifies a timeout for that message. */
const int LOOKUP_RUNNING_TIMEOUT=2000;
const int DISAPPEAR_DISTANCE=400; // it is the square of the distance.
const int FLOATWIN_BORDER_WIDTH=4;
const int FLOATWIN_OFFSET_X=8;
const int FLOATWIN_OFFSET_Y=8; //when the window is too right,the x will change,so make offset_y not equal to 0 will make button box not show all the same.

class FloatWin {
public:
	FloatWin();
	void Create();
	void End();
	void StartLookup(const char* sWord, bool IgnoreScanModifierKey = false);
	void EndLookup(void);
	void AppendTextLocalDict(gchar ***pppWord, gchar **** ppppWordData, const gchar * sOriginWord);
	void AppendTextFuzzy(gchar ****ppppWord, gchar ***** pppppWordData, const gchar ** ppOriginWord, gint count, const gchar * sOriginWord);
	void AppendTextStarDictNet(const struct STARDICT::LookupResponse::DictResponse *dict_response);
	void AppendTextNetDict(NetDictResponse *resp);
	void ShowPangoTips(const char *word, const char *text);
	void Show();
	void Hide();
	void set_bg(void);
	void set_bookname_style(BookNameStyle style);
	const std::string& getQueryingWord(void) const { return QueryingWord; }
private:
	enum ContentState {
		ContentState_Empty,
		ContentState_Waiting,
		ContentState_Found,
		ContentState_NotFound
	};
	gint press_x_root,press_y_root,press_window_x,press_window_y;
	gint popup_pointer_x, popup_pointer_y;
	gint hide_window_timer;
	gint lookup_running_timer;
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
	ContentState content_state;
	/* have content besides messages "Not found" and "Looking up" */
	bool have_real_content;
	/* The Float window has been shown and positioned properly.
	We need this flag to avoid repositioning the window multiple times 
	when responses from different sources arrive. */
	bool window_positioned;
	bool IgnoreScanModifierKey;
	std::unique_ptr<ArticleView> view;
	
	static void on_query_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_save_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_play_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_stop_click(GtkWidget *widget, FloatWin *oFloatWin);
#ifndef CONFIG_GPE
	static void on_help_click(GtkWidget *widget, FloatWin *oFloatWin);
	static void on_quit_click(GtkWidget *widget, FloatWin *oFloatWin);
#endif
	static void vLockCallback(GtkWidget *widget, FloatWin *oFloatWin);
	static gint vHideWindowTimeOutCallback(gpointer data);
	static gint vLookupRunningTimeOutCallback(gpointer data);
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
	static const gchar* get_lock_image_stock_id(void);
	void restore_locked_position(void);
	static std::string get_not_found_markup(const gchar* sWord, const gchar* sReason);
	static std::string get_looking_up_markup(const gchar* sWord);
	static std::string get_head_word_markup(const gchar* sWord);
	void set_busy_cursor(void);
	void set_normal_cursor(void);
	void start_hide_window_timer(void);
	void destroy_hide_window_timer(void);
	void start_lookup_running_timer(void);
	void destroy_lookup_running_timer(void);
};

#endif
