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

#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#ifdef _WIN32
#  include <io.h>
#else
#  include <unistd.h>
#endif
#include "storage.h"
#include "storage_impl.h"
#include "ifo_file.h"
#include "stddict.h"
#include "utils.h"
#include "dictziplib.h"

/* permanent or temporary file */
class FileBase
{
private:
	/* url_ in file name encoding */
	explicit FileBase(const std::string& url_, bool temp_)
	:
		url(url_),
		cnt(0),
		temp(temp_)
	{
	}
	virtual ~FileBase(void)
	{
		if(!url.empty() && temp)
			if(g_remove(url.c_str()))
				g_warning("Unable to remove temporary file: %s", url.c_str());
	}
public:
	/* url in file name encoding */
	static FileBase* create(const std::string& url, bool temp)
	{
		return new FileBase(url, temp);
	}
	const std::string& get_url(void) const { return url; }
	void AddRef(void) { ++cnt; }
	void Release(void)
	{ 
		--cnt;
		if(cnt == 0)
			delete this;
	}
private:
	std::string url; // in file name encoding
	size_t cnt;
	bool temp;
};


FileHolder::FileHolder(void)
:
	pFileBase(NULL)
{
	
}

FileHolder::FileHolder(const std::string &url, bool temp)
:
	pFileBase(NULL)
{
	if(!url.empty()) {
		pFileBase = FileBase::create(url, temp);
		if(pFileBase)
			pFileBase->AddRef();
	}
}

FileHolder::FileHolder(const FileHolder& right)
{
	pFileBase = right.pFileBase;
	if(pFileBase)
		pFileBase->AddRef();
}

FileHolder::~FileHolder(void)
{
	if(pFileBase)
		pFileBase->Release();
}

const char* FileHolder::get_url(void) const
{
	if(pFileBase)
		return pFileBase->get_url().c_str();
	else
		return NULL;
}

FileHolder& FileHolder::operator=(const FileHolder& right)
{
	if(this == &right)
		return *this;
	if(right.pFileBase)
		right.pFileBase->AddRef();
	if(pFileBase)
		pFileBase->Release();
	pFileBase = right.pFileBase;
	return *this;
}

bool FileHolder::operator==(const FileHolder& right) const
{
	if(pFileBase == right.pFileBase)
		return true;
	if(!pFileBase || !right.pFileBase)
		return false;
	return pFileBase->get_url() == right.pFileBase->get_url();
}

void FileHolder::clear(void)
{
	if(pFileBase)
		pFileBase->Release();
	pFileBase = NULL;
}

/* Open a temporary file for writing
 * 
 * Parameters:
 * pattern - file name pattern in file name encoding
 * 1. blank, then default pattern is used
 * 2. may be a string with "XXXXXX" substring indicating variable part
 * 3. may be a simple string, then "XXXXXX" will be added to the front
 * 
 * fh - file handle, close it with 
#if defined(_WIN32)
 * _close()
#else
 * close() 
#endif
 * when not needed. 
 * -1 if file open failed. */
static FileHolder open_temp_file(const std::string &pattern, gint &fh)
{
	std::string name_pattern(pattern);
	if(!name_pattern.empty()) {
		if(name_pattern.find("XXXXXX")==std::string::npos)
			name_pattern.insert(0, "XXXXXX");
	}
	gchar * tmp_url = NULL;
	fh = stardict_file_open_tmp(name_pattern.empty() ? NULL : name_pattern.c_str(), 
		&tmp_url, NULL);
	FileHolder file(fh == -1 ? "" : tmp_url, true);
	g_free(tmp_url);
	return file;
}

struct ResCacheItem {
	guint32 offset;
	guint32 size;
	gchar *data;
	ResCacheItem(void) { data = NULL; }
	~ResCacheItem(void) { g_free(data); }
};

class ResDict {
public:
	ResDict(void);
	~ResDict(void);
	bool load(const std::string& base_url);
	gchar *GetData(guint32 offset, guint32 size);
private:
	static const size_t RES_DICT_CACHE_SIZE = 10;
	FILE *dictfile;
	std::unique_ptr<dictData> dictdzfile;
	ResCacheItem cache[RES_DICT_CACHE_SIZE];
	gint cache_cur;
};

rindex_file::rindex_file(void)
:
	filecount(0)
{
	
}

rindex_file* rindex_file::Create(const std::string& filebasename,
			const char* mainext, std::string& fullfilename)
{
	rindex_file *rindex = NULL;

	fullfilename = filebasename + "." + mainext + ".gz";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		rindex = new compressed_rindex;
	} else {
		fullfilename = filebasename + "." + mainext;
		rindex = new offset_rindex;
	}
	return rindex;
}

offset_rindex::offset_rindex(void)
:
	oft_file(CacheFileType_oft, COLLATE_FUNC_NONE),
	idxfile(NULL),
	npages(0)
{

}

offset_rindex::~offset_rindex(void)
{
	if (idxfile)
		fclose(idxfile);
}

bool offset_rindex::load(const std::string& url, gulong _filecount, gulong fsize,
	bool CreateCacheFile)
{
	filecount = _filecount;
	npages=(filecount-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, npages*sizeof(guint32))) {
		MapFile map_file;
		if (!map_file.open(url.c_str(), fsize))
			return false;
		const gchar *idxdatabuffer=map_file.begin();
		/* oft_file.wordoffset[i] holds offset of the i-th page in the index file */
		oft_file.allocate_wordoffset(npages);
		const gchar *p1 = idxdatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<filecount; i++) {
			index_size=strlen(p1) +1 + 2*sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.get_wordoffset(j)=p1-idxdatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.get_wordoffset(j)=p1-idxdatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url))
				g_printerr("Cache update failed.\n");
		}
	}

	if (!(idxfile = fopen(url.c_str(), "rb"))) {
		return false;
	}

	first.assign(0, read_first_on_page_key(0));
	last.assign(npages-2, read_first_on_page_key(npages-2));
	middle.assign((npages-2)/2, read_first_on_page_key((npages-2)/2));
	real_last.assign(filecount-1, get_key(filecount-1));

	return true;
}

bool offset_rindex::lookup(const char *str, guint32 &entry_offset, guint32 &entry_size)
{
	glong idx;
	if(!lookup(str, idx))
		return false;
	get_data(idx, entry_offset, entry_size);
	return true;
}

bool offset_rindex::lookup(const char *str, glong &idx)
{
	bool bFound=false;
	glong iTo=npages-2;
	glong iFrom;
	glong iThisIndex;
	gint cmpint;
	if (strcmp(str, first.keystr.c_str())<0) {
		return false;
	} else if (strcmp(str, real_last.keystr.c_str()) >0) {
		return false;
	} else {
		iFrom=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = strcmp(str, get_first_on_page_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound) {
			// iTo - prev. page
			// the key must on page iTo if anywhere
			idx = iTo; 
		} else {
			idx = iThisIndex; // the key found in the first position of this page
		}
	}
	// idx now is a page number where the key may be
	if (!bFound) {
		gulong netr=load_page(idx);
		iFrom=1; // Needn't search the first word anymore.
		iTo=netr-1;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = strcmp(str, page.entries[iThisIndex].keystr);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if(!bFound)
			return false;
		idx *= ENTR_PER_PAGE;
		idx += iThisIndex;
		return true;
	} else {
		idx *= ENTR_PER_PAGE;
		return true;
	}
}

const gchar *offset_rindex::read_first_on_page_key(glong page_idx)
{
	fseek(idxfile, oft_file.get_wordoffset(page_idx), SEEK_SET);
	size_t size_to_read = DEFAULT_KEY_SIZE;
	const char* end_of_line;
	int inewdata = 0;
	size_t size_read;
	while(true) {
		buffer.resize(inewdata+size_to_read+1);
		size_read = fread(&buffer[inewdata], 1, size_to_read, idxfile);
		// size_read <= size_to_read
		if(size_read == 0) // nothing read
			return NULL;
		buffer[inewdata+size_read]='\0';
		end_of_line = strchr(&buffer[inewdata], '\0');
		if(end_of_line != &buffer[inewdata+size_read])
			return &buffer[0];
		if(size_read != size_to_read) // no more data
			return NULL;
		inewdata += size_read;
		size_to_read *= 2;
	}
	return NULL;
}

const gchar *offset_rindex::get_first_on_page_key(glong page_idx)
{
	if (page_idx<middle.page_idx) {
		if (page_idx==first.page_idx)
			return first.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else if (page_idx>middle.page_idx) {
		if (page_idx==last.page_idx)
			return last.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else
		return middle.keystr.c_str();
}

const gchar *offset_rindex::get_key(glong idx)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	return page.entries[idx_in_page].keystr;
}

void offset_rindex::get_data(glong idx, guint32 &entry_offset, guint32 &entry_size)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	entry_offset = page.entries[idx_in_page].off;
	entry_size = page.entries[idx_in_page].size;
}

gulong offset_rindex::load_page(glong page_idx)
{
	g_assert(gulong(page_idx+1)<npages);
	gulong nentr=ENTR_PER_PAGE;
	if (page_idx==glong(npages-2))
		if ((nentr=filecount%ENTR_PER_PAGE)==0)
			nentr=ENTR_PER_PAGE;


	if (page_idx!=page.page_idx) {
		page_data.resize(oft_file.get_wordoffset(page_idx+1)-oft_file.get_wordoffset(page_idx));
		fseek(idxfile, oft_file.get_wordoffset(page_idx), SEEK_SET);
		size_t fread_size;
		size_t page_data_size = page_data.size();
		fread_size = fread(&page_data[0], 1, page_data_size, idxfile);
		if (fread_size != page_data_size) {
			g_print("fread error!\n");
		}
		page.fill(&page_data[0], nentr, page_idx);
	}

	return nentr;
}

void offset_rindex::page_t::fill(gchar *data, gint nent, glong page_idx_)
{
	page_idx=page_idx_;
	gchar *p=data;
	glong len;
	for (gint i=0; i<nent; ++i) {
		entries[i].keystr=p;
		len=strlen(p);
		p+=len+1;
		entries[i].off=g_ntohl(get_uint32(p));
		p+=sizeof(guint32);
		entries[i].size=g_ntohl(get_uint32(p));
		p+=sizeof(guint32);
	}
}

compressed_rindex::compressed_rindex(void)
:
	idxdatabuf(NULL)
{
	
}

compressed_rindex::~compressed_rindex(void)
{
	g_free(idxdatabuf);
}

bool compressed_rindex::load(const std::string& url, gulong _filecount,
	gulong fsize, bool CreateCacheFile)
{
	filecount = _filecount;
	gzFile in = gzopen(url.c_str(), "rb");
	if (in == NULL)
		return false;

	idxdatabuf = (gchar *)g_malloc(fsize);

	gulong len = gzread(in, idxdatabuf, fsize);
	gzclose(in);
	if (len < 0)
		return false;

	if (len != fsize)
		return false;

	filelist.resize(filecount+1);
	gchar *p1 = idxdatabuf;
	guint32 i;
	for (i=0; i<filecount; i++) {
		filelist[i] = p1;
		p1 += strlen(p1) +1 + 2*sizeof(guint32);
	}
	/* pointer to the next to last file entry */
	filelist[filecount] = p1;

	return true;
}

bool compressed_rindex::lookup(const char *str, guint32 &entry_offset, guint32 &entry_size)
{
	glong idx;
	if(!lookup(str, idx))
		return false;
	get_data(idx, entry_offset, entry_size);
	return true;
}

bool compressed_rindex::lookup(const char *str, glong &idx)
{
	bool bFound=false;
	glong iTo=filelist.size()-2;

	if (strcmp(str, get_key(0))<0) {
	} else if (strcmp(str, get_key(iTo)) >0) {
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = strcmp(str, get_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (bFound) {
			idx = iThisIndex;
		}
	}
	return bFound;
}

const gchar *compressed_rindex::get_key(glong idx)
{
	return filelist[idx];
}

void compressed_rindex::get_data(glong idx, guint32 &entry_offset, guint32 &entry_size)
{
	gchar *p1 = filelist[idx]+strlen(filelist[idx])+sizeof(gchar);
	entry_offset = g_ntohl(get_uint32(p1));
	p1 += sizeof(guint32);
	entry_size = g_ntohl(get_uint32(p1));
}

ResDict::ResDict(void)
:
	dictfile(NULL),
	cache_cur(0)
{
}

ResDict::~ResDict(void)
{
	if(dictfile)
		fclose(dictfile);
}

bool ResDict::load(const std::string& base_url)
{
	std::string url;
	url = base_url + ".rdic.dz";
	if (g_file_test(url.c_str(), G_FILE_TEST_EXISTS)) {
		dictdzfile.reset(new dictData);
		if (!dictdzfile->open(url, 0)) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	} else {
		url = base_url + ".rdic";
		dictfile = fopen(url.c_str(),"rb");
		if (!dictfile) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	}
	return true;
}

gchar *ResDict::GetData(guint32 offset, guint32 size)
{
	for(size_t i=0; i<RES_DICT_CACHE_SIZE; ++i)
		if(cache[i].data && cache[i].offset == offset && cache[i].size == size)
			return cache[i].data;

	gchar *data = (gchar*)g_malloc(size + sizeof(guint32));
	memcpy(data, &size, sizeof(guint32));
	if(dictfile) {
		fseek(dictfile, offset, SEEK_SET);
		size_t fread_size;
		fread_size = fread(data+sizeof(guint32), size, 1, dictfile);
		if (fread_size != 1) {
			g_print("fread error!\n");
		}
	} else {
		dictdzfile->read(data+sizeof(guint32), offset, size);
	}
	g_free(cache[cache_cur].data);
	cache[cache_cur].data = data;
	cache[cache_cur].offset = offset;
	cache[cache_cur].size = size;
	cache_cur = (cache_cur + 1) % RES_DICT_CACHE_SIZE;
	return data;
}

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
ResourceStorage* ResourceStorage::create(const std::string &dirname,
	bool CreateCacheFile, show_progress_t *sp)
{
	ResourceStorage *storage = NULL;
	std::string fullfilename;
	fullfilename = build_path(dirname, "res.rifo");
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		storage = new ResourceStorage();
		if(!storage->load_database(fullfilename, CreateCacheFile, sp)) {
			delete storage;
			storage = NULL;
		}
	} else {
		fullfilename = build_path(dirname, "res");
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
			storage = new ResourceStorage();
			if(!storage->load_filesdir(fullfilename, sp)) {
				delete storage;
				storage = NULL;
			}
		}
	}
	return storage;
}

bool ResourceStorage::load_filesdir(const std::string &resdir, show_progress_t *sp)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	file_storage = new File_ResourceStorage(resdir);
	storage_type = StorageType_FILE;
	return true;
}

bool ResourceStorage::load_database(const std::string &rifofilename, 
	bool CreateCacheFile, show_progress_t *sp)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	database_storage = new Database_ResourceStorage();
	if(!database_storage->load(rifofilename, CreateCacheFile)) {
		delete database_storage;
		database_storage = NULL;
		return false;
	}
	storage_type = StorageType_DATABASE;
	return true;
}

FileHolder ResourceStorage::get_file_path(const std::string &key)
{
	switch(storage_type) {
	case StorageType_FILE:
		return FileHolder(file_storage->get_file_path(key), false);
	case StorageType_DATABASE:
		return database_storage->get_file_path(key);
	default:
		g_assert_not_reached();
		return FileHolder();
	}
}

const char *ResourceStorage::get_file_content(const std::string &key)
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

File_ResourceStorage::File_ResourceStorage(const std::string &resdir_)
:
	resdir(resdir_),
	data(NULL)
{
}

File_ResourceStorage::~File_ResourceStorage(void)
{
	g_free(data);
}

const std::string& File_ResourceStorage::get_file_path(const std::string &key)
{
	filepath.clear();
	if(key.empty())
		return filepath;
	std::string fs_key;
	if(!utf8_to_file_name(dir_separator_db_to_fs(key), fs_key))
		return filepath;
	filepath = resdir;
	filepath += G_DIR_SEPARATOR;
	filepath += fs_key;
	if(!g_file_test(filepath.c_str(), G_FILE_TEST_EXISTS))
		filepath.clear();
	return filepath;
}

const char *File_ResourceStorage::get_file_content(const std::string &key)
{
	if(key.empty())
		return NULL;
	if(data) {
		g_free(data);
		data = NULL;
	}
	std::string fs_key;
	if(!utf8_to_file_name(dir_separator_db_to_fs(key), fs_key))
		return NULL;
	std::string filename = resdir;
	filename += G_DIR_SEPARATOR;
	filename += fs_key;
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
cur_cache_ind(0),
ridx_file(NULL),
dict(NULL)
{
}

Database_ResourceStorage::~Database_ResourceStorage(void)
{
	delete ridx_file;
	delete dict;
}

bool Database_ResourceStorage::load(const std::string& rifofilename,
	bool CreateCacheFile)
{
	gulong filecount, indexfilesize;
	if(!load_rifofile(rifofilename, filecount, indexfilesize))
		return false;

	// rifofilename without extension - base file name
	std::string filebasename
		= rifofilename.substr(0, rifofilename.length()-sizeof(".rifo")+1);
	delete dict;
	dict = NULL;
	dict = new ResDict;
	if(!dict->load(filebasename))
		return false;

	std::string fullfilename;
	delete ridx_file;
	ridx_file = NULL;
	ridx_file = rindex_file::Create(filebasename, "ridx", fullfilename);
	if (!ridx_file->load(fullfilename, filecount, indexfilesize, CreateCacheFile))
		return false;
	return true;
}

FileHolder Database_ResourceStorage::get_file_path(const std::string& key)
{
	int ind = find_in_cache(key);
	if(ind >= 0)
		return FileCache[ind].file;

	guint32 entry_offset, entry_size;
	if(!ridx_file->lookup(key.c_str(), entry_offset, entry_size))
		return FileHolder(); // key not found
	gchar *data = dict->GetData(entry_offset, entry_size);
	if(!data)
		return FileHolder();

	std::string name_pattern; // in file name encoding
	if(!utf8_to_file_name(key, name_pattern))
		return FileHolder();
	std::string::size_type pos = name_pattern.find_last_of("." DB_DIR_SEPARATOR_S);
	if(pos != std::string::npos) {
		if(name_pattern[pos] == '.')
			name_pattern = name_pattern.substr(pos);
		else
			name_pattern.clear();
	} else
		name_pattern.clear();
	gint fd;
	FileHolder file(open_temp_file(name_pattern, fd));
	if(file.empty())
		return file;
	ssize_t write_size;
#ifdef _WIN32
	write_size = _write(fd, data+sizeof(guint32), entry_size);
	if (write_size == -1) {
		g_print("write error!\n");
	}
	_close(fd);
#else
	write_size = write(fd, data+sizeof(guint32), entry_size);
	if (write_size == -1) {
		g_print("write error!\n");
	}
	close(fd);
#endif
	ind = put_in_cache(key, file);
	return FileCache[ind].file;
}

const char *Database_ResourceStorage::get_file_content(const std::string &key)
{
	guint32 entry_offset, entry_size;
	if(!ridx_file->lookup(key.c_str(), entry_offset, entry_size))
		return NULL; // key not found
	return dict->GetData(entry_offset, entry_size);
}

void Database_ResourceStorage::clear_cache(void)
{
	for(size_t i=0; i<FILE_CACHE_SIZE; ++i) {
		FileCache[i].key.clear();
		FileCache[i].file.clear();
	}
}

int Database_ResourceStorage::find_in_cache(const std::string& key) const
{
	for(size_t i=0; i<FILE_CACHE_SIZE; ++i)
		if(FileCache[i].key == key)
			return i;
	return -1;
}

size_t Database_ResourceStorage::put_in_cache(const std::string& key, const FileHolder& file)
{
	size_t ind = cur_cache_ind;
	FileCache[ind].key = key;
	FileCache[ind].file = file;
	cur_cache_ind = (ind + 1) % FILE_CACHE_SIZE;
	return ind;
}

bool Database_ResourceStorage::load_rifofile(const std::string& rifofilename,
	gulong& filecount, gulong& indexfilesize)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(rifofilename, DictInfoType_ResDb))
		return false;

	filecount = dict_info.get_filecount();
	indexfilesize = dict_info.get_index_file_size();
	return true;
}
