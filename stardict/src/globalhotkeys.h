#ifndef __SD_GLOBALHOTKEYS_H__
#define __SD_GLOBALHOTKEYS_H__

#include <string>

#if defined(_WIN32)
#include "win32/hotkey.h"
#endif

class GlobalHotkeys
{
private:
	std::string scan_key, mw_key;
public:
	void Init();
	void End();
	void start_scan(const char *hotkey);
	void stop_scan();
	void start_mainwindow(const char *hotkey);
	void stop_mainwindow();
};

#endif
