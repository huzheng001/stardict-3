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

#ifndef _STARDICT_NETDICT_PLUGIN_H_
#define _STARDICT_NETDICT_PLUGIN_H_

struct NetDictResponse {
	~NetDictResponse();
	const char *bookname;
	const char *booklink;
	char *word;
	char *data;
};

struct StarDictNetDictPlugInObject{
	StarDictNetDictPlugInObject();

	typedef void (*lookup_func_t)(const char *word, bool ismainwin);
	lookup_func_t lookup_func;
	const char *dict_name;
	const char *dict_link;
	const char *dict_cacheid;
};

#endif
