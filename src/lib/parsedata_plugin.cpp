#include "parsedata_plugin.h"

ParseResultItem::ParseResultItem()
{
	type = ParseResultItemType_unknown;
}

ParseResultItem::~ParseResultItem()
{
	if (type == ParseResultItemType_mark) {
		delete mark;
	} else if (type == ParseResultItemType_link) {
		delete link;
	} else if (type == ParseResultItemType_res) {
		delete res;
	}
}

StarDictParseDataPlugInObject::StarDictParseDataPlugInObject()
{
	parse_func = 0;
}
