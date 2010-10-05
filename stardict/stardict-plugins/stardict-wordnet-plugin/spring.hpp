//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------

#ifndef __SPRING_H__
#define __SPRING_H__


#include "partic.hpp"

class spring_t {
private:
	partic_t & _a;
	partic_t & _b;
	single _len;	// 原始长度
	single _k;		// 弹簧系数
public:
	static const int node_count = 2;
	//--------------------------------------------------------------------------
	// constructor
	//--------------------------------------------------------------------------
	spring_t(partic_t & a, partic_t & b, single len, single k = 0.1f): _a(a), _b(b), 
		_len(len), _k(k) {}
	
public:
	//--------------------------------------------------------------------------
	// operator
	//--------------------------------------------------------------------------
	

	//--------------------------------------------------------------------------
	// operator
	//--------------------------------------------------------------------------
	partic_t & getA() const { return _a; }
	partic_t & getB() const { return _b; }

	single getF() { 
		return (getA().getP().distance(getB().getP())-_len)*_k; 
	}

	vector_t getFa() { 
		vector_t v = getB().getP() - getA().getP();
		v.scaleto(getF()); 
		return v;
	}

	vector_t getFb() {
		vector_t v = getFa();
		v.mul(-1);
		return v;
	}
	
};


#endif //__SPRING_H__


