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

#ifndef _STARDICT_DICT_MANAGE_H_
#define _STARDICT_DICT_MANAGE_H_

#include <list>
#include <string>
#include "lib/dictitemid.h"
#include "lib/utils.h"

enum DictManageItemType {
	LOCAL_DICT,
	VIRTUAL_DICT,
	NET_DICT,
};

struct DictManageItem {
	DictManageItemType type;
	bool enable;
	/* When type is LOCAL_DICT, file_or_id is path of the ifo file.
	When type is VIRTUAL_DICT or NET_DICT, file_or_id is path of the library file 
	implementing the respective plug-in. */
	DictItemId file_or_id;
};

struct DictManageGroup {
	std::string name;
	std::list<DictManageItem> querydict;
	std::list<DictManageItem> scandict;
};

struct DictManageInfo {
	std::list<DictManageGroup>::iterator get_active_group(void);
	std::list<DictManageGroup>::iterator set_active_group(const std::string& new_group);
	std::list<DictManageGroup> groups;
	std::string active_group;
};

class StarDictPlugins;
class show_progress_t;

extern void LoadDictInfo();
extern void GetUsedDictList(std::list<DictItemId> &dict_list);
extern void UpdateDictMask();
extern void UpdateDictList(std::list<DictItemId> &dict_new_install_list, show_progress_t *sp, bool verify_dict);
extern void UpdatePluginList(std::list<DictItemId> &plugin_new_install_list);
extern void UpdateConfigXML(
	const std::list<DictItemId> &dict_new_install_list,
	const std::list<DictItemId> &plugin_new_install_list,
	const StarDictPlugins* oStarDictPlugins);
extern void RemoveCacheFiles(void);

#endif
