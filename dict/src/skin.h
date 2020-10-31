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

#ifndef __SD_SKIN_H__
#define __SD_SKIN_H__

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#include "lib/utils.h"
#include "libcommon.h"

#if GTK_MAJOR_VERSION >= 3
typedef ResourceWrapper<GdkCursor, void*, void, g_object_unref> SkinCursor;
#else
typedef ResourceWrapper<GdkCursor, GdkCursor*, void, gdk_cursor_unref> SkinCursor;
#endif
typedef ResourceWrapper<GdkPixbuf, void*, void, g_object_unref> Skin_pixbuf_1;

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
	Skin_pixbuf_1 docklet_normal_icon;
	Skin_pixbuf_1 docklet_scan_icon;
	Skin_pixbuf_1 docklet_stop_icon;
	Skin_pixbuf_1 index_wazard;
	Skin_pixbuf_1 index_appendix;
	Skin_pixbuf_1 index_dictlist;
	Skin_pixbuf_1 index_history;
	Skin_pixbuf_1 index_translate;
	Skin_pixbuf_1 pronounce;
	Skin_pixbuf_1 video;
	Skin_pixbuf_1 attachment;

	void load();
	void load(const std::string &path);
};

#endif
