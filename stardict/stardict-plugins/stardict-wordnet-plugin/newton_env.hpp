#ifndef __NEWTON_ENV_H__
#define __NEWTON_ENV_H__

#include "utils.hpp"

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
