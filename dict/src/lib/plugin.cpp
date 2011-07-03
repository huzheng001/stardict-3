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

#include "plugin.h"

StarDictPluginSystemInfo oStarDictPluginSystemInfo;
StarDictPluginSystemService oStarDictPluginSystemService;


StarDictPlugInObject::StarDictPlugInObject()
{
	version_str = PLUGIN_SYSTEM_VERSION;
	type = StarDictPlugInType_UNKNOWN;
	info_xml = NULL;
	configure_func = NULL;
	plugin_info = &oStarDictPluginSystemInfo;
	plugin_service = &oStarDictPluginSystemService;
}

StarDictPlugInObject::~StarDictPlugInObject()
{
	g_free(info_xml);
}
