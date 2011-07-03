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

#ifndef _DICTITEMID_H_
#define _DICTITEMID_H_

#include <string>
#include <list>
#include "utils.h"
#include "libcommon.h"

class DictItemId {
public:
	DictItemId() {}
	DictItemId(const DictItemId& right)
		: id(right.id)
	{
	}
	DictItemId(const std::string& id)
		: id(id)
	{
	}
	const char* c_str() const
	{
		return id.c_str();
	}
	const std::string& str() const
	{
		return id;
	}
	bool operator==(const DictItemId& right) const
	{
		return is_equal_paths(id, right.id);
	}
	static void convert(std::list<std::string>& to, const std::list<DictItemId>& from)
	{
		to.clear();
		for(std::list<DictItemId>::const_iterator it=from.begin(); it!=from.end(); ++it)
			to.push_back(it->str());
	}
	static void convert(std::list<DictItemId>& to, const std::list<std::string>& from)
	{
		to.clear();
		for(std::list<std::string>::const_iterator it=from.begin(); it!=from.end(); ++it)
			to.push_back(DictItemId(*it));
	}
private:
	std::string id;
};

#endif // _DICTITEMID_H_
