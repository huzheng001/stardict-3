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

#ifndef _STARDICT_RESOURCE_STORAGE_H_
#define _STARDICT_RESOURCE_STORAGE_H_

#include <string>
#include <list>
#include <glib.h>
#include "lib_res_store.h"

class show_progress_t;
class FileBase;
class File_ResourceStorage;
class Database_ResourceStorage;

/* a wrapper for temporary and normal files
 * For normal files it does nothing, for temporary files it manages temp file 
 * life time. A temp file is deleted when the last object pointing to it is 
 * destroyed. */
class FileHolder
{
public:
	FileHolder(void);
	/* url in file name encoding */
	FileHolder(const std::string &url, bool temp);
	FileHolder(const FileHolder& right);
	~FileHolder(void);
	/* return value in file name encoding */
	const char* get_url(void) const;
	bool empty(void) const { return !pFileBase; }
	void clear(void);
	FileHolder& operator=(const FileHolder& right);
	bool operator==(const FileHolder& right) const;
private:
	FileBase *pFileBase;
};

class ResourceStorage {
public:
	ResourceStorage();
	~ResourceStorage();
	/* dirname in file name encoding */
	static ResourceStorage* create(const std::string &dirname, bool CreateCacheFile,
		show_progress_t *sp);
	/* key in utf-8, DB_DIR_SEPARATOR path separator */
	FileHolder get_file_path(const std::string& key);
	/* key in utf-8, DB_DIR_SEPARATOR path separator */
	const char *get_file_content(const std::string &key);
	StorageType get_storage_type(void) const { return storage_type; }
private:
	/* resdir in file name encoding */
	bool load_filesdir(const std::string &resdir, show_progress_t *sp);
	/* rifofilename in file name encoding */
	bool load_database(const std::string &rifofilename, bool CreateCacheFile,
		show_progress_t *sp);
	StorageType storage_type;
	File_ResourceStorage *file_storage;
	Database_ResourceStorage *database_storage;
};

#endif
