//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------


#ifndef __PARTIC_H__
#define __PARTIC_H__

#include "vector_t.hpp"
#include "geom.hpp"

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
