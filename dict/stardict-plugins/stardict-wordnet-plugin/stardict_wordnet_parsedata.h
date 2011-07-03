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

#ifndef _STARDICT_WORDNET_PARSEDATA_PLUGIN_H_
#define _STARDICT_WORDNET_PARSEDATA_PLUGIN_H_

#ifdef _WIN32
#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */
#else
# define DLLIMPORT
#endif

#include "../../src/lib/plugin.h"
#include "../../src/lib/parsedata_plugin.h"
#include "../../src/lib/iappdirs.h"

extern "C" {
	DLLIMPORT extern bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs);
	DLLIMPORT extern void stardict_plugin_exit(void);
	DLLIMPORT extern bool stardict_parsedata_plugin_init(StarDictParseDataPlugInObject *obj);
}

#endif
