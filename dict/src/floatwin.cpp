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

#include <cstring>
#include <glib/gi18n.h>

#ifdef _WIN32
#  include <gdk/gdkwin32.h>
#endif

#include <string>

#include "stardict.h"
#include "desktop.h"
#include "conf.h"
#include "lib/utils.h"
#include "iskeyspressed.h"

#include "floatwin.h"

FloatWin::FloatWin()
{
	hide_window_timer = 0;
	lookup_running_timer = 0;
	now_window_width = 0;
	now_window_height = 0;
	button_box_once_shown = false;
	ismoving = false;
	menu = NULL;
	content_state = ContentState_Empty;
	have_real_content = false;
	window_positioned = false;
	IgnoreScanModifierKey = false;
}

void FloatWin::Create()
{
	FloatWindow = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_widget_set_events(FloatWindow, GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_BUTTON1_MOTION_MASK);
	g_signal_connect (G_OBJECT (FloatWindow), "button_press_event", G_CALLBACK (vButtonPressCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "button_release_event", G_CALLBACK (vButtonReleaseCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "motion_notify_event", G_CALLBACK (vMotionNotifyCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "enter_notify_event", G_CALLBACK (vEnterNotifyCallback), this);
	g_signal_connect (G_OBJECT (FloatWindow), "leave_notify_event", G_CALLBACK (vLeaveNotifyCallback), this);

	GtkWidget *frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_ETCHED_OUT);
	gtk_container_add(GTK_CONTAINER(FloatWindow),frame);
	GtkWidget *vbox;
#if GTK_MAJOR_VERSION >= 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#else
	vbox = gtk_vbox_new(false,0);
#endif
	gtk_container_set_border_width (GTK_CONTAINER (vbox), FLOATWIN_BORDER_WIDTH);
	gtk_container_add(GTK_CONTAINER(frame),vbox);

	create_button_hbox();
	gtk_box_pack_start(GTK_BOX(vbox),button_hbox,false,false,0);
	view.reset(new ArticleView(GTK_BOX(vbox), (BookNameStyle)conf->get_int_at("dictionary/bookname_style"), true));
	
	gtk_widget_show_all(frame);
	gtk_widget_hide(button_hbox); //show all will show hbox's children,now hide hbox only.

	set_bg();
	set_transparent(conf->get_int_at("floating_window/transparent"));
	restore_locked_position();

	conf->notify_add("/apps/stardict/preferences/floating_window/lock",
			 sigc::mem_fun(this, &FloatWin::on_lock_changed));
	conf->notify_add("/apps/stardict/preferences/dictionary/scan_selection",
			 sigc::mem_fun(this, &FloatWin::on_dict_scan_select_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/lock_x",
			 sigc::mem_fun(this, &FloatWin::on_lock_x_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/lock_y",
			 sigc::mem_fun(this, &FloatWin::on_lock_y_changed));
	conf->notify_add("/apps/stardict/preferences/floating_window/transparent",
			 sigc::mem_fun(this, &FloatWin::on_transparent_changed));

	conf->notify_add("/apps/stardict/preferences/floating_window/use_custom_bg",
			 sigc::mem_fun(this, &FloatWin::on_use_custom_bg_changed));
	gtk_widget_realize(FloatWindow);
}

void FloatWin::End()
{
	if (conf->get_bool_at("floating_window/lock")) {
		gint x, y;
		gtk_window_get_position(GTK_WINDOW(FloatWindow), &x, &y);
		conf->set_int_at("floating_window/lock_x", x);
		conf->set_int_at("floating_window/lock_y", y);
	}

	destroy_hide_window_timer();
	if (menu)
		gtk_widget_destroy(menu);
	menu = NULL;
	if (FloatWindow)
		gtk_widget_destroy(FloatWindow);
	FloatWindow = NULL;
	content_state = ContentState_Empty;
	have_real_content = false;
}

void FloatWin::StartLookup(const char* sWord, bool IgnoreScanModifierKey)
{
	QueryingWord = sWord;
	content_state = ContentState_Waiting;
	/* Set the text telling that lookup is running, but do not show the window. 
	The window will be shown when a response arrives or when LOOKUP_RUNNING_TIMEOUT
	elapses. */
	const std::string markup = get_looking_up_markup(QueryingWord.c_str());
	view->set_pango_text(markup.c_str());
	set_busy_cursor();
	have_real_content = false;
	window_positioned = false;
	this->IgnoreScanModifierKey = IgnoreScanModifierKey;
	start_lookup_running_timer();
}

void FloatWin::EndLookup(void)
{
	content_state = have_real_content ? ContentState_Found : ContentState_NotFound;
	if(!have_real_content) {
		const std::string markup = get_not_found_markup(QueryingWord.c_str(), _("<Not Found!>"));
		view->set_pango_text(markup.c_str());
		if (conf->get_bool_at("floating_window/show_if_not_found")) {
			Popup(!window_positioned);
			window_positioned = true;
		}
	}
	set_normal_cursor();
	destroy_lookup_running_timer();
}

void FloatWin::AppendTextLocalDict(gchar ***Word, gchar ****WordData, const gchar *sOriginWord)
{
	view->begin_update();
	if(!have_real_content)
		view->clear();
	const std::string mark = get_head_word_markup(sOriginWord);
	view->append_pango_text(mark.c_str());
	int j,k;
	for (size_t i=0; i<gpAppFrame->scan_dictmask.size(); i++) {
		if (Word[i]) {
			view->AppendNewline();
			view->SetDictIndex(gpAppFrame->scan_dictmask[i]);
			if (gpAppFrame->scan_dictmask[i].type == InstantDictType_LOCAL)
				view->AppendHeader(gpAppFrame->oLibs.dict_name(gpAppFrame->scan_dictmask[i].index).c_str());
			else if (gpAppFrame->scan_dictmask[i].type == InstantDictType_VIRTUAL)
				view->AppendHeader(gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(gpAppFrame->scan_dictmask[i].index));
			else if (gpAppFrame->scan_dictmask[i].type == InstantDictType_NET)
				view->AppendHeader(gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(gpAppFrame->scan_dictmask[i].index));
			j=0;
			do {
				if (j==0) {
					if (strcmp(Word[i][j], sOriginWord))
						view->AppendWord(Word[i][j]);
				} else {
					view->AppendNewline();
					view->AppendWord(Word[i][j]);
				}
				view->AppendData(WordData[i][j][0], Word[i][j],
						 sOriginWord);
				k=1;
				while (WordData[i][j][k]) {
					view->AppendNewline();
					view->AppendDataSeparate();
					view->AppendData(WordData[i][j][k],
							 Word[i][j], sOriginWord);
					k++;
				}
				j++;
			} while (Word[i][j]);
		}
	}
	view->end_update();
	have_real_content = true;
	destroy_lookup_running_timer();

	gboolean pronounced = false;
	readwordtype = gpAppFrame->oReadWord.canRead(sOriginWord);
	if (readwordtype != READWORD_CANNOT) {
		if (PronounceWord == sOriginWord) {
			pronounced = true;
		} else {
			PronounceWord = sOriginWord;
		}
	} else {
		for (size_t i=0;i< gpAppFrame->scan_dictmask.size(); i++) {
			if (Word[i] && strcmp(Word[i][0], sOriginWord)) {
				readwordtype = gpAppFrame->oReadWord.canRead(Word[i][0]);
				if (readwordtype != READWORD_CANNOT) {
					if (PronounceWord == Word[i][0])
						pronounced = true;
					else
						PronounceWord = Word[i][0];
				}
				break;
			}
		}
	}
	gtk_widget_set_sensitive(PronounceWordButton, readwordtype != READWORD_CANNOT);

	Popup(!window_positioned);
	window_positioned = true;
	if ((readwordtype != READWORD_CANNOT) && (!pronounced) && conf->get_bool_at("floating_window/pronounce_when_popup"))
		gpAppFrame->oReadWord.read(PronounceWord.c_str(), readwordtype);
}

void FloatWin::AppendTextNetDict(NetDictResponse *resp)
{
	if(!resp->data)
		return;

	view->begin_update();
	if(!have_real_content)
		view->clear();
	if (!have_real_content) {
		std::string mark = get_head_word_markup(resp->word);
		view->append_pango_text(mark.c_str());
	}
	InstantDictIndex dict_index;
	dict_index.type = InstantDictType_UNKNOWN;
	view->SetDictIndex(dict_index);
	view->AppendNewline();
	view->AppendHeader(resp->bookname);
	view->AppendWord(resp->word);
	view->AppendData(resp->data, resp->word, resp->word);
	view->end_update();
	have_real_content = true;
	destroy_lookup_running_timer();

	gboolean pronounced = false;
	readwordtype = gpAppFrame->oReadWord.canRead(resp->word);
	if (readwordtype != READWORD_CANNOT) {
		if (PronounceWord == resp->word)
			pronounced = true;
		else
			PronounceWord = resp->word;
	}
	gtk_widget_set_sensitive(PronounceWordButton, readwordtype != READWORD_CANNOT);

	Popup(!window_positioned);
	window_positioned = true;
	if ((readwordtype != READWORD_CANNOT) && (!pronounced) && conf->get_bool_at("floating_window/pronounce_when_popup"))
		gpAppFrame->oReadWord.read(PronounceWord.c_str(), readwordtype);
}

void FloatWin::AppendTextStarDictNet(const struct STARDICT::LookupResponse::DictResponse *dict_response)
{
	if(dict_response->dict_result_list.empty())
		return;

	view->begin_update();
	if(!have_real_content)
		view->clear();
	if (!have_real_content) {
		std::string mark = get_head_word_markup(dict_response->oword);
		view->append_pango_text(mark.c_str());
	}
	InstantDictIndex dict_index;
	dict_index.type = InstantDictType_UNKNOWN;
	view->SetDictIndex(dict_index);
	for (std::list<struct STARDICT::LookupResponse::DictResponse::DictResult *>::const_iterator i = dict_response->dict_result_list.begin(); i != dict_response->dict_result_list.end(); ++i) {
		view->AppendNewline();
		view->AppendHeader((*i)->bookname);
		for (std::list<struct STARDICT::LookupResponse::DictResponse::DictResult::WordResult *>::iterator j = (*i)->word_result_list.begin(); j != (*i)->word_result_list.end(); ++j) {
			if (j == (*i)->word_result_list.begin()) {
				if (strcmp((*j)->word, dict_response->oword)) {
					view->AppendWord((*j)->word);
				}
			} else {
				view->AppendNewline();
				view->AppendWord((*j)->word);
			}
			std::list<char *>::iterator k = (*j)->datalist.begin();
			view->AppendData(*k, (*j)->word, dict_response->oword);
			for (++k; k != (*j)->datalist.end(); ++k) {
				view->AppendNewline();
				view->AppendDataSeparate();
				view->AppendData(*k, (*j)->word, dict_response->oword);
			}
		}
	}
	view->end_update();
	have_real_content = true;
	destroy_lookup_running_timer();

	gboolean pronounced = false;
	readwordtype = gpAppFrame->oReadWord.canRead(dict_response->oword);
	if (readwordtype != READWORD_CANNOT) {
		if (PronounceWord == dict_response->oword)
			pronounced = true;
		else
			PronounceWord = dict_response->oword;
	} else {
		for (std::list<struct STARDICT::LookupResponse::DictResponse::DictResult *>::const_iterator i = dict_response->dict_result_list.begin(); i != dict_response->dict_result_list.end(); ++i) {
			std::list<struct STARDICT::LookupResponse::DictResponse::DictResult::WordResult *>::iterator j = (*i)->word_result_list.begin();
			if (j != (*i)->word_result_list.end() && strcmp((*j)->word, dict_response->oword)) {
				readwordtype = gpAppFrame->oReadWord.canRead((*j)->word);
				if (readwordtype != READWORD_CANNOT) {
					if (PronounceWord == (*j)->word)
						pronounced = true;
					else
						PronounceWord = (*j)->word;
				}
				break;
			}
		}
	}
	gtk_widget_set_sensitive(PronounceWordButton, readwordtype != READWORD_CANNOT);

	Popup(!window_positioned);
	window_positioned = true;
	if ((readwordtype != READWORD_CANNOT) && (!pronounced) && conf->get_bool_at("floating_window/pronounce_when_popup"))
		gpAppFrame->oReadWord.read(PronounceWord.c_str(), readwordtype);
}

void FloatWin::AppendTextFuzzy(gchar ****ppppWord, gchar *****pppppWordData, const gchar ** ppOriginWord, gint count, const gchar *sOriginWord)
{
	view->begin_update();
	if(!have_real_content)
		view->clear();

	std::string mark;
	gchar *m_str;
	mark = _("Fuzzy query");
	mark += " ";
	mark += get_head_word_markup(sOriginWord);
	mark += " ";
	mark += _("has succeeded.\n");
	if (count ==1)
		mark+= _("Found 1 word:\n");
	else {
		m_str=g_strdup_printf(_("Found %d words:\n"),count);
		mark += m_str;
		g_free(m_str);
	}
	
	int j;
	for (j=0; j<count-1; j++) {
		mark += "<span size=\"x-large\">";
		m_str = g_markup_escape_text(ppOriginWord[j], -1);
		mark += m_str;
		g_free(m_str);
		mark += "</span> ";
	}
	mark += "<span size=\"x-large\">";
	m_str = g_markup_escape_text(ppOriginWord[count-1],-1);
	mark += m_str;
	g_free(m_str);
	mark += "</span>";
	view->append_pango_text(mark.c_str());

	int m,n;
	for (j=0; j<count; j++) {
		if (!ppppWord[j])
			continue;
		mark = "\n\n";
		mark += get_head_word_markup(ppOriginWord[j]);
		view->append_pango_text(mark.c_str());
		for (size_t i=0; i<gpAppFrame->scan_dictmask.size(); i++) {
			if (ppppWord[j][i]) {
				view->AppendNewline();
				view->SetDictIndex(gpAppFrame->scan_dictmask[i]);
				if (gpAppFrame->scan_dictmask[i].type == InstantDictType_LOCAL)
					view->AppendHeader(gpAppFrame->oLibs.dict_name(gpAppFrame->scan_dictmask[i].index).c_str());
				else if (gpAppFrame->scan_dictmask[i].type == InstantDictType_VIRTUAL)
					view->AppendHeader(gpAppFrame->oStarDictPlugins->VirtualDictPlugins.dict_name(gpAppFrame->scan_dictmask[i].index));
				else if (gpAppFrame->scan_dictmask[i].type == InstantDictType_NET)
					view->AppendHeader(gpAppFrame->oStarDictPlugins->NetDictPlugins.dict_name(gpAppFrame->scan_dictmask[i].index));
				m=0;
				do {
					if (m==0) {
						if (strcmp(ppppWord[j][i][m], ppOriginWord[j]))
							view->AppendWord(ppppWord[j][i][m]);
					} else {
						view->AppendNewline();
						view->AppendWord(ppppWord[j][i][m]);
					}
					view->AppendData(pppppWordData[j][i][m][0], ppppWord[j][i][m],
							 sOriginWord);
					n=1;
					while (pppppWordData[j][i][m][n]) {
						view->AppendNewline();
						view->AppendDataSeparate();
						view->AppendData(pppppWordData[j][i][m][n],
								 ppppWord[j][i][m], sOriginWord);
						n++;
					}
					m++;
				} while (ppppWord[j][i][m]);
			}
		}
	}
	view->end_update();
	have_real_content = true;
	destroy_lookup_running_timer();

	readwordtype = gpAppFrame->oReadWord.canRead(sOriginWord);
	if (readwordtype != READWORD_CANNOT)
		PronounceWord = sOriginWord;
	gtk_widget_set_sensitive(PronounceWordButton, readwordtype != READWORD_CANNOT);

 
	Popup(!window_positioned);
	window_positioned = true;
	/*bool pronounce_when_popup=
		conf->get_bool_at("floating_window/pronounce_when_popup");

	if (canRead && pronounce_when_popup)
		gpAppFrame->oReadWord.read(PronounceWord.c_str());*/
}

void FloatWin::ShowPangoTips(const char *sWord, const char *text)
{
	QueryingWord = sWord;
	view->set_pango_text(text);
	have_real_content = true;
	IgnoreScanModifierKey = false;
	destroy_lookup_running_timer();

	//gboolean pronounced = false;
	readwordtype = gpAppFrame->oReadWord.canRead(sWord);
	if (readwordtype != READWORD_CANNOT) {
		if (PronounceWord == sWord) {
			//pronounced = true;
		} else {
			PronounceWord = sWord;
		}
	}
	gtk_widget_set_sensitive(PronounceWordButton, readwordtype != READWORD_CANNOT);

	Popup(true);
	window_positioned = true;
}

void FloatWin::Show()
{
	gtk_widget_show(FloatWindow);
#ifdef _WIN32
	gtk_window_present(GTK_WINDOW(FloatWindow));
#endif
	start_hide_window_timer();
}

void FloatWin::Hide()
{
	destroy_hide_window_timer();
	button_box_once_shown = false;
	window_positioned = false;
	PronounceWord.clear();
	gtk_widget_hide(FloatWindow);
}

void FloatWin::on_query_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");

	if (!conf->get_bool_at("floating_window/lock"))
		oFloatWin->Hide();
	gpAppFrame->Query(oFloatWin->QueryingWord.c_str());
	gpAppFrame->oDockLet->maximize_from_tray();
}

void FloatWin::on_save_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	const std::string& filePath = conf->get_string_at("dictionary/export_file");
#ifdef _WIN32
	FILE *fp = fopen(abs_path_to_data_dir(filePath).c_str(), "a+");
#else
	FILE *fp = fopen(filePath.c_str(), "a+");
#endif
	if(fp) {
		if (conf->get_bool_at("dictionary/only_export_word")) {
			fputs(oFloatWin->QueryingWord.c_str(),fp);
			fputs("\n",fp);
		} else {
			fputs(oFloatWin->view->get_text().c_str(),fp);
			fputs("\n\n",fp);
		}
		fclose(fp);
	}
	play_sound_on_event("buttonactive");
}

void FloatWin::on_play_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	gpAppFrame->oReadWord.read(oFloatWin->PronounceWord.c_str(), oFloatWin->readwordtype);
}

void FloatWin::on_stop_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	conf->set_bool_at("dictionary/scan_selection", false);
}

#ifndef CONFIG_GPE
void FloatWin::on_help_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");

	if (!conf->get_bool_at("floating_window/lock"))
		oFloatWin->Hide();
	show_help("stardict-scan-selection");
}

void FloatWin::on_quit_click(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	gpAppFrame->Quit();
}
#endif
void FloatWin::vLockCallback(GtkWidget *widget, FloatWin *oFloatWin)
{
	play_sound_on_event("buttonactive");
	conf->set_bool_at("floating_window/lock", 
		!conf->get_bool_at("floating_window/lock"));
}

gint FloatWin::vHideWindowTimeOutCallback(gpointer data)
{
	FloatWin *oFloatWin = static_cast<FloatWin *>(data);
	bool lock=
		conf->get_bool_at("floating_window/lock");
	if(lock || oFloatWin->ismoving || !gtk_widget_get_visible(GTK_WIDGET(oFloatWin->FloatWindow)))
		return true;

	bool only_scan_while_modifier_key=
		conf->get_bool_at("dictionary/only_scan_while_modifier_key");
	bool hide_floatwin_when_modifier_key_released=
		conf->get_bool_at("dictionary/hide_floatwin_when_modifier_key_released");
	if (only_scan_while_modifier_key
		&& hide_floatwin_when_modifier_key_released
		&& !oFloatWin->IgnoreScanModifierKey) {
		GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(oFloatWin->FloatWindow));
		GdkDisplay *display = gdk_screen_get_display(screen);
		gint iCurrentX,iCurrentY;
#if GTK_MAJOR_VERSION >= 3
		GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
		GdkDevice  *pointer = gdk_device_manager_get_client_pointer (device_manager);
		gdk_device_get_position(pointer, NULL, &iCurrentX, &iCurrentY);
#else
		gdk_display_get_pointer(display, NULL, &iCurrentX, &iCurrentY, NULL);
#endif
		if (iCurrentX == oFloatWin->popup_pointer_x && iCurrentY==oFloatWin->popup_pointer_y) {
			bool released = !gpAppFrame->unlock_keys->is_pressed();

			if (released) {
				oFloatWin->Hide();
				gpAppFrame->oSelection.LastClipWord.clear(); //so press modifier key again will pop up the floatwin.
				return true;
			}
		}
	}
	
	if (oFloatWin->get_distance_pointer_to_window() > DISAPPEAR_DISTANCE){
		oFloatWin->Hide();
	}

	return true;
}

gint FloatWin::vLookupRunningTimeOutCallback(gpointer data)
{
	FloatWin *oFloatWin = static_cast<FloatWin *>(data);
	if(!oFloatWin->have_real_content) {
		if (conf->get_bool_at("floating_window/show_if_not_found")) {
			oFloatWin->Popup(!oFloatWin->window_positioned);
			oFloatWin->window_positioned = true;
		}
	}
	oFloatWin->lookup_running_timer = 0;
	return FALSE;
}

gboolean FloatWin::vEnterNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin)
{
#ifdef _WIN32
	if ((event->detail==GDK_NOTIFY_ANCESTOR) || (event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL)) {
#else
	if ((event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL)) {
#endif
		if (!gtk_widget_get_visible(GTK_WIDGET(oFloatWin->button_hbox))) {
			gtk_widget_show(oFloatWin->button_hbox);
		
			if (!oFloatWin->button_box_once_shown) {
				oFloatWin->button_box_once_shown = true;
				oFloatWin->button_box_show_first_time();
			}
		}
	}
	return true;
}

gboolean FloatWin::vLeaveNotifyCallback (GtkWidget *widget, GdkEventCrossing *event, FloatWin *oFloatWin)
{
#ifdef _WIN32
	if (((event->detail==GDK_NOTIFY_ANCESTOR) || (event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL))&&(!oFloatWin->ismoving)) {
#else
	if (((event->detail==GDK_NOTIFY_NONLINEAR) || (event->detail==GDK_NOTIFY_NONLINEAR_VIRTUAL))&&(!oFloatWin->ismoving)) {
#endif
		gtk_widget_hide(oFloatWin->button_hbox);
	}
	return true;
}

gboolean FloatWin::vButtonPressCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin)
{
	if (event->type == GDK_BUTTON_PRESS) {
		if (event->button == 1) {
			gtk_window_get_position(GTK_WINDOW(widget),&(oFloatWin->press_window_x),&(oFloatWin->press_window_y));
			oFloatWin->press_x_root = (gint)(event->x_root);
			oFloatWin->press_y_root = (gint)(event->y_root);
			oFloatWin->ismoving = true;
		}
		else if (event->button == 3) {
			oFloatWin->show_popup_menu(event);
		}
	} else if (event->type == GDK_2BUTTON_PRESS) {
		if (oFloatWin->content_state == ContentState_NotFound) {
			std::string QueryingWord(oFloatWin->QueryingWord);
			gpAppFrame->LookupWithFuzzyToFloatWin(QueryingWord.c_str());
		} else
			oFloatWin->Hide();
	}

	return true;
}

gboolean FloatWin::vButtonReleaseCallback (GtkWidget * widget, GdkEventButton * event , FloatWin *oFloatWin)
{
	if (event->button == 1) {
		oFloatWin->ismoving = false;
	}

	return true;
}

gboolean FloatWin::vMotionNotifyCallback (GtkWidget * widget, GdkEventMotion * event , FloatWin *oFloatWin)
{
	if (event->state & GDK_BUTTON1_MASK) {
		gint x,y;
		x = oFloatWin->press_window_x + (gint)(event->x_root) - oFloatWin->press_x_root;
		y = oFloatWin->press_window_y + (gint)(event->y_root) - oFloatWin->press_y_root;
		if (x<0)
			x = 0;
		if (y<0)
			y = 0;
		gtk_window_move(GTK_WINDOW(oFloatWin->FloatWindow), x, y);
	}

	return true;
}

void FloatWin::on_menu_copy_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, oFloatWin->view->get_text().c_str(), -1);
}

void FloatWin::on_menu_save_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_save_click(widget, oFloatWin);
}

void FloatWin::on_menu_query_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_query_click(widget, oFloatWin);
}

void FloatWin::on_menu_play_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	oFloatWin->on_play_click(widget, oFloatWin);
}

void FloatWin::on_menu_fuzzyquery_activate(GtkWidget * widget, FloatWin *oFloatWin)
{
	/* do not use oFloatWin->QueryingWord directly,
	LookupWithFuzzyToFloatWin overwrite oFloatWin->QueryingWord */
	const std::string tQueryingWord(oFloatWin->QueryingWord);
	gpAppFrame->LookupWithFuzzyToFloatWin(tQueryingWord.c_str());
}

void FloatWin::on_lock_changed(const baseconfval* )
{
	gtk_image_set_from_stock(GTK_IMAGE(LockImage),
		get_lock_image_stock_id(), GTK_ICON_SIZE_MENU);
}

void FloatWin::on_dict_scan_select_changed(const baseconfval* scanval)
{
	bool scan = static_cast<const confval<bool> *>(scanval)->val_;

	gtk_widget_set_sensitive(StopButton, scan);
}

void FloatWin::on_lock_x_changed(const baseconfval* lock_x_val)
{
	int lock_x=static_cast<const confval<int> *>(lock_x_val)->val_;
	if (conf->get_bool_at("floating_window/lock")) {
		gint old_x, old_y;
		gtk_window_get_position(GTK_WINDOW(FloatWindow), &old_x, &old_y);
		if (lock_x!=old_x) {
			gtk_window_move(GTK_WINDOW(FloatWindow), lock_x, old_y);
		}
	}
}

void FloatWin::on_lock_y_changed(const baseconfval* lock_y_val)
{
	int lock_y=static_cast<const confval<int> *>(lock_y_val)->val_;

	if (conf->get_bool_at("floating_window/lock")) {
		gint old_x,old_y;
		gtk_window_get_position(GTK_WINDOW(FloatWindow), &old_x, &old_y);
		if (lock_y!=old_y) {
			gtk_window_move(GTK_WINDOW(FloatWindow), old_x, lock_y);
		}
	}
}

void FloatWin::on_transparent_changed(const baseconfval* val)
{
	int transparent = static_cast<const confval<int> *>(val)->val_;
	set_transparent(transparent);
}

void FloatWin::on_use_custom_bg_changed(const baseconfval* )
{
	set_bg();
}

gint FloatWin::get_vscrollbar_width(void)
{
	static gint vscrollbar_width = 0;
	if (!vscrollbar_width) {
		if (view->vscroll_bar()) {
			GtkRequisition vscrollbar_requisition;
#if GTK_MAJOR_VERSION >= 3
			gtk_widget_get_preferred_size(view->vscroll_bar(), NULL, &vscrollbar_requisition);
#else
			gtk_widget_size_request(view->vscroll_bar(), &vscrollbar_requisition);
#endif
			vscrollbar_width = vscrollbar_requisition.width;
			vscrollbar_width += view->scroll_space();
		}
	}
	return vscrollbar_width;
}

gint FloatWin::get_window_border_width(void)
{
	return 2*(FLOATWIN_BORDER_WIDTH+2); // 2 is the frame 's width. Or get it by gtk function? i am lazy,hoho
}

void FloatWin::float_window_size(gint& window_width, gint& window_height)
{
	GtkRequisition requisition;
#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_preferred_size(view->widget(), NULL, &requisition);
#else
	gtk_widget_size_request(view->widget(), &requisition);
#endif
	int max_window_width=conf->get_int_at("floating_window/max_window_width");
	if (requisition.width > max_window_width) {
		// it is not really max window width setting.
		gtk_widget_set_size_request(view->widget(), max_window_width, -1);
		gtk_label_set_line_wrap(GTK_LABEL(view->widget()), true);
#if GTK_MAJOR_VERSION >= 3
		gtk_widget_get_preferred_size(view->widget(), NULL, &requisition); //update requisition
#else
		gtk_widget_size_request(view->widget(), &requisition); //update requisition
#endif
	}
	window_width = get_window_border_width() + requisition.width;
	int max_window_height=
		conf->get_int_at("floating_window/max_window_height");
	if (requisition.height > max_window_height) {
		const gint vscrollbar_width = get_vscrollbar_width();
		view->set_size(requisition.width + vscrollbar_width, max_window_height);
		window_height = get_window_border_width() + max_window_height;
		window_width += vscrollbar_width;
	} else {
		view->set_size(requisition.width, requisition.height);
		window_height = get_window_border_width() + requisition.height;
	}
	
	gboolean button_hbox_visible = gtk_widget_get_visible(GTK_WIDGET(button_hbox));
	if (button_hbox_visible) {
		GtkAllocation allocation;
		gtk_widget_get_allocation(button_hbox, &allocation);
		window_height += allocation.height;
		if (window_width < allocation.width + get_window_border_width())
			window_width = allocation.width + get_window_border_width();
	}
}

void FloatWin::float_window_position(gboolean usePointerPosition,
	gint window_width, gint window_height, gint& x, gint& y)
{
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
	if (usePointerPosition) {
		GdkDisplay *display = gdk_screen_get_display(screen);
#if GTK_MAJOR_VERSION >= 3
		GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
		GdkDevice  *pointer = gdk_device_manager_get_client_pointer (device_manager);
		gdk_device_get_position (pointer, NULL, &x, &y);
#else
		gdk_display_get_pointer(display, NULL, &x, &y, NULL);
#endif
		x += FLOATWIN_OFFSET_X;
		y += FLOATWIN_OFFSET_Y;
	} else {
		gtk_window_get_position(GTK_WINDOW(FloatWindow),&x,&y);
	}
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);
	if (x + window_width > screen_width)
		x = screen_width - window_width;
	if (y + window_height > screen_height)
		y = screen_height - window_height;
}

void FloatWin::remember_pointer_position(void)
{
	/* TODO: the user of this object should tell why the floating windows has to be shown. */
	bool pressed = gpAppFrame->unlock_keys->is_pressed();

	if (pressed) {
		GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
		GdkDisplay *display = gdk_screen_get_display(screen);
#if GTK_MAJOR_VERSION >= 3
		GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
		GdkDevice  *pointer = gdk_device_manager_get_client_pointer (device_manager);
		gdk_device_get_position(pointer, NULL, &popup_pointer_x, &popup_pointer_y);
#else
		gdk_display_get_pointer(display, NULL, &popup_pointer_x, &popup_pointer_y, NULL);
#endif
	} else {
		// popup by middle click on the notification area icon, 
		// so never hiden the floating window even mouse didn't moved as in FloatWin::vHideWindowTimeOutCallback().
		popup_pointer_x = -1;
		popup_pointer_y = -1;
	}
}

void FloatWin::Popup(gboolean updatePosition)
{
	ismoving = true;
	gint window_width, window_height;
	float_window_size(window_width, window_height);

	if (conf->get_bool_at("floating_window/lock")) {
		gtk_window_resize(GTK_WINDOW(FloatWindow),window_width,window_height);
		// need to make window's resize relate to other corner?
	} else {
		gint iCurrentX,iCurrentY;
		gboolean newPosition = (!gtk_widget_get_visible(GTK_WIDGET(FloatWindow))) || updatePosition;
		float_window_position(newPosition, window_width, window_height, iCurrentX, iCurrentY);
		if (newPosition) {
			button_box_once_shown = false;
			remember_pointer_position();
		}
		// don't use gdk_window_resize,it make the window can't be smaller latter!
		// note: must do resize before move should be better,as the vHideWindowTimeOutCallback() may hide it.
		gtk_window_resize(GTK_WINDOW(FloatWindow),window_width,window_height);
		gtk_window_move(GTK_WINDOW(FloatWindow),iCurrentX,iCurrentY);
	}
	now_window_width = window_width;
	now_window_height = window_height;
	
	Show();
	ismoving = false;
}


void FloatWin::create_button_hbox(void)
{
#if GTK_MAJOR_VERSION >= 3
	button_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
	button_hbox = gtk_hbox_new(false,0);
#endif
	
	GtkWidget *button;
	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_FIND,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_query_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Query in the main window"));

	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_SAVE,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", 
			 G_CALLBACK(on_save_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Save to file"));

	PronounceWordButton= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(PronounceWordButton),gtk_image_new_from_stock(GTK_STOCK_EXECUTE,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (PronounceWordButton), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(PronounceWordButton),"clicked", G_CALLBACK(on_play_click), this);
	g_signal_connect(G_OBJECT(PronounceWordButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),PronounceWordButton,false,false,0);
	gtk_widget_set_tooltip_text(PronounceWordButton,_("Pronounce the word"));
	
	gtk_widget_set_sensitive(PronounceWordButton, false);

	StopButton= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(StopButton),gtk_image_new_from_stock(GTK_STOCK_STOP,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (StopButton), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(StopButton),"clicked", G_CALLBACK(on_stop_click), this);
	g_signal_connect(G_OBJECT(StopButton),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),StopButton,false,false,0);
	gtk_widget_set_tooltip_text(StopButton, _("Stop selection-scanning"));

	gtk_widget_set_sensitive(gpAppFrame->oFloatWin.StopButton, 
													 conf->get_bool_at("dictionary/scan_selection"));

#ifndef CONFIG_GPE
	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_HELP,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_help_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Help"));

	button= gtk_button_new();
	gtk_container_add(GTK_CONTAINER(button),gtk_image_new_from_stock(GTK_STOCK_QUIT,GTK_ICON_SIZE_MENU));
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_quit_click), this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_start(GTK_BOX(button_hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Quit"));
#endif

	button = gtk_button_new();

	LockImage= gtk_image_new_from_stock(get_lock_image_stock_id(),GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(button),LockImage);
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(vLockCallback),this);
	g_signal_connect(G_OBJECT(button),"enter_notify_event", G_CALLBACK(stardict_on_enter_notify), NULL);
	gtk_box_pack_end(GTK_BOX(button_hbox),button,false,false,0);
	gtk_widget_set_tooltip_text(button,_("Lock floating window"));
}

int FloatWin::get_distance_pointer_to_window(void)
{
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
	GdkDisplay *display = gdk_screen_get_display(screen);
	gint iCurrentX,iCurrentY;
#if GTK_MAJOR_VERSION >= 3
	GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
	GdkDevice  *pointer = gdk_device_manager_get_client_pointer (device_manager);
	gdk_device_get_position(pointer, NULL, &iCurrentX, &iCurrentY);
#else
	gdk_display_get_pointer(display, NULL, &iCurrentX, &iCurrentY, NULL);
#endif
	gint window_x,window_y,window_width,window_height;
	gtk_window_get_position(GTK_WINDOW(FloatWindow),&window_x,&window_y);
	//notice: gtk_window_get_size() is not really uptodate,don't use it! see "gtk reference".
	//gtk_window_get_size(GTK_WINDOW(FloatWindow),&window_width,&window_height);
	window_width = now_window_width;
	window_height = now_window_height;

	int distance;
	if (iCurrentX < window_x) {
		distance = (iCurrentX-window_x)*(iCurrentX-window_x);
		if (iCurrentY < window_y)
			distance += (iCurrentY-window_y)*(iCurrentY-window_y);
		else if (iCurrentY > window_y+window_height)
			distance += (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
	} else if (iCurrentX > window_x+window_width) {
		distance = (iCurrentX-window_x-window_width)*(iCurrentX-window_x-window_width);
		if (iCurrentY < window_y)
			distance += (iCurrentY-window_y)*(iCurrentY-window_y);
		else if ( iCurrentY > window_y+window_height )
			distance += (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
	} else {
		if (iCurrentY < window_y)
			distance = (window_y - iCurrentY)*(window_y - iCurrentY);
		else if (iCurrentY > window_y+window_height)
			distance = (iCurrentY-window_y-window_height)*(iCurrentY-window_y-window_height);
		else
			distance = 0; //in the floating window.
	}
	return distance;
}

void FloatWin::button_box_show_first_time(void)
{
	gint iCurrentX,iCurrentY;
	gtk_window_get_position(GTK_WINDOW(FloatWindow),&iCurrentX,&iCurrentY);
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);
	GtkRequisition requisition;
#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_preferred_size(button_hbox, NULL, &requisition);
#else
	gtk_widget_size_request(button_hbox,&requisition);
#endif
	now_window_height += requisition.height;
	requisition.width += get_window_border_width();
	if (requisition.width > now_window_width)
		now_window_width = requisition.width;
	bool changed=false;
	if (iCurrentX < 0) {
		iCurrentX = 0;
		changed = true;
	} else if (iCurrentX + now_window_width > screen_width) {
		iCurrentX = screen_width - now_window_width;
		changed = true;
	}
	if (iCurrentY < 0) {
		iCurrentY = 0;
		changed = true;
	} else if (iCurrentY + now_window_height > screen_height) {
		iCurrentY = screen_height - now_window_height;
		changed = true;
	}
	if (changed) {
		gtk_window_move(GTK_WINDOW(FloatWindow),iCurrentX,iCurrentY);
	}
}

void FloatWin::show_popup_menu(GdkEventButton * event)
{
	if (menu)
		gtk_widget_destroy(menu);
	menu = gtk_menu_new();

	GtkWidget *menuitem;
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Copy"));
	GtkWidget *image;
	image = gtk_image_new_from_stock(GTK_STOCK_COPY, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_copy_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Save"));
	image = gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_save_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Query"));
	image = gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_query_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (gtk_widget_get_sensitive(GTK_WIDGET(PronounceWordButton))) {
		menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Play"));
		image = gtk_image_new_from_stock(GTK_STOCK_EXECUTE, GTK_ICON_SIZE_MENU);
		gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
		gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_play_activate), this);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	menuitem = gtk_image_menu_item_new_with_mnemonic(_("_Fuzzy query"));
	image = gtk_image_new_from_stock(GTK_STOCK_FIND_AND_REPLACE, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem), image);
	gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(on_menu_fuzzyquery_activate), this);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	
	gtk_widget_show_all(menu);
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
	play_sound_on_event("menushow");
}

void FloatWin::set_transparent(int transparent)
{
	g_assert(0 <= transparent && transparent <= 100);
#if GTK_MAJOR_VERSION >= 3
	gtk_widget_set_opacity(GTK_WIDGET(FloatWindow), (100-transparent)/100.0);
#else
	gtk_window_set_opacity(GTK_WINDOW(FloatWindow), (100-transparent)/100.0);
#endif
}

void FloatWin::set_bg(void)
{
#if GTK_MAJOR_VERSION >= 3
	GdkRGBA color;
	const GdkRGBA *pcolor = NULL;
	if (conf->get_bool_at("floating_window/use_custom_bg")) {
		color.red = conf->get_double_at("floating_window/bg_red");
		color.green = conf->get_double_at("floating_window/bg_green");
		color.blue = conf->get_double_at("floating_window/bg_blue");
		color.alpha = 1;
		pcolor = &color;
	}
	gtk_widget_override_background_color(FloatWindow, GTK_STATE_FLAG_NORMAL, pcolor);
	view->modify_bg(GTK_STATE_FLAG_NORMAL, pcolor);
#else
	GdkColor color;
	const GdkColor *pcolor = NULL;
	if (conf->get_bool_at("floating_window/use_custom_bg")) {
		color.red = conf->get_int_at("floating_window/bg_red");
		color.green = conf->get_int_at("floating_window/bg_green");
		color.blue = conf->get_int_at("floating_window/bg_blue");
		pcolor = &color;
	}
	gtk_widget_modify_bg(FloatWindow, GTK_STATE_NORMAL, pcolor);
	view->modify_bg(GTK_STATE_NORMAL, pcolor);
#endif
}

void FloatWin::set_bookname_style(BookNameStyle style)
{
	if (view.get()) {
		view->set_bookname_style(style);
	}
}

const gchar* FloatWin::get_lock_image_stock_id(void)
{
	if (conf->get_bool_at("floating_window/lock"))
		return GTK_STOCK_GOTO_LAST;
	else
		return GTK_STOCK_GO_FORWARD;
}

/* Restore locked position of the window. When may this be usefull?
The floating window is hiden initially. */
void FloatWin::restore_locked_position(void)
{
	GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(FloatWindow));
	gint screen_width = gdk_screen_get_width(screen);
	gint screen_height = gdk_screen_get_height(screen);
	int max_window_width=
		conf->get_int_at("floating_window/max_window_width");
	int max_window_height=
		conf->get_int_at("floating_window/max_window_height");
	if (max_window_width < MIN_MAX_FLOATWIN_WIDTH || 
			max_window_width > screen_width)
		conf->set_int_at("floating_window/max_window_width",
									DEFAULT_MAX_FLOATWIN_WIDTH);
	if (max_window_height < MIN_MAX_FLOATWIN_HEIGHT ||
			max_window_height > screen_height)
		conf->set_int_at("floating_window/max_window_height", DEFAULT_MAX_FLOATWIN_HEIGHT);

	max_window_width=
		conf->get_int_at("floating_window/max_window_width");
	max_window_height=
		conf->get_int_at("floating_window/max_window_height");
	
	int lock_x=
		conf->get_int_at("floating_window/lock_x");
	int lock_y=
		conf->get_int_at("floating_window/lock_y");
	if (lock_x<0)
		lock_x=0;
	else if (lock_x > (screen_width - max_window_width))
		lock_x = screen_width - max_window_width;
	if (lock_y<0)
		lock_y=0;
	else if (lock_y > (screen_height - max_window_height))
		lock_y = screen_height - max_window_height;
	gtk_window_move(GTK_WINDOW(FloatWindow),lock_x,lock_y);
}

std::string FloatWin::get_not_found_markup(const gchar* sWord, const gchar* sReason)
{
	glib::CharStr str(g_markup_printf_escaped(
		"<b><big>%s</big></b>\n<span foreground=\"blue\">%s</span>", sWord, sReason));
	return get_impl(str);
}

std::string FloatWin::get_looking_up_markup(const gchar* sWord)
{
	glib::CharStr str(g_markup_printf_escaped(
		"<b><big>%s</big></b>\n<span foreground=\"blue\">%s</span>", sWord, _("Looking up...")));
	return get_impl(str);
}

std::string FloatWin::get_head_word_markup(const gchar* sWord)
{
	std::string markup = "<b><span size=\"x-large\">";
	glib::CharStr str(g_markup_escape_text(sWord, -1));
	markup += get_impl(str);
	markup += "</span></b>";
	return markup;
}

void FloatWin::set_busy_cursor(void)
{
	gdk_window_set_cursor(gtk_widget_get_window(FloatWindow), get_impl(gpAppFrame->oAppSkin.watch_cursor));
}

void FloatWin::set_normal_cursor(void)
{
	gdk_window_set_cursor(gtk_widget_get_window(FloatWindow), get_impl(gpAppFrame->oAppSkin.normal_cursor));
}

void FloatWin::start_hide_window_timer(void)
{
	if (!hide_window_timer)
		hide_window_timer = g_timeout_add(FLOAT_TIMEOUT, vHideWindowTimeOutCallback, this);
}

void FloatWin::destroy_hide_window_timer(void)
{
	if (hide_window_timer) {
		g_source_remove(hide_window_timer);
		hide_window_timer = 0;
	}
}

void FloatWin::start_lookup_running_timer(void)
{
	if (!lookup_running_timer)
		lookup_running_timer = g_timeout_add(LOOKUP_RUNNING_TIMEOUT, vLookupRunningTimeOutCallback, this);
}

void FloatWin::destroy_lookup_running_timer(void)
{
	if (lookup_running_timer) {
		g_source_remove(lookup_running_timer);
		lookup_running_timer = 0;
	}
}
