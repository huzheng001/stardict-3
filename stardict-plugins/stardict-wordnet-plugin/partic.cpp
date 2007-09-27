//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------

#include "partic.hpp"

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
