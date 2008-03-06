#ifndef __SD_HOTKEY_H__
#define __SD_HOTKEY_H__

#include <windows.h>
#include <gdk/gdktypes.h>
#include <gdk/gdkkeysyms.h>

typedef void (*HotkeyHandler)();

//class Hotkey
//{
//private:

//public:
	void win32hotkey_Init(int nhotkey);
	void win32hotkey_End();
	void win32hotkey_enable(int index, HotkeyHandler handler, guint key, GdkModifierType modifiers);
	void win32hotkey_disable(int index);
//};

#endif
