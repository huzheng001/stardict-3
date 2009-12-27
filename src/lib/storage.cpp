#include <glib.h>
#include <string.h>
#include <glib/gi18n.h>
#include "storage.h"
#include "storage_impl.h"
#include "common.hpp"
#include "stddict.hpp"
#include "getuint32.h"

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
	oft_file(CacheFileType_oft),
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
	if (!oft_file.load_cache(url, url, CollateFunctions(0), npages*sizeof(guint32))) {
		MapFile map_file;
		if (!map_file.open(url.c_str(), fsize))
			return false;
		const gchar *idxdatabuffer=map_file.begin();
		/* oft_file.wordoffset[i] holds offset of the i-th page in the index file */
		oft_file.allocate_wordoffset(npages*sizeof(guint32));
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
			if (!oft_file.save_cache(url, CollateFunctions(0), npages))
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
		fread(&page_data[0], 1, page_data.size(), idxfile);
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
	bool CreateCacheFile, show_progress_t *sp)
{
	ResourceStorage *storage = NULL;
	std::string fullfilename;
	fullfilename = dirname;
	fullfilename += G_DIR_SEPARATOR_S "res.rifo";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		storage = new ResourceStorage();
		if(!storage->load_database(fullfilename.c_str(), CreateCacheFile, sp)) {
			delete storage;
			storage = NULL;
		}
	} else {
		fullfilename = dirname;
		fullfilename += G_DIR_SEPARATOR_S "res";
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
			storage = new ResourceStorage();
			if(!storage->load_filesdir(fullfilename.c_str(), sp)) {
				delete storage;
				storage = NULL;
			}
		}
	}
	return storage;
}

bool ResourceStorage::load_filesdir(const char *resdir, show_progress_t *sp)
{
	g_assert(storage_type == StorageType_UNKNOWN);
	g_assert(file_storage == NULL && database_storage == NULL);
	file_storage = new File_ResourceStorage(resdir);
	storage_type = StorageType_FILE;
	return true;
}

bool ResourceStorage::load_database(const char *rifofilename, 
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
ridx_file(NULL)
{
}

Database_ResourceStorage::~Database_ResourceStorage(void)
{
	if(ridx_file)
		delete ridx_file;
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
	//if(!DictBase::load(filebasename, "rdic"))
	//return false;

	std::string fullfilename;
	if(ridx_file)
		delete ridx_file;
	ridx_file = NULL;
	ridx_file = rindex_file::Create(filebasename, "ridx", fullfilename);
	if (!ridx_file->load(fullfilename, filecount, indexfilesize, CreateCacheFile))
		return false;
	return true;
}

const char *Database_ResourceStorage::get_file_path(const char *key)
{
	guint32 entry_offset, entry_size;
	if(!ridx_file->lookup(key, entry_offset, entry_size))
		return NULL; // key not found
	return NULL;
}

const char *Database_ResourceStorage::get_file_content(const char *key)
{
	guint32 entry_offset, entry_size;
	if(!ridx_file->lookup(key, entry_offset, entry_size))
		return NULL; // key not found
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
