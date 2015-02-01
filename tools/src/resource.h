#ifndef _RESOURCE_HPP_
#define _RESOURCE_HPP_

#include <glib.h>

template <typename T, typename unref_res_t, void (*unref_res)(unref_res_t *)>
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
	typedef ResourceWrapper<gchar, void, g_free> CharStr;
	typedef ResourceWrapper<GError, GError, g_error_free> Error;
	typedef ResourceWrapper<gchar *, gchar *, g_strfreev> CharStrArr;
	typedef ResourceWrapper<GOptionContext, GOptionContext,
				g_option_context_free> OptionContext;
};


#endif//!_RESOURCE_HPP_
