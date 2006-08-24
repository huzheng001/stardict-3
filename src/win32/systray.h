/*
 *  systray.h
 *
 *  Author: Herman Bloggs <hermanator12002@yahoo.com>
 *  Date: November, 2002
 *  Description: Gaim systray functionality
 */

// Come from gaim. changed by Hu Zheng <huzheng_001@163.com> for StarDict.
// http://stardict.sourceforge.net 2003.09.22

#ifndef _SYSTRAY_H_
#define _SYSTRAY_H_

#include <windows.h>
#include <gtk/gtk.h>
#include "tray.hpp"

class DockLet : public TrayBase {
public:	
	DockLet(GtkWidget *, bool);
	~DockLet();	
	void minimize_to_tray();
	void maximize_from_tray();
private:	
	HWND systray_hwnd;
	HMENU systray_menu; // gtk menu don't work fine here.
	NOTIFYICONDATA stardict_nid;
	HICON normal_icon_;
	HICON scan_icon_;
	HICON stop_icon_;
	
	HWND create_hiddenwin();
	void create_menu();
	void show_menu(int x, int y);
	void init_icon(HWND hWnd, HICON icon);
	void change_icon(HICON icon, char* text);
	static LRESULT CALLBACK mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	void create();
	void scan_on();
	void scan_off();
	void show_normal_icon();
	void on_left_btn();
};

#endif /* _SYSTRAY_H_ */
