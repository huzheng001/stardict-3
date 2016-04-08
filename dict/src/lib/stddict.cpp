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

/*
 * Implementation of class to work with standard StarDict's dictionaries
 * lookup word, get articles and so on.
 *
 * Notice: read doc/StarDictFileFormat for the dictionary
 * file's format information!
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <algorithm>
#include <memory>

#include "ifo_file.h"
#include "edit-distance.h"
//#include "kmp.h"
#include "mapfile.h"
#include "iappdirs.h"

#include "stddict.h"
#include "utils.h"

static gint stardict_collate(const gchar *str1, const gchar *str2, CollateFunctions func)
{
	gint x = utf8_collate(str1, str2, func);
	if (x == 0)
		return strcmp(str1, str2);
	else
		return x;
}

gint stardict_server_collate(const gchar *str1, const gchar *str2, CollationLevelType CollationLevel, CollateFunctions func, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return stardict_strcmp(str1, str2);
	if (CollationLevel == CollationLevel_SINGLE)
		return stardict_collate(str1, str2, func);
	if (servercollatefunc == 0)
		return stardict_strcmp(str1, str2);
	return stardict_collate(str1, str2, (CollateFunctions)(servercollatefunc-1));
}

// not perfect case-insensitive comparison of strings
static gint stardict_strcasecmp(const gchar *s1, const gchar *s2)
{
	gchar *sci1 = g_utf8_casefold(s1, -1);
	gchar *sci2 = g_utf8_casefold(s2, -1);
	gint res = g_utf8_collate(sci1, sci2);
	g_free(sci1);
	g_free(sci2);
	return res;
}

static gint stardict_casecmp(const gchar *s1, const gchar *s2, CollationLevelType CollationLevel, CollateFunctions func, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return stardict_strcasecmp(s1, s2);
	if (CollationLevel == CollationLevel_SINGLE)
		return utf8_collate(s1, s2, func);
	if (servercollatefunc == 0)
		return stardict_strcasecmp(s1, s2);
	return utf8_collate(s1, s2, (CollateFunctions)(servercollatefunc-1));
}

/* return the length of the common prefix of two strings in characters 
 * comparison is case-insensitive */
static inline gint prefix_match(const gchar *s1, const gchar *s2)
{
    if(!s1 || !s2)
        return 0;
    gint ret=-1;
    gunichar u1, u2;
    do {
        u1 = g_utf8_get_char(s1);
        u2 = g_utf8_get_char(s2);
        s1 = g_utf8_next_char(s1);
        s2 = g_utf8_next_char(s2);
        ret++;
    } while (u1 && g_unichar_tolower(u1) == g_unichar_tolower(u2));
    return ret;
}

/* check that string str has length allowed for index word 
 * strlen(str) < MAX_INDEX_KEY_SIZE 
 * This function does not read more than MAX_INDEX_KEY_SIZE or buf_size chars,
 * which one is smaller.
 * return value: 
 * true - ok,
 * false - string length exceeded. */
static bool check_key_str_len(const gchar* str, size_t buf_size)
{
	size_t max = MAX_INDEX_KEY_SIZE;
	if(buf_size < max)
		max = buf_size;
	for(size_t i = 0; i < max; ++i)
		if(!str[i])
			return true;
	return false;
}

static inline bool bIsVowel(gchar inputchar)
{
  gchar ch = g_ascii_toupper(inputchar);
  return( ch=='A' || ch=='E' || ch=='I' || ch=='O' || ch=='U' );
}

class offset_index : public index_file {
public:
	offset_index();
	~offset_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, CollationLevelType CollationLevel,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
private:
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);

	static const gint ENTR_PER_PAGE=32;

	/* oft_file.get_wordoffset(page_num) - offset of the first element on the page
	 * number page_num. 0<= page_num <= npages-2
	 * oft_file.get_wordoffset(npages-1) - offset of the next to the last element 
	 * in the index file
	 * oft_file.get_wordoffset(page_num+1) - oft_file.get_wordoffset(page_num) 
	 * - size of data on the page number page_num, in bytes. */
	cache_file oft_file;
	FILE *idxfile;
	/* number of pages = ((wordcount-1)/ENTR_PER_PAGE) + 2 
	 * The page number npages-2 always contains at least one element.
	 * It may contain from 1 to ENTR_PER_PAGE elements.
	 * To be exact it contains nentr elements, that may be calculated as follows:
	 * nentr = wordcount%ENTR_PER_PAGE;
	 * if(nentr == 0)
	 *   nentr = ENTR_PER_PAGE;
	 * The page number npages-1 (the last) is always empty. */
	gulong npages;

	// The length of "word_str" should be less than MAX_INDEX_KEY_SIZE. 
	// See doc/StarDictFileFormat.
	gchar wordentry_buf[MAX_INDEX_KEY_SIZE+sizeof(guint32)*2];
	struct index_entry {
		glong idx; // page number
		std::string keystr;
		void assign(glong i, const std::string& str) {
			idx=i;
			keystr.assign(str);
		}
	};
	/* first - first word on the first page - first word in the index
	 * last - first word on the pre-last page (last page addressing real data)
	 * middle - first word on the middle page
	 * read_last - last word in the index */
	index_entry first, last, middle, real_last;

	struct page_entry {
		gchar *keystr;
		guint32 off, size;
	};
	std::vector<gchar> page_data;
	struct page_t {
		glong idx;
		page_entry entries[ENTR_PER_PAGE];

		page_t(): idx(-1) {}
		void fill(gchar *data, gint nent, glong idx_);
	} page;
	gulong load_page(glong page_idx);
	const gchar *read_first_on_page_key(glong page_idx);
	const gchar *get_first_on_page_key(glong page_idx);
};

/* class for compressed index (file ends with ".gz") */
class compressed_index : public index_file {
public:
	compressed_index();
	~compressed_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, CollationLevelType CollationLevel,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
private:
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);

	/* whole uncompressed index file in memory */
	gchar *idxdatabuf;
	/* pointers to the words-keys in idxdatabuf. Each word is '\0'-terminated and 
	 * followed by data offset and size. See ".idx" file format. 
	 * wordlist.size() == number of words + 1 */
	std::vector<gchar *> wordlist;
};

offset_index::offset_index() : oft_file(CacheFileType_oft, COLLATE_FUNC_NONE)
{
	idxfile = NULL;
	npages = 0;
}

offset_index::~offset_index()
{
	if (idxfile)
		fclose(idxfile);
}

void offset_index::page_t::fill(gchar *data, gint nent, glong idx_)
{
	idx=idx_;
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

inline const gchar *offset_index::read_first_on_page_key(glong page_idx)
{
	g_assert(gulong(page_idx+1) < npages);
	fseek(idxfile, oft_file.get_wordoffset(page_idx), SEEK_SET);
	guint32 page_size=oft_file.get_wordoffset(page_idx+1)-oft_file.get_wordoffset(page_idx);
	gulong minsize = sizeof(wordentry_buf);
	if (page_size < minsize) {
		minsize = page_size;
	}
	size_t fread_size;
	fread_size = fread(wordentry_buf, minsize, 1, idxfile);
	if (fread_size != 1) {
		g_print("fread error!\n");
	}
	if(!check_key_str_len(wordentry_buf, minsize)) {
		wordentry_buf[minsize-1] = '\0';
		g_critical("Index key length exceeds allowed limit. Key: %s, "
			"max length = %i", wordentry_buf, MAX_INDEX_KEY_SIZE - 1);
		return NULL;
	}
	return wordentry_buf;
}

inline const gchar *offset_index::get_first_on_page_key(glong page_idx)
{
	if (page_idx<middle.idx) {
		if (page_idx==first.idx)
			return first.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else if (page_idx>middle.idx) {
		if (page_idx==last.idx)
			return last.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else
		return middle.keystr.c_str();
}

cache_file::cache_file(CacheFileType _cachefiletype, CollateFunctions _cltfunc)
{
	wordoffset = NULL;
	npages = 0;
	mf = NULL;
	cachefiletype = _cachefiletype;
	cltfunc = _cltfunc;
}


cache_file::~cache_file()
{
	if (mf)
		delete mf;
	else
		g_free(wordoffset);
}

#define OFFSETFILE_MAGIC_DATA "StarDict's oft file\nversion=2.4.8\n"
#define COLLATIONFILE_MAGIC_DATA "StarDict's clt file\nversion=2.4.8\n"

MapFile* cache_file::find_and_load_cache_file(const gchar *filename,
	const std::string &url, const std::string &saveurl,
	glong filedatasize, int next) const
{
	stardict_stat_t cachestat;
	if (g_stat(filename, &cachestat)!=0)
		return NULL;
	std::unique_ptr<MapFile> mf(new MapFile);
	if (!mf->open(filename, cachestat.st_size))
		return NULL;
	guint32  word_off_size = (get_uint32(mf->begin()) + 1) * sizeof(guint32);
	if (word_off_size >= static_cast<guint32>(cachestat.st_size) ||
	    *(mf->begin() + cachestat.st_size - 1) != '\0')
		return NULL;

	gchar *p = mf->begin() + word_off_size;
	gboolean has_prefix;
	if (cachefiletype == CacheFileType_oft)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix)
		return NULL;
	if (cachefiletype == CacheFileType_oft)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2)
		return NULL;
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3)
		return NULL;
	std::string tmpstr(p2, p3-p2);
#ifdef _WIN32
	tmpstr = abs_path_to_data_dir(tmpstr);
#endif
	if (is_equal_paths(saveurl, tmpstr)) {
		if (cachefiletype == CacheFileType_clt) {
			p2 = strstr(p, "\nfunc=");
			if (!p2)
				return NULL;
			p2 += sizeof("\nfunc=")-1;
			p3 = strchr(p2, '\n');
			if (!p3)
				return NULL;
			tmpstr.assign(p2, p3-p2);
			if (atoi(tmpstr.c_str())!=cltfunc)
				return NULL;
		}

		if (static_cast<gulong>(cachestat.st_size)
			!= static_cast<gulong>(filedatasize + sizeof(guint32) + strlen(mf->begin() + word_off_size) +1))
			return NULL;
		stardict_stat_t idxstat;
		if (g_stat(url.c_str(), &idxstat)!=0)
			return NULL;
		if (cachestat.st_mtime<idxstat.st_mtime)
			return NULL;
		//g_print("Using map file: %s\n", filename);
		return mf.release();
	}
	mf.reset();
	glib::CharStr basename(g_path_get_basename(saveurl.c_str()));
	p = strrchr(get_impl(basename), '.');
	if (!p)
		return NULL;
	*p='\0';
	gchar *extendname = p+1;
	glib::CharStr dirname(g_path_get_dirname(filename));
	glib::CharStr nextfilename(get_next_filename(get_impl(dirname),
		get_impl(basename), next, extendname));
	return find_and_load_cache_file(get_impl(nextfilename), url, saveurl, filedatasize, next+1);
}

bool cache_file::load_cache(const std::string& url, const std::string& saveurl,
	glong filedatasize)
{
	g_assert(!wordoffset);
	std::string oftfilename;
	build_primary_cache_filename(saveurl, oftfilename);
	/* First search the file in the dictionary directory, then in the cache 
	 * directory. */
	for (int i=0; i<2; i++) {
		if (i==1) {
			if (!build_primary_cache_filename_in_user_cache(saveurl, oftfilename, false))
				break;
		}
		mf = find_and_load_cache_file(oftfilename.c_str(), url, saveurl, filedatasize, 2);
		if (!mf)
			continue;
		wordoffset = reinterpret_cast<guint32 *>(mf->begin()) + 1;
		npages = get_uint32(mf->begin());
		return true;
	}
	return false;
}

bool cache_file::build_primary_cache_filename_in_user_cache(const std::string& url, std::string &cachefilename, bool create) const
{
	const std::string cache_dir(app_dirs->get_user_cache_dir());
	if (create) {
		if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_EXISTS)) {
			if (-1 == g_mkdir_with_parents(cache_dir.c_str(), 0700))
				return false;
		}
	}
	if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_IS_DIR))
		return false;

	gchar *base=g_path_get_basename(url.c_str());
	build_primary_cache_filename(build_path(cache_dir, base), cachefilename);
	g_free(base);
	return true;
}

FILE* cache_file::find_and_open_for_overwrite_cache_file(const gchar *filename, const std::string &saveurl, int next, std::string &cfilename) const
{
	cfilename = filename;
	stardict_stat_t oftstat;
	if (g_stat(filename, &oftstat)!=0) {
		return fopen(filename, "wb");
	}
	MapFile mf;
	if (!mf.open(filename, oftstat.st_size)) {
		return fopen(filename, "wb");
	}
	guint32  word_off_size = (get_uint32(mf.begin()) + 1) * sizeof(guint32);
	if (word_off_size >= static_cast<guint32>(oftstat.st_size) ||
	    *(mf.begin() + oftstat.st_size - 1) != '\0')
		return fopen(filename, "wb");

	gchar *p = mf.begin() + word_off_size;
	bool has_prefix;
	if (cachefiletype == CacheFileType_oft)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix) {
		return fopen(filename, "wb");
	}
	if (cachefiletype == CacheFileType_oft)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2) {
		return fopen(filename, "wb");
	}
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3) {
		return fopen(filename, "wb");
	}
	std::string tmpstr(p2, p3-p2);
#ifdef _WIN32
	tmpstr = abs_path_to_data_dir(tmpstr);
#endif
	if (is_equal_paths(saveurl, tmpstr)) {
		return fopen(filename, "wb");
	}
	mf.close();
	glib::CharStr basename(g_path_get_basename(saveurl.c_str()));
	p = strrchr(get_impl(basename), '.');
	if (!p)
		return NULL;
	*p='\0';
	gchar *extendname = p+1;
	glib::CharStr dirname(g_path_get_dirname(filename));
	glib::CharStr nextfilename(get_next_filename(get_impl(dirname),
		get_impl(basename), next, extendname));
	return find_and_open_for_overwrite_cache_file(get_impl(nextfilename), saveurl, next+1, cfilename);
}

bool cache_file::save_cache(const std::string& saveurl) const
{
	std::string oftfilename;
	build_primary_cache_filename(saveurl, oftfilename);
	for (int i=0;i<2;i++) {
		if (i==1) {
			if (!build_primary_cache_filename_in_user_cache(saveurl, oftfilename, true))
				break;
		}
		std::string cfilename;
		FILE *out= find_and_open_for_overwrite_cache_file(oftfilename.c_str(), saveurl, 2, cfilename);
		if (!out)
			continue;
		guint32 nentries = npages;
		fwrite(&nentries, sizeof(nentries), 1, out);
		fwrite(wordoffset, sizeof(guint32), npages, out);
		if (cachefiletype == CacheFileType_oft)
			fwrite(OFFSETFILE_MAGIC_DATA, 1, sizeof(OFFSETFILE_MAGIC_DATA)-1, out);
		else
			fwrite(COLLATIONFILE_MAGIC_DATA, 1, sizeof(COLLATIONFILE_MAGIC_DATA)-1, out);
		fwrite("url=", 1, sizeof("url=")-1, out);
#ifdef _WIN32
		const std::string url_rel(rel_path_to_data_dir(saveurl));
		fwrite(url_rel.c_str(), 1, url_rel.length(), out);
#else
		fwrite(saveurl.c_str(), 1, saveurl.length(), out);
#endif
		if (cachefiletype == CacheFileType_clt) {
#ifdef _MSC_VER
			fprintf_s(out, "\nfunc=%d", cltfunc);
#else
			fprintf(out, "\nfunc=%d", cltfunc);
#endif
		}
		fwrite("\n", 1, 2, out);
		fclose(out);
		g_print("Save cache file: %s\n", cfilename.c_str());
		return true;
	}
	return false;
}

void cache_file::allocate_wordoffset(size_t _npages)
{
	g_assert(!wordoffset);
	if(mf) {
		delete mf;
		mf = NULL;
	}
	wordoffset = (guint32 *)g_malloc(_npages * sizeof(guint32));
	npages = _npages;
}

gchar *cache_file::get_next_filename(
	const gchar *dirname, const gchar *basename, int num,
	const gchar *extendname) const
{
	if (cachefiletype == CacheFileType_oft)
		return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.oft", dirname, basename, num, extendname);
	else if (cachefiletype == CacheFileType_clt)
		return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.clt", dirname, basename, num, extendname);
	else
		return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.%d.clt", dirname, basename, num, extendname, cltfunc);
}

void cache_file::build_primary_cache_filename(const std::string &url,
	std::string &filename) const
{
	if (cachefiletype == CacheFileType_oft) {
		filename=url+".oft";
	} else if (cachefiletype == CacheFileType_clt) {
		filename=url+".clt";
	} else {
		gchar *func = g_strdup_printf("%d", cltfunc);
		filename=url+'.'+func+".clt";
		g_free(func);
	}
}

collation_file::collation_file(idxsyn_file *_idx_file, CacheFileType _cachefiletype,
	CollateFunctions _CollateFunction)
: cache_file(_cachefiletype, _CollateFunction),
	idx_file(_idx_file)
{
	g_assert(_cachefiletype == CacheFileType_clt || _cachefiletype == CacheFileType_server_clt);

}

const gchar *collation_file::GetWord(glong idx)
{
	return idx_file->get_key(get_wordoffset(idx));
}

glong collation_file::GetOrigIndex(glong cltidx)
{
	return get_wordoffset(cltidx);
}

bool collation_file::lookup(const char *sWord, glong &idx, glong &idx_suggest)
{
	bool bFound=false;
	glong iTo=idx_file->get_word_count()-1;
	if (stardict_collate(sWord, GetWord(0), get_CollateFunction())<0) {
		idx = 0;
		idx_suggest = 0;
	} else if (stardict_collate(sWord, GetWord(iTo), get_CollateFunction()) >0) {
		idx = INVALID_INDEX;
		idx_suggest = iTo;
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_collate(sWord, GetWord(iThisIndex), get_CollateFunction());
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
			idx = iFrom;    //next
			idx_suggest = iFrom;
			gint best, back;
			best = prefix_match (sWord, GetWord(idx_suggest));
			for (;;) {
				if ((iTo=idx_suggest-1) < 0)
					break;
				back = prefix_match (sWord, GetWord(iTo));
				if (!back || back < best)
					break;
				best = back;
				idx_suggest = iTo;
			}
		} else {
			idx = iThisIndex;
			idx_suggest = iThisIndex;
		}
	}
	return bFound;
}

struct sort_collation_index_user_data {
	idxsyn_file *idx_file;
	CollateFunctions cltfunc;
};

static gint sort_collation_index(gconstpointer a, gconstpointer b, gpointer user_data)
{
	sort_collation_index_user_data *data = (sort_collation_index_user_data*)user_data;
	gchar *str1 = g_strdup(data->idx_file->get_key(*((guint32 *)a)));
	const gchar *str2 = data->idx_file->get_key(*((guint32 *)b));
	gint x = stardict_collate(str1, str2, data->cltfunc);
	g_free(str1);
	if (x==0)
		return *((guint32 *)a) - *((guint32 *)b);
	else
		return x;
}

idxsyn_file::idxsyn_file()
:
	clt_file(NULL),
	wordcount(0)
{
	memset(clt_files, 0, sizeof(clt_files));
}

idxsyn_file::~idxsyn_file()
{
	delete clt_file;
	for(size_t i=0; i<COLLATE_FUNC_NUMS; ++i)
		delete clt_files[i];
}

const gchar *idxsyn_file::getWord(glong idx, CollationLevelType CollationLevel, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return get_key(idx);
	if (CollationLevel == CollationLevel_SINGLE)
		return clt_file->GetWord(idx);
	if (servercollatefunc == 0)
		return get_key(idx);
	collate_load((CollateFunctions)(servercollatefunc-1), CollationLevel_MULTI);
	return clt_files[servercollatefunc-1]->GetWord(idx);
}

bool idxsyn_file::Lookup(const char *str, glong &idx, glong &idx_suggest, CollationLevelType CollationLevel, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return lookup(str, idx, idx_suggest);
	if (CollationLevel == CollationLevel_SINGLE)
		return clt_file->lookup(str, idx, idx_suggest);
	if (servercollatefunc == 0)
		return lookup(str, idx, idx_suggest);
	collate_load((CollateFunctions)(servercollatefunc-1), CollationLevel_MULTI);
	return clt_files[servercollatefunc-1]->lookup(str, idx, idx_suggest);
}

void idxsyn_file::collate_save_info(const std::string& _url, const std::string& _saveurl)
{
	url = _url;
	saveurl = _saveurl;
}

void idxsyn_file::collate_load(CollateFunctions collf, CollationLevelType CollationLevel, show_progress_t *sp)
{
	g_assert(CollationLevel == CollationLevel_SINGLE || CollationLevel == CollationLevel_MULTI);
	if(CollationLevel == CollationLevel_SINGLE) {
		if(clt_file)
			return;
		clt_file = collate_load_impl(url, saveurl, collf, sp, CacheFileType_clt);
	} else if(CollationLevel == CollationLevel_MULTI) {
		if (clt_files[collf])
			return;
		clt_files[collf] = collate_load_impl(url, saveurl, collf, sp, CacheFileType_server_clt);
	}
}

collation_file * idxsyn_file::collate_load_impl(
	const std::string& _url, const std::string& _saveurl,
	CollateFunctions collf, show_progress_t *sp, CacheFileType CacheType)
{
	collation_file * _clt_file = new collation_file(this, CacheType, collf);
	if (!_clt_file->load_cache(_url, _saveurl, wordcount*sizeof(guint32))) {
		if(sp)
			sp->notify_about_start(_("Sorting, please wait..."));
		_clt_file->allocate_wordoffset(wordcount);
		for (glong i=0; i<wordcount; i++)
			_clt_file->get_wordoffset(i) = i;
		sort_collation_index_user_data data;
		data.idx_file = this;
		data.cltfunc = collf;
		g_qsort_with_data(_clt_file->get_wordoffset(), wordcount, sizeof(guint32), sort_collation_index, &data);
		if (!_clt_file->save_cache(_saveurl))
			g_printerr("Cache update failed.\n");
	}
	return _clt_file;
}

bool offset_index::load(const std::string& url, gulong wc, gulong fsize,
			bool CreateCacheFile, CollationLevelType CollationLevel,
			CollateFunctions _CollateFunction, show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
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
		for (guint32 i=0; i<wc; i++) {
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
	real_last.assign(wc-1, get_key(wc-1));

	if (CollationLevel == CollationLevel_NONE) {
	} else if (CollationLevel == CollationLevel_SINGLE) {
		collate_save_info(url, url);
		collate_load(_CollateFunction, CollationLevel_SINGLE, sp);
	} else if (CollationLevel == CollationLevel_MULTI) {
		collate_save_info(url, url);
	}

	return true;
}

inline gulong offset_index::load_page(glong page_idx)
{
	gulong nentr=ENTR_PER_PAGE;
	if (page_idx==glong(npages-2))
		if ((nentr=wordcount%ENTR_PER_PAGE)==0)
			nentr=ENTR_PER_PAGE;


	if (page_idx!=page.idx) {
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

const gchar *offset_index::get_key(glong idx)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	wordentry_offset=page.entries[idx_in_page].off;
	wordentry_size=page.entries[idx_in_page].size;

	return page.entries[idx_in_page].keystr;
}

void offset_index::get_data(glong idx)
{
	get_key(idx);
}

const gchar *offset_index::get_key_and_data(glong idx)
{
	return get_key(idx);
}

/* Search for string str. 
 * Returns true if the string is found and false otherwise.
 * If the string is found, idx - index of the search string.
 * If the string is not found, idx - index of the "next" item in the index.
 * idx == INVALID_INDEX if the search word is greater then the last word of
 * the index. 
 * idx_suggest - index of the closest word in the index.
 * It's always a valid index. */
bool offset_index::lookup(const char *str, glong &idx, glong &idx_suggest)
{
	bool bFound=false;
	glong iFrom;
	glong iTo=npages-2;
	gint cmpint;
	glong iThisIndex;
	if (stardict_strcmp(str, first.keystr.c_str())<0) {
		idx = 0;
		idx_suggest = 0;
		return false;
	} else if (stardict_strcmp(str, real_last.keystr.c_str()) >0) {
		idx = INVALID_INDEX;
		idx_suggest = wordcount-1;
		return false;
	} else {
		// find the page number where the search word might be
		iFrom=0;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_first_on_page_key(iThisIndex));
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
			idx = iTo;    //prev
		} else {
			idx = iThisIndex;
		}
	}
	if (!bFound) {
		// the search word is on the page number idx if it's anywhere
		gulong netr=load_page(idx);
		iFrom=1; // Needn't search the first word anymore.
		iTo=netr-1;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, page.entries[iThisIndex].keystr);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		idx*=ENTR_PER_PAGE;
		if (!bFound) {
			idx += iFrom;    //next
			idx_suggest = idx;
			gint best, back;
			best = prefix_match (str, page.entries[idx_suggest % ENTR_PER_PAGE].keystr);
			for (;;) {
				if ((iTo=idx_suggest-1) < 0)
					break;
				if (idx_suggest % ENTR_PER_PAGE == 0)
					load_page(iTo / ENTR_PER_PAGE);
				back = prefix_match (str, page.entries[iTo % ENTR_PER_PAGE].keystr);
				if (!back || back < best)
					break;
				best = back;
				idx_suggest = iTo;
			}
		} else {
			idx += iThisIndex;
			idx_suggest = idx;
		}
	} else {
		idx*=ENTR_PER_PAGE;
		idx_suggest = idx;
	}
	return bFound;
}

compressed_index::compressed_index()
{
	idxdatabuf = NULL;
}

compressed_index::~compressed_index()
{
	g_free(idxdatabuf);
}

/* Parameters:
 * url - index file path, has suffix ".idx.gz".
 * wc - number of words in the index
 * fsize - uncompressed index size
 * */
bool compressed_index::load(const std::string& url, gulong wc, gulong fsize,
			  bool CreateCacheFile, CollationLevelType CollationLevel,
			  CollateFunctions _CollateFunction, show_progress_t *sp)
{
	wordcount=wc;
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

	wordlist.resize(wc+1);
	gchar *p1 = idxdatabuf;
	guint32 i;
	for (i=0; i<wc; i++) {
		wordlist[i] = p1;
		p1 += strlen(p1) +1 + 2*sizeof(guint32);
	}
	/* pointer to the next to last word entry */
	wordlist[wc] = p1;

	if (CollationLevel == CollationLevel_NONE) {
	} else {
		std::string saveurl = url;
		saveurl.erase(saveurl.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		if (CollationLevel == CollationLevel_SINGLE) {
			collate_save_info(url, saveurl);
			collate_load(_CollateFunction, CollationLevel_SINGLE, sp);
		} else if (CollationLevel == CollationLevel_MULTI) {
			collate_save_info(url, saveurl);
		}
	}
	return true;
}

const gchar *compressed_index::get_key(glong idx)
{
	return wordlist[idx];
}

void compressed_index::get_data(glong idx)
{
	gchar *p1 = wordlist[idx]+strlen(wordlist[idx])+sizeof(gchar);
	wordentry_offset = g_ntohl(get_uint32(p1));
	p1 += sizeof(guint32);
	wordentry_size = g_ntohl(get_uint32(p1));
}

const gchar *compressed_index::get_key_and_data(glong idx)
{
	get_data(idx);
	return get_key(idx);
}

bool compressed_index::lookup(const char *str, glong &idx, glong &idx_suggest)
{
	bool bFound=false;
	glong iTo=wordlist.size()-2;

	if (stardict_strcmp(str, get_key(0))<0) {
		idx = 0;
		idx_suggest = 0;
	} else if (stardict_strcmp(str, get_key(iTo)) >0) {
		idx = INVALID_INDEX;
		idx_suggest = iTo;
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_key(iThisIndex));
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
			idx = iFrom;    //next
			idx_suggest = iFrom;
			gint best, back;
			best = prefix_match (str, get_key(idx_suggest));
			for (;;) {
				if ((iTo=idx_suggest-1) < 0)
					break;
				back = prefix_match (str, get_key(iTo));
				if (!back || back < best)
					break;
				best = back;
				idx_suggest = iTo;
			}
		} else {
			idx = iThisIndex;
			idx_suggest = iThisIndex;
		}
	}
	return bFound;
}

//===================================================================
index_file* index_file::Create(const std::string& filebasename, 
		const char* mainext, std::string& fullfilename)
{
	index_file *index = NULL;

	fullfilename = filebasename + "." + mainext + ".gz";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		index = new compressed_index;
	} else {
		fullfilename = filebasename + "." + mainext;
		index = new offset_index;
	}
	return index;
}

//===================================================================
void synonym_file::page_t::fill(gchar *data, gint nent, glong idx_)
{
	idx=idx_;
	gchar *p=data;
	glong len;
	for (gint i=0; i<nent; ++i) {
		entries[i].keystr=p;
		len=strlen(p);
		p+=len+1;
		entries[i].index=g_ntohl(get_uint32(p));
		p+=sizeof(guint32);
	}
}

synonym_file::synonym_file() : oft_file(CacheFileType_oft, COLLATE_FUNC_NONE)
{
}

synonym_file::~synonym_file()
{
	if (synfile)
		fclose(synfile);
}

inline const gchar *synonym_file::read_first_on_page_key(glong page_idx)
{
	fseek(synfile, oft_file.get_wordoffset(page_idx), SEEK_SET);
	guint32 page_size=oft_file.get_wordoffset(page_idx+1)-oft_file.get_wordoffset(page_idx);
	gulong minsize = sizeof(wordentry_buf);
        if (page_size < minsize) {
                minsize = page_size;
	}
	size_t fread_size;
	fread_size = fread(wordentry_buf, minsize, 1, synfile);
	if (fread_size != 1) {
		g_print("fread error!\n");
	}
	if(!check_key_str_len(wordentry_buf, minsize)) {
		wordentry_buf[minsize-1] = '\0';
		g_critical("Synonym index key length exceeds allowed limit. Synonym key: %s, "
			"max length = %i", wordentry_buf, MAX_INDEX_KEY_SIZE - 1);
		return NULL;
	}
	return wordentry_buf;
}

inline const gchar *synonym_file::get_first_on_page_key(glong page_idx)
{
	if (page_idx<middle.idx) {
		if (page_idx==first.idx)
			return first.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else if (page_idx>middle.idx) {
		if (page_idx==last.idx)
			return last.keystr.c_str();
		return read_first_on_page_key(page_idx);
	} else
		return middle.keystr.c_str();
}

bool synonym_file::load(const std::string& url, gulong wc, bool CreateCacheFile,
	CollationLevelType CollationLevel, CollateFunctions _CollateFunction,
	show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, npages*sizeof(guint32))) {
		stardict_stat_t stats;
		if (g_stat(url.c_str(), &stats) == -1)
			return false;
		MapFile map_file;
		if (!map_file.open(url.c_str(), stats.st_size))
			return false;
		const gchar *syndatabuffer=map_file.begin();
		oft_file.allocate_wordoffset(npages);
		const gchar *p1 = syndatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<wc; i++) {
			index_size=strlen(p1) +1 + sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.get_wordoffset(j)=p1-syndatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.get_wordoffset(j)=p1-syndatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url))
				g_printerr("Cache update failed.\n");
		}
	}

	if (!(synfile = fopen(url.c_str(), "rb"))) {
		return false;
	}

	first.assign(0, read_first_on_page_key(0));
	last.assign(npages-2, read_first_on_page_key(npages-2));
	middle.assign((npages-2)/2, read_first_on_page_key((npages-2)/2));
	real_last.assign(wc-1, get_key(wc-1));

	if (CollationLevel == CollationLevel_NONE) {
	} else if (CollationLevel == CollationLevel_SINGLE) {
		collate_save_info(url, url);
		collate_load(_CollateFunction,CollationLevel_SINGLE, sp);
	} else if (CollationLevel == CollationLevel_MULTI) {
		collate_save_info(url, url);
	}

	return true;
}

inline gulong synonym_file::load_page(glong page_idx)
{
	gulong nentr=ENTR_PER_PAGE;
	if (page_idx==glong(npages-2))
		if ((nentr=wordcount%ENTR_PER_PAGE)==0)
			nentr=ENTR_PER_PAGE;


	if (page_idx!=page.idx) {
		page_data.resize(oft_file.get_wordoffset(page_idx+1)-oft_file.get_wordoffset(page_idx));
		fseek(synfile, oft_file.get_wordoffset(page_idx), SEEK_SET);
		size_t fread_size;
		size_t page_data_size = page_data.size();
		fread_size = fread(&page_data[0], 1, page_data_size, synfile);
		if (fread_size != page_data_size) {
			g_print("fread error!\n");
		}
		page.fill(&page_data[0], nentr, page_idx);
	}

	return nentr;
}

const gchar *synonym_file::get_key(glong idx)
{
	load_page(idx/ENTR_PER_PAGE);
	glong idx_in_page=idx%ENTR_PER_PAGE;
	wordentry_index=page.entries[idx_in_page].index;

	return page.entries[idx_in_page].keystr;
}

bool synonym_file::lookup(const char *str, glong &idx, glong &idx_suggest)
{
	bool bFound=false;
	glong iFrom;
	glong iTo=npages-2;
	gint cmpint;
	glong iThisIndex;
	if (stardict_strcmp(str, first.keystr.c_str())<0) {
		idx = 0;
		idx_suggest = 0;
		return false;
	} else if (stardict_strcmp(str, real_last.keystr.c_str()) >0) {
		idx = INVALID_INDEX;
		idx_suggest = wordcount-1;
		return false;
	} else {
		iFrom=0;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, get_first_on_page_key(iThisIndex));
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		if (!bFound)
			idx = iTo;    //prev
		else
			idx = iThisIndex;
	}
	if (!bFound) {
		gulong netr=load_page(idx);
		iFrom=1; // Needn't search the first word anymore.
		iTo=netr-1;
		iThisIndex=0;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_strcmp(str, page.entries[iThisIndex].keystr);
			if (cmpint>0)
				iFrom=iThisIndex+1;
			else if (cmpint<0)
				iTo=iThisIndex-1;
			else {
				bFound=true;
				break;
			}
		}
		idx*=ENTR_PER_PAGE;
		if (!bFound) {
			idx += iFrom;    //next
			idx_suggest = idx;
			gint best, back;
			best = prefix_match (str, page.entries[idx_suggest % ENTR_PER_PAGE].keystr);
			for (;;) {
				if ((iTo=idx_suggest-1) < 0)
					break;
				if (idx_suggest % ENTR_PER_PAGE == 0)
					load_page(iTo / ENTR_PER_PAGE);
				back = prefix_match (str, page.entries[iTo % ENTR_PER_PAGE].keystr);
				if (!back || back < best)
					break;
				best = back;
				idx_suggest = iTo;
			}
		} else {
			idx += iThisIndex;
			idx_suggest = idx;
		}
	} else {
		idx*=ENTR_PER_PAGE;
		idx_suggest = idx;
	}
	return bFound;
}

//===================================================================
Dict::Dict()
{
	storage = NULL;
}

Dict::~Dict()
{
	delete storage;
}

bool Dict::load(const std::string& ifofilename, bool CreateCacheFile,
	CollationLevelType CollationLevel, CollateFunctions CollateFunction,
	show_progress_t *sp)
{
	gulong idxfilesize;
	glong wordcount, synwordcount;
	if (!load_ifofile(ifofilename, idxfilesize, wordcount, synwordcount))
		return false;
	sp->notify_about_start(_("Loading..."));

	// ifofilename without extension - base file name
	std::string filebasename
		= ifofilename.substr(0, ifofilename.length()-sizeof(".ifo")+1);
	if(!DictBase::load(filebasename, "dict"))
		return false;

	std::string fullfilename;
	idx_file.reset(index_file::Create(filebasename, "idx", fullfilename));
	if (!idx_file->load(fullfilename, wordcount, idxfilesize,
			    CreateCacheFile, CollationLevel,
			    CollateFunction, sp))
		return false;

	if (synwordcount) {
		fullfilename = filebasename + ".syn";
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
			syn_file.reset(new synonym_file);
			if (!syn_file->load(fullfilename, synwordcount,
					    CreateCacheFile, CollationLevel,
					    CollateFunction, sp))
				return false;
		}
	}

	gchar *dirname = g_path_get_dirname(ifofilename.c_str());
	storage = ResourceStorage::create(dirname, CreateCacheFile, sp);
	g_free(dirname);

	g_print("bookname: %s, wordcount %lu\n", bookname.c_str(), wordcount);
	return true;
}

bool Dict::load_ifofile(const std::string& ifofilename, gulong &idxfilesize, glong &wordcount, glong &synwordcount)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(ifofilename, DictInfoType_NormDict))
		return false;

	ifo_file_name=dict_info.ifo_file_name;
	bookname=dict_info.get_bookname();

	idxfilesize=dict_info.get_index_file_size();
	wordcount=dict_info.get_wordcount();
	synwordcount=dict_info.get_synwordcount();

	sametypesequence=dict_info.get_sametypesequence();
	dicttype=dict_info.get_dicttype();

	return true;
}

glong Dict::nsynarticles() const
{
	if (syn_file.get() == NULL)
		return 0;
	return syn_file->get_word_count();
}

bool Dict::GetWordPrev(glong idx, glong &pidx, bool isidx, CollationLevelType CollationLevel, int servercollatefunc)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	if (idx==INVALID_INDEX) {
		pidx = is_file->get_word_count()-1;
		return true;
	}
	pidx = idx;
	gchar *cWord = g_strdup(is_file->getWord(pidx, CollationLevel, servercollatefunc));
	const gchar *pWord;
	bool found=false;
	while (pidx>0) {
		pWord = is_file->getWord(pidx-1, CollationLevel, servercollatefunc);
		if (strcmp(pWord, cWord)!=0) {
			found=true;
			break;
		}
		pidx--;
	}
	g_free(cWord);
	if (found) {
		pidx--;
		return true;
	} else {
		return false;
	}
}

void Dict::GetWordNext(glong &idx, bool isidx, CollationLevelType CollationLevel, int servercollatefunc)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	gchar *cWord = g_strdup(is_file->getWord(idx, CollationLevel, servercollatefunc));
	const gchar *pWord;
	bool found=false;
	while (idx < is_file->get_word_count()-1) {
		pWord = is_file->getWord(idx+1, CollationLevel, servercollatefunc);
		if (strcmp(pWord, cWord)!=0) {
			found=true;
			break;
		}
		idx++;
	}
	g_free(cWord);
	if (found)
		idx++;
	else
		idx=INVALID_INDEX;
}

gint Dict::GetOrigWordCount(glong& idx, bool isidx)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	gchar *cWord = g_strdup(is_file->get_key(idx));
	const gchar *pWord;
	gint count = 1;
	glong idx1 = idx;
	while (idx1>0) {
		pWord = is_file->get_key(idx1-1);
		if (strcmp(pWord, cWord)!=0)
			break;
		count++;
		idx1--;
	}
	glong idx2=idx;
	while (idx2<is_file->get_word_count()-1) {
		pWord = is_file->get_key(idx2+1);
		if (strcmp(pWord, cWord)!=0)
			break;
		count++;
		idx2++;
	}
	idx=idx1;
	g_free(cWord);
	return count;
}

bool Dict::LookupSynonym(const char *str, glong &synidx, glong &synidx_suggest, CollationLevelType CollationLevel, int servercollatefunc)
{
	if (syn_file.get() == NULL) {
		synidx = UNSET_INDEX;
		synidx_suggest = UNSET_INDEX;
		return false;
	}
	return syn_file->Lookup(str, synidx, synidx_suggest, CollationLevel, servercollatefunc);
}

bool Dict::LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	int iIndexCount=0;
	for (glong i=0; i<narticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_pattern_match_string(pspec, idx_file->getWord(i, CollationLevel_NONE, 0)))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

bool Dict::LookupWithRuleSynonym(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	if (syn_file.get() == NULL)
		return false;
	int iIndexCount=0;
	for (glong i=0; i<nsynarticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_pattern_match_string(pspec, syn_file->getWord(i, CollationLevel_NONE, 0)))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

bool Dict::LookupWithRegex(GRegex *regex, glong *aIndex, int iBuffLen)
{
	int iIndexCount=0;
	for (glong i=0; i<narticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_regex_match(regex, idx_file->getWord(i, CollationLevel_NONE, 0), (GRegexMatchFlags)0, NULL))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

bool Dict::LookupWithRegexSynonym(GRegex *regex, glong *aIndex, int iBuffLen)
{
	if (syn_file.get() == NULL)
		return false;
	int iIndexCount=0;
	for (glong i=0; i<nsynarticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_regex_match(regex, syn_file->getWord(i, CollationLevel_NONE, 0), (GRegexMatchFlags)0, NULL))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

//===================================================================
show_progress_t Libs::default_show_progress;

Libs::Libs(show_progress_t *sp, bool create_cache_files, CollationLevelType level, CollateFunctions func)
:
	iMaxFuzzyDistance(MAX_FUZZY_DISTANCE),
	show_progress(NULL),
	CreateCacheFile(create_cache_files)
{
#ifdef SD_SERVER_CODE
	root_info_item = NULL;
#endif
	ValidateCollateParams(level, func);
	CollationLevel = level;
	CollateFunction = func;
	set_show_progress(sp);
	init_collations();
}

Libs::~Libs()
{
#ifdef SD_SERVER_CODE
	if (root_info_item)
		delete root_info_item;
#endif
	for (std::vector<Dict *>::iterator p=oLib.begin(); p!=oLib.end(); ++p)
		delete *p;
	free_collations();
}

bool Libs::load_dict(const std::string& url, show_progress_t *sp)
{
	Dict *lib=new Dict;
	if (lib->load(url, CreateCacheFile, CollationLevel, CollateFunction, sp)) {
		oLib.push_back(lib);
		return true;
	} else {
		delete lib;
		return false;
	}
}

#ifdef SD_SERVER_CODE
void Libs::LoadFromXML()
{
	root_info_item = new DictInfoItem();
	root_info_item->isdir = 1;
	root_info_item->dir = new DictInfoDirItem();
	root_info_item->dir->name='/';
	LoadXMLDir("/usr/share/stardict/dic", root_info_item);
	GenLinkDict(root_info_item);
}

void Libs::GenLinkDict(DictInfoItem *info_item)
{
	std::list<std::list<DictInfoItem *>::iterator> eraselist;
	for (std::list<DictInfoItem *>::iterator i = info_item->dir->info_item_list.begin(); i!= info_item->dir->info_item_list.end(); ++i) {
		if ((*i)->isdir == 1) {
			GenLinkDict(*i);
		} else if ((*i)->isdir == 2) {
			std::map<std::string, DictInfoDictItem *>::iterator uid_iter;
			uid_iter = uidmap.find(*((*i)->linkuid));
			if (uid_iter!=uidmap.end()) {
				delete (*i)->linkuid;
				(*i)->dict = uid_iter->second;
			} else {
				g_print("Error, linkdict uid not found! %s\n", (*i)->linkuid->c_str());
				delete (*i)->linkuid;
				eraselist.push_back(i);
			}
		}
	}
	for (std::list<std::list<DictInfoItem *>::iterator>::iterator i = eraselist.begin(); i!= eraselist.end(); ++i) {
		info_item->dir->info_item_list.erase(*i);
	}
}

void Libs::func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dict")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->indict = true;
		Data->path.clear();
		Data->uid.clear();
		Data->level.clear();
		Data->download.clear();
		Data->from.clear();
		Data->to.clear();
	} else if (strcmp(element_name, "linkdict")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->inlinkdict = true;
		Data->linkuid.clear();
	}
}

void Libs::func_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error)
{
	if (strcmp(element_name, "dict")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->indict = false;
		if (!Data->path.empty() && !Data->uid.empty()) {
			std::string url;
			url = Data->dir;
			url += G_DIR_SEPARATOR;
			url += Data->path;
			if (Data->oLibs->load_dict(url, Data->oLibs->show_progress)) {
				DictInfoItem *sub_info_item = new DictInfoItem();
				sub_info_item->isdir = 0;
				sub_info_item->dict = new DictInfoDictItem();
				sub_info_item->dict->uid = Data->uid;
				sub_info_item->dict->download = Data->download;
				sub_info_item->dict->from = Data->from;
				sub_info_item->dict->to = Data->to;
				if (Data->level.empty())
					sub_info_item->dict->level = 0;
				else
					sub_info_item->dict->level = atoi(Data->level.c_str());
				sub_info_item->dict->id = Data->oLibs->oLib.size()-1;
				Data->info_item->dir->info_item_list.push_back(sub_info_item);
				Data->oLibs->uidmap[Data->uid] = sub_info_item->dict;
			}
		}
	} else if (strcmp(element_name, "linkdict")==0) {
		ParseUserData *Data = (ParseUserData *)user_data;
		Data->inlinkdict = false;
		if (!Data->linkuid.empty()) {
			DictInfoItem *sub_info_item = new DictInfoItem();
			sub_info_item->isdir = 2;
			sub_info_item->linkuid = new std::string(Data->linkuid);
			Data->info_item->dir->info_item_list.push_back(sub_info_item);
		}
	}
}

void Libs::func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error)
{
	const gchar *element = g_markup_parse_context_get_element(context);
	if (!element)
		return;
	ParseUserData *Data = (ParseUserData *)user_data;
	if (strcmp(element, "subdir")==0) {
		std::string subdir;
		subdir = Data->dir;
		subdir += G_DIR_SEPARATOR;
		subdir.append(text, text_len);
		DictInfoItem *sub_info_item = new DictInfoItem();
		sub_info_item->isdir = 1;
		sub_info_item->dir = new DictInfoDirItem();
		sub_info_item->dir->name.assign(text, text_len);
		Data->oLibs->LoadXMLDir(subdir.c_str(), sub_info_item);
		Data->info_item->dir->info_item_list.push_back(sub_info_item);
	} else if (strcmp(element, "dirname")==0) {
		Data->info_item->dir->dirname.assign(text, text_len);
	} else if (strcmp(element, "path")==0) {
		Data->path.assign(text, text_len);
	} else if (strcmp(element, "uid")==0) {
		if (Data->indict) {
			std::string uid(text, text_len);
			if (uid.find_first_of(' ')!=std::string::npos) {
				g_print("Error: uid contains space! %s: %s\n", Data->dir, uid.c_str());
			} else {
				std::map<std::string, DictInfoDictItem *>::iterator uid_iter;
				uid_iter = Data->oLibs->uidmap.find(uid);
				if (uid_iter!=Data->oLibs->uidmap.end()) {
					g_print("Error: uid duplicated! %s: %s\n", Data->dir, uid.c_str());
				} else {
					Data->uid = uid;
				}
			}
		} else if (Data->inlinkdict) {
			Data->linkuid.assign(text, text_len);
		}
	} else if (strcmp(element, "level")==0) {
		Data->level.assign(text, text_len);
	} else if (strcmp(element, "download")==0) {
		Data->download.assign(text, text_len);
	} else if (strcmp(element, "from")==0) {
		Data->from.assign(text, text_len);
	} else if (strcmp(element, "to")==0) {
		Data->to.assign(text, text_len);
	}
}

void Libs::LoadXMLDir(const char *dir, DictInfoItem *info_item)
{
	std::string filename;
	filename = build_path(dir, "stardictd.xml");
	stardict_stat_t filestat;
	if (g_stat(filename.c_str(), &filestat)!=0)
		return;
	MapFile mf;
	if (!mf.open(filename.c_str(), filestat.st_size))
		return;
	ParseUserData Data;
	Data.oLibs = this;
	Data.dir = dir;
	Data.info_item = info_item;
	Data.indict = false;
	Data.inlinkdict = false;
	GMarkupParser parser;
	parser.start_element = func_parse_start_element;
	parser.end_element = func_parse_end_element;
	parser.text = func_parse_text;
	parser.passthrough = NULL;
	parser.error = NULL;
	GMarkupParseContext* context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, &Data, NULL);
	g_markup_parse_context_parse(context, mf.begin(), filestat.st_size, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);
	mf.close();
	info_item->dir->dictcount = 0;
	for (std::list<DictInfoItem *>::iterator i = info_item->dir->info_item_list.begin(); i!= info_item->dir->info_item_list.end(); ++i) {
		if ((*i)->isdir == 1) {
			info_item->dir->dictcount += (*i)->dir->dictcount;
		} else if ((*i)->isdir == 0) {
			info_item->dir->dictcount++;
		}
	}
}

const std::string &Libs::get_fromto_info() {
	if(cache_fromto.empty()){
		std::map<std::string, std::list<FromTo> > map_fromto;
		gen_fromto_info(root_info_item, map_fromto);
		cache_fromto+="<lang>";
		for (std::map<std::string, std::list<FromTo> >::iterator map_it = map_fromto.begin(); map_it != map_fromto.end(); ++map_it){
			cache_fromto+="<from lang=\"";
			cache_fromto+=map_it->first;
			cache_fromto+="\">";
			std::list<FromTo> &fromTo = map_it->second;
			for (std::list<FromTo>::iterator i = fromTo.begin() ; i!= fromTo.end(); ++i){
				cache_fromto+="<to lang=\"";
				cache_fromto+= i->to;
				cache_fromto+="\">";
				std::list<FromToInfo> &fromtoinfo = i->fromto_info;
				for (std::list<FromToInfo>::iterator j = fromtoinfo.begin() ; j!= fromtoinfo.end(); ++j){
					cache_fromto+="<dict><uid>";
					cache_fromto+=j->uid;
					cache_fromto+="</uid><bookname>";
					cache_fromto+= j->bookname;
					cache_fromto+="</bookname></dict>";
				}
				cache_fromto+="</to>";
			}
			cache_fromto+="</from>";
		}
		cache_fromto+="</lang>";
	}
	return cache_fromto;
}

void Libs::gen_fromto_info(struct DictInfoItem *info_item, std::map<std::string, std::list<FromTo> > &map_fromto) {
	gchar *etext;
	for(std::list<DictInfoItem *>::iterator i = info_item->dir->info_item_list.begin() ; i!= info_item->dir->info_item_list.end(); ++i){
		if ((*i)->isdir == 1) {
			gen_fromto_info((*i), map_fromto);
		} else {
			std::string from_str     = (*i)->dict->from;
			std::string to_str       = (*i)->dict->to;
			if(from_str.empty() || to_str.empty()){
				continue;
			}
			std::string uid_str      = (*i)->dict->uid;
			etext = g_markup_escape_text(oLib[(*i)->dict->id]->dict_name().c_str(), -1);
			std::string bookname_str = etext;
			g_free(etext);
			std::map<std::string, std::list<FromTo> >::iterator fromto1 = map_fromto.find(from_str);
			if (fromto1==map_fromto.end()) {
				//if an from_str element not already in map,  add new from_str to map
				FromToInfo fromtoinfo;
				fromtoinfo.uid = uid_str;
				fromtoinfo.bookname = bookname_str;
				std::list<FromToInfo> list_fromtoinfo ;
				list_fromtoinfo.push_back(fromtoinfo);
				FromTo new_fromTo;
				new_fromTo.to = to_str;
				new_fromTo.fromto_info = list_fromtoinfo;
				std::list<FromTo> list_fromTo;
				list_fromTo.push_back(new_fromTo);
				map_fromto[from_str] = list_fromTo;
			} else {
				// else if from_str already in map, so comparison to_str and from_to1 , then choose insert.
				std::list<FromTo> &fromTo_list = fromto1->second;
				std::string from_name1 = fromto1->first;
				bool found = false;
				for (std::list<FromTo>::iterator new_fromTo = fromTo_list.begin(); new_fromTo != fromTo_list.end(); ++new_fromTo) {
					if(to_str == new_fromTo->to) {
						std::list<FromToInfo> &fromtoinfo1 = new_fromTo->fromto_info;
						FromToInfo fromtoinfo;
						fromtoinfo.uid = uid_str;
						fromtoinfo.bookname = bookname_str;
						fromtoinfo1.push_back(fromtoinfo);
						found = true;
						break;
					}
				}
				if(!found){
					FromToInfo fromtoinfo;
					fromtoinfo.uid = uid_str;
					fromtoinfo.bookname = bookname_str;
					std::list<FromToInfo> fromtoinfo1;
					fromtoinfo1.push_back(fromtoinfo);
					FromTo fromTo;
					fromTo.to = to_str;
					fromTo.fromto_info = fromtoinfo1;
					fromTo_list.push_back(fromTo);
				}				
			}
		}
	}
}

const std::string *Libs::get_dir_info(const char *path)
{
	if (path[0]!='/')
		return NULL;
	DictInfoItem *info_item = root_info_item;
	std::string item;
	const char *p = path+1;
	const char *p1;
	bool found;
	do {
		p1 = strchr(p, '/');
		if (p1) {
			item.assign(p, p1-p);
			if (!item.empty()) {
				found = false;
				for (std::list<DictInfoItem *>::iterator i = info_item->dir->info_item_list.begin(); i!= info_item->dir->info_item_list.end(); ++i) {
					if ((*i)->isdir == 1) {
						if ((*i)->dir->name == item) {
							info_item = (*i);
							found = true;
							break;
						}
					}
				}
				if (!found)
					return NULL;
			}
			p = p1+1;
		}
	} while (p1);
	if (*p)
		return NULL; // Not end by '/'.
	DictInfoDirItem *dir = info_item->dir;
	if (dir->info_string.empty()) {
		dir->info_string += "<parent>";
		dir->info_string += path;
		dir->info_string += "</parent>";
		gchar *etext;
		for (std::list<DictInfoItem *>::iterator i = info_item->dir->info_item_list.begin(); i!= info_item->dir->info_item_list.end(); ++i) {
			if ((*i)->isdir == 1) {
				dir->info_string += "<dir><name>";
				dir->info_string += (*i)->dir->name;
				dir->info_string += "</name><dirname>";
				dir->info_string += (*i)->dir->dirname;
				dir->info_string += "</dirname><dictcount>";
				gchar *dictcount = g_strdup_printf("%u", (*i)->dir->dictcount);
				dir->info_string += dictcount;
				g_free(dictcount);
				dir->info_string += "</dictcount></dir>";
			} else {
				dir->info_string += "<dict>";
				if ((*i)->isdir == 2)
					dir->info_string += "<islink>1</islink>";
				if ((*i)->dict->level != 0) {
					dir->info_string += "<level>";
					gchar *level = g_strdup_printf("%u", (*i)->dict->level);
					dir->info_string += level;
					g_free(level);
					dir->info_string += "</level>";
				}
				dir->info_string += "<uid>";
				dir->info_string += (*i)->dict->uid;
				dir->info_string += "</uid><bookname>";
				etext = g_markup_escape_text(oLib[(*i)->dict->id]->dict_name().c_str(), -1);
				dir->info_string += etext;
				g_free(etext);
				dir->info_string += "</bookname><wordcount>";
				gchar *wc = g_strdup_printf("%ld", oLib[(*i)->dict->id]->narticles());
				dir->info_string += wc;
				g_free(wc);
				dir->info_string += "</wordcount></dict>";
			}
		}
	}
	return &(dir->info_string);
}

int Libs::get_dict_level(const char *uid)
{
	std::map<std::string, DictInfoDictItem *>::iterator uid_iter;
	uid_iter = uidmap.find(uid);
	if (uid_iter==uidmap.end())
		return -1;
	return uid_iter->second->level;
}

std::string Libs::get_dicts_list(const char *dictmask, int max_dict_count, int userLevel)
{
	std::list<std::string> uid_list;
	std::string uid;
	const char *p, *p1;
	p = dictmask;
	do {
		p1 = strchr(p, ' ');
		if (p1) {
			uid.assign(p, p1-p);
			if (!uid.empty())
				uid_list.push_back(uid);
			p = p1+1;
		}
	} while (p1);
	uid = p;
	if (!uid.empty())
		uid_list.push_back(uid);

	std::string dictmask_str;
	int count = 0;
	const std::string *info_string;
	int level;
	for (std::list<std::string>::iterator i = uid_list.begin(); i!= uid_list.end(); ++i) {
		level = get_dict_level((*i).c_str());
		if (level < 0 || level > userLevel)
			continue;
		info_string = get_dict_info(i->c_str(), true);
		if (info_string) {
			if (count>=max_dict_count)
				break;
			dictmask_str += info_string->c_str();
			count++;
		}
	}
	return dictmask_str;
}

const std::string *Libs::get_dict_info(const char *uid, bool is_short)
{
	std::map<std::string, DictInfoDictItem *>::iterator uid_iter;
	uid_iter = uidmap.find(uid);
	if (uid_iter==uidmap.end())
		return NULL;
	DictInfoDictItem *dict;
	dict = uid_iter->second;
	if (is_short) {
		if (dict->short_info_string.empty()) {
			gchar *etext;
			dict->short_info_string += "<dict><uid>";
			dict->short_info_string += uid;
			dict->short_info_string += "</uid><bookname>";
			etext = g_markup_escape_text(oLib[dict->id]->dict_name().c_str(), -1);
			dict->short_info_string += etext;
			g_free(etext);
			dict->short_info_string += "</bookname><wordcount>";
			gchar *wc = g_strdup_printf("%ld", oLib[dict->id]->narticles());
			dict->short_info_string += wc;
			g_free(wc);
			dict->short_info_string += "</wordcount></dict>";
		}
		return &(dict->short_info_string);
	} else {
		if (dict->info_string.empty()) {
			gchar *etext;
			DictInfo dict_info;
			if (!dict_info.load_from_ifo_file(oLib[dict->id]->ifofilename(), 
				DictInfoType_NormDict))
				return NULL;
			dict->info_string += "<dictinfo><bookname>";
			etext = g_markup_escape_text(dict_info.bookname.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</bookname><wordcount>";
			gchar *wc = g_strdup_printf("%u", dict_info.wordcount);
			dict->info_string += wc;
			g_free(wc);
			dict->info_string += "</wordcount>";
			if (dict_info.synwordcount!=0) {
				dict->info_string += "<synwordcount>";
				wc = g_strdup_printf("%u", dict_info.synwordcount);
				dict->info_string += wc;
				g_free(wc);
				dict->info_string += "</synwordcount>";
			}
			dict->info_string += "<author>";
			etext = g_markup_escape_text(dict_info.author.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</author><email>";
			etext = g_markup_escape_text(dict_info.email.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</email><website>";
			etext = g_markup_escape_text(dict_info.website.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</website><description>";
			etext = g_markup_escape_text(dict_info.description.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</description><date>";
			etext = g_markup_escape_text(dict_info.date.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</date><download>";
			etext = g_markup_escape_text(dict->download.c_str(), -1);
			dict->info_string += etext;
			g_free(etext);
			dict->info_string += "</download></dictinfo>";
		}
		return &(dict->info_string);
	}
}

void Libs::SetServerDictMask(std::vector<InstantDictIndex> &dictmask, const char *dicts, int max, int userLevel)
{
	InstantDictIndex instance_dict_index;
	instance_dict_index.type = InstantDictType_LOCAL;
	dictmask.clear();
	std::list<std::string> uid_list;
	std::string uid;
	const char *p, *p1;
	p = dicts;
	do {
		p1 = strchr(p, ' ');
		if (p1) {
			uid.assign(p, p1-p);
			if (!uid.empty())
				uid_list.push_back(uid);
			p = p1+1;
		}
	} while (p1);
	uid = p;
	if (!uid.empty())
		uid_list.push_back(uid);
	int count = 0;
	std::map<std::string, DictInfoDictItem *>::iterator uid_iter;
	for (std::list<std::string>::iterator i = uid_list.begin(); i!= uid_list.end(); ++i) {
		uid_iter = uidmap.find(*i);
		if (uid_iter!=uidmap.end()) {
			if (max>=0 && count >= max)
				break;
			if (userLevel>=0 && (unsigned int)userLevel< uid_iter->second->level)
				continue;
			instance_dict_index.index = uid_iter->second->id;
			dictmask.push_back(instance_dict_index);
			count++;
		}
	}
}

void Libs::LoadCollateFile(std::vector<InstantDictIndex> &dictmask, CollateFunctions cltfuc)
{
	for (std::vector<InstantDictIndex>::iterator i = dictmask.begin(); i!=dictmask.end(); ++i) {
		if ((*i).type == InstantDictType_LOCAL) {
			oLib[(*i).index]->idx_file->collate_load(cltfuc, CollationLevel_MULTI);
			if (oLib[(*i).index]->syn_file.get() != NULL)
				oLib[(*i).index]->syn_file->collate_load(cltfuc, CollationLevel_MULTI);
		}
	}
}
#endif

#ifdef SD_CLIENT_CODE
bool Libs::find_lib_by_id(const DictItemId& id, size_t &iLib)
{
	for (std::vector<Dict *>::size_type i =0; i < oLib.size(); i++) {
		if (oLib[i]->id() == id) {
			iLib = i;
			return true;
		}
	}
	return false;
}

void Libs::load(const std::list<std::string> &load_list)
{
	for (std::list<std::string>::const_iterator i = load_list.begin(); i != load_list.end(); ++i) {
		load_dict(*i, show_progress);
	}
}

void Libs::reload(const std::list<std::string> &load_list, CollationLevelType NewCollationLevel, CollateFunctions collf)
{
	ValidateCollateParams(NewCollationLevel, collf);
	if (NewCollationLevel == CollationLevel && collf == CollateFunction) {
		std::vector<Dict *> prev(oLib);
		oLib.clear();
		for (std::list<std::string>::const_iterator i = load_list.begin(); i != load_list.end(); ++i) {
			std::vector<Dict *>::iterator it;
			for (it=prev.begin(); it!=prev.end(); ++it) {
				if ((*it)->ifofilename()==*i)
					break;
			}
			if (it==prev.end()) {
				load_dict(*i, show_progress);
			} else {
				Dict *res=*it;
				prev.erase(it);
				oLib.push_back(res);
			}
		}
		for (std::vector<Dict *>::iterator it=prev.begin(); it!=prev.end(); ++it) {
			delete *it;
		}
	} else {
		for (std::vector<Dict *>::iterator it = oLib.begin(); it != oLib.end(); ++it)
			delete *it;
		oLib.clear();
		free_collations();
		CollationLevel = NewCollationLevel;
		CollateFunction = CollateFunctions(collf);
		init_collations();
		load(load_list);
	}
}
#endif

glong Libs::CltIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return cltidx;
	if (CollationLevel == CollationLevel_SINGLE) {
		if (cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->idx_file->get_clt_file()->GetOrigIndex(cltidx);
	}
	if (servercollatefunc == 0)
		return cltidx;
	if (cltidx == INVALID_INDEX)
		return cltidx;
	oLib[iLib]->idx_file->collate_load((CollateFunctions)(servercollatefunc-1), CollationLevel_MULTI);
	return oLib[iLib]->idx_file->get_clt_file(servercollatefunc-1)->GetOrigIndex(cltidx);
}

glong Libs::CltSynIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc)
{
	if (CollationLevel == CollationLevel_NONE)
		return cltidx;
	if (CollationLevel == CollationLevel_SINGLE) {
		if (cltidx == UNSET_INDEX || cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->syn_file->get_clt_file()->GetOrigIndex(cltidx);
	}
	if (servercollatefunc == 0)
		return cltidx;
	if (cltidx == UNSET_INDEX || cltidx == INVALID_INDEX)
		return cltidx;
	oLib[iLib]->syn_file->collate_load((CollateFunctions)(servercollatefunc-1), CollationLevel_MULTI);
	return oLib[iLib]->syn_file->get_clt_file(servercollatefunc-1)->GetOrigIndex(cltidx);
}

const gchar *Libs::GetSuggestWord(const gchar *sWord, CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc)
{
	const gchar *poCurrentWord = NULL;
	const gchar *word;
	gint best =0;
	gint back;
	std::vector<InstantDictIndex>::size_type iLib;
	std::vector<Dict *>::size_type iRealLib;
	for (iLib=0; iLib < dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		if(iCurrent[iLib].idx_suggest == INVALID_INDEX || iCurrent[iLib].idx_suggest == UNSET_INDEX)
			continue;
		iRealLib = dictmask[iLib].index;
		if ( poCurrentWord == NULL ) {
			poCurrentWord = poGetWord(iCurrent[iLib].idx_suggest, iRealLib, servercollatefunc);
			best = prefix_match (sWord, poCurrentWord);
		} else {
			word = poGetWord(iCurrent[iLib].idx_suggest, iRealLib, servercollatefunc);
			back = prefix_match (sWord, word);
			if (back > best) {
				best = back;
				poCurrentWord = word;
			} else if (back == best) {
				gint x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
				if (x > 0) {
					poCurrentWord = word;
				}
			}
		}
	}
	for (iLib=0; iLib<dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		if (iCurrent[iLib].synidx_suggest==INVALID_INDEX || iCurrent[iLib].synidx_suggest==UNSET_INDEX)
			continue;
		iRealLib = dictmask[iLib].index;
		if ( poCurrentWord == NULL ) {
			poCurrentWord = poGetSynonymWord(iCurrent[iLib].synidx_suggest, iRealLib, servercollatefunc);
			best = prefix_match (sWord, poCurrentWord);
		} else {
			word = poGetSynonymWord(iCurrent[iLib].synidx_suggest, iRealLib, servercollatefunc);
			back = prefix_match (sWord, word);
			if (back > best) {
				best = back;
				poCurrentWord = word;
			} else if (back == best) {
				gint x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
				if (x > 0) {
					poCurrentWord = word;
				}
			}
		}
	}
	return poCurrentWord;
}

const gchar *Libs::poGetCurrentWord(CurrentIndex * iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc)
{
	const gchar *poCurrentWord = NULL;
	const gchar *word;
	std::vector<InstantDictIndex>::size_type iLib;
	std::vector<Dict *>::size_type iRealLib;
	for (iLib=0; iLib < dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (iCurrent[iLib].idx==INVALID_INDEX)
			continue;
		if ( iCurrent[iLib].idx>=narticles(iRealLib) || iCurrent[iLib].idx<0)
			continue;
		if ( poCurrentWord == NULL ) {
			poCurrentWord = poGetWord(iCurrent[iLib].idx, iRealLib, servercollatefunc);
		} else {
			word = poGetWord(iCurrent[iLib].idx, iRealLib, servercollatefunc);
			gint x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
			if (x > 0) {
				poCurrentWord = word;
			}
		}
	}
	for (iLib=0; iLib<dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx==INVALID_INDEX)
			continue;
		if ( iCurrent[iLib].synidx>=nsynarticles(iRealLib) || iCurrent[iLib].synidx<0)
			continue;
		if ( poCurrentWord == NULL ) {
			poCurrentWord = poGetSynonymWord(iCurrent[iLib].synidx, iRealLib, servercollatefunc);
		} else {
			word = poGetSynonymWord(iCurrent[iLib].synidx, iRealLib, servercollatefunc);
			gint x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
			if (x > 0) {
				poCurrentWord = word;
			}
		}
	}
	return poCurrentWord;
}

const gchar *
Libs::poGetNextWord(const gchar *sWord, CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc)
{
	// the input can be:
	// (word,iCurrent),read word,write iNext to iCurrent,and return next word. used by TopWin::NextCallback();
	// (NULL,iCurrent),read iCurrent,write iNext to iCurrent,and return next word. used by AppCore::ListWords();
	const gchar *poCurrentWord = NULL;
	std::vector<Dict *>::size_type iCurrentLib=0, iCurrentRealLib=0;
	bool isLib = false;
	const gchar *word;

	std::vector<InstantDictIndex>::size_type iLib;
	std::vector<Dict *>::size_type iRealLib;
	for (iLib=0; iLib < dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->Lookup(sWord, iCurrent[iLib].idx, iCurrent[iLib].idx_suggest, CollationLevel, servercollatefunc);
		}
		if (iCurrent[iLib].idx==INVALID_INDEX)
			continue;
		if (iCurrent[iLib].idx>=narticles(iRealLib) || iCurrent[iLib].idx<0)
			continue;
		if (poCurrentWord == NULL ) {
			poCurrentWord = poGetWord(iCurrent[iLib].idx, iRealLib, servercollatefunc);
			iCurrentLib = iLib;
			iCurrentRealLib = iRealLib;
			isLib=true;
		} else {
			gint x;
			word = poGetWord(iCurrent[iLib].idx, iRealLib, servercollatefunc);
			x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
			if (x > 0) {
				poCurrentWord = word;
				iCurrentLib = iLib;
				iCurrentRealLib = iRealLib;
				isLib=true;
			}
		}
	}
	for (iLib=0; iLib < dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->LookupSynonym(sWord, iCurrent[iLib].synidx, iCurrent[iLib].synidx_suggest, CollationLevel, servercollatefunc);
		}
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx==INVALID_INDEX)
			continue;
		if (iCurrent[iLib].synidx>=nsynarticles(iRealLib) || iCurrent[iLib].synidx<0)
			continue;
		if (poCurrentWord == NULL ) {
			poCurrentWord = poGetSynonymWord(iCurrent[iLib].synidx, iRealLib, servercollatefunc);
			iCurrentLib = iLib;
			iCurrentRealLib = iRealLib;
			isLib=false;
		} else {
			gint x;
			word = poGetSynonymWord(iCurrent[iLib].synidx, iRealLib, servercollatefunc);
			x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
			if (x > 0 ) {
				poCurrentWord = word;
				iCurrentLib = iLib;
				iCurrentRealLib = iRealLib;
				isLib=false;
			}
		}
	}
	if (poCurrentWord) {
		for (iLib=0; iLib < dictmask.size(); iLib++) {
			if (dictmask[iLib].type != InstantDictType_LOCAL)
				continue;
			iRealLib = dictmask[iLib].index;
			if (isLib && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].idx==INVALID_INDEX)
				continue;
			if (iCurrent[iLib].idx>=narticles(iRealLib) || iCurrent[iLib].idx<0)
				continue;
			word = poGetWord(iCurrent[iLib].idx, iRealLib, servercollatefunc);
			if (strcmp(poCurrentWord, word) == 0) {
				GetWordNext(iCurrent[iLib].idx, iRealLib, true, servercollatefunc);
			}
		}
		for (iLib=0; iLib < dictmask.size(); iLib++) {
			if (dictmask[iLib].type != InstantDictType_LOCAL)
				continue;
			iRealLib = dictmask[iLib].index;
			if ((!isLib) && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].synidx==UNSET_INDEX)
				continue;
			if (iCurrent[iLib].synidx==INVALID_INDEX)
				continue;
			if (iCurrent[iLib].synidx>=nsynarticles(iRealLib) || iCurrent[iLib].synidx<0)
				continue;
			word = poGetSynonymWord(iCurrent[iLib].synidx, iRealLib, servercollatefunc);
			if (strcmp(poCurrentWord, word) == 0) {
				GetWordNext(iCurrent[iLib].synidx, iRealLib, false, servercollatefunc);
			}
		}
		//GetWordNext will change poCurrentWord's content, so do it at the last.
		if (isLib) {
			GetWordNext(iCurrent[iCurrentLib].idx, iCurrentRealLib, true, servercollatefunc);
		} else {
			GetWordNext(iCurrent[iCurrentLib].synidx, iCurrentRealLib, false, servercollatefunc);
		}
		poCurrentWord = poGetCurrentWord(iCurrent, dictmask, servercollatefunc);
	}
	return poCurrentWord;
}

const gchar *
Libs::poGetPreWord(const gchar *sWord, CurrentIndex* iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc)
{
	// used by TopWin::PreviousCallback(); the iCurrent is cached by AppCore::TopWinWordChange();
	const gchar *poCurrentWord = NULL;
	std::vector<Dict *>::size_type iCurrentLib=0, iCurrentRealLib=0;
	bool isLib = false;

	const gchar *word;
	glong pidx;
	std::vector<InstantDictIndex>::size_type iLib;
	std::vector<Dict *>::size_type iRealLib;
	// lookup in index
	for (iLib=0;iLib<dictmask.size();iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->Lookup(sWord, iCurrent[iLib].idx, iCurrent[iLib].idx_suggest, CollationLevel, servercollatefunc);
		}
		if (iCurrent[iLib].idx!=INVALID_INDEX) {
			if ( iCurrent[iLib].idx>=narticles(iRealLib) || iCurrent[iLib].idx<=0)
				continue;
		}
		if ( poCurrentWord == NULL ) {
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iRealLib, true, servercollatefunc)) {
				poCurrentWord = poGetWord(pidx, iRealLib, servercollatefunc);
				iCurrentLib = iLib;
				iCurrentRealLib = iRealLib;
				isLib=true;
			}
		} else {
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iRealLib, true, servercollatefunc)) {
				gint x;
				word = poGetWord(pidx, iRealLib, servercollatefunc);
				x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
				if (x < 0 ) {
					poCurrentWord = word;
					iCurrentLib = iLib;
					iCurrentRealLib = iRealLib;
					isLib=true;
				}
			}
		}
	}
	// lookup synonyms
	for (iLib=0;iLib<dictmask.size();iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->LookupSynonym(sWord, iCurrent[iLib].synidx, iCurrent[iLib].synidx_suggest, CollationLevel, servercollatefunc);
		}
		if (iCurrent[iLib].synidx==UNSET_INDEX)
			continue;
		if (iCurrent[iLib].synidx!=INVALID_INDEX) {
			if ( iCurrent[iLib].synidx>=nsynarticles(iRealLib) || iCurrent[iLib].synidx<=0)
				continue;
		}
		if ( poCurrentWord == NULL ) {
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iRealLib, false, servercollatefunc)) {
				poCurrentWord = poGetSynonymWord(pidx, iRealLib, servercollatefunc);
				iCurrentLib = iLib;
				iCurrentRealLib = iRealLib;
				isLib=false;
			}
		} else {
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iRealLib, false, servercollatefunc)) {
				gint x;
				word = poGetSynonymWord(pidx,iRealLib, servercollatefunc);
				x = stardict_server_collate(poCurrentWord, word, CollationLevel, CollateFunction, servercollatefunc);
				if (x < 0 ) {
					poCurrentWord = word;
					iCurrentLib = iLib;
					iCurrentRealLib = iRealLib;
					isLib=false;
				}
			}
		}
	}
	if (poCurrentWord) {
		/* poCurrentWord - the "previous" word for the sWord word among all word in 
		 * all local dictionaries specified by dictmask */
		for (iLib=0;iLib<dictmask.size();iLib++) {
			if (dictmask[iLib].type != InstantDictType_LOCAL)
				continue;
			iRealLib = dictmask[iLib].index;
			if (isLib && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].idx!=INVALID_INDEX) {
				if (iCurrent[iLib].idx>=narticles(iRealLib) || iCurrent[iLib].idx<=0)
					continue;
			}
			if (GetWordPrev(iCurrent[iLib].idx, pidx, iRealLib, true, servercollatefunc)) {
				word = poGetWord(pidx, iRealLib, servercollatefunc);
				if (strcmp(poCurrentWord, word) == 0) {
					iCurrent[iLib].idx=pidx;
				}
			}
		}
		for (iLib=0;iLib<dictmask.size();iLib++) {
			if (dictmask[iLib].type != InstantDictType_LOCAL)
				continue;
			iRealLib = dictmask[iLib].index;
			if ((!isLib) && (iLib == iCurrentLib))
				continue;
			if (iCurrent[iLib].synidx==UNSET_INDEX)
				continue;
			if (iCurrent[iLib].synidx!=INVALID_INDEX) {
				if (iCurrent[iLib].synidx>=nsynarticles(iRealLib) || iCurrent[iLib].synidx<=0)
					continue;
			}
			if (GetWordPrev(iCurrent[iLib].synidx, pidx, iRealLib, false, servercollatefunc)) {
				word = poGetSynonymWord(pidx, iRealLib, servercollatefunc);
				if (strcmp(poCurrentWord, word) == 0) {
					iCurrent[iLib].synidx=pidx;
				}
			}
		}
		if (isLib) {
			GetWordPrev(iCurrent[iCurrentLib].idx, pidx, iCurrentRealLib, true, servercollatefunc);
			iCurrent[iCurrentLib].idx = pidx;
		} else {
			GetWordPrev(iCurrent[iCurrentLib].synidx, pidx, iCurrentRealLib, false, servercollatefunc);
			iCurrent[iCurrentLib].synidx = pidx;
		}
	}
	return poCurrentWord;
}

bool Libs::LookupSynonymSimilarWord(const gchar* sWord, glong &iSynonymWordIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc)
{
	if (oLib[iLib]->syn_file.get() == NULL)
		return false;

	glong iIndex;
	glong iIndex_suggest;
	bool bFound=false;
	gchar *casestr;
	bool bLookup;

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, CollationLevel, servercollatefunc);
			if(bLookup)
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, CollationLevel, servercollatefunc);
				if(bLookup)
					bFound=true;
			}
			g_free(casestr);
		}
		// Upper the first character and lower others.
		if (!bFound) {
			gchar *nextchar = g_utf8_next_char(sWord);
			gchar *firstchar = g_utf8_strup(sWord, nextchar - sWord);
			nextchar = g_utf8_strdown(nextchar, -1);
			casestr = g_strdup_printf("%s%s", firstchar, nextchar);
			g_free(firstchar);
			g_free(nextchar);
			if (strcmp(casestr, sWord)) {
				bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, CollationLevel, servercollatefunc);
				if(bLookup)
					bFound=true;
			}
			g_free(casestr);
		}
		if (!bFound) {
			iIndex = iSynonymWordIndex;
			glong pidx;
			const gchar *cword;
			do {
				if (GetWordPrev(iIndex, pidx, iLib, false, servercollatefunc)) {
					cword = poGetSynonymWord(pidx, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, CollationLevel, CollateFunction, servercollatefunc)==0) {
						iIndex = pidx;
						bFound=true;
					} else {
						break;
					}
				} else {
					break;
				}
			} while (true);
			if (!bFound) {
				if (iIndex!=INVALID_INDEX) {
					cword = poGetSynonymWord(iIndex, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, CollationLevel, CollateFunction, servercollatefunc)==0) {
						bFound=true;
					}
				}
			}
		}
	}
	if (bFound) {
		iSynonymWordIndex = iIndex;
		synidx_suggest = iIndex_suggest;
	}
	return bFound;
}

/* A helper function for LookupSimilarWord method.
 * It accepts too many parameters but simplifies the main function a bit... 
 * Return value - whether the lookup was successful.
 * idx_suggest is updated if a better partial match is found. */
bool Libs::LookupSimilarWordTryWord(const gchar *sTryWord, const gchar *sWord,
	int servercollatefunc, size_t iLib,
	glong &iIndex, glong &idx_suggest, gint &best_match)
{
	glong iIndexSuggest;
	if(oLib[iLib]->Lookup(sTryWord, iIndex, iIndexSuggest, CollationLevel, servercollatefunc)) {
		best_match = g_utf8_strlen(sTryWord, -1);
		idx_suggest = iIndexSuggest;
		return true;
	} else {
		gint cur_match = prefix_match(sWord, poGetWord(iIndexSuggest, iLib, servercollatefunc));
		if(cur_match > best_match) {
			best_match = cur_match;
			idx_suggest = iIndexSuggest;
		}
		return false;
	}
}

/* Search for a word similar to sWord.
 * Return true if a similar word is found. 
 * If a similar word is found, iWordIndex and idx_suggest point to the found word. 
 * If a similar word is not found, idx_suggest points to the best partial match
 * found so far, iWordIndex does not change. 
 * Input parameters:
 * iWordIndex must be initialized with a valid index. The value is used a basis
 * for searching a similar word. iWordIndex may be INVALID_INDEX. 
 * idx_suggest must be initialized. If it is a valid index, it participates in
 * searching for the best partial match. */
bool Libs::LookupSimilarWord(const gchar* sWord, glong & iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc)
{
	glong iIndex;
	bool bFound=false;
	gchar *casestr;
	gint best_match = 0;
	
	if(idx_suggest != UNSET_INDEX && idx_suggest != INVALID_INDEX) {
		best_match = prefix_match(sWord, poGetWord(idx_suggest, iLib, servercollatefunc));
	}

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
			}
			g_free(casestr);
		}
		// Upper the first character and lower others.
		if (!bFound) {
			gchar *nextchar = g_utf8_next_char(sWord);
			gchar *firstchar = g_utf8_strup(sWord, nextchar - sWord);
			nextchar = g_utf8_strdown(nextchar, -1);
			casestr = g_strdup_printf("%s%s", firstchar, nextchar);
			g_free(firstchar);
			g_free(nextchar);
			if (strcmp(casestr, sWord)) {
				if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
			}
			g_free(casestr);
		}
		// compare with the preceding words in the index case-insensitive
		// iWordIndex - the base index
		if (!bFound) {
			iIndex = iWordIndex;
			glong pidx;
			const gchar *cword;
			do {
				if (GetWordPrev(iIndex, pidx, iLib, true, servercollatefunc)) {
					cword = poGetWord(pidx, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, CollationLevel, CollateFunction, servercollatefunc)==0) {
						iIndex = pidx;
						bFound=true;
					} else {
						gint cur_match = prefix_match(sWord, cword);
						if(cur_match > best_match) {
							best_match = cur_match;
							idx_suggest = pidx;
						}
						break;
					}
				} else {
					break;
				}
			} while (true);
			if (!bFound) {
				if (iIndex!=INVALID_INDEX) {
					cword = poGetWord(iIndex, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, CollationLevel, CollateFunction, servercollatefunc)==0) {
						bFound=true;
					} else {
						gint cur_match = prefix_match(sWord, cword);
						if(cur_match > best_match) {
							best_match = cur_match;
							idx_suggest = iIndex;
						}
					}
				}
			}
			if(bFound) {
				best_match = g_utf8_strlen(poGetWord(iIndex, iLib, servercollatefunc), -1);
				idx_suggest = iIndex;
			}
		}
	}

	if (IsASCII(sWord)) {
		// If not Found, try other status of sWord.
		size_t iWordLen=strlen(sWord);
		bool isupcase;

		gchar *sNewWord = (gchar *)g_malloc(iWordLen + 1);

		//cut one char "s" or "d"
		if(!bFound && iWordLen>1) {
			isupcase = sWord[iWordLen-1]=='S' || !strncmp(&sWord[iWordLen-2],"ED",2);
			if (isupcase || sWord[iWordLen-1]=='s' || !strncmp(&sWord[iWordLen-2],"ed",2)) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-1]='\0'; // cut "s" or "d"
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		//cut "ly"
		if(!bFound && iWordLen>2) {
			isupcase = !strncmp(&sWord[iWordLen-2],"LY",2);
			if (isupcase || (!strncmp(&sWord[iWordLen-2],"ly",2))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';  // cut "ly"
				if (iWordLen>5 && sNewWord[iWordLen-3]==sNewWord[iWordLen-4]
				    && !bIsVowel(sNewWord[iWordLen-4]) &&
				    bIsVowel(sNewWord[iWordLen-5])) {//doubled

					sNewWord[iWordLen-3]='\0';
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		//cut "ing"
		if(!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"ING",3);
			if (isupcase || !strncmp(&sWord[iWordLen-3],"ing",3) ) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if ( iWordLen>6 && (sNewWord[iWordLen-4]==sNewWord[iWordLen-5])
				     && !bIsVowel(sNewWord[iWordLen-5]) &&
				     bIsVowel(sNewWord[iWordLen-6])) {  //doubled
					sNewWord[iWordLen-4]='\0';
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-4]=sNewWord[iWordLen-5];  //restore
					}
				}
				if( !bFound ) {
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
								bFound=true;
						}
						g_free(casestr);
					}
				}
				if(!bFound) {
					if (isupcase)
						strcat(sNewWord,"E"); // add a char "E"
					else
						strcat(sNewWord,"e"); // add a char "e"
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		//cut two char "es"
		if(!bFound && iWordLen>3) {
			isupcase = (!strncmp(&sWord[iWordLen-2],"ES",2) &&
				    (sWord[iWordLen-3] == 'S' ||
				     sWord[iWordLen-3] == 'X' ||
				     sWord[iWordLen-3] == 'O' ||
				     (iWordLen >4 && sWord[iWordLen-3] == 'H' &&
				      (sWord[iWordLen-4] == 'C' ||
				       sWord[iWordLen-4] == 'S'))));
			if (isupcase ||
			    (!strncmp(&sWord[iWordLen-2],"es",2) &&
			     (sWord[iWordLen-3] == 's' || sWord[iWordLen-3] == 'x' ||
			      sWord[iWordLen-3] == 'o' ||
			      (iWordLen >4 && sWord[iWordLen-3] == 'h' &&
			       (sWord[iWordLen-4] == 'c' || sWord[iWordLen-4] == 's'))))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		//cut "ed"
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-2],"ED",2);
			if (isupcase || !strncmp(&sWord[iWordLen-2],"ed",2)) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if (iWordLen>5 && (sNewWord[iWordLen-3]==sNewWord[iWordLen-4])
				    && !bIsVowel(sNewWord[iWordLen-4]) &&
				    bIsVowel(sNewWord[iWordLen-5])) {//doubled
					sNewWord[iWordLen-3]='\0';
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
								bFound=true;
						}
						g_free(casestr);
					}
				}
			}
		}

		// cut "ied" , add "y".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"IED",3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"ied",3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if (isupcase)
					strcat(sNewWord,"Y"); // add a char "Y"
				else
					strcat(sNewWord,"y"); // add a char "y"
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "ies" , add "y".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3],"IES",3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"ies",3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if (isupcase)
					strcat(sNewWord,"Y"); // add a char "Y"
				else
					strcat(sNewWord,"y"); // add a char "y"
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "er".
		if (!bFound && iWordLen>2) {
			isupcase = !strncmp(&sWord[iWordLen-2],"ER",2);
			if (isupcase || (!strncmp(&sWord[iWordLen-2],"er",2))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-2]='\0';
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		// cut "est".
		if (!bFound && iWordLen>3) {
			isupcase = !strncmp(&sWord[iWordLen-3], "EST", 3);
			if (isupcase || (!strncmp(&sWord[iWordLen-3],"est", 3))) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-3]='\0';
				if(LookupSimilarWordTryWord(sNewWord, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(LookupSimilarWordTryWord(casestr, sWord, servercollatefunc, iLib, iIndex, idx_suggest, best_match))
							bFound=true;
					}
					g_free(casestr);
				}
			}
		}

		g_free(sNewWord);
	}

	if (bFound)
		iWordIndex = iIndex;
#if 0
	else {
		//don't change iWordIndex here.
		//when LookupSimilarWord all failed too, we want to use the old LookupWord index to list words.
		//iWordIndex = INVALID_INDEX;
	}
#endif
	return bFound;
}

bool Libs::SimpleLookupWord(const gchar* sWord, glong & iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc)
{
	bool bFound = oLib[iLib]->Lookup(sWord, iWordIndex, idx_suggest, CollationLevel, servercollatefunc);
	if (!bFound)
		bFound = LookupSimilarWord(sWord, iWordIndex, idx_suggest, iLib, servercollatefunc);
	return bFound;
}

bool Libs::SimpleLookupSynonymWord(const gchar* sWord, glong & iWordIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc)
{
	bool bFound = oLib[iLib]->LookupSynonym(sWord, iWordIndex, synidx_suggest, CollationLevel, servercollatefunc);
	if (!bFound)
		bFound = LookupSynonymSimilarWord(sWord, iWordIndex, synidx_suggest, iLib, servercollatefunc);
	return bFound;
}

struct Fuzzystruct {
	char * pMatchWord;
	int iMatchWordDistance;
};

static inline bool operator<(const Fuzzystruct & lh, const Fuzzystruct & rh) {
	if (lh.iMatchWordDistance!=rh.iMatchWordDistance)
		return lh.iMatchWordDistance<rh.iMatchWordDistance;

	if (lh.pMatchWord && rh.pMatchWord)
		return stardict_strcmp(lh.pMatchWord, rh.pMatchWord)<0;

	return false;
}

static inline void unicode_strdown(gunichar *str)
{
	while (*str) {
		*str=g_unichar_tolower(*str);
		++str;
	}
}

bool Libs::LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size, std::vector<InstantDictIndex> &dictmask)
{
	if (sWord[0] == '\0')
		return false;

	std::vector<Fuzzystruct> oFuzzystruct(reslist_size);

	for (int i=0; i<reslist_size; i++) {
		oFuzzystruct[i].pMatchWord = NULL;
		oFuzzystruct[i].iMatchWordDistance = iMaxFuzzyDistance;
	}
	int iMaxDistance = iMaxFuzzyDistance;
	int iDistance;
	bool Found = false;
	EditDistance oEditDistance;

	glong iCheckWordLen;
	const char *sCheck;
	gunichar *ucs4_str1, *ucs4_str2;
	glong ucs4_str2_len;

	ucs4_str2 = g_utf8_to_ucs4_fast(sWord, -1, &ucs4_str2_len);
	unicode_strdown(ucs4_str2);

	std::vector<Dict *>::size_type iRealLib;
	for (std::vector<InstantDictIndex>::size_type iLib=0; iLib<dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		for (gint synLib=0; synLib<2; synLib++) {
			if (synLib==1) {
				if (oLib[iRealLib]->syn_file.get()==NULL)
					break;
			}
			show_progress->notify_about_work();

			//if (stardict_strcmp(sWord, poGetWord(0,iRealLib))>=0 && stardict_strcmp(sWord, poGetWord(narticles(iRealLib)-1,iRealLib))<=0) {
			//there are Chinese dicts and English dicts...
			if (TRUE) {
				glong iwords;
				if (synLib==0)
					iwords = narticles(iRealLib);
				else
					iwords = nsynarticles(iRealLib);
				for (glong index=0; index<iwords; index++) {
					// Need to deal with same word in index? But this will slow down processing in most case.
					if (synLib==0)
						sCheck = poGetOrigWord(index,iRealLib);
					else
						sCheck = poGetOrigSynonymWord(index,iRealLib);
					// tolower and skip too long or too short words
					iCheckWordLen = g_utf8_strlen(sCheck, -1);
					if (iCheckWordLen-ucs4_str2_len>=iMaxDistance ||
					    ucs4_str2_len-iCheckWordLen>=iMaxDistance)
						continue;
					ucs4_str1 = g_utf8_to_ucs4_fast(sCheck, -1, NULL);
					if (iCheckWordLen > ucs4_str2_len)
						ucs4_str1[ucs4_str2_len]=0;
					unicode_strdown(ucs4_str1);

					iDistance = oEditDistance.CalEditDistance(ucs4_str1, ucs4_str2, iMaxDistance);
					g_free(ucs4_str1);
					if (iDistance<iMaxDistance && iDistance < ucs4_str2_len) {
						// when ucs4_str2_len=1,2 we need less fuzzy.
						Found = true;
						bool bAlreadyInList = false;
						int iMaxDistanceAt=0;
						for (int j=0; j<reslist_size; j++) {
							if (oFuzzystruct[j].pMatchWord &&
							    strcmp(oFuzzystruct[j].pMatchWord,sCheck)==0 ) {//already in list
								bAlreadyInList = true;
								break;
							}
							//find the position,it will certainly be found (include the first time) as iMaxDistance is set by last time.
							if (oFuzzystruct[j].iMatchWordDistance == iMaxDistance ) {
								iMaxDistanceAt = j;
							}
						}
						if (!bAlreadyInList) {
							if (oFuzzystruct[iMaxDistanceAt].pMatchWord)
								g_free(oFuzzystruct[iMaxDistanceAt].pMatchWord);
							oFuzzystruct[iMaxDistanceAt].pMatchWord = g_strdup(sCheck);
							oFuzzystruct[iMaxDistanceAt].iMatchWordDistance = iDistance;
							// calc new iMaxDistance
							iMaxDistance = iDistance;
							for (int j=0; j<reslist_size; j++) {
								if (oFuzzystruct[j].iMatchWordDistance > iMaxDistance)
									iMaxDistance = oFuzzystruct[j].iMatchWordDistance;
							} // calc new iMaxDistance
						}   // add to list
					}   // find one
				}   // each word
			}   // ok for search
		}  // synLib
	}   // each lib
	g_free(ucs4_str2);

	if (Found)// sort with distance
		std::sort(oFuzzystruct.begin(), oFuzzystruct.end());

	for (gint i=0; i<reslist_size; ++i)
		reslist[i]=oFuzzystruct[i].pMatchWord;

	return Found;
}

static inline bool less_for_compare(const char *lh, const char *rh) {
	return stardict_strcmp(lh, rh)<0;
}

gint Libs::LookupWithRule(const gchar *word, gchar **ppMatchWord, std::vector<InstantDictIndex> &dictmask)
{
	glong aiIndex[MAX_MATCH_ITEM_PER_LIB+1];
	gint iMatchCount = 0;
	GPatternSpec *pspec = g_pattern_spec_new(word);

	const gchar * sMatchWord;
	bool bAlreadyInList;
	std::vector<Dict *>::size_type iRealLib;
	for (std::vector<InstantDictIndex>::size_type iLib=0; iLib<dictmask.size(); iLib++) {
		//if(oLibs.LookdupWordsWithRule(pspec,aiIndex,MAX_MATCH_ITEM_PER_LIB+1-iMatchCount,iLib))
		// -iMatchCount,so save time,but may got less result and the word may repeat.
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (oLib[iRealLib]->LookupWithRule(pspec, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetOrigWord(aiIndex[i],iRealLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
		if (oLib[iRealLib]->LookupWithRuleSynonym(pspec, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetOrigSynonymWord(aiIndex[i],iRealLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
	}
	g_pattern_spec_free(pspec);

	if (iMatchCount)// sort it.
		std::sort(ppMatchWord, ppMatchWord+iMatchCount, less_for_compare);
	return iMatchCount;
}

gint Libs::LookupWithRegex(const gchar *word, gchar **ppMatchWord, std::vector<InstantDictIndex> &dictmask)
{
	glong aiIndex[MAX_MATCH_ITEM_PER_LIB+1];
	gint iMatchCount = 0;
	GRegex *regex = g_regex_new(word, G_REGEX_OPTIMIZE, (GRegexMatchFlags)0, NULL);

	const gchar * sMatchWord;
	bool bAlreadyInList;
	std::vector<Dict *>::size_type iRealLib;
	for (std::vector<InstantDictIndex>::size_type iLib=0; iLib<dictmask.size(); iLib++) {
		//if(oLibs.LookdupWordsWithRule(pspec,aiIndex,MAX_MATCH_ITEM_PER_LIB+1-iMatchCount,iLib))
		// -iMatchCount,so save time,but may got less result and the word may repeat.
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (oLib[iRealLib]->LookupWithRegex(regex, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetOrigWord(aiIndex[i],iRealLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
		if (oLib[iRealLib]->LookupWithRegexSynonym(regex, aiIndex, MAX_MATCH_ITEM_PER_LIB+1)) {
			show_progress->notify_about_work();
			for (int i=0; aiIndex[i]!=-1; i++) {
				sMatchWord = poGetOrigSynonymWord(aiIndex[i],iRealLib);
				bAlreadyInList = false;
				for (int j=0; j<iMatchCount; j++) {
					if (strcmp(ppMatchWord[j],sMatchWord)==0) {//already in list
						bAlreadyInList = true;
						break;
					}
				}
				if (!bAlreadyInList)
					ppMatchWord[iMatchCount++] = g_strdup(sMatchWord);
			}
		}
	}
	g_regex_unref(regex);

	if (iMatchCount)// sort it.
		std::sort(ppMatchWord, ppMatchWord+iMatchCount, less_for_compare);
	return iMatchCount;
}

bool Libs::LookupData(const gchar *sWord, std::vector<gchar *> *reslist, updateSearchDialog_func search_func, gpointer search_data, bool *cancel, std::vector<InstantDictIndex> &dictmask)
{
	std::vector<std::string> SearchWords;
	std::string SearchWord;
	const char *p=sWord;
	while (*p) {
		if (*p=='\\') {
			p++;
			switch (*p) {
			case ' ':
				SearchWord+=' ';
				break;
			case '\\':
				SearchWord+='\\';
				break;
			case 't':
				SearchWord+='\t';
				break;
			case 'n':
				SearchWord+='\n';
				break;
			default:
				SearchWord+=*p;
			}
		} else if (*p == ' ') {
			if (!SearchWord.empty()) {
				SearchWords.push_back(SearchWord);
				SearchWord.clear();
			}
		} else {
			SearchWord+=*p;
		}
		p++;
	}
	if (!SearchWord.empty()) {
		SearchWords.push_back(SearchWord);
		SearchWord.clear();
	}
	if (SearchWords.empty())
		return false;

	glong search_count=0;
	glong total_count=0;
	if (search_func) {
		for (std::vector<InstantDictIndex>::size_type i=0; i<dictmask.size(); ++i) {
			if (dictmask[i].type == InstantDictType_LOCAL)
				total_count += narticles(dictmask[i].index);
		}
	}

	guint32 max_size =0;
	gchar *origin_data = NULL;
	std::vector<InstantDictIndex>::size_type iRealLib;
	for (std::vector<InstantDictIndex>::size_type i=0; i<dictmask.size(); ++i) {
		if (dictmask[i].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[i].index;
		if (!oLib[iRealLib]->containSearchData())
			continue;
		const gulong iwords = narticles(iRealLib);
		const gchar *key;
		guint32 offset, size;
		for (gulong j=0; j<iwords; ++j) {
			if (search_func) {
				if (*cancel)
					goto search_out;
				if (search_count % 10000 == 0) {
					search_func(search_data, (gdouble)search_count/(gdouble)total_count);
				}
				search_count++;
			}
			oLib[iRealLib]->get_key_and_data(j, &key, &offset, &size);
			if (size>max_size) {
				origin_data = (gchar *)g_realloc(origin_data, size);
				max_size = size;
			}
			if (oLib[iRealLib]->SearchData(SearchWords, offset, size, origin_data)) {
				if (reslist[i].empty() || strcmp(reslist[i].back(), key))
					reslist[i].push_back(g_strdup(key));
			}
		}
	}
search_out:
	g_free(origin_data);
	//KMP_end();

	std::vector<InstantDictIndex>::size_type i;
	for (i=0; i<dictmask.size(); ++i)
		if (!reslist[i].empty())
			break;

	return i!=dictmask.size();
}

StorageType Libs::GetStorageType(size_t iLib)
{
	if (oLib[iLib]->storage == NULL)
		return StorageType_UNKNOWN;
	return oLib[iLib]->storage->get_storage_type();
}

FileHolder Libs::GetStorageFilePath(size_t iLib, const std::string &key)
{
	if (oLib[iLib]->storage == NULL)
		return FileHolder();
	return oLib[iLib]->storage->get_file_path(key);
}

const char *Libs::GetStorageFileContent(size_t iLib, const std::string &key)
{
	if (oLib[iLib]->storage == NULL)
		return NULL;
	return oLib[iLib]->storage->get_file_content(key);
}

void Libs::init_collations()
{
	if (CollationLevel == CollationLevel_SINGLE) {
		if (utf8_collate_init(CollateFunction))
			g_print("Init collate function failed!\n");
	} else if (CollationLevel == CollationLevel_MULTI){
		if (utf8_collate_init_all())
			g_print("Init collate functions failed!\n");
	}
}

void Libs::free_collations()
{
	if(CollationLevel == CollationLevel_SINGLE)
		utf8_collate_end(CollateFunction);
	else if(CollationLevel == CollationLevel_MULTI)
		utf8_collate_end_all();
}

void Libs::ValidateCollateParams(CollationLevelType& level, CollateFunctions& func)
{
	if(level == CollationLevel_SINGLE) {
		if(func == COLLATE_FUNC_NONE) {
			g_print(_("Invalid collate function. Disable collation."));
			level = CollationLevel_NONE;
		}
	} else {
		func = COLLATE_FUNC_NONE;
	}
}
