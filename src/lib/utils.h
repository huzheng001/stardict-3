#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include <string.h>

#if defined(ARM) || defined(__sparc__)
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
#ifdef _WIN32
typedef std::basic_string<TCHAR> std_win_string;
extern bool utf8_to_windows(const std::string& str_utf8, std_win_string& out);
extern bool windows_to_utf8(const std_win_string& str, std::string& out_utf8);
#endif
#define DB_DIR_SEPARATOR '/'
#define DB_DIR_SEPARATOR_S "/"
/* functions to convert directory separator characters 
 * 1. file system separator character = G_DIR_SEPARATOR
 * 2. database separator character = DB_DIR_SEPARATOR */
#if DB_DIR_SEPARATOR == G_DIR_SEPARATOR
inline std::string dir_separator_fs_to_db(const std::string& path)
{
	return path;
}
inline std::string dir_separator_db_to_fs(const std::string& path)
{
	return path;
}
#else
extern std::string dir_separator_fs_to_db(const std::string& path);
extern std::string dir_separator_db_to_fs(const std::string& path);
#endif

void html_decode(const char *str, std::string& decoded);
void GetPureEnglishAlpha(char *dst, const char *src); // not used
bool IsASCII(const char *str);
const char* skip_spaces(const char *str);
char* copy_normalize_spaces(char *dst, const char *src);
void copy_normalize_trim_spaces(char *dst, const char *src);
char* delete_trailing_spaces_ASCII(const char *begin, char *end);
char* delete_trailing_word_ASCII(const char *begin, char *end);
char* delete_trailing_char(char *begin, char *end);
void extract_word(char *dst, const char* src, int BeginPos, gboolean (*is_splitter)(gunichar c));
void extract_word_in_place(const char **begin, const char **end, 
	const char* src, int BeginPos, gboolean (*is_splitter)(gunichar c));
void extract_capitalized_word(char *dst, const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c));
void extract_capitalized_word_in_place(const char **begin, const char **end,
	const char* src, int BeginPos, 
	gboolean (*is_first_letter)(gunichar c), gboolean (*is_second_letter)(gunichar c));
const char* find_first(const char* src, gboolean (*isfunc)(gunichar c));
const char* find_first_not(const char* src, gboolean (*isfunc)(gunichar c));
gboolean is_space_or_punct(gunichar c);
gboolean is_not_alpha(gunichar c);
gboolean is_not_upper(gunichar c);
gboolean is_not_lower(gunichar c);

#endif/*UTILS_H*/
