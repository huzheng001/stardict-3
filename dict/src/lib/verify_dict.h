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

#ifndef _VERIFY_DICT_H_
#define _VERIFY_DICT_H_

#include <list>
#include <string>

/* Verify dictionaries before loading them in StarDict.
 * Corrupted dictionaries may crash StarDict. */

class show_progress_t;

void filter_verify_dicts(const std::list<std::string>& dict_all_list,
	std::list<std::string>& dict_valid_list, show_progress_t *sp);

#endif // _VERIFY_DICT_H_
