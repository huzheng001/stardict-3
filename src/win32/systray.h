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

enum DockLetIconType {
	DOCKLET_NORMAL_ICON,
	DOCKLET_SCAN_ICON,
	DOCKLET_STOP_ICON,
};

class DockLet : public TrayBase {
public:	
	DockLet(GtkWidget *mainwin);
	~DockLet();
	void Create();
	void SetIcon(DockLetIconType icon_type);	
	void minimize_to_tray();
	void maximize_from_tray();
private:
	DockLetIconType current_icon;
	HWND systray_hwnd;
	HMENU systray_menu; // gtk menu don't work fine here.
	NOTIFYICONDATA stardict_nid;
	HICON sysicon_normal;
	HICON sysicon_scan;
	HICON sysicon_stop;
	
	HWND systray_create_hiddenwin();
	void systray_create_menu();
	void systray_show_menu(int x, int y);
	void systray_init_icon(HWND hWnd, HICON icon);
	void systray_change_icon(HICON icon, char* text);
	static LRESULT CALLBACK systray_mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);		
};

#endif /* _SYSTRAY_H_ */
