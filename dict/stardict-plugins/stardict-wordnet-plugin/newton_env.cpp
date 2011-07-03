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

#include "newton_env.h"

newton_env_t::newton_env_t() : 
min_friction_factor(0.3f), 
max_friction_factor(6.5f),
max_limt_powner_v(6.0f),
min_repulsion_distance(0.01f),
G(9.8f)
{
	reset();
}

void newton_env_t::reset() {
	the_friction_factor = min_friction_factor;
}

void newton_env_t::update_friction_factor() {
	// 暂时取消，看看用scene刷新年龄的方式看看效果
	the_friction_factor = 
		the_friction_factor > max_friction_factor? max_friction_factor : 
		the_friction_factor*1.06f;
}
