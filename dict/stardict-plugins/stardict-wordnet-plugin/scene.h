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

#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include <cstddef>

#include "partic.h"
#include "spring.h"

#include <vector>
#include <cstring>
using namespace std;

class scene_t {
private:
	vector<partic_t *> _partics;
	vector<spring_t *> _springs;
	partic_t * _c;
public:
	scene_t(): _c(NULL) {}
	partic_t * create_partic(const single m, single w = 10, single h = 10);
	spring_t * create_spring(partic_t & a, partic_t & b, float orilen, float k=0.1);
	void clear();

public:
	vector<partic_t *> & get_partics() { return _partics; }
	vector<spring_t *> & get_springs() { return _springs; }

	size_t get_partic_count() { return _partics.size(); }
	size_t get_spring_count() { return _springs.size(); }
	bool hit(const vector_t & b, /*out*/ partic_t ** out);
	void center_to(const vector_t & c);
	void set_center(partic_t * p);
	partic_t * get_center() { return _c; }
	void pan(const vector_t & d);
	bool checkstable();
};

#endif //__PHYSICS_H__

