/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <vector>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "utils.h"

#include "inifile.hpp"

static const guchar NEW_STRING_SEP = 1;
static const guchar OLD_STRING_SEP = 0xFF;
static const gchar *myversion = "1.0";

typedef ResourceWrapper<GError, GError, g_error_free> MyGError;

void inifile::create_empty()
{
	g_key_file_set_string(gkeyfile_, "stardict-private", "version",
			      myversion);
	save();
	g_key_file_free(gkeyfile_);
}

void inifile::convert_from_locale_enc()
{
	MyGError err;
	glib::CharStr data;

	if (!g_file_get_contents(fname_.c_str(), get_addr(data), NULL,
				 get_addr(err))) {
		g_error(("Can not read %s, reason %s\n"), fname_.c_str(),
			err->message);		
		exit(EXIT_SUCCESS);
	}

	glib::CharStr utfdata(g_locale_to_utf8(get_impl(data), -1, NULL, NULL,
					  NULL));
	if (!utfdata) {
		g_error(("Can not convert ini file content to current locale\n"));
		exit(EXIT_SUCCESS);
	}

	if (!g_file_set_contents(fname_.c_str(), get_impl(utfdata), -1, get_addr(err))) {
		g_error("can not save content of ini file %s, reason %s\n",
			fname_.c_str(), err->message);
		exit(EXIT_SUCCESS);
	}
}

inifile::inifile(const std::string& path)
{
	fname_ = path;
	bool done = false;
	while (!done) {
		gkeyfile_ = g_key_file_new();
		g_key_file_set_list_separator(gkeyfile_, NEW_STRING_SEP);
/* create file if not exist, because of g_key_file can not do that */
		if (!g_file_test(path.c_str(),
				 GFileTest(G_FILE_TEST_EXISTS |
					   G_FILE_TEST_IS_REGULAR))) {
			create_empty();
			continue;
		}

		MyGError err;
		if (!g_key_file_load_from_file(gkeyfile_, path.c_str(),
					       GKeyFileFlags(G_KEY_FILE_KEEP_COMMENTS |
							     G_KEY_FILE_KEEP_TRANSLATIONS),
					       get_addr(err))) {
			if (err->code == G_KEY_FILE_ERROR_UNKNOWN_ENCODING) {
				g_key_file_free(gkeyfile_);
				convert_from_locale_enc();
				continue;
			}
			g_error(("Can not open config file: %s, reason: %s\n"),
				path.c_str(), err->message);
			exit(EXIT_FAILURE);//just in case
		}

		glib::CharStr version(g_key_file_get_string(gkeyfile_, "stardict-private",
						       "version", get_addr(err)));
		if (err) {
			if (err->code != G_KEY_FILE_ERROR_KEY_NOT_FOUND &&
			    err->code != G_KEY_FILE_ERROR_GROUP_NOT_FOUND) {
				g_error(("internal error, reason: %s\n"),
					err->message);
				exit(EXIT_FAILURE);//just in case
			}
			create_empty();
			continue;
		}
		if (strcmp(get_impl(version), myversion)) {
			g_error(("unsupported ini file format\n"));
			exit(EXIT_FAILURE);
		}
		done = true;
	}

}

void inifile::save()
{
	gsize len;
	MyGError err;
	glib::CharStr data(
		g_key_file_to_data(gkeyfile_, &len, get_addr(err)));

	if (err) {
		g_warning(("internal error, reason: %s\n"),
			  err->message);
		return;
	}
	FILE *f = g_fopen(fname_.c_str(), "w");
	if (!f) {
		g_warning(("can not open file: %s\n"),
			  fname_.c_str());
		return;
	}
	size_t writeb = fwrite(get_impl(data), 1, len, f);
	fclose(f);
	if (writeb < len)
		g_warning(("write to %s failed, instead of %lu,"
			    " we wrote %lu\n"), fname_.c_str(), gulong(len), gulong(writeb));
}

inifile::~inifile()
{
	save();
	g_key_file_free(gkeyfile_);
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
                        std::make_pair(name,
                                       sigc::signal<void, const baseconfval*>())).first;
        it->second.connect(slot);
}
