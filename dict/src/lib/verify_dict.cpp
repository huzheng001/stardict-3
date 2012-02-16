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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <ctime>
#include <sstream>
#include <errno.h>
#include "verify_dict.h"
#include "lib_dict_verify.h"
#include "iappdirs.h"
#include "utils.h"
#include "stddict.h"

struct dict_file_timestamp
{
	dict_file_timestamp(void)
	:
		mtime(0),
		present(false)
	{
	}
	/* Contains copy of the st_mtime field of the stat structure.
	 * Note. Do not name this field st_mtime!
	 * It is a macro in Linux that is expanded to st_mtim.tv_sec. */
	std::time_t mtime;
	/* true if the file is present and mtime contains meaningful value. */
	bool present;
	bool operator==(const dict_file_timestamp& right)
	{
		if(!present)
			return present == right.present;
		return present == right.present && mtime == right.mtime;
	}
};

/* encapsulate timestamp information about one dictionary */
struct dict_timestamp
{
	std::string ifofilename;
	bool valid;
	// dictionary files
	dict_file_timestamp ts_ifo;
	dict_file_timestamp ts_idx;
	dict_file_timestamp ts_idx_gz;
	dict_file_timestamp ts_dict;
	dict_file_timestamp ts_dict_dz;
	dict_file_timestamp ts_syn;
	// resource database
	dict_file_timestamp ts_res_rifo;
	dict_file_timestamp ts_res_ridx;
	dict_file_timestamp ts_res_ridx_gz;
	dict_file_timestamp ts_res_rdic;
	dict_file_timestamp ts_res_rdic_dz;
	// resource directory
	dict_file_timestamp ts_res;

	bool load(const std::string& ifofilename);
	std::string serialize() const;
	bool is_dict_changed(const dict_timestamp& right);
};

/* return value: true - OK */
bool dict_timestamp::load(const std::string& _ifofilename)
{
	if(!is_path_end_with(_ifofilename, ".ifo")) {
		return false;
	}
	stardict_stat_t stats;
	ifofilename = _ifofilename;
	if (g_stat(ifofilename.c_str(), &stats)) {
		return false;
	} else {
		ts_ifo.present = true;
		ts_ifo.mtime = stats.st_mtime;
	}

	std::string basefilename(ifofilename, 0, ifofilename.length()-4);
	glib::CharStr cdirname(g_path_get_dirname(ifofilename.c_str()));
	std::string dirname(get_impl(cdirname));
	std::string filename;

	// dictionary files
	filename = basefilename + ".idx";
	if (g_stat(filename.c_str(), &stats)) {
		ts_idx.present = false;
		ts_idx.mtime = 0;
	} else {
		ts_idx.present = true;
		ts_idx.mtime = stats.st_mtime;
	}
	filename = basefilename + ".idx.gz";
	if (g_stat(filename.c_str(), &stats)) {
		ts_idx_gz.present = false;
		ts_idx_gz.mtime = 0;
	} else {
		ts_idx_gz.present = true;
		ts_idx_gz.mtime = stats.st_mtime;
	}
	filename = basefilename + ".dict";
	if (g_stat(filename.c_str(), &stats)) {
		ts_dict.present = false;
		ts_dict.mtime = 0;
	} else {
		ts_dict.present = true;
		ts_dict.mtime = stats.st_mtime;
	}
	filename = basefilename + ".dict.dz";
	if (g_stat(filename.c_str(), &stats)) {
		ts_dict_dz.present = false;
		ts_dict_dz.mtime = 0;
	} else {
		ts_dict_dz.present = true;
		ts_dict_dz.mtime = stats.st_mtime;
	}
	filename = basefilename + ".syn";
	if (g_stat(filename.c_str(), &stats)) {
		ts_syn.present = false;
		ts_syn.mtime = 0;
	} else {
		ts_syn.present = true;
		ts_syn.mtime = stats.st_mtime;
	}
	// resource database
	filename = build_path(dirname, "res.rifo");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res_rifo.present = false;
		ts_res_rifo.mtime = 0;
	} else {
		ts_res_rifo.present = true;
		ts_res_rifo.mtime = stats.st_mtime;
	}
	filename = build_path(dirname, "res.ridx");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res_ridx.present = false;
		ts_res_ridx.mtime = 0;
	} else {
		ts_res_ridx.present = true;
		ts_res_ridx.mtime = stats.st_mtime;
	}
	filename = build_path(dirname, "res.ridx_gz");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res_ridx_gz.present = false;
		ts_res_ridx_gz.mtime = 0;
	} else {
		ts_res_ridx_gz.present = true;
		ts_res_ridx_gz.mtime = stats.st_mtime;
	}
	filename = build_path(dirname, "res.rdic");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res_rdic.present = false;
		ts_res_rdic.mtime = 0;
	} else {
		ts_res_rdic.present = true;
		ts_res_rdic.mtime = stats.st_mtime;
	}
	filename = build_path(dirname, "res.rdic_dz");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res_rdic_dz.present = false;
		ts_res_rdic_dz.mtime = 0;
	} else {
		ts_res_rdic_dz.present = true;
		ts_res_rdic_dz.mtime = stats.st_mtime;
	}
	// resource directory
	filename = build_path(dirname, "res");
	if (g_stat(filename.c_str(), &stats)) {
		ts_res.present = false;
		ts_res.mtime = 0;
	} else {
		ts_res.present = true;
		ts_res.mtime = stats.st_mtime;
	}
	return true;
}

std::string dict_timestamp::serialize() const
{
#ifdef _WIN32
	glib::CharStr ifofilename_escaped(g_markup_escape_text(rel_path_to_data_dir(ifofilename).c_str(), -1));
#else
	glib::CharStr ifofilename_escaped(g_markup_escape_text(ifofilename.c_str(), -1));
#endif
	std::stringstream buf;
	buf << "<dict ifofilename=\"" << get_impl(ifofilename_escaped) << "\"";
	buf << " valid=\"" << (valid ? "true" : "false") << "\"";
	if(ts_ifo.present)
		buf << " ts_ifo=\"" << (guint64)ts_ifo.mtime << "\"";
	if(ts_idx.present)
		buf << " ts_idx=\"" << (guint64)ts_idx.mtime << "\"";
	if(ts_idx_gz.present)
		buf << " ts_idx_gz=\"" << (guint64)ts_idx_gz.mtime << "\"";
	if(ts_dict.present)
		buf << " ts_dict=\"" << (guint64)ts_dict.mtime << "\"";
	if(ts_dict_dz.present)
		buf << " ts_dict_dz=\"" << (guint64)ts_dict_dz.mtime << "\"";
	if(ts_syn.present)
		buf << " ts_syn=\"" << (guint64)ts_syn.mtime << "\"";
	if(ts_res_rifo.present)
		buf << " ts_res_rifo=\"" << (guint64)ts_res_rifo.mtime << "\"";
	if(ts_res_ridx.present)
		buf << " ts_res_ridx=\"" << (guint64)ts_res_ridx.mtime << "\"";
	if(ts_res_ridx_gz.present)
		buf << " ts_res_ridx_gz=\"" << (guint64)ts_res_ridx_gz.mtime << "\"";
	if(ts_res_rdic.present)
		buf << " ts_res_rdic=\"" << (guint64)ts_res_rdic.mtime << "\"";
	if(ts_res_rdic_dz.present)
		buf << " ts_res_rdic_dz=\"" << (guint64)ts_res_rdic_dz.mtime << "\"";
	if(ts_res.present)
		buf << " ts_res=\"" << (guint64)ts_res.mtime << "\"";
	buf << "/>\n";
	return buf.str();
}

/* compares all fields except valid. */
bool dict_timestamp::is_dict_changed(const dict_timestamp& right)
{
	bool is_not_changed =
			is_equal_paths(ifofilename, right.ifofilename)
		&& ts_ifo == right.ts_ifo
		&& ts_idx == right.ts_idx
		&& ts_idx_gz == right.ts_idx_gz
		&& ts_dict == right.ts_dict
		&& ts_dict_dz == right.ts_dict_dz
		&& ts_syn == right.ts_syn
		&& ts_res_rifo == right.ts_res_rifo
		&& ts_res_ridx == right.ts_res_ridx
		&& ts_res_ridx_gz == right.ts_res_ridx_gz
		&& ts_res_rdic == right.ts_res_rdic
		&& ts_res_rdic_dz == right.ts_res_rdic_dz
		&& ts_res == right.ts_res
		;
	return !is_not_changed;
}

class VerifCache
{
public:
	explicit VerifCache(show_progress_t *sp);
	void load();
	void save();
	bool verify(const std::string& ifofilename);
private:
	bool load_cache_file();
	void cleanup_dict_list();
	dict_timestamp* find_dict(const std::string& ifofilename);
	static void parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
	static void parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
	static bool deserialize_time(std::time_t& time, const gchar* value);
	static void invalid_attr_value_err(GMarkupParseContext *context, const char* attr_name);
	static void missing_attr_err(GMarkupParseContext *context, const char* attr_name);
	static void unknown_attr_err(GMarkupParseContext *context, const char* attr_name);
	static void unknown_elem_err(GMarkupParseContext *context, const char* elem_name);
private:
	static const char* const verif_cache_file_name;
	std::string verif_cache_file_path;
	std::list<dict_timestamp> dicts;
	show_progress_t *show_progress;
};

const char* const VerifCache::verif_cache_file_name = "verif_cache.xml";

VerifCache::VerifCache(show_progress_t *sp)
{
	verif_cache_file_path = build_path(app_dirs->get_user_cache_dir(), verif_cache_file_name);
	show_progress = sp;
}

void VerifCache::load()
{
	load_cache_file();
	cleanup_dict_list();
}

void VerifCache::save()
{
	std::string buf;
	buf += "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
	buf += "<dicts>\n";
	for(std::list<dict_timestamp>::iterator it=dicts.begin(); it != dicts.end(); ++it) {
		buf += it->serialize();
	}
	buf += "</dicts>\n";
	glib::Error error;
	glib::CharStr verif_cache_dir(g_path_get_dirname(verif_cache_file_path.c_str()));
	if(!g_file_test(get_impl(verif_cache_dir), G_FILE_TEST_EXISTS)) {
		if(-1 == g_mkdir_with_parents(get_impl(verif_cache_dir), S_IRWXU)) {
			std::string error(g_strerror(errno));
			g_warning(_("Unable to create directory: '%s'. "
					"Verification cache file will not be saved. Error: '%s'"),
				get_impl(verif_cache_dir), error.c_str());
			return;
		}
	}
	if(!g_file_set_contents(verif_cache_file_path.c_str(),
		buf.c_str(), -1, get_addr(error))) {
		g_warning(_("Unable to save the verification cache file '%s'. Error: '%s'"),
			verif_cache_file_path.c_str(), error->message);
	}
}

// return value: true - OK
bool VerifCache::verify(const std::string& ifofilename)
{
	show_progress->notify_about_start(_("Verifying..."));
	dict_timestamp ts;
	if(!ts.load(ifofilename)) {
		g_warning(_("Unable to load information for dictionary '%s'"), ifofilename.c_str());
		return false;
	}
	dict_timestamp* pts = find_dict(ifofilename);
	if(pts) {
		if(ts.is_dict_changed(*pts))
			*pts = ts;
		else
			return pts->valid;
	} else {
		dicts.push_back(ts);
		pts = &*dicts.rbegin();
	}
	pts->valid = (stardict_verify(ifofilename.c_str()) <= VERIF_RESULT_WARNING);
	return pts->valid;
}

bool VerifCache::load_cache_file()
{
	dicts.clear();
	if(!g_file_test(verif_cache_file_path.c_str(), G_FILE_TEST_EXISTS))
		return false;

	glib::CharStr contents;
	glib::Error error;
	if(!g_file_get_contents(verif_cache_file_path.c_str(),
			get_addr(contents),
			NULL, get_addr(error))) {
		g_warning(_("Unable to open the verification cache file '%s'. Error: '%s'"),
				verif_cache_file_path.c_str(), get_impl(error)->message);
		return false;
	}
	GMarkupParser parser;
	parser.start_element = parse_start_element;
	parser.end_element = parse_end_element;
	parser.text = NULL;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, this, NULL);
	if(!g_markup_parse_context_parse(context, get_impl(contents), -1, get_addr(error))) {
		g_warning(_("Unable to parse contents of the verification cache file '%s'. Error: '%s'"),
			verif_cache_file_path.c_str(), get_impl(error)->message);
		dicts.clear();
		g_markup_parse_context_free(context);
		return false;
	}
	if(!g_markup_parse_context_end_parse(context, get_addr(error))) {
		g_warning(_("Unable to parse contents of the verification cache file '%s'. Error: '%s'"),
			verif_cache_file_path.c_str(), get_impl(error)->message);
		dicts.clear();
		g_markup_parse_context_free(context);
		return false;
	}
	g_markup_parse_context_free(context);
	return true;
}

/* cleanup dictionary list
 * remove records for missing files,
 * remove duplicates */
void VerifCache::cleanup_dict_list()
{
	std::list<dict_timestamp>::iterator it = dicts.begin();
	while(it != dicts.end()) {
		bool remove_this = false;
		if(!g_file_test(it->ifofilename.c_str(), G_FILE_TEST_EXISTS))
			remove_this = true;
		if(!remove_this && it != dicts.begin()) {
			for(std::list<dict_timestamp>::const_iterator it2 = dicts.begin(); it2 != it; ++it2) {
				if(is_equal_paths(it2->ifofilename, it->ifofilename)) {
					remove_this = true;
					break;
				}
			}
		}
		if(remove_this) {
			std::list<dict_timestamp>::iterator it2 = it;
			++it;
			dicts.erase(it2);
		} else
			++it;
	}
}

dict_timestamp* VerifCache::find_dict(const std::string& ifofilename)
{
	for(std::list<dict_timestamp>::iterator it=dicts.begin(); it != dicts.end(); ++it) {
		if(is_equal_paths(ifofilename, it->ifofilename))
			return &*it;
	}
	return NULL;
}


void VerifCache::parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	VerifCache *data = (VerifCache*)user_data;
	if (strcmp(element_name, "dicts")==0) {
		// top element
	} else if(strcmp(element_name, "dict") == 0) {
		dict_timestamp dict;
		bool valid_attr = false;
		bool ifofilename_attr = false;
		for(size_t i=0; attribute_names[i]; ++i) {
			if (strcmp(attribute_names[i], "ifofilename")==0) {
				if(attribute_values[i][0]=='\0') {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
#ifdef _WIN32
				dict.ifofilename = abs_path_to_data_dir(attribute_values[i]);
#else
				dict.ifofilename = attribute_values[i];
#endif
				ifofilename_attr = true;
				continue;
			}
			if(strcmp(attribute_names[i], "valid") == 0) {
				if(strcmp(attribute_values[i], "true") == 0) {
					dict.valid = true;
					valid_attr = true;
				} else if(strcmp(attribute_values[i], "false") == 0) {
					dict.valid = false;
					valid_attr = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_ifo") == 0) {
				if(deserialize_time(dict.ts_ifo.mtime, attribute_values[i])) {
					dict.ts_ifo.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_idx") == 0) {
				if(deserialize_time(dict.ts_idx.mtime, attribute_values[i])) {
					dict.ts_idx.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_idx_gz") == 0) {
				if(deserialize_time(dict.ts_idx_gz.mtime, attribute_values[i])) {
					dict.ts_idx_gz.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_dict") == 0) {
				if(deserialize_time(dict.ts_dict.mtime, attribute_values[i])) {
					dict.ts_dict.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_dict_dz") == 0) {
				if(deserialize_time(dict.ts_dict_dz.mtime, attribute_values[i])) {
					dict.ts_dict_dz.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_syn") == 0) {
				if(deserialize_time(dict.ts_syn.mtime, attribute_values[i])) {
					dict.ts_syn.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res_rifo") == 0) {
				if(deserialize_time(dict.ts_res_rifo.mtime, attribute_values[i])) {
					dict.ts_res_rifo.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res_ridx") == 0) {
				if(deserialize_time(dict.ts_res_ridx.mtime, attribute_values[i])) {
					dict.ts_res_ridx.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res_ridx_gz") == 0) {
				if(deserialize_time(dict.ts_res_ridx_gz.mtime, attribute_values[i])) {
					dict.ts_res_ridx_gz.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res_rdic") == 0) {
				if(deserialize_time(dict.ts_res_rdic.mtime, attribute_values[i])) {
					dict.ts_res_rdic.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res_rdic_dz") == 0) {
				if(deserialize_time(dict.ts_res_rdic_dz.mtime, attribute_values[i])) {
					dict.ts_res_rdic_dz.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			if(strcmp(attribute_names[i], "ts_res") == 0) {
				if(deserialize_time(dict.ts_res.mtime, attribute_values[i])) {
					dict.ts_res.present = true;
				} else {
					invalid_attr_value_err(context, attribute_names[i]);
					return;
				}
				continue;
			}
			unknown_attr_err(context, attribute_names[i]);
		} // for
		if(!ifofilename_attr) {
			missing_attr_err(context, "ifofilename");
			return;
		}
		if(!valid_attr) {
			missing_attr_err(context, "valid");
			return;
		}
		data->dicts.push_back(dict);
	} else {
		unknown_elem_err(context, element_name);
	}
}

void VerifCache::parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	//VerifCache *data = (VerifCache*)user_data;
}

/* convert string representation of std::time_t to binary form
 * return value: true - OK */
bool VerifCache::deserialize_time(std::time_t& time, const gchar* value)
{
	if(!value)
		return false;
	if(!g_ascii_isdigit(value[0]))
		return false;
	gchar* endptr = NULL;
	/* We do not know how larger the std::time_t value may be,
	 * hence we use the largest data type. */
	guint64 t = g_ascii_strtoull(value, &endptr, 10);
	if(endptr == value || endptr == NULL || *endptr != '\0')
		return false;
	time = (std::time_t)t;
	return true;
}

void VerifCache::invalid_attr_value_err(GMarkupParseContext *context, const char* attr_name)
{
	gint line_number;
	gint char_number;
	g_markup_parse_context_get_position(context, &line_number, &char_number);
	g_warning(_("Error in parsing verification cache file: invalid value of the '%s' attribute. "
		"Line: %d, character: %d."),
		attr_name, line_number, char_number);
}

void VerifCache::missing_attr_err(GMarkupParseContext *context, const char* attr_name)
{
	gint line_number;
	gint char_number;
	g_markup_parse_context_get_position(context, &line_number, &char_number);
	g_warning(_("Error in parsing verification cache file: missing mandatory attribute '%s'. "
		"Line: %d, character: %d."),
		attr_name, line_number, char_number);
}

void VerifCache::unknown_attr_err(GMarkupParseContext *context, const char* attr_name)
{
	gint line_number;
	gint char_number;
	g_markup_parse_context_get_position(context, &line_number, &char_number);
	g_warning(_("Error in parsing verification cache file: unknown attribute '%s'. "
		"Line: %d, character: %d."),
		attr_name, line_number, char_number);
}

void VerifCache::unknown_elem_err(GMarkupParseContext *context, const char* elem_name)
{
	gint line_number;
	gint char_number;
	g_markup_parse_context_get_position(context, &line_number, &char_number);
	g_warning(_("Error in parsing verification cache file: unknown element '%s'. "
		"Line: %d, character: %d."),
		elem_name, line_number, char_number);
}

void filter_verify_dicts(const std::list<std::string>& dict_all_list,
	std::list<std::string>& dict_valid_list, show_progress_t *sp)
{
	dict_valid_list.clear();
	VerifCache cache(sp);
	cache.load();
	for(std::list<std::string>::const_iterator it=dict_all_list.begin(); it!=dict_all_list.end(); ++it) {
		if(cache.verify(*it)) {
			dict_valid_list.push_back(*it);
			g_debug(_("Verification status of '%s': dictionary is OK"), it->c_str());
		} else
			g_debug(_("Verification status of '%s': dictionary is broken"), it->c_str());
	}
	cache.save();
}
