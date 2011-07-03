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

//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------

#include "partic.h"

rect_t<single> partic_t::get_box() {
	return rect_t<single>(_p.x, _p.y, _size.w, _size.h);
}

///-------------------------------------------------
// 只考虑平面先
///-------------------------------------------------
bool partic_t::hit(const vector_t & b) {
	return ((tabs<single>(b.x-_p.x) < _size.w/2) && 
		(tabs<single>(b.y-_p.y) < _size.h/2));
}
