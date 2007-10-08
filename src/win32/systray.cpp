#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gdk/gdkwin32.h>
#include <glib/gi18n.h>
#include "resource.h"
#include "MinimizeToTray.h"

#include "systray.h"

#ifndef LR_VGACOLOR
//if use gcc on windows this constant not defined
#  define LR_VGACOLOR         0x0080
#endif
extern HINSTANCE stardictexe_hInstance;

#define WM_TRAYMESSAGE WM_USER /* User defined WM Message */

enum SYSTRAY_CMND {
	SYSTRAY_CMND_MENU_QUIT=100,
	SYSTRAY_CMND_MENU_SCAN,
};


DockLet::DockLet(GtkWidget *mainwin, bool is_scan_on) :
		TrayBase(mainwin, is_scan_on)
{
	create();
}

static bool is_windows_xp()
{
	if (!G_WIN32_IS_NT_BASED ())
		return false;

    OSVERSIONINFO version;

    memset(&version, 0, sizeof(version));
    version.dwOSVersionInfoSize = sizeof(version);
    return GetVersionEx (&version)
      && version.dwPlatformId == VER_PLATFORM_WIN32_NT
      && (version.dwMajorVersion > 5
          || (version.dwMajorVersion == 5 && version.dwMinorVersion >= 1));  
}

void DockLet::create()
{
	systray_hwnd = create_hiddenwin();
	::SetWindowLongPtr(systray_hwnd, GWL_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));

	create_menu();
	UINT fuLoad = 0;
	if (!is_windows_xp())
		fuLoad = LR_VGACOLOR;
	/* Load icons, and init systray notify icon */
	normal_icon_ = (HICON)LoadImage(stardictexe_hInstance,
		MAKEINTRESOURCE(STARDICT_NORMAL_TRAY_ICON),
		IMAGE_ICON, 16, 16, fuLoad);
	scan_icon_ = (HICON)LoadImage(stardictexe_hInstance,
		MAKEINTRESOURCE(STARDICT_SCAN_TRAY_ICON),
		IMAGE_ICON, 16, 16, fuLoad);
	stop_icon_ = (HICON)LoadImage(stardictexe_hInstance,
		MAKEINTRESOURCE(STARDICT_STOP_TRAY_ICON),
		IMAGE_ICON, 16, 16, fuLoad);

	/* Create icon in systray */
	init_icon(systray_hwnd, normal_icon_);
}

/* Create hidden window to process systray messages */
HWND DockLet::create_hiddenwin()
{
	WNDCLASSEX wcex;
	TCHAR wname[32];

	strcpy(wname, "StarDictSystray");

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style	        = 0;
	wcex.lpfnWndProc	= (WNDPROC)mainmsg_handler;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= stardictexe_hInstance;
	wcex.hIcon		= NULL;
	wcex.hCursor		= NULL,
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= wname;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);

	// Create the window
	return (CreateWindow(wname, "", 0, 0, 0, 0, 0, GetDesktopWindow(), NULL, stardictexe_hInstance, 0));
}

void DockLet::create_menu()
{
	char* locenc = NULL;

	/* create popup menu */
	if((systray_menu = CreatePopupMenu())) {
		AppendMenu(systray_menu, MF_CHECKED, SYSTRAY_CMND_MENU_SCAN,
			       (locenc=g_locale_from_utf8(_("Scan"), -1, NULL, NULL, NULL)));
		g_free(locenc);
		AppendMenu(systray_menu, MF_SEPARATOR, 0, 0);
		AppendMenu(systray_menu, MF_STRING, SYSTRAY_CMND_MENU_QUIT,
			       (locenc=g_locale_from_utf8(_("Quit"), -1, NULL, NULL, NULL)));
		g_free(locenc);
	}
}

void DockLet::show_menu(int x, int y)
{
	/* need to call this so that the menu disappears if clicking outside
           of the menu scope */
	SetForegroundWindow(systray_hwnd);

  if (is_scan_on())
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(systray_menu, SYSTRAY_CMND_MENU_SCAN, MF_BYCOMMAND | MF_UNCHECKED);

	TrackPopupMenu(systray_menu,         // handle to shortcut menu
		       TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_LEFTBUTTON,
		       x,                   // horizontal position, in screen coordinates
		       y,                   // vertical position, in screen coordinates
		       0,                   // reserved, must be zero
		       systray_hwnd,        // handle to owner window
		       NULL                 // ignored
		       );
}

void DockLet::on_left_btn()
{
	if (GTK_WIDGET_VISIBLE(mainwin_)) {
		HWND hwnd =
			static_cast<HWND>(GDK_WINDOW_HWND(mainwin_->window));
		if (IsIconic(hwnd))
			ShowWindow(hwnd, SW_RESTORE);
		else
			minimize_to_tray();
	} else {
		maximize_from_tray();
        on_maximize_.emit();
	}
}

LRESULT CALLBACK DockLet::mainmsg_handler(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	static UINT taskbarRestartMsg; /* static here means value is kept across multiple calls to this func */
	DockLet *dock =
        reinterpret_cast<DockLet *>(::GetWindowLongPtr(hwnd, GWL_USERDATA));

	switch (msg) {
	case WM_CREATE:
		taskbarRestartMsg = RegisterWindowMessage("TaskbarCreated");
		break;

	case WM_COMMAND:
		switch(LOWORD(wparam)) {
		case SYSTRAY_CMND_MENU_SCAN:
			if (GetMenuState(dock->systray_menu, SYSTRAY_CMND_MENU_SCAN,
					MF_BYCOMMAND) & MF_CHECKED)
				dock->on_change_scan_.emit(false);
			else
				dock->on_change_scan_.emit(true);

			break;
		case SYSTRAY_CMND_MENU_QUIT:
			dock->on_quit_.emit();
			break;
		}
		break;
	case WM_TRAYMESSAGE:
	{
		if ( lparam == WM_LBUTTONDOWN ) {
			if (GetKeyState(VK_CONTROL) < 0)
				dock->on_change_scan_.emit(!dock->is_scan_on());
		} else if (lparam == WM_LBUTTONDBLCLK) {
			// Only use left button will conflict with the menu.
			dock->on_left_btn();
		} else if (lparam == WM_MBUTTONDOWN) {
			dock->on_middle_btn_click_.emit();
		} else if (lparam == WM_RBUTTONUP) {
			/* Right Click */
			POINT mpoint;

			GetCursorPos(&mpoint);
			dock->show_menu(mpoint.x, mpoint.y);
		}
		break;
	}
	default:
		if (msg == taskbarRestartMsg) {
			/* explorer crashed and left us hanging...
			   This will put the systray icon back in it's place, when it restarts */
			Shell_NotifyIcon(NIM_ADD, &dock->stardict_nid);
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}


void DockLet::init_icon(HWND hWnd, HICON icon)
{
	char* locenc=NULL;

	ZeroMemory(&stardict_nid,sizeof(stardict_nid));
	stardict_nid.cbSize=sizeof(NOTIFYICONDATA);
	stardict_nid.hWnd=hWnd;
	stardict_nid.uID=0;
	stardict_nid.uFlags=NIF_ICON | NIF_MESSAGE | NIF_TIP;
	stardict_nid.uCallbackMessage=WM_TRAYMESSAGE;
	stardict_nid.hIcon=icon;
	locenc=g_locale_from_utf8(_("StarDict"), -1, NULL, NULL, NULL);
	strcpy(stardict_nid.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_ADD,&stardict_nid);
}

void DockLet::change_icon(HICON icon, char* text)
{
	char *locenc=NULL;
	stardict_nid.hIcon = icon;
	locenc = g_locale_from_utf8(text, -1, NULL, NULL, NULL);
	lstrcpy(stardict_nid.szTip, locenc);
	g_free(locenc);
	Shell_NotifyIcon(NIM_MODIFY,&stardict_nid);
}

DockLet::~DockLet()
{
	Shell_NotifyIcon(NIM_DELETE,&stardict_nid);
	DestroyMenu(systray_menu);
	DestroyWindow(systray_hwnd);
}

void DockLet::minimize_to_tray()
{
	MinimizeWndToTray((HWND)(GDK_WINDOW_HWND(mainwin_->window)));
	TrayBase::minimize_to_tray();
}

void DockLet::maximize_from_tray()
{
	if (!GTK_WIDGET_VISIBLE(mainwin_))
		RestoreWndFromTray((HWND)(GDK_WINDOW_HWND(mainwin_->window)));
	TrayBase::maximize_from_tray();
}

void DockLet::scan_on()
{
        change_icon(scan_icon_, _("StarDict - Scanning"));
}

void DockLet::scan_off()
{
        change_icon(stop_icon_, _("StarDict - Stopped"));
}

void DockLet::show_normal_icon()
{
        change_icon(normal_icon_, _("StarDict"));
}
