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

#ifndef __NEWTON_ENV_H__
#define __NEWTON_ENV_H__

#include "utils.h"

// 牛顿环境的系数. 一个数据类.
struct newton_env_t {
	single min_friction_factor;		// 摩擦系数，这个是空气摩擦
	single max_friction_factor;
	single the_friction_factor;
	single max_limt_powner_v;		// 限速
	single min_repulsion_distance;	//计算斥力，最小的距离
	/* 
	** 万有斥力系数
	*/
	single G;		
	// init value
	newton_env_t();
	virtual ~newton_env_t() {}
	void reset();
	////
	single get_friction_factor() { return the_friction_factor; }
	virtual void update_friction_factor();
};

#endif //__NEWTON_ENV_H__
