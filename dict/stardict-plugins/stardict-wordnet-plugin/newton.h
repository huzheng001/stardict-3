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

#ifndef __NEWTON_H__
#define __NEWTON_H__

//---------------------------------//---------------------------------
//	author: 边蓬
//	date: 2007-06-07
//---------------------------------//---------------------------------


#include "utils.h"
#include "spring.h"
#include "partic.h"
#include "scene.h"
#include "vector_t.h"
#include "newton_env.h"

class newton_t {
private:
	scene_t & _scene;
	newton_env_t & _env;
	bool _statchanged;
private:
	void init_newton_calculate();
	void calculate_spring_factor();	
	void calculate_repulsion_factor();
	void calculate_friction_factor();
	void calculate_collide_factor();
	void calculate_new_position(single t);
public:
	newton_t(scene_t & s, newton_env_t & env): _scene(s), _env(env) { }
	void set_scene(scene_t & s) { _scene = s; }
	void update(single t);
	newton_env_t & get_env() { return _env; }
	// 是否有发生移动，有则需要更新画面
	bool is_stat_changed() { return _statchanged; }
};

#endif

