#ifndef _STARDICT_DICT_MANAGE_H_
#define _STARDICT_DICT_MANAGE_H_

#include <list>
#include <string>

enum DictManageItemType {
	LOCAL_DICT,
	VIRTUAL_DICT,
	NET_DICT,
};

struct DictManageItem {
	DictManageItemType type;
	bool enable;
	std::string file_or_id;
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

extern void DictConfigXmlToInfo(const char *configxml, DictManageInfo &info);
extern void LoadDictInfo(const std::list<std::string> &plugin_new_install_list);
extern void GetDictList(std::list<std::string> &dict_list);
extern void UpdateDictMask();
extern void UpdatePluginList(std::list<std::string> &plugin_new_install_list);

#endif
