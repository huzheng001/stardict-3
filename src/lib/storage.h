#ifndef _STARDICT_RESOURCE_STORAGE_H_
#define _STARDICT_RESOURCE_STORAGE_H_

#include <string>
#include <list>
#include <glib.h>

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

enum StorageType {
	StorageType_UNKNOWN,
	StorageType_FILE,
	StorageType_DATABASE
};

class ResourceStorage {
public:
	ResourceStorage();
	~ResourceStorage();
	/* dirname in file name encoding */
	static ResourceStorage* create(const std::string &dirname, bool CreateCacheFile,
		show_progress_t *sp);
	/* key in utf-8 */
	FileHolder get_file_path(const std::string& key);
	/* key in utf-8 */
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
