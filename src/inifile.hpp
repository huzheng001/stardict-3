#ifndef _INIFILE_HPP_
#define _INIFILE_HPP_

#include <map>
#include <string>
#include <glib.h>

#include "config_file.hpp"

class inifile : public config_file {
public:
	explicit inifile(const std::string& path);
	~inifile();
	bool read_bool(const gchar *sect, const gchar *key, bool& val);
	bool read_int(const gchar *sect, const gchar *key, int& val);
	bool read_string(const gchar * sect, const gchar *key, std::string& val);
	bool read_strlist(const gchar * sect, const gchar *key, std::list<std::string>& slist);

	void write_bool(const gchar *sect, const gchar *key, bool val);
	void write_int(const gchar *sect, const gchar *key, int val);
	void write_string(const gchar *sect, const gchar *key, const std::string& val);
	void write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist);

	void notify_add(const gchar *sect, const gchar *key, 
			const sigc::slot<void, const baseconfval*>&);
private:

	typedef std::map<std::string, sigc::signal<void, const baseconfval *> >
        ChangeEventsMap;
	ChangeEventsMap change_events_map_;
	GKeyFile *gkeyfile_;
	std::string fname_;

	template <typename T>
	void expose_event(const gchar *sect, const gchar *key, const T& val) {
		std::string name = std::string(sect) + "/" + key;
		ChangeEventsMap::iterator p = change_events_map_.find(name);
		if (p == change_events_map_.end())
			return;
		confval<T> cv(val);
		p->second.emit(&cv);
	}
	void save();
	void create_empty();
	void convert_from_locale_enc();
};

#endif//!_INIFILE_HPP_
