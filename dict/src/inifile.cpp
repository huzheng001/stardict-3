/*
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
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

#include <vector>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "lib/utils.h"
#include "libcommon.h"

#include "inifile.h"

static const guchar NEW_STRING_SEP = 1;
//static const guchar OLD_STRING_SEP = 0xFF;
static const gchar *myversion = "1.0";

bool inifile::save()
{
	g_assert(!read_only_);
	gsize len;
	glib::Error err;
	glib::CharStr data(
		g_key_file_to_data(gkeyfile_, &len, get_addr(err)));

	if (err) {
		g_warning(("internal error, reason: %s\n"),
			  err->message);
		return false;
	}
	FILE *f = g_fopen(fname_.c_str(), "w");
	if (!f) {
		g_warning(("can not open file for writing: %s\n"),
			  fname_.c_str());
		return false;
	}
	size_t writeb = fwrite(get_impl(data), 1, len, f);
	fclose(f);
	if (writeb < len) {
		g_warning(("write to %s failed, instead of %lu,"
			    " we wrote %lu\n"), fname_.c_str(), gulong(len), gulong(writeb));
		return false;
	}
	return true;
}

bool inifile::fix_and_save()
{
	g_assert(!read_only_);
	g_assert(gkeyfile_);
	g_key_file_set_string(gkeyfile_, "stardict-private", "version",
			      myversion);
	if(!save())
		return false;
	g_key_file_free(gkeyfile_);
	gkeyfile_ = NULL;
	return true;
}

bool inifile::convert_from_locale_enc()
{
	g_assert(!read_only_);
	glib::Error err;
	glib::CharStr data;

	if (!g_file_get_contents(fname_.c_str(), get_addr(data), NULL,
				 get_addr(err))) {
		g_error(("Can not read %s, reason %s\n"), fname_.c_str(),
			err->message);
		return false;
	}

	glib::CharStr utfdata(g_locale_to_utf8(get_impl(data), -1, NULL, NULL,
					  NULL));
	if (!utfdata) {
		g_error(("ini file %s was saved in incorrect encoding, must be utf-8\n"), 
			fname_.c_str());
		return false;
	}

	if (!g_file_set_contents(fname_.c_str(), get_impl(utfdata), -1, get_addr(err))) {
		g_error("Can not save content of ini file %s, reason %s\n",
			fname_.c_str(), err->message);
		return false;
	}
	return true;
}

inifile::CheckResult inifile::check_config_version()
{
	glib::Error err;
	glib::CharStr version(g_key_file_get_string(gkeyfile_, "stardict-private",
		"version", get_addr(err)));
	if (err) {
		if (err->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND &&
		    err->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
			g_error(("internal error, reason: %s\n"),
				err->message);
			return crError;
		}
		if(read_only_) {
			g_error("ini file %s is missing parameter 'version' in "
				"section 'stardict-private'", fname_.c_str());
			return crError;
		}
		if(!fix_and_save())
			return crError;
		return crRetry;
	}
	if (strcmp(get_impl(version), myversion)) {
		g_error(("unsupported ini file format, supported version is %s\n"), myversion);
		return crError;
	}
	return crOK;
}

inifile::CheckResult inifile::load_ini_file()
{
	glib::Error err;
	if (!g_key_file_load_from_file(gkeyfile_, fname_.c_str(),
		GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS |
			G_KEY_FILE_KEEP_TRANSLATIONS),
		get_addr(err)))
	{
		if(!read_only_) {
			if (err->code == G_KEY_FILE_ERROR_UNKNOWN_ENCODING) {
				g_key_file_free(gkeyfile_);
				gkeyfile_ = NULL;
				if(!convert_from_locale_enc())
					return crError;
				return crRetry;
			}
		}
		g_error(("Can not open config file: %s, reason: %s\n"),
			fname_.c_str(), err->message);
		return crError;
	}
	return crOK;
}

inifile::inifile(void)
:
	gkeyfile_(NULL),
	read_only_(false)
{
	
}

inifile::~inifile()
{
	if(gkeyfile_) {
		if(!read_only_)
			save();
		g_key_file_free(gkeyfile_);
	}
}

bool inifile::load(const std::string& path, bool read_only, bool strict)
{
	fname_ = path;
	read_only_ = read_only;
	while (true) {
		g_assert(!gkeyfile_);
		gkeyfile_ = g_key_file_new();
		g_key_file_set_list_separator(gkeyfile_, NEW_STRING_SEP);
		/* create file if it does not exist, because g_key_file can not do that */
		if (!g_file_test(fname_.c_str(), G_FILE_TEST_EXISTS))
		{
			if(read_only_)
				return false;
			if(!fix_and_save())
				return false;
			continue;
		}

		CheckResult res = load_ini_file();
		if(res == crError)
			return false;
		else if(res == crRetry)
			continue;
		g_assert(res == crOK);

		if(strict) {
			res = check_config_version();
			if(res == crError)
				return false;
			else if(res == crRetry)
				continue;
			g_assert(res == crOK);
		}
		
		break;
	}
	return true;
}

static bool report_error(GError *err, const gchar *sect, const gchar *key)
{
	bool res = false;
	if (err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND ||
	    err->code == G_KEY_FILE_ERROR_GROUP_NOT_FOUND)
		res = true;
	else
		g_warning("Can not read %s/%s value,"
			  " reason %s\n", sect, key, err->message);
	g_error_free(err);
	return res;
}

bool inifile::read_bool(const gchar *sect, const gchar *key, bool& val)
{
	GError *err = NULL;

	gboolean newval = g_key_file_get_boolean(gkeyfile_, sect, key, &err);
	if (err)
		return report_error(err, sect, key);

	val = newval;
	return true;
}

bool inifile::read_int(const gchar *sect, const gchar *key, int& val)
{
	GError *err = NULL;
	gint newval = g_key_file_get_integer(gkeyfile_, sect, key, &err);
	if (err)
		return report_error(err, sect, key);

	val = newval;
	return true;
}

bool inifile::read_double(const gchar *sect, const gchar *key, gdouble& val)
{
	GError *err = NULL;
	gdouble newval = g_key_file_get_double(gkeyfile_, sect, key, &err);
	if (err)
		return report_error(err, sect, key);

	val = newval;
	return true;
}

bool inifile::read_string(const gchar * sect, const gchar *key, std::string& val)
{
	GError *err = NULL;
	gchar *newval = g_key_file_get_string(gkeyfile_, sect, key, &err);
	if (err) {
		g_free(newval);
		return report_error(err, sect, key);
	}
	val = newval;
	g_free(newval);
	return true;
}

bool inifile::read_strlist(const gchar *sect, const gchar * key,
			   std::list<std::string>& slist)
{
	GError *err = NULL;
	gchar **newval = g_key_file_get_string_list(gkeyfile_, sect, key,
						    NULL, &err);
	if (err) {
		g_strfreev(newval);
		return report_error(err, sect, key);
	}
	slist.clear();
	gchar **p = newval;
	while (*p) {
		slist.push_back(*p);
		++p;
	}
	g_strfreev(newval);
	return true;
}

void inifile::write_bool(const gchar *sect, const gchar *key, bool val)
{
	g_key_file_set_boolean(gkeyfile_, sect, key, val);
	expose_event(sect, key, val);
}

void inifile::write_int(const gchar *sect, const gchar *key, int val)
{
	g_key_file_set_integer(gkeyfile_, sect, key, val);
	expose_event(sect, key, val);
}

void inifile::write_double(const gchar *sect, const gchar *key, gdouble val)
{
	g_key_file_set_double(gkeyfile_, sect, key, val);
	expose_event(sect, key, val);
}

void inifile::write_string(const gchar *sect, const gchar *key,
			   const std::string& val)
{
	g_key_file_set_string(gkeyfile_, sect, key, val.c_str());
	expose_event(sect, key, val);
}

void inifile::write_strlist(const gchar *sect, const gchar *key,
			    const std::list<std::string>& slist)
{
	size_t len = slist.size();
	std::vector<const gchar *> glib_list(len + 1);

	std::list<std::string>::const_iterator it;
	size_t i;

	for (it = slist.begin(), i = 0; it != slist.end(); ++it, ++i)
		glib_list[i] = it->c_str();
	glib_list[i] = NULL;
	g_key_file_set_string_list(gkeyfile_, sect, key, &glib_list[0], len);
	expose_event(sect, key, slist);
}

void inifile::notify_add(const gchar *sect, const gchar *key,
			 const sigc::slot<void, const baseconfval*>& slot)
{
	std::string name = std::string(sect) + "/" + key;
	
	ChangeEventsMap::iterator it =
		change_events_map_.insert(
			std::make_pair(name, sigc::signal<void, const baseconfval*>())).first;
	it->second.connect(slot);
}
