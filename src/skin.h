#ifndef __SD_SKIN_H__
#define __SD_SKIN_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "lib/utils.h"

typedef ResourceWrapper<GdkCursor, GdkCursor, gdk_cursor_unref> SkinCursor;
typedef ResourceWrapper<GdkPixbuf, void, g_object_unref> Skin_pixbuf_1;

class SkinStorage {
private:
	std::string m_path, m_name;
public:
	SkinStorage(const char *path);
	bool is_valid() const;
	const char *get_name() const;
	
	void load_icon(Skin_pixbuf_1 &icon, const char *name) const;
	void load_stock_icons(GtkIconFactory *factory) const;
	void load_gtk_engine() const;
};

class AppSkin {
public:
	int width,height;
	SkinCursor normal_cursor;
	SkinCursor watch_cursor;
	Skin_pixbuf_1 icon;
#ifndef _WIN32
	Skin_pixbuf_1 docklet_normal_icon;
	Skin_pixbuf_1 docklet_scan_icon;
	Skin_pixbuf_1 docklet_stop_icon;
#endif
	Skin_pixbuf_1 index_wazard;
	Skin_pixbuf_1 index_appendix;
	Skin_pixbuf_1 index_dictlist;
	Skin_pixbuf_1 index_translate;
	Skin_pixbuf_1 pronounce;

	void load();
	void load(const std::string &path);
};

#endif
