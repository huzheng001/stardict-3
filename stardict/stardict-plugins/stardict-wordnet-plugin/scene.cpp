//---------------------------------
//	author: 边蓬
//	date: 2007-05-09
//---------------------------------

#include "scene.hpp"

partic_t * scene_t::create_partic(const single m, single w, single h) {
	partic_t * p = new partic_t(m, w, h);
	_partics.push_back(p);
	return p;
}

spring_t * scene_t::create_spring(partic_t & a, partic_t & b, float springlength, float k) {
	spring_t * s = new spring_t(a, b, springlength, k);
	_springs.push_back(s);
	return s;
}

//  清除所有资源
void scene_t::clear() {
	for(vector<spring_t *>::const_iterator it_p = _springs.begin();
		it_p != _springs.end(); ++it_p) {
			delete *it_p;
	}
	_springs.clear();
	for(vector<partic_t *>::const_iterator it_p = _partics.begin();
		it_p != _partics.end(); ++it_p) {
			delete *it_p;
	}
	_partics.clear();
}

bool scene_t::hit(const vector_t & b, /*out*/ partic_t ** out) {
	for(vector<partic_t *>::iterator it = _partics.begin();
		it != _partics.end(); ++it) {
			if((*it)->hit(b)) {
				*out = *it;
				return true;
			}
	}
	return false;
}

void scene_t::center_to(const vector_t & c) {
	vector_t d = c - _c->getP();
	for(vector<partic_t *>::iterator it = _partics.begin();
		it != _partics.end(); ++it) {
			(*it)->getP().add(d);
	}
}

void scene_t::pan(const vector_t & d) {
	for(vector<partic_t *>::iterator it = _partics.begin();
		it != _partics.end(); ++it) {
			(*it)->getP().add(d);
	}
}

void scene_t::set_center(partic_t * p) { _c = p;/* _c->getM() *= 10.0f; */}

bool scene_t::checkstable() {
	for(vector<partic_t *>::iterator it = _partics.begin(); 
		it != _partics.end(); ++it) {
			if ((*it)->getV().powerlength() > 0.1 || 
				(*it)->getF().powerlength() > 0.1 )
				return false;
	}
	return true;
}
