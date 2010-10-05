#include "globalhotkeys.h"
#include "stardict.h"
#include "tomboykeybinder.h"

void ToggleScan()
{
	conf->set_bool_at("dictionary/scan_selection",
					!conf->get_bool_at("dictionary/scan_selection"));
}

void ShowMainwindow()
{
	gpAppFrame->oDockLet->maximize_from_tray();
	if (gpAppFrame->oTopWin.get_text()[0]) {
		 //so user can input word directly.
		gtk_widget_grab_focus(gpAppFrame->oMidWin.oTextWin.view->widget());
	} else {
		 //this won't change selection text.
		gpAppFrame->oTopWin.grab_focus();
	}
}

// Win32 implementation
#if defined(_WIN32)

void GlobalHotkeys::Init()
{
	win32hotkey_Init(2);
}

void GlobalHotkeys::End()
{
	win32hotkey_End();
}

void GlobalHotkeys::start_scan(const char *hotkey)
{
	guint keyval;
	GdkModifierType mods;
	gtk_accelerator_parse(hotkey, &keyval, &mods);
	win32hotkey_enable(0, ToggleScan, keyval, mods);
}

void GlobalHotkeys::stop_scan()
{
	win32hotkey_disable(0);
}

void GlobalHotkeys::start_mainwindow(const char *hotkey)
{
	guint keyval;
	GdkModifierType mods;
	gtk_accelerator_parse(hotkey, &keyval, &mods);
	win32hotkey_enable(1, ShowMainwindow, keyval, mods);
}

void GlobalHotkeys::stop_mainwindow()
{
	win32hotkey_disable(1);
}

// MacOS implementation
#elif defined(CONFIG_DARWIN)

void GlobalHotkeys::Init()
{
}

void GlobalHotkeys::End()
{
}

void GlobalHotkeys::start_scan(const char *key)
{
}

void GlobalHotkeys::stop_scan()
{
}

void GlobalHotkeys::start_mainwindow(const char *key)
{
}

void GlobalHotkeys::stop_mainwindow()
{
}

// X11 implementation
#else

void GlobalHotkeys::Init()
{
	tomboy_keybinder_init();
}

void GlobalHotkeys::End()
{
}

void GlobalHotkeys::start_scan(const char *hotkey)
{
	scan_key = hotkey;
	tomboy_keybinder_bind(hotkey, ToggleScan, NULL);
}

void GlobalHotkeys::stop_scan()
{
	tomboy_keybinder_unbind(scan_key.c_str(), ToggleScan);
}

void GlobalHotkeys::start_mainwindow(const char *hotkey)
{
	mw_key = hotkey;
	tomboy_keybinder_bind(hotkey, ShowMainwindow, NULL);
}

void GlobalHotkeys::stop_mainwindow()
{
	tomboy_keybinder_unbind(mw_key.c_str(), ShowMainwindow);
}

#endif
