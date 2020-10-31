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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstdlib>
#include <cstring>

#include "conf.h"
#include "lib/utils.h"

#include "skin.h"

struct StockIconInfo {
	const gchar *stock_id;
	const char *filename;
} stock_icons[] = {
{GTK_STOCK_ABOUT, "about"},
{GTK_STOCK_ADD, "add"},
{GTK_STOCK_CANCEL, "cancel"},
{GTK_STOCK_CLEAR, "clear"},
{GTK_STOCK_CLOSE, "close"},
{GTK_STOCK_CONVERT, "convert"},
{GTK_STOCK_COPY, "copy"},
{GTK_STOCK_DELETE, "delete"},
{GTK_STOCK_DIALOG_INFO, "info"},
{GTK_STOCK_DISCONNECT, "disconnect"},
{GTK_STOCK_DND, "dnd"},
{GTK_STOCK_EDIT, "edit"},
{GTK_STOCK_EXECUTE, "run"},
{GTK_STOCK_FIND, "find"},
{GTK_STOCK_FIND_AND_REPLACE, "replace"},
{GTK_STOCK_GO_DOWN, "go-down"},
{GTK_STOCK_GOTO_BOTTOM, "go-up"},
{GTK_STOCK_GOTO_FIRST, "go-first"},
{GTK_STOCK_GOTO_LAST, "go-last"},
{GTK_STOCK_GOTO_TOP, "go-top"},
{GTK_STOCK_GO_UP, "go-up"},
{GTK_STOCK_HARDDISK, "harddisk"},
{GTK_STOCK_HELP, "help"},
{GTK_STOCK_HOME, "home"},
{GTK_STOCK_INDEX, "index"},
{GTK_STOCK_INFO, "info"},
{GTK_STOCK_JUMP_TO, "jump"},
{GTK_STOCK_NETWORK, "network"},
{GTK_STOCK_OK, "ok"},
{GTK_STOCK_OPEN, "open"},
{GTK_STOCK_PREFERENCES, "preferences"},
{GTK_STOCK_PRINT, "print"},
{GTK_STOCK_PROPERTIES, "properties"},
{GTK_STOCK_QUIT, "exit"},
{GTK_STOCK_REFRESH, "refresh"},
{GTK_STOCK_REMOVE, "remove"},
{GTK_STOCK_SAVE, "save"},
{GTK_STOCK_SELECT_ALL, "select-all"},
{GTK_STOCK_SELECT_FONT, "font"},
{GTK_STOCK_STOP, "stop"},
{GTK_STOCK_UNDO, "undo"},
{GTK_STOCK_YES, "ok"},
{GTK_STOCK_ZOOM_FIT, "zoom-fit"},
{NULL, NULL}
};

SkinStorage::SkinStorage(const char *path)
{
	if(!path || !path[0])
		return;
	m_path = path;
	gchar *buf = NULL;
	if(g_file_get_contents(build_path(m_path, "name").c_str(),
		&buf, NULL, NULL)) {
		m_name = buf;
		g_free(buf);
	}
}

bool SkinStorage::is_valid() const
{
	return !m_name.empty();
}

const char *SkinStorage::get_name() const
{
	return m_name.c_str();
}

void SkinStorage::load_icon(Skin_pixbuf_1 &icon, const char *name) const
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(build_path(m_path, name).c_str(), NULL);
	if (pixbuf != NULL)
		icon.reset(pixbuf);
}

void SkinStorage::load_stock_icons(GtkIconFactory *factory) const
{
	for (int i = 0; stock_icons[i].stock_id != NULL; i++) {
		std::string filename = build_path(m_path, std::string("stock" G_DIR_SEPARATOR_S) + stock_icons[i].filename + ".png");
		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(filename.c_str(), NULL);
		if (pixbuf != NULL) {
			GtkIconSet *iconset = gtk_icon_set_new_from_pixbuf(pixbuf);
			gtk_icon_factory_add(factory, stock_icons[i].stock_id, iconset);
		}
	}
}

void SkinStorage::load_gtk_engine() const
{
	/* A new gtk_engine is loaded only on application startup,
	 * hence we may not care about growing value of the GTK_PATH variable. */
	const gchar *gtkpath = g_getenv("GTK_PATH");
	if (gtkpath == NULL)
		gtkpath = "";
	std::string new_gtkpath(gtkpath);
	if(!new_gtkpath.empty())
		new_gtkpath += G_SEARCHPATH_SEPARATOR_S;
	new_gtkpath += m_path;
	g_setenv("GTK_PATH", new_gtkpath.c_str(), 1);
	/* Maybe it would be better to simply load the new resource file, instead of 
	 * reloading the whole thing? Consider using gtk_rc_parse, 
	 * gtk_rc_add_default_file routines. */

#if GTK_MAJOR_VERSION >= 3
#else
	gchar *gtkrc_files[2] = {NULL, NULL};
	std::string gtkrc_path = build_path(m_path, "gtkrc");
	gtkrc_files[0] = (gchar *)gtkrc_path.c_str();
	gtk_rc_set_default_files(gtkrc_files);
	gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
#endif
}

void AppSkin::load()
{
	watch_cursor.reset(gdk_cursor_new(GDK_WATCH));
	std::string pixmaps_dir(build_path(conf_dirs->get_data_dir(), "pixmaps" G_DIR_SEPARATOR_S));
	std::string filename;
#ifdef _WIN32
	filename=pixmaps_dir+"stardict.png";
	icon.reset(load_image_from_file(filename));
#else // #ifdef _WIN32
	filename=build_path(conf_dirs->get_system_icon_dir(), "stardict.png");
	icon.reset(load_image_from_file(filename));
#endif // #ifdef _WIN32
#ifdef CONFIG_GPE
	filename=pixmaps_dir+"docklet_gpe_normal.png";
#else
	filename=pixmaps_dir+"docklet_normal.png";
#endif
	docklet_normal_icon.reset(load_image_from_file(filename));
#ifdef CONFIG_GPE
	filename=pixmaps_dir+"docklet_gpe_scan.png";
#else
	filename=pixmaps_dir+"docklet_scan.png";
#endif
	docklet_scan_icon.reset(load_image_from_file(filename));
#ifdef CONFIG_GPE
	filename=pixmaps_dir+"docklet_gpe_stop.png";
#else
	filename=pixmaps_dir+"docklet_stop.png";
#endif
	docklet_stop_icon.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"index_wazard.png";
	index_wazard.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"index_appendix.png";
	index_appendix.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"index_dictlist.png";
	index_dictlist.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"index_history.png";
	index_history.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"index_translate.png";
	index_translate.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"pronounce.png";
	pronounce.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"video.png";
	video.reset(load_image_from_file(filename));
	filename=pixmaps_dir+"attachment.png";
	attachment.reset(load_image_from_file(filename));
}

void AppSkin::load(const std::string &path)
{
	load();
	SkinStorage storage(path.c_str());
	if (! storage.is_valid())
		return;
	storage.load_icon(index_wazard, "index_wazard.png");
	storage.load_icon(index_appendix, "index_appendix.png");
	storage.load_icon(index_dictlist, "index_dictlist.png");
	storage.load_icon(index_translate, "index_translate.png");
	storage.load_icon(pronounce, "pronounce.png");
	storage.load_icon(video, "video.png");
	storage.load_icon(attachment, "attachment.png");
	GtkIconFactory *factory = gtk_icon_factory_new();
	storage.load_stock_icons(factory);
	gtk_icon_factory_add_default(factory);
	storage.load_gtk_engine();
}
