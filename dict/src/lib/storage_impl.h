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

#ifndef STORAGE_IMPL_H_
#define STORAGE_IMPL_H_

#include "stddict.h"
#include "storage.h"

class ResDict;

class rindex_file
{
public:
	rindex_file(void);
	virtual ~rindex_file() {}
	/* filebasename, mainext, fullfilename in file name encoding */
	static rindex_file* Create(const std::string& filebasename,
			const char* mainext, std::string& fullfilename);
	/* url in file name encoding */
	virtual bool load(const std::string& url, gulong _filecount, gulong fsize,
		bool CreateCacheFile) = 0;
	/* str in utf-8 */
	virtual bool lookup(const char *str, guint32 &entry_offset, guint32 &entry_size) = 0;
protected:
	// number of files in the index
	gulong filecount;
};

class offset_rindex: public rindex_file
{
public:
	offset_rindex(void);
	~offset_rindex(void);
	/* url in file name encoding */
	bool load(const std::string& url, gulong _filecount, gulong fsize,
		bool CreateCacheFile);
	/* str in utf-8 */
	bool lookup(const char *str, guint32 &entry_offset, guint32 &entry_size);
private:
	/* str in utf-8 */
	bool lookup(const char *str, glong &idx);
	const gchar *read_first_on_page_key(glong page_idx);
	const gchar *get_first_on_page_key(glong page_idx);
	const gchar *get_key(glong idx);
	void get_data(glong idx, guint32 &entry_offset, guint32 &entry_size);
	gulong load_page(glong page_idx);

	static const gint ENTR_PER_PAGE=32;
	/* default length of a key in the index. may be any integer > 0.
	 * Size of a key in index is not limited. Use this value to allocate a buffer
	 * and read the first chunk from the index file. */
	static const size_t DEFAULT_KEY_SIZE=256;
	/* oft_file.get_wordoffset(page_num) - offset of the first element on the page
	 * number page_num. 0<= page_num <= npages-2
	 * oft_file.get_wordoffset(npages-1) - offset of the next to the last element 
	 * in the index file
	 * oft_file.get_wordoffset(page_num+1) - oft_file.get_wordoffset(page_num) 
	 * - size of data on the page number page_num, in bytes. */
	cache_file oft_file;
	FILE *idxfile;
	/* use this buffer to read keys */
	std::vector<char> buffer;
	/* number of pages = ((filecount-1)/ENTR_PER_PAGE) + 2 
	 * The page number npages-2 always contains at least one element.
	 * It may contain from 1 to ENTR_PER_PAGE elements.
	 * To be exact it contains nentr elements, that may be calculated as follows:
	 * nentr = filecount%ENTR_PER_PAGE;
	 * if(nentr == 0)
	 *   nentr = ENTR_PER_PAGE;
	 * The page number npages-1 (the last) is always empty. */
	gulong npages;
	struct index_entry {
		glong page_idx;
		std::string keystr; // in utf-8
		void assign(glong i, const std::string& str) {
			page_idx=i;
			keystr.assign(str);
		}
	};
	/* first - first key on the first page - first key in the index
	 * last - first key on the pre-last page (last page addressing real data)
	 * middle - first key on the middle page
	 * read_last - last key in the index */
	index_entry first, last, middle, real_last;

	struct page_entry {
		gchar *keystr; // in utf-8
		guint32 off, size;
	};
	std::vector<gchar> page_data;
	struct page_t {
		glong page_idx;
		page_entry entries[ENTR_PER_PAGE];

		page_t(): page_idx(-1) {}
		void fill(gchar *data, gint nent, glong page_idx_);
	} page;
};

class compressed_rindex: public rindex_file
{
public:
	compressed_rindex(void);
	~compressed_rindex(void);
	/* url in file name encoding */
	bool load(const std::string& url, gulong _filecount, gulong fsize,
		bool CreateCacheFile);
	/* str in utf-8 */
	bool lookup(const char *str, guint32 &entry_offset, guint32 &entry_size);
private:
	/* str in utf-8 */
	bool lookup(const char *str, glong &idx);
	/* return value in utf-8 */
	const gchar *get_key(glong idx);
	void get_data(glong idx, guint32 &entry_offset, guint32 &entry_size);

	/* whole uncompressed index file in memory */
	gchar *idxdatabuf;
	/* pointers to the files-keys in idxdatabuf. Each word is '\0'-terminated and 
	 * followed by data offset and size. See ".ridx" file format. 
	 * filelist.size() == number of files + 1 */
	std::vector<gchar *> filelist;
};

class File_ResourceStorage {
public:
	/* resdir in file name encoding */
	explicit File_ResourceStorage(const std::string &resdir);
	~File_ResourceStorage(void);
	/* key in utf-8, DB_DIR_SEPARATOR path separator, 
	 * return value in file name encoding */
	const std::string& get_file_path(const std::string &key);
	/* key in utf-8, DB_DIR_SEPARATOR path separator */
	const char *get_file_content(const std::string &key);
private:
	std::string resdir; // in file name encoding
	/* get_file_path function result, in file name encoding */
	std::string filepath;
	gchar* data;
};

class Database_ResourceStorage {
public:
	Database_ResourceStorage(void);
	~Database_ResourceStorage(void);
	/* rifofilename in file name encoding */
	bool load(const std::string& rifofilename, bool CreateCacheFile);
	/* key in utf-8, DB_DIR_SEPARATOR path separator */
	FileHolder get_file_path(const std::string &key);
	/* key in utf-8, DB_DIR_SEPARATOR path separator */
	const char *get_file_content(const std::string &key);
private:
	/* rifofilename in file name encoding */
	bool load_rifofile(const std::string& rifofilename, gulong& filecount,
		gulong& indexfilesize);
	void clear_cache(void);
	/* key in utf-8 */
	int find_in_cache(const std::string& key) const;
	/* key in utf-8 */
	size_t put_in_cache(const std::string& key, const FileHolder& file);
private:
	static const size_t FILE_CACHE_SIZE = 20;
	/* the entity is not used if key.empty() */
	struct FileCacheEntity
	{
		std::string key; // in utf-8
		FileHolder file;
	};
	FileCacheEntity FileCache[FILE_CACHE_SIZE];
	size_t cur_cache_ind; // index to be reused
	rindex_file *ridx_file;
	ResDict *dict;
};

#endif /* STORAGE_IMPL_H_ */
