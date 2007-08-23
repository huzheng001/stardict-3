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
	std::list<DictManageGroup> groups;
	std::string active_group;
};

extern void DictConfigXmlToInfo(const char *configxml, DictManageInfo &info);
extern void LoadDictInfo();
extern void GetDictList(std::list<std::string> &dict_list);
extern void UpdateDictMask();

#endif
