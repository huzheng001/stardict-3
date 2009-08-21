#include "storage.h"
#include <glib.h>
#include <string.h>

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

bool ResourceStorage::load(const char *dirname)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	std::string resdir(dirname);
	resdir += G_DIR_SEPARATOR_S "res";
	if (g_file_test(resdir.c_str(), G_FILE_TEST_IS_DIR)) {
		file_storage = new File_ResourceStorage(resdir.c_str());
		storage_type = StorageType_FILE;
		return false;
	}
	std::string rifofilename(dirname);
	rifofilename += G_DIR_SEPARATOR_S "res.rifo";
	if (g_file_test(rifofilename.c_str(), G_FILE_TEST_EXISTS)) {
		database_storage = new Database_ResourceStorage();
		bool failed = database_storage->load(rifofilename.c_str());
		if (failed) {
			delete database_storage;
			database_storage = NULL;
			return true;
		}
		storage_type = StorageType_DATABASE;
		return false;
	}
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

Database_ResourceStorage::Database_ResourceStorage()
{
}

bool Database_ResourceStorage::load(const char *rifofilename)
{
	return false;
}

const char *Database_ResourceStorage::get_file_path(const char *key)
{
	return NULL;
}

const char *Database_ResourceStorage::get_file_content(const char *key)
{
	return NULL;
}
