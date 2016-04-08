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

#include <memory>
#include <glib/gi18n.h>

#include "gconf_file.h"

gconf_file::gconf_file(const std::string& path)
{
	cfgname=path;
	if ((gconf_client = gconf_client_get_default())==NULL)
		g_warning("Cannot connect to gconf.\n");
	else
		gconf_client_add_dir(gconf_client, cfgname.c_str(), GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}

gconf_file::~gconf_file()
{
	for (std::vector<guint>::iterator it=notification_ids.begin();
	     it!=notification_ids.end(); ++it)
		gconf_client_notify_remove(gconf_client, *it);

	if (!gconf_client)
		return;
	gconf_client_remove_dir(gconf_client, cfgname.c_str(), NULL);
	g_object_unref(gconf_client);
}

bool gconf_file::read_bool(const gchar *sect, const gchar *key, bool& val)
{
	if (!gconf_client)
		return false;
  
	std::string real_key(std::string(sect)+"/"+key);

	GConfValue *gval=gconf_client_get(gconf_client, real_key.c_str(), NULL);
	if (!gval)
		return false;
	val=gconf_value_get_bool(gval);
	gconf_value_free(gval);

	return true;
}

bool gconf_file::read_int(const gchar *sect, const gchar *key, int& val)
{
	if (!gconf_client)
		return false;
	std::string real_key(std::string(sect)+"/"+key);
	GConfValue *gval=gconf_client_get(gconf_client, real_key.c_str(), NULL);
	if (!gval)
		return false;
	val=gconf_value_get_int(gval);
	gconf_value_free(gval);

	return true;
}

bool gconf_file::read_double(const gchar *sect, const gchar *key, double& val)
{
	if (!gconf_client)
		return false;
	std::string real_key(std::string(sect)+"/"+key);
	GConfValue *gval=gconf_client_get(gconf_client, real_key.c_str(), NULL);
	if (!gval)
		return false;
	val=gconf_value_get_float(gval);
	gconf_value_free(gval);

	return true;
}

bool gconf_file::read_string(const gchar * sect, const gchar *key, std::string& val)
{
	if (!gconf_client)
		return false;

	std::string real_key(std::string(sect)+"/"+key);
	gchar *gconf_val = gconf_client_get_string(gconf_client, real_key.c_str(), NULL);
	if (gconf_val!=NULL) 
		val=gconf_val;

	g_free(gconf_val); 

	return true;
}

bool gconf_file::read_strlist(const gchar * sect, const gchar * key, std::list<std::string>& slist)
{
	if (!gconf_client)
		return false;  

	std::string real_key(std::string(sect)+"/"+key);
	GSList  *gslist = gconf_client_get_list(gconf_client, real_key.c_str(), GCONF_VALUE_STRING, NULL);
	if (!gslist)
		return false;

	slist.clear();
	GSList *p = gslist; 
	while (p) {
		slist.push_back(static_cast<char *>(p->data));
		g_free(p->data);		
		p=g_slist_next(p);
	}
	g_slist_free(gslist);

	return true;
}

void gconf_file::write_bool(const gchar *sect, const gchar *key, bool val)
{
	if (!gconf_client)
		return;
	gchar *real_key=g_strdup_printf("%s/%s", sect, key);
	gconf_client_set_bool(gconf_client, real_key, val, NULL);
	g_free(real_key);
}

void gconf_file::write_int(const gchar *sect, const gchar *key, int val)
{
	if (!gconf_client)
		return;
	gchar *real_key=g_strdup_printf("%s/%s", sect, key);
	gconf_client_set_int(gconf_client, real_key, val, NULL);
	g_free(real_key);
}

void gconf_file::write_double(const gchar *sect, const gchar *key, double val)
{
	if (!gconf_client)
		return;
	gchar *real_key=g_strdup_printf("%s/%s", sect, key);
	gconf_client_set_float(gconf_client, real_key, val, NULL);
	g_free(real_key);
}

void gconf_file::write_string(const gchar *sect, const gchar *key, const std::string& val)
{
	if(!gconf_client)
		return;
	gchar *real_key=g_strdup_printf("%s/%s", sect, key);
	gconf_client_set_string(gconf_client, real_key, val.c_str(), NULL);
	g_free(real_key);
}

void gconf_file::write_strlist(const gchar *sect, const gchar *key, const std::list<std::string>& slist)
{
	if (!gconf_client)
		return;

	GSList *gslist = NULL;
	for (std::list<std::string>::const_iterator p = slist.begin();
	     p!=slist.end(); ++p)
		gslist = g_slist_append(gslist, const_cast<char *>(p->c_str()));
 
	gchar *real_key=g_strdup_printf("%s/%s", sect, key);
	gconf_client_set_list(gconf_client, real_key, GCONF_VALUE_STRING, gslist, NULL);
	g_free(real_key);
	g_slist_free(gslist);
}

static void gconf_client_notify_func(GConfClient *client, guint cnxn_id,
				     GConfEntry *entry, gpointer user_data)
{
	sigc::signal<void, const baseconfval*> *ch =
                static_cast< sigc::signal<void, const baseconfval*> *>(user_data);
	std::unique_ptr<baseconfval> cv;
	switch (entry->value->type) {
	case GCONF_VALUE_BOOL:
		cv.reset(new confval<bool>);
		static_cast<confval<bool> *>(cv.get())->val_ =
			gconf_value_get_bool(entry->value);
		break;
	case GCONF_VALUE_INT:
		cv.reset(new confval<int>);
		static_cast<confval<int> *>(cv.get())->val_ =
			gconf_value_get_int(entry->value);
		break;
	case GCONF_VALUE_FLOAT:
		cv.reset(new confval<double>);
		static_cast<confval<double> *>(cv.get())->val_ =
			gconf_value_get_float(entry->value);
		break;
	case GCONF_VALUE_STRING: {
		cv.reset(new confval<std::string>);
		const gchar *gconf_val = gconf_value_get_string(entry->value);
		if (gconf_val)
			static_cast<confval<std::string> *>(cv.get())->val_ =
				gconf_val;		
	}
	case GCONF_VALUE_LIST: {
		confval<std::list<std::string> > *newval = 
			new confval<std::list<std::string> >;
		cv.reset(newval);
		GSList *gslist = gconf_value_get_list(entry->value);
		

		GSList *p = gslist; 
		while (p) {
			newval->val_.push_back(static_cast<char *>(p->data));
			p = g_slist_next(p);
		}
		
	}
		break;
	default:
		g_assert(false);
		return;
	}
	ch->emit(cv.get());
}

static void gfree_func(gpointer data)
{
	sigc::signal<void, const baseconfval*> *bcv =
                static_cast< sigc::signal<void, const baseconfval*> *>(data);
	delete bcv;
}

void gconf_file::notify_add(const gchar *sect, const gchar *key, 
			    const sigc::slot<void, const baseconfval*>& slot)
{
	std::string name = std::string(sect) + "/" + key;
	sigc::signal<void, const baseconfval*> *ch =
                new sigc::signal<void, const baseconfval*>;
        ch->connect(slot);
	guint id = gconf_client_notify_add(gconf_client, name.c_str(), 
					   gconf_client_notify_func, ch,
					   gfree_func, NULL);
	notification_ids.push_back(id);
}
