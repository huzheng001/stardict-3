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
//	author: bianpeng
//	date: 2007-05-09
//---------------------------------

#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "utils.h"
#include <math.h>

struct vector_t {
public:
	single x;
	single y;
	single z;
	single w;

	vector_t():x(0), y(0), z(0), w(1.0) {}
	vector_t(single len, single rad): x(len*cos(rad)), y(len*sin(rad)), z(0), w(1.0) { }
	//------------------------------------------------------------------------------
	// construct
	//------------------------------------------------------------------------------
	vector_t(single ax, single ay, single az): x(ax), y(ay), z(az), w(1.0) {}
	//------------------------------------------------------------------------------
	//  property
	//------------------------------------------------------------------------------
	single length() const { return sqrt(powerlength()); }
	single powerlength() const { return (x*x+y*y+z*z)/*/(w*w)*/; }
	//------------------------------------------------------------------------------
	//  action
	//------------------------------------------------------------------------------
	void add(const vector_t & b) {
		x+=b.x;y+=b.y;z+=b.z;
	}

	void reset(single len, single rad) {
		x=(len*cos(rad));
		y=(len*sin(rad));
		z=0; w=1;
	}

	single angle() const {
		single l = this->length();
		if (l < con_tol)
			return 0;
		single r = acos(this->x/l);
		if (y < 0)
			r = M_2_PI-r;
		return r;
	}

	single angle(const vector_t & b) {
		single r = tabs<single>(b.angle() - this->angle());
		if (r > M_PI)
			r = M_2_PI - r;
		return r;
	}

	// project to b
	single proj(const vector_t & b) {
		return cos(this->angle(b));
	}

	// morrir on b
	void mirror(const vector_t & b) {
		single l = this->length();
		single anglea = this->angle();
		single angleb = b.angle();
		anglea = angleb + (angleb-anglea);
		x = l*cos(anglea);
		y = l*sin(anglea);
	}

	void rot(const single rad) {
		single l = this->length();
		single a = this->angle();
		a += rad;
		x = l*cos(a);
		y = l*sin(a);
	}

	void sub(const vector_t & b) {
		x-=b.x;y-=b.y;z-=b.z;
	}

	vector_t & mul(const single s) {
		x*=s;y*=s;z*=s;
		return *this;
	}

	vector_t & div(const single s) {
		mul(1/s);
		return *this;
	}

	void scaleto(const single l) {
		set_length(l);
	}

	void set_length(const single l) {
		this->norm();
		this->mul(l);
	}

	void norm() {
		if (! this->equals(zero)) {
			single l = this->length();
			x/=l;y/=l;z/=l;
		}
		else {
			x=y=(single)(M_1_SQRT_2);
			z=0;
		}
	}

	single powerdistance(const vector_t & b) {
		return (tsqr<single>(b.x-x)+tsqr<single>(b.y-y)+tsqr<single>(b.z-z));
	}

	single distance(const vector_t & b) {
		return sqrt(powerdistance(b));
	}

	bool equals(const vector_t & b) const {
		return (tabs(b.x-x)+tabs(b.y-y)+tabs(b.z-z)) < con_tol; 
	}

	bool iszero() {
		return this->equals(zero);
	}

	inline vector_t operator + (const vector_t & b) const {
		vector_t v(*this);
		v.add(b);
		return v;
	}

	inline vector_t & operator += (const vector_t & b) {
		this->operator +(b);
		return *this;
	}

	inline vector_t operator - (const vector_t & b) const {
		vector_t v(*this);
		v.sub(b);
		return v;
	}

	inline vector_t & operator -= (const vector_t & b) {
		this->operator -(b);
		return *this;
	}

	inline vector_t operator *(const single s) {
		vector_t v(*this);
		v.mul(s);
		return v;
	}

	static const vector_t zero;
};

inline bool operator == (const vector_t & a, const vector_t & b) {
	return a.equals(b);
}

#endif   //__VECTOR_H__


