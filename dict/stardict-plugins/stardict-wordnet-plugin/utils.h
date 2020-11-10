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


#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstddef>
#include <math.h>

typedef float single;

#ifndef M_PI
#	define M_PI       (3.14159265358979323846f)
#endif
#ifndef M_2_PI
#	define M_2_PI       (2*M_PI)
#endif
#ifndef M_PI_2
#	define M_PI_2	(M_PI/2)
#endif
#ifndef M_PI_10
#	define M_PI_10	(M_PI/10)
#endif


#ifndef M_1_SQRT_3
#	define	M_1_SQRT_3		(0.57735026918962576450914878050196)
#endif


#define M_1_SQRT_2	(0.70710678118654757)

const single con_tol = 1e-3f;

template <class T>
inline T tsqr(const T t) {
  return t*t;
}

template <class T>
inline T tabs(const T t) {
  return t < 0 ? -t : t;
}

#endif //__TYPES_H__
