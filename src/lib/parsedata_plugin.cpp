#include "parsedata_plugin.h"

ParseResult::~ParseResult()
{
	clear();
}

void ParseResult::clear()
{
	for (std::list<ParseResultItem>::iterator i = item_list.begin(); i != item_list.end(); ++i) {
		if (i->type == ParseResultItemType_mark) {
			delete i->mark;
		} else if (i->type == ParseResultItemType_link) {
			delete i->link;
		} else if (i->type == ParseResultItemType_res) {
			delete i->res;
		} else if (i->type == ParseResultItemType_widget) {
			delete i->widget;
		}
	}
	item_list.clear();
}

StarDictParseDataPlugInObject::StarDictParseDataPlugInObject()
{
	parse_func = 0;
}
