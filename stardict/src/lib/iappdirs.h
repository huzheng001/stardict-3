#ifndef _IAPPDIRS_H_
#define _IAPPDIRS_H_

#include <gtk/gtk.h>
#include <string>

class IAppDirs
{
public:
	virtual std::string get_user_config_dir(void) const = 0;
};

#endif
