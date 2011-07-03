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

#include "newton.h"


void newton_t::update(single t) {
	init_newton_calculate();
	calculate_spring_factor();
	calculate_repulsion_factor();
	calculate_friction_factor();
	calculate_collide_factor();
	calculate_new_position(t);
}

void newton_t::calculate_new_position(single t) {
	_statchanged = false;
	for(vector<partic_t *>::iterator it = _scene.get_partics().begin();
		it != _scene.get_partics().end(); ++it) {
			if (!(*it)->get_anchor()) {
				vector_t v2 = (*it)->getV() + (*it)->getA().mul(t);
				// 这里做了限速，让我很担忧
				if (v2.powerlength() > _env.max_limt_powner_v*_env.max_limt_powner_v)
					v2.scaleto(_env.max_limt_powner_v);

				vector_t avgv = ((*it)->getV() + v2).div(2);
				vector_t d = avgv.mul(t);
				if (d.powerlength() > 0.5f) {
					/// todo matic number, avoid tiny motion/ thread
					(*it)->getP().add(d);
					if (!_statchanged) _statchanged = true;
				}
				(*it)->getV() = v2;
			}
	}
}

//-----------------------------------------------------
//  计算弹簧的拉力的影响因素
//-----------------------------------------------------
void newton_t::calculate_spring_factor() {
	for(vector<spring_t *>::iterator it = _scene.get_springs().begin();
		it != _scene.get_springs().end(); ++it) {
			(*it)->getA().getF().add((*it)->getFa());
			(*it)->getB().getF().add((*it)->getFb());
	}
}

//--------------------------------------------------
// 清零操作，这样就可以开始计算了
//--------------------------------------------------
void newton_t::init_newton_calculate() {
	for(vector<partic_t *>::iterator it = _scene.get_partics().begin();
		it != _scene.get_partics().end(); ++it) {
			(*it)->getF() = vector_t::zero;
	}
}

//--------------------------------------------------
// 万有斥力的计算，让整个场景有一个较好的分布
//--------------------------------------------------
void newton_t::calculate_repulsion_factor() {
	for(size_t i = 0; i < _scene.get_partics().size(); ++i) {
		partic_t * a = _scene.get_partics()[i];//outloop.cur();
		for(size_t j = 0; j < _scene.get_partics().size(); ++j ) {
			partic_t * b = _scene.get_partics()[j]; // innerloop.cur();
			// 每次都需要把两个球都算好, 分别计算
			vector_t f = a->getP() - b->getP();
			single dd = f.powerlength();
			dd = dd > _env.min_repulsion_distance ? dd : _env.min_repulsion_distance;
			single g = _env.G * a->getM()*b->getM()/dd;
			f.set_length(g);
			a->getF().add(f);
			f.mul(-1);
			b->getF().add(f);
		}
	}
}

//--------------------------------------------------
// 摩擦阻尼运动
// 摩擦力大小和速度有关，如果速度为零，则没有摩擦，哈哈这个设定很好
// 可以用于避免重叠那里
//--------------------------------------------------
void newton_t::calculate_friction_factor() {
	for(vector<partic_t *>::iterator it = _scene.get_partics().begin();
			it != _scene.get_partics().end(); ++it) {
		vector_t f = (*it)->getV() * (-get_env().get_friction_factor());
		(*it)->getF().add(f);
	}
	get_env().update_friction_factor();
}

// 碰撞检测，用矩形做碰撞检测，计算喷装，用质点算了
// 还是不用碰撞了，虚拟一个力出来，弹开即可，不过不能制造出新的能量来啊
// 否则的话，就不能收敛下来了
void newton_t::calculate_collide_factor() {
	for(size_t i = 0; i < _scene.get_partics().size(); ++i) {
		partic_t * a = _scene.get_partics()[i]; // outloop.cur();
		for(size_t j = i+1; j < _scene.get_partics().size(); ++j) {
			partic_t * b = _scene.get_partics()[j];// innerloop.cur();
			// 碰撞了, 需要在坐标系里面交换速度
			if(a->get_box().overlay(b->get_box())) {
				vector_t d = a->getV() - b->getV();
				vector_t t = d;
				t.rot(M_PI_2);

				//single area = a->get_box().overlayarea(b->get_box());
				vector_t f = d;
				f.norm();

				//////////////////////////////////////////////////////////////////////////
				// 这段代码，直接弹开的效果过于激烈了，做成斥力好了
				///// method 1
				//if (b->getV().proj(d) > 0)
				//	b->getV().mirror(t);
				//else
				//	b->getF().add(f*b->getM()); // 在这里计算斥力

				//if (a->getV().proj(d) < 0)
				//	a->getV().mirror(t);
				//else
				//	a->getF().add(f.mul(-a->getM()));

				// method 2
				b->getF().add(f*(-b->getM()));
				a->getF().add(f.mul(a->getM()));

				/////
			}
		}
	}	
}
