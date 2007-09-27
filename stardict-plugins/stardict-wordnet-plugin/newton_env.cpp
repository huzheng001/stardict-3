#include "newton_env.hpp"

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
