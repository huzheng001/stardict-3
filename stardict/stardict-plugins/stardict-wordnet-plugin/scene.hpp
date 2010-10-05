//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------

#ifndef __PHYSICS_H__
#define __PHYSICS_H__

#include "partic.hpp"
#include "spring.hpp"

#include <vector>
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

