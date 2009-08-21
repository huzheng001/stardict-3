#ifndef _STARDICT_RESOURCE_STORAGE_H_
#define _STARDICT_RESOURCE_STORAGE_H_

#include <string>
#include <glib.h>

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
	Database_ResourceStorage();
	bool load(const char *rifofilename);
	const char *get_file_path(const char *key);
	const char *get_file_content(const char *key);
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
	bool load(const char *dirname);
	StorageType storage_type;
	const char *get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	File_ResourceStorage *file_storage;
	Database_ResourceStorage *database_storage;
};

#endif
