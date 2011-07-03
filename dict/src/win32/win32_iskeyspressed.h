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

#ifndef _WIN32_ISKEYSPRESSED_HPP_
#define _WIN32_ISKEYSPRESSED_HPP_

#include "../iskeyspressed.h"

class win32_hotkeys : public hotkeys {
public:
	win32_hotkeys();
	const std::list<std::string>& possible_combs();
	void set_comb(const std::string& comb);
	bool is_pressed();
private:
  int comb_mask;
  static std::list<std::string> posb_combs;
};

#endif//!_WIN32_ISKEYSPRESSED_HPP_
