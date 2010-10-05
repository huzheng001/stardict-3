#ifndef __NEWTON_H__
#define __NEWTON_H__

//---------------------------------//---------------------------------
//	author: 边蓬
//	date: 2007-06-07
//---------------------------------//---------------------------------


#include "utils.hpp"
#include "spring.hpp"
#include "partic.hpp"
#include "scene.hpp"
#include "vector_t.hpp"
#include "newton_env.hpp"

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

