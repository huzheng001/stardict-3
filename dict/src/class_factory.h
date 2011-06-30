#ifndef _CLASS_FACTORY_HPP_
#define _CLASS_FACTORY_HPP_

#include <gtk/gtk.h>
#include <string>

class TrayBase;
class AppSkin;

class PlatformFactory {
public:
	static void *create_class_by_name(const std::string& name, void *param=NULL);
	static TrayBase *create_tray_icon(GtkWidget *, bool, const AppSkin&);
};

#endif//!_CLASS_FACTORY_HPP_
