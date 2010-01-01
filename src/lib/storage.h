#ifndef _STARDICT_RESOURCE_STORAGE_H_
#define _STARDICT_RESOURCE_STORAGE_H_

#include <string>
#include <list>
#include <glib.h>

class rindex_file;
class ResDict;
class show_progress_t;
class FileBase;

/* a wrapper for temporary and normal files
 * For normal files it does nothing, for temporary files it manages temp file 
 * life time. A temp file is deleted when the last object pointing to it is 
 * destroyed. */
class FileHolder
{
public:
	FileHolder(void);
	FileHolder(const std::string &url, bool temp);
	FileHolder(const FileHolder& right);
	~FileHolder(void);
	const char* get_url(void) const;
	bool empty(void) const { return !pFileBase; }
	void clear(void);
	FileHolder& operator=(const FileHolder& right);
	bool operator==(const FileHolder& right) const;
private:
	FileBase *pFileBase;
};

class File_ResourceStorage {
public:
	explicit File_ResourceStorage(const char *resdir);
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
	bool load(const std::string& rifofilename, bool CreateCacheFile);
	FileHolder get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	bool load_rifofile(const std::string& rifofilename, gulong& filecount,
		gulong& indexfilesize);
	void clear_cache(void);
	int find_in_cache(const std::string& key) const;
	size_t put_in_cache(const std::string& key, const FileHolder& file);
private:
	static const size_t FILE_CACHE_SIZE = 20;
	/* the entity is not used if key.empty() */
	struct FileCacheEntity
	{
		std::string key;
		FileHolder file;
	};
	FileCacheEntity FileCache[FILE_CACHE_SIZE];
	size_t cur_cache_ind; // index to be reused
	rindex_file *ridx_file;
	ResDict *dict;
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
	static ResourceStorage* create(const char *dirname, bool CreateCacheFile,
		show_progress_t *sp);
	StorageType storage_type;
	FileHolder get_file_path(const char *key);
	const char *get_file_content(const char *key);
private:
	bool load_filesdir(const char *resdir, show_progress_t *sp);
	bool load_database(const char *rifofilename, bool CreateCacheFile,
		show_progress_t *sp);
	File_ResourceStorage *file_storage;
	Database_ResourceStorage *database_storage;
};

#endif
