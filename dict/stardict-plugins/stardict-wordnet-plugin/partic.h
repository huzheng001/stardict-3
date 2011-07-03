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


#ifndef __PARTIC_H__
#define __PARTIC_H__

#include "vector_t.h"
#include "geom.h"

class partic_t {
private:
	single _m;		// 质量
	vector_t _p;	// 位置
	vector_t _v;	// 速度
	vector_t _f;	// 受力，是一个合力，用于计算加速度
	tsize_t<single> _size;	// 宽度和高度
	bool _anchor;	// 锚定，不能移动了

public:
	partic_t(single am = 1, single w = 10, single h = 10): _m(am), _size(w, h), _anchor(false) {}
	single & getM() { return _m; }
	vector_t & getP() { return _p; }
	vector_t & getV() { return _v; }
	vector_t & getF() { return _f; }
	tsize_t<single> & get_size() { return _size; }
	point_t<single> get_left_top() { return point_t<single>(_p.x-_size.w/2, _p.y-_size.h/2); }

	// a templ var, calculated at fly
	vector_t getA() { vector_t v = _f; v.div(_m); return v; }
	rect_t<single> get_box(); // 包络矩形
	bool hit(const vector_t & b);
	void set_anchor(bool b) { _anchor = b; }
	bool get_anchor() { return _anchor; } 
};


#endif // __PARTIC_H__
