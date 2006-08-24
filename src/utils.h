#ifndef UTILS_H
#define UTILS_H

#include <glib.h>
#include <string>
#include <vector>
#include <gdk-pixbuf/gdk-pixbuf.h>

template <typename T, typename unref_res_t, void (*unref_res)(unref_res_t *)>
struct ResourceWrapper {
	ResourceWrapper(T *p = NULL) : p_(p) {}
	~ResourceWrapper() { free_resource(); }
	T* get() const { return p_; }
	void reset(T *newp) {
		if (p_ != newp) {
			free_resource();
			p_ = newp;
		}
	}
private:
	T *p_;

	void free_resource() { if (p_) unref_res(p_); }
};

extern void play_wav_file(const std::string& filename);
extern void show_help(const gchar *section);
extern void show_url(const gchar *url);
extern void ProcessGtkEvent();
extern void play_sound_on_event(const gchar *eventname);

//sinse glib 2.6 we have g_get_user_config_dir
//but because of compability with other 2.x...
extern std::string get_user_config_dir();
extern std::string combnum2str(gint comb_code);
extern std::vector<std::string> split(const std::string& str, char sep);
extern GdkPixbuf *load_image_from_file(const std::string& filename);

#endif/*UTILS_H*/
