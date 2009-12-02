#ifndef _STARDICT_RESOURCE_STORAGE_H_
#define _STARDICT_RESOURCE_STORAGE_H_

#include <string>
#include <glib.h>

class index_file;
class show_progress_t;

class File_ResourceStorage {
public:
	File_ResourceStorage(const char *resdir);
	~File_ResourceStorage(void);
	const char *get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	std::string resdir;
	std::string filepath;
	gchar* data;
};

class Database_ResourceStorage {
public:
	Database_ResourceStorage(void);
	~Database_ResourceStorage(void);
	bool load(const std::string& rifofilename, show_progress_t *sp);
	const char *get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	bool load_rifofile(const std::string& rifofilename, gulong& filecount,
		gulong& indexfilesize);
private:
	index_file *idx_file;
};

enum StorageType {
	StorageType_UNKNOWN,
	StorageType_FILE,
	StorageType_DATABASE
};

class ResourceStorage {
public:
	ResourceStorage();
	~ResourceStorage();
	static ResourceStorage* create(const char *dirname, show_progress_t *sp);
	StorageType storage_type;
	const char *get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	bool load_filesdir(const char *resdir, show_progress_t *sp);
	bool load_database(const char *rifofilename, show_progress_t *sp);
	File_ResourceStorage *file_storage;
	Database_ResourceStorage *database_storage;
};

#endif
