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
		} else if (i->type == ParseResultItemType_FormatBeg) {
			delete i->format_beg;
		} else if (i->type == ParseResultItemType_FormatEnd) {
			delete i->format_end;
		}
	}
	item_list.clear();
}

StarDictParseDataPlugInObject::StarDictParseDataPlugInObject()
{
	parse_func = 0;
}
