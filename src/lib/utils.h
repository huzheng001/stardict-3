#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef ARM
static inline guint32 get_uint32(const gchar *addr)
{
	guint32 result;
	memcpy(&result, addr, sizeof(guint32));
	return result;
}
#else
#define get_uint32(x) *reinterpret_cast<const guint32 *>(x)
#endif

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
	operator Tester*() const
	{
		if (!*this) return 0;
		static Tester t;
		return &t;
	}
};

namespace glib {
	typedef ResourceWrapper<gchar, void, g_free> CharStr;
	typedef ResourceWrapper<GError, GError, g_error_free> Error;
}

extern void ProcessGtkEvent();

//sinse glib 2.6 we have g_get_user_config_dir
//but because of compability with other 2.x...
extern std::string get_user_config_dir();
extern std::string combnum2str(gint comb_code);
extern std::vector<std::string> split(const std::string& str, char sep);
extern GdkPixbuf *load_image_from_file(const std::string& filename);
extern void xml_decode(const char *str, std::string& res);
extern char *common_encode_uri_string(const char *string);
extern bool file_name_to_utf8(const std::string& str, std::string& out);
extern bool utf8_to_file_name(const std::string& str, std::string& out);

#endif/*UTILS_H*/
