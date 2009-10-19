#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "storage.h"
#include "common.hpp"
#include "stddict.hpp"

ResourceStorage::ResourceStorage()
:
	storage_type(StorageType_UNKNOWN),
	file_storage(NULL),
	database_storage(NULL)
{
}

ResourceStorage::~ResourceStorage()
{
	delete file_storage;
	delete database_storage;
}

/* Create a new object and return it if resources exit.
 * Otherwise return NULL. */
ResourceStorage* ResourceStorage::create(const char *dirname,
	show_progress_t *sp)
{
	ResourceStorage *storage = NULL;
	std::string fullfilename;
	fullfilename = dirname;
	fullfilename += G_DIR_SEPARATOR_S "res.rifo";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		storage = new ResourceStorage();
		if(!storage->load_file(fullfilename.c_str(), sp)) {
			delete storage;
			storage = NULL;
		}
	} else {
		fullfilename = dirname;
		fullfilename += G_DIR_SEPARATOR_S "res";
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
			storage = new ResourceStorage();
			if(!storage->load_database(fullfilename.c_str(), sp)) {
				delete storage;
				storage = NULL;
			}
		}
	}
	return storage;
}

bool ResourceStorage::load_file(const char *resdir, show_progress_t *sp)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	file_storage = new File_ResourceStorage(resdir);
	storage_type = StorageType_FILE;
	return true;
}

bool ResourceStorage::load_database(const char *rifofilename, show_progress_t *sp)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	database_storage = new Database_ResourceStorage();
	if(!database_storage->load(rifofilename, sp)) {
		delete database_storage;
		database_storage = NULL;
		return false;
	}
	storage_type = StorageType_DATABASE;
	return true;
}

const char *ResourceStorage::get_file_path(const char *key)
{
	switch(storage_type) {
	case StorageType_FILE:
		return file_storage->get_file_path(key);
	case StorageType_DATABASE:
		return database_storage->get_file_path(key);
	default:
		g_assert_not_reached();
		return NULL;
	}
}

const char *ResourceStorage::get_file_content(const char *key)
{
	switch(storage_type) {
	case StorageType_FILE:
		return file_storage->get_file_content(key);
	case StorageType_DATABASE:
		return database_storage->get_file_content(key);
	default:
		g_assert_not_reached();
		return NULL;
	}
}

File_ResourceStorage::File_ResourceStorage(const char *resdir_)
:
	resdir(resdir_),
	data(NULL)
{
}

File_ResourceStorage::~File_ResourceStorage(void)
{
	if(data)
		g_free(data);
}

const char *File_ResourceStorage::get_file_path(const char *key)
{
	filepath = resdir;
	filepath += G_DIR_SEPARATOR;
	filepath += key;
	if(g_file_test(filepath.c_str(), G_FILE_TEST_EXISTS))
		return filepath.c_str();
	else
		return NULL;
}

const char *File_ResourceStorage::get_file_content(const char *key)
{
	if(key == NULL)
		return NULL;
	if(data) {
		g_free(data);
		data = NULL;
	}
	std::string filename = resdir;
	filename += G_DIR_SEPARATOR;
	filename += key;
	gchar* contents = NULL;
	gsize length = 0;
	if(g_file_get_contents(filename.c_str(), &contents, &length, NULL)) {
		data = (gchar *)g_malloc(length + sizeof(guint32));
		guint32 t = length;
		memcpy(data, &t, sizeof(guint32));
		memcpy(data+sizeof(guint32), contents, length);
		g_free(contents);
		return static_cast<const char*>(data);
	} else
		return NULL;
}

Database_ResourceStorage::Database_ResourceStorage(void)
:
idx_file(NULL)
{
}

Database_ResourceStorage::~Database_ResourceStorage(void)
{
	if(idx_file)
		delete idx_file;
}

bool Database_ResourceStorage::load(const std::string& rifofilename,
	show_progress_t *sp)
{
	gulong filecount, indexfilesize;
	if(!load_rifofile(rifofilename, filecount, indexfilesize))
		return false;
	sp->notify_about_start(_("Loading..."));

	// rifofilename without extension - base file name
	std::string filebasename
		= rifofilename.substr(0, rifofilename.length()-sizeof(".rifo")+1);
	//if(!DictBase::load(filebasename, "rdic"))
	return false;

#if 0
	std::string fullfilename;
	if(idx_file)
		delete idx_file;
	idx_file = NULL;
	idx_file = index_file::Create(filebasename, "ridx", fullfilename);
	idx_file->key_comp_func = strcmp;
	if (!idx_file->load(fullfilename, filecount, indexfilesize,
		false, false, UTF8_GENERAL_CI, sp))
		return false;

	return true;
#endif
}

const char *Database_ResourceStorage::get_file_path(const char *key)
{
	return NULL;
}

const char *Database_ResourceStorage::get_file_content(const char *key)
{
	glong idx, idx_suggest;
	if(!idx_file->lookup(key, idx, idx_suggest))
		return NULL; // key not found
	idx_file->get_data(idx);
	// we got: idx_file->wordentry_offset, idx_file->wordentry_size
	return NULL;
}

bool Database_ResourceStorage::load_rifofile(const std::string& rifofilename,
	gulong& filecount, gulong& indexfilesize)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(rifofilename, DictInfoType_ResDb))
		return false;

	filecount = dict_info.filecount;
	indexfilesize = dict_info.index_file_size;
	return true;
}
