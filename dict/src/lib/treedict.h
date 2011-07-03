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

#ifndef _TREEDICT_HPP_
#define _TREEDICT_HPP_

#include <gtk/gtk.h>

#include "ifo_file.h"
#include "dictbase.h"

typedef std::list<std::string> strlist_t;

class TreeDict : public DictBase {
public:
	TreeDict();
	bool load(const std::string& ifofilename);
	static GtkTreeStore *get_model() { return model; }
private:
	static GtkTreeStore *model;

	bool load_ifofile(const std::string& ifofilename, gulong *tdxfilesize);
	void load_model(gchar **buffer, GtkTreeIter *parent, guint32 count);
};

class TreeDicts {
public:
	TreeDicts();
	~TreeDicts();
	void load_dict(const std::string& url);
	GtkTreeStore* Load(const strlist_t& tree_dicts_dirs,
			   const strlist_t& order_list,
			   const strlist_t& disable_list);
	gchar * poGetWordData(guint32 offset, guint32 size, int iTreeDict);
private:
	std::vector<TreeDict *> oTreeDict;
};

#endif//!_TREEDICT_HPP_
