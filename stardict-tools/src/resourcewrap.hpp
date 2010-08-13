#ifndef _RESOURCE_HPP_
#define _RESOURCE_HPP_

#include <glib.h>
#include <cstdio>
#include <zlib.h>

// borrowed from makedict program, part of xdxf project
// http://sourceforge.net/projects/xdxf/

template <typename T, typename unref_res_par_t, typename unref_res_ret_t, 
	unref_res_ret_t (*unref_res)(unref_res_par_t *)>
class ResourceWrapper {
public: 
	ResourceWrapper(T *p = NULL) : p_(p) {}
	~ResourceWrapper() { free_resource(); }
	T *operator->() const { return p_; }
	bool operator!() const { return p_ == NULL; }

	void reset(T *newp) {
		if (p_ != newp) {
			free_resource();
			p_ = newp;
		}
	}

	friend inline T *get_impl(const ResourceWrapper& rw) {
		return rw.p_;
	}

	friend inline T **get_addr(ResourceWrapper& rw) {
		return &rw.p_;
	}
private:
	T *p_;

	void free_resource() { if (p_) unref_res(p_); }

// Helper for enabling 'if (sp)'
	struct Tester {
		Tester() {}
	private:
		void operator delete(void*);
	};

public:
// enable 'if (sp)'
	operator const Tester*() const
	{
		if (!*this) return 0;
		static Tester t;
		return &t;
	}
};

namespace glib {
	typedef ResourceWrapper<gchar, void, void, g_free> CharStr;
	typedef ResourceWrapper<GError, GError, void, g_error_free> Error;
	typedef ResourceWrapper<gchar *, gchar *, void, g_strfreev> CharStrArr;
	typedef ResourceWrapper<GOptionContext, GOptionContext, void,
		g_option_context_free> OptionContext;
	typedef ResourceWrapper<GDir, GDir, void, g_dir_close> Dir;
}

namespace clib {
	typedef ResourceWrapper<FILE, FILE, int, fclose> File;
}

namespace zip {
typedef ResourceWrapper<void, void, int, gzclose> gzFile;
}

#endif//!_RESOURCE_HPP_
