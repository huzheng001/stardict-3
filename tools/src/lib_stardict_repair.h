/*
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

#ifndef _LIB_STARDICT_REPAIR_H_
#define _LIB_STARDICT_REPAIR_H_

#include <string>

struct RepairOptions
{
	bool lot_of_memory;
	bool compress_dict;
	bool copy_res_store;
};

int stardict_repair(const std::string& ifofilepath, const std::string& outdirpath,
		RepairOptions options);

#endif // _LIB_STARDICT_REPAIR_H_
