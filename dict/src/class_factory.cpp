/* 
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
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

#include "tray.h"
#include "libcommon.h"

#if defined(CONFIG_GNOME) || defined(CONFIG_MAEMO)
#  include "gconf_file.h"
#else
#  include "inifile.h"
#endif

#ifdef _WIN32
#  include "win32/win32_iskeyspressed.h"
#else
#ifdef CONFIG_DARWIN
#  include "gtk_iskeyspressed.h"
#else
#  include "x11_iskeyspressed.h"
#endif
#endif
#ifndef CONFIG_DARWIN
#  include "docklet.h"
#endif

#include "lib/utils.h"
#include "conf.h"
#include "class_factory.h"

void *PlatformFactory::create_class_by_name(const std::string& name, void *param)
{
	if (name=="config_file") {
#if defined(CONFIG_GNOME) || defined(CONFIG_MAEMO)
		return new gconf_file("/apps/stardict");
#else
		inifile *iniconf = new inifile();
		if(!iniconf->load(build_path(conf_dirs->get_user_config_dir(), "stardict.cfg")))
			exit(EXIT_FAILURE);
		return iniconf;
#endif
	} else if (name=="hotkeys") {
#ifdef _WIN32
		return new win32_hotkeys();
#else
#ifdef CONFIG_DARWIN
		return new gtk_hotkeys(GTK_WINDOW(param));
#else
		return new x11_hotkeys(GTK_WINDOW(param));
#endif
#endif
	}
	return NULL;
}

TrayBase *PlatformFactory::create_tray_icon(GtkWidget *win, bool scan,
					    const AppSkin& skin)
{
#ifdef CONFIG_DARWIN
	return new TrayBase(win, scan);
#else
	return new DockLet(win, scan, skin);
#endif
}
