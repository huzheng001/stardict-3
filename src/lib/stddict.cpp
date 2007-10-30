/*
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * Implementation of class to work with standard StarDict's dictionaries
 * lookup word, get articles and so on.
 *
 * Notice: read doc/DICTFILE_FORMAT for the dictionary
 * file's format information!
 */
#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "common.hpp"
#include "distance.h"
#include "kmp.h"
#include "mapfile.hpp"

#include "stddict.hpp"
#include <algorithm>
#include "getuint32.h"

static inline gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
  gint a=g_ascii_strcasecmp(s1, s2);
  if (a == 0)
    return strcmp(s1, s2);
  else
    return a;
}

static gint stardict_collate(const gchar *str1, const gchar *str2, CollateFunctions func)
{
	gint x = utf8_collate(str1, str2, func);
	if (x == 0)
		return strcmp(str1, str2);
	else
		return x;
}

gint stardict_server_collate(const gchar *str1, const gchar *str2, int EnableCollationLevel, CollateFunctions func, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return stardict_strcmp(str1, str2);
	if (EnableCollationLevel == 1)
		return stardict_collate(str1, str2, func);
	if (servercollatefunc == 0)
		return stardict_strcmp(str1, str2);
	return stardict_collate(str1, str2, (CollateFunctions)(servercollatefunc-1));
}

gint stardict_casecmp(const gchar *s1, const gchar *s2, int EnableCollationLevel, CollateFunctions func, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return g_ascii_strcasecmp(s1, s2);
	if (EnableCollationLevel == 1)
		return utf8_collate(s1, s2, func);
	if (servercollatefunc == 0)
		return g_ascii_strcasecmp(s1, s2);
	return utf8_collate(s1, s2, (CollateFunctions)(servercollatefunc-1));
}

static inline gint prefix_match (const gchar *s1, const gchar *s2)
{
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

static inline bool bIsVowel(gchar inputchar)
{
  gchar ch = g_ascii_toupper(inputchar);
  return( ch=='A' || ch=='E' || ch=='I' || ch=='O' || ch=='U' );
}


bool bIsPureEnglish(const gchar *str)
{
  // i think this should work even when it is UTF8 string :).
  for (int i=0; str[i]!=0; i++)
    //if(str[i]<0)
    //if(str[i]<32 || str[i]>126) // tab equal 9,so this is not OK.
    // Better use isascii() but not str[i]<0 while char is default unsigned in arm
    if (!isascii(str[i]))
            return false;
  return true;
}

class offset_index : public index_file {
public:
	offset_index();
	~offset_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, int EnableCollationLevel,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
private:
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);

	static const gint ENTR_PER_PAGE=32;

	cache_file oft_file;
	FILE *idxfile;
	gulong npages;

	gchar wordentry_buf[256+sizeof(guint32)*2]; // The length of "word_str" should be less than 256. See doc/DICTFILE_FORMAT.
	struct index_entry {
		glong idx;
		std::string keystr;
		void assign(glong i, const std::string& str) {
			idx=i;
			keystr.assign(str);
		}
	};
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

class wordlist_index : public index_file {
public:
	wordlist_index();
	~wordlist_index();
	bool load(const std::string& url, gulong wc, gulong fsize,
		  bool CreateCacheFile, int EnableCollationLevel,
		  CollateFunctions _CollateFunction, show_progress_t *sp);
	void get_data(glong idx);
	const gchar *get_key_and_data(glong idx);
private:
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);

	gchar *idxdatabuf;
	std::vector<gchar *> wordlist;
};

offset_index::offset_index() : oft_file(CacheFileType_oft)
{
	clt_file = NULL;
	idxfile = NULL;
}

offset_index::~offset_index()
{
	delete clt_file;
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
	fseek(idxfile, oft_file.wordoffset[page_idx], SEEK_SET);
	guint32 page_size=oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx];
	gulong minsize = sizeof(wordentry_buf);
	if (page_size < minsize)
		minsize = page_size;
	fread(wordentry_buf, minsize, 1, idxfile); //TODO: check returned values, deal with word entry that strlen>255.
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

cache_file::cache_file(CacheFileType _cachefiletype)
{
	wordoffset = NULL;
	mf = NULL;
	cachefiletype = _cachefiletype;
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

MapFile* cache_file::get_cache_loadfile(const gchar *filename, const std::string &url, const std::string &saveurl, CollateFunctions cltfunc, glong filedatasize, int next)
{
	struct stat cachestat;
	if (g_stat(filename, &cachestat)!=0)
		return NULL;
	MapFile *mf = new MapFile;
	if (!mf->open(filename, cachestat.st_size)) {
		delete mf;
		return NULL;
	}

	gchar *p = mf->begin();
	gboolean has_prefix;
	if (cachefiletype == CacheFileType_oft)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix) {
		delete mf;
		return NULL;
	}
	if (cachefiletype == CacheFileType_oft)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2) {
		delete mf;
		return NULL;
	}
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3) {
		delete mf;
		return NULL;
	}
	gchar *tmpstr;
	tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
	tmpstr[p3-p2] = '\0';
	if (saveurl == tmpstr) {
		g_free(tmpstr);
		if (cachefiletype == CacheFileType_clt) {
			p2 = strstr(p, "\nfunc=");
			if (!p2) {
				delete mf;
				return NULL;
			}
			p2+=sizeof("\nfunc=")-1;
			p3 = strchr(p2, '\n');
			if (!p3) {
				delete mf;
				return NULL;
			}
			tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
			tmpstr[p3-p2] = '\0';
			if (atoi(tmpstr)!=cltfunc) {
				g_free(tmpstr);
				delete mf;
				return NULL;
			}
			g_free(tmpstr);
		}
		if (cachestat.st_size!=glong(filedatasize + strlen(mf->begin()) +1)) {
			delete mf;
			return NULL;
		}
		struct stat idxstat;
		if (g_stat(url.c_str(), &idxstat)!=0) {
			delete mf;
			return NULL;
		}
		if (cachestat.st_mtime<idxstat.st_mtime) {
			delete mf;
			return NULL;
		}
		//g_print("Using map file: %s\n", filename);
		return mf;
	}
	g_free(tmpstr);
	delete mf;
	gchar *basename = g_path_get_basename(saveurl.c_str());
	p = strrchr(basename, '.');
	if (!p) {
		g_free(basename);
		return NULL;
	}
	*p='\0';
	gchar *extendname = p+1;
	gchar *dirname = g_path_get_dirname(filename);
	gchar *nextfilename;
	if (cachefiletype == CacheFileType_oft)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.oft", dirname, basename, next, extendname);
	else if (cachefiletype == CacheFileType_clt)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.clt", dirname, basename, next, extendname);
	else
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.%d.clt", dirname, basename, next, extendname, cltfunc);
	MapFile *out = get_cache_loadfile(nextfilename, url, saveurl, cltfunc, filedatasize, next+1);
	g_free(basename);
	g_free(dirname);
	g_free(nextfilename);
	return out;
}

bool cache_file::load_cache(const std::string& url, const std::string& saveurl, CollateFunctions cltfunc, glong filedatasize)
{
	std::string oftfilename;
	if (cachefiletype == CacheFileType_oft)
		oftfilename=saveurl+".oft";
	else if (cachefiletype == CacheFileType_clt)
		oftfilename=saveurl+".clt";
	else {
		gchar *func = g_strdup_printf("%d", cltfunc);
		oftfilename=saveurl+'.'+func+".clt";
		g_free(func);
	}
	for (int i=0;i<2;i++) {
		if (i==1) {
			if (!get_cache_filename(saveurl, oftfilename, false, cltfunc))
				break;
		}
		mf = get_cache_loadfile(oftfilename.c_str(), url, saveurl, cltfunc, filedatasize, 2);
		if (!mf)
			continue;
		wordoffset = (guint32 *)(mf->begin()+strlen(mf->begin())+1);
		return true;
	}
	return false;
}

bool cache_file::get_cache_filename(const std::string& url, std::string &cachefilename, bool create, CollateFunctions cltfunc)
{
	if (create) {
		if (!g_file_test(g_get_user_cache_dir(), G_FILE_TEST_EXISTS) &&
		    g_mkdir(g_get_user_cache_dir(), 0700)==-1)
			return false;
	}

	std::string cache_dir=g_get_user_cache_dir();
	cache_dir += G_DIR_SEPARATOR_S "stardict";

	if (create) {
		if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_EXISTS)) {
			if (g_mkdir(cache_dir.c_str(), 0700)==-1)
				return false;
		} else if (!g_file_test(cache_dir.c_str(), G_FILE_TEST_IS_DIR))
			return false;
	}

	gchar *base=g_path_get_basename(url.c_str());
	if (cachefiletype == CacheFileType_oft) {
		cachefilename = cache_dir+G_DIR_SEPARATOR_S+base+".oft";
	} else if (cachefiletype == CacheFileType_clt) {
		cachefilename = cache_dir+G_DIR_SEPARATOR_S+base+".clt";
	} else {
		gchar *func = g_strdup_printf("%d", cltfunc);
		cachefilename = cache_dir+G_DIR_SEPARATOR_S+base+'.'+func+".clt";
		g_free(func);
	}
	g_free(base);
	return true;
}

FILE* cache_file::get_cache_savefile(const gchar *filename, const std::string &url, int next, std::string &cfilename, CollateFunctions cltfunc)
{
	cfilename = filename;
	struct stat oftstat;
	if (g_stat(filename, &oftstat)!=0) {
		return fopen(filename, "wb");
	}
	MapFile mf;
	if (!mf.open(filename, oftstat.st_size)) {
		return fopen(filename, "wb");
	}
	gchar *p = mf.begin();
	bool has_prefix;
	if (cachefiletype == CacheFileType_oft)
		has_prefix = g_str_has_prefix(p, OFFSETFILE_MAGIC_DATA);
	else
		has_prefix = g_str_has_prefix(p, COLLATIONFILE_MAGIC_DATA);
	if (!has_prefix) {
		mf.close();
		return fopen(filename, "wb");
	}
	if (cachefiletype == CacheFileType_oft)
		p+= sizeof(OFFSETFILE_MAGIC_DATA)-1-1;
	else
		p+= sizeof(COLLATIONFILE_MAGIC_DATA)-1-1;
	gchar *p2;
	p2 = strstr(p, "\nurl=");
	if (!p2) {
		mf.close();
		return fopen(filename, "wb");
	}
	p2+=sizeof("\nurl=")-1;
	gchar *p3;
	p3 = strchr(p2, '\n');
	if (!p3) {
		mf.close();
		return fopen(filename, "wb");
	}
	gchar *tmpstr;
	tmpstr = (gchar *)g_memdup(p2, p3-p2+1);
	tmpstr[p3-p2] = '\0';
	if (url == tmpstr) {
		g_free(tmpstr);
		mf.close();
		return fopen(filename, "wb");
	}
	g_free(tmpstr);
	mf.close();
	gchar *basename = g_path_get_basename(url.c_str());
	p = strrchr(basename, '.');
	if (!p) {
		g_free(basename);
		return NULL;
	}
	*p='\0';
	gchar *extendname = p+1;
	gchar *dirname = g_path_get_dirname(filename);
	gchar *nextfilename;
	if (cachefiletype == CacheFileType_oft)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.oft", dirname, basename, next, extendname);
	else if (cachefiletype == CacheFileType_clt)
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.clt", dirname, basename, next, extendname);
	else
		nextfilename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s(%d).%s.%d.clt", dirname, basename, next, extendname, cltfunc);
	FILE *out = get_cache_savefile(nextfilename, url, next+1, cfilename, cltfunc);
	g_free(basename);
	g_free(dirname);
	g_free(nextfilename);
	return out;
}

bool cache_file::save_cache(const std::string& url, CollateFunctions cltfunc, gulong npages)
{
	std::string oftfilename;
	if (cachefiletype == CacheFileType_oft) {
		oftfilename=url+".oft";
	} else if (cachefiletype == CacheFileType_clt) {
		oftfilename=url+".clt";
	} else {
		gchar *func = g_strdup_printf("%d", cltfunc);
		oftfilename=url+'.'+func+".clt";
		g_free(func);
	}
	for (int i=0;i<2;i++) {
		if (i==1) {
			if (!get_cache_filename(url, oftfilename, true, cltfunc))
				break;
		}
		std::string cfilename;
		FILE *out= get_cache_savefile(oftfilename.c_str(), url, 2, cfilename, cltfunc);
		if (!out)
			continue;
		if (cachefiletype == CacheFileType_oft)
			fwrite(OFFSETFILE_MAGIC_DATA, 1, sizeof(OFFSETFILE_MAGIC_DATA)-1, out);
		else
			fwrite(COLLATIONFILE_MAGIC_DATA, 1, sizeof(COLLATIONFILE_MAGIC_DATA)-1, out);
		fwrite("url=", 1, sizeof("url=")-1, out);
		fwrite(url.c_str(), 1, url.length(), out);
		if (cachefiletype == CacheFileType_clt) {
#ifdef _MSC_VER
			fprintf_s(out, "\nfunc=%d", cltfunc);
#else
			fprintf(out, "\nfunc=%d", cltfunc);
#endif
		}
		fwrite("\n", 1, 2, out);
		fwrite(wordoffset, sizeof(guint32), npages, out);
		fclose(out);
		g_print("Save cache file: %s\n", cfilename.c_str());
		return true;
	}
	return false;
}

collation_file::collation_file(idxsyn_file *_idx_file, CacheFileType _cachefiletype) : cache_file(_cachefiletype)
{
	idx_file = _idx_file;
}

const gchar *collation_file::GetWord(glong idx)
{
	return idx_file->get_key(wordoffset[idx]);
}

glong collation_file::GetOrigIndex(glong cltidx)
{
	return wordoffset[cltidx];
}

bool collation_file::lookup(const char *sWord, glong &idx, glong &idx_suggest)
{
	bool bFound=false;
	glong iTo=idx_file->wordcount-1;
	if (stardict_collate(sWord, GetWord(0), CollateFunction)<0) {
		idx = 0;
		idx_suggest = 0;
	} else if (stardict_collate(sWord, GetWord(iTo), CollateFunction) >0) {
		idx = INVALID_INDEX;
		idx_suggest = iTo;
	} else {
		glong iThisIndex=0;
		glong iFrom=0;
		gint cmpint;
		while (iFrom<=iTo) {
			iThisIndex=(iFrom+iTo)/2;
			cmpint = stardict_collate(sWord, GetWord(iThisIndex), CollateFunction);
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
{
	memset(clt_files, 0, sizeof(clt_files));
}

const gchar *idxsyn_file::getWord(glong idx, int EnableCollationLevel, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return get_key(idx);
	if (EnableCollationLevel == 1)
		return clt_file->GetWord(idx);
	if (servercollatefunc == 0)
		return get_key(idx);
	collate_load((CollateFunctions)(servercollatefunc-1));
	return clt_files[servercollatefunc-1]->GetWord(idx);
}

bool idxsyn_file::Lookup(const char *str, glong &idx, glong &idx_suggest, int EnableCollationLevel, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return lookup(str, idx, idx_suggest);
	if (EnableCollationLevel == 1)
		return clt_file->lookup(str, idx, idx_suggest);
	if (servercollatefunc == 0)
		return lookup(str, idx, idx_suggest);
	collate_load((CollateFunctions)(servercollatefunc-1));
	return clt_files[servercollatefunc-1]->lookup(str, idx, idx_suggest);
}

void idxsyn_file::collate_sort(const std::string& url,
			       const std::string& saveurl,
			       CollateFunctions collf,
			       show_progress_t *sp)
{
	clt_file = new collation_file(this, CacheFileType_clt);
	clt_file->CollateFunction = collf;
	if (!clt_file->load_cache(url, saveurl, collf, wordcount*sizeof(guint32))) {
		sp->notify_about_start(_("Sorting, please wait..."));
		clt_file->wordoffset = (guint32 *)g_malloc(wordcount*sizeof(guint32));
		for (glong i=0; i<wordcount; i++)
			clt_file->wordoffset[i] = i;
		sort_collation_index_user_data data;
		data.idx_file = this;
		data.cltfunc = collf;
		g_qsort_with_data(clt_file->wordoffset, wordcount, sizeof(guint32), sort_collation_index, &data);
		if (!clt_file->save_cache(saveurl, collf, wordcount))
			g_printerr("Cache update failed.\n");
	}
}

void idxsyn_file::collate_save_info(const std::string& _url, const std::string& _saveurl)
{
	url = _url;
	saveurl = _saveurl;
}

void idxsyn_file::collate_load(CollateFunctions collf)
{
	if (clt_files[collf])
		return;
	clt_files[collf] = new collation_file(this, CacheFileType_server_clt);
	clt_files[collf]->CollateFunction = collf;
	if (!clt_files[collf]->load_cache(url, saveurl, collf, wordcount*sizeof(guint32))) {
		clt_files[collf]->wordoffset = (guint32 *)g_malloc(wordcount*sizeof(guint32));
		for (glong i=0; i<wordcount; i++)
			clt_files[collf]->wordoffset[i] = i;
		sort_collation_index_user_data data;
		data.idx_file = this;
		data.cltfunc = collf;
		g_qsort_with_data(clt_files[collf]->wordoffset, wordcount, sizeof(guint32), sort_collation_index, &data);
		if (!clt_files[collf]->save_cache(saveurl, collf, wordcount))
			g_printerr("Cache update failed.\n");
	}
}

bool offset_index::load(const std::string& url, gulong wc, gulong fsize,
			bool CreateCacheFile, int EnableCollationLevel,
			CollateFunctions _CollateFunction, show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, _CollateFunction, npages*sizeof(guint32))) {
		MapFile map_file;
		if (!map_file.open(url.c_str(), fsize))
			return false;
		const gchar *idxdatabuffer=map_file.begin();
		oft_file.wordoffset = (guint32 *)g_malloc(npages*sizeof(guint32));
		const gchar *p1 = idxdatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<wc; i++) {
			index_size=strlen(p1) +1 + 2*sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.wordoffset[j]=p1-idxdatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.wordoffset[j]=p1-idxdatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url, _CollateFunction, npages))
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

	if (EnableCollationLevel == 0) {
	} else if (EnableCollationLevel == 1) {
		collate_sort(url, url, _CollateFunction, sp);
	} else if (EnableCollationLevel == 2) {
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
		page_data.resize(oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx]);
		fseek(idxfile, oft_file.wordoffset[page_idx], SEEK_SET);
		fread(&page_data[0], 1, page_data.size(), idxfile);
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
		idx_suggest = iTo;
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
		if (!bFound) {
			idx = iTo;    //prev
		} else {
			idx = iThisIndex;
		}
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

wordlist_index::wordlist_index()
{
	clt_file = NULL;
	idxdatabuf = NULL;
}

wordlist_index::~wordlist_index()
{
	delete clt_file;
	g_free(idxdatabuf);
}

bool wordlist_index::load(const std::string& url, gulong wc, gulong fsize,
			  bool CreateCacheFile, int EnableCollationLevel,
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
	wordlist[wc] = p1;

	if (EnableCollationLevel == 0) {
	} else {
		std::string saveurl = url;
		saveurl.erase(saveurl.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		if (EnableCollationLevel == 1) {
			collate_sort(url, saveurl, _CollateFunction, sp);
		} else if (EnableCollationLevel == 2) {
			collate_save_info(url, saveurl);
		}
	}
	return true;
}

const gchar *wordlist_index::get_key(glong idx)
{
	return wordlist[idx];
}

void wordlist_index::get_data(glong idx)
{
	gchar *p1 = wordlist[idx]+strlen(wordlist[idx])+sizeof(gchar);
	wordentry_offset = g_ntohl(get_uint32(p1));
	p1 += sizeof(guint32);
	wordentry_size = g_ntohl(get_uint32(p1));
}

const gchar *wordlist_index::get_key_and_data(glong idx)
{
	get_data(idx);
	return get_key(idx);
}

bool wordlist_index::lookup(const char *str, glong &idx, glong &idx_suggest)
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

synonym_file::synonym_file() : oft_file(CacheFileType_oft)
{
	clt_file = NULL;
}

synonym_file::~synonym_file()
{
	delete clt_file;
	if (synfile)
		fclose(synfile);
}

inline const gchar *synonym_file::read_first_on_page_key(glong page_idx)
{
	fseek(synfile, oft_file.wordoffset[page_idx], SEEK_SET);
	guint32 page_size=oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx];
	gulong minsize = sizeof(wordentry_buf);
        if (page_size < minsize)
                minsize = page_size;
	fread(wordentry_buf, minsize, 1, synfile); //TODO: check returned values, deal with word entry that strlen>255.
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
			int EnableCollationLevel, CollateFunctions _CollateFunction,
			show_progress_t *sp)
{
	wordcount=wc;
	npages=(wc-1)/ENTR_PER_PAGE+2;
	if (!oft_file.load_cache(url, url, _CollateFunction, npages*sizeof(guint32))) {
		struct stat stats;
		if (stat (url.c_str(), &stats) == -1)
			return false;
		MapFile map_file;
		if (!map_file.open(url.c_str(), stats.st_size))
			return false;
		const gchar *syndatabuffer=map_file.begin();
		oft_file.wordoffset = (guint32 *)g_malloc(npages*sizeof(guint32));
		const gchar *p1 = syndatabuffer;
		gulong index_size;
		guint32 j=0;
		for (guint32 i=0; i<wc; i++) {
			index_size=strlen(p1) +1 + sizeof(guint32);
			if (i % ENTR_PER_PAGE==0) {
				oft_file.wordoffset[j]=p1-syndatabuffer;
				++j;
			}
			p1 += index_size;
		}
		oft_file.wordoffset[j]=p1-syndatabuffer;
		map_file.close();
		if (CreateCacheFile) {
			if (!oft_file.save_cache(url, _CollateFunction, npages))
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

	if (EnableCollationLevel == 0) {
	} else if (EnableCollationLevel == 1)
		collate_sort(url, url, _CollateFunction, sp);
	else if (EnableCollationLevel == 2) {
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
		page_data.resize(oft_file.wordoffset[page_idx+1]-oft_file.wordoffset[page_idx]);
		fseek(synfile, oft_file.wordoffset[page_idx], SEEK_SET);
		fread(&page_data[0], 1, page_data.size(), synfile);
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
		idx_suggest = iTo;
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
		int EnableCollationLevel, CollateFunctions CollateFunction,
		show_progress_t *sp)
{
	gulong idxfilesize;
	glong wordcount, synwordcount;
	if (!load_ifofile(ifofilename, idxfilesize, wordcount, synwordcount))
		return false;
	sp->notify_about_start(_("Loading..."));
	std::string fullfilename(ifofilename);
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "dict.dz");

	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		dictdzfile.reset(new dictData);
		if (!dictdzfile->open(fullfilename, 0)) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".dz")+1, sizeof(".dz")-1);
		dictfile = fopen(fullfilename.c_str(),"rb");
		if (!dictfile) {
			//g_print("open file %s failed!\n",fullfilename);
			return false;
		}
	}

	fullfilename=ifofilename;
	fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "idx.gz");

	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
		idx_file.reset(new wordlist_index);
	} else {
		fullfilename.erase(fullfilename.length()-sizeof(".gz")+1, sizeof(".gz")-1);
		idx_file.reset(new offset_index);
	}

	if (!idx_file->load(fullfilename, wordcount, idxfilesize,
			    CreateCacheFile, EnableCollationLevel,
			    CollateFunction, sp))
		return false;

	if (synwordcount) {
		fullfilename=ifofilename;
		fullfilename.replace(fullfilename.length()-sizeof("ifo")+1, sizeof("ifo")-1, "syn");
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
			syn_file.reset(new synonym_file);
			if (!syn_file->load(fullfilename, synwordcount,
					    CreateCacheFile, EnableCollationLevel,
					    CollateFunction, sp))
				return false;
		}
	}

	bool has_res = false;
	gchar *dirname = g_path_get_dirname(ifofilename.c_str());
	fullfilename = dirname;
	fullfilename += G_DIR_SEPARATOR_S "res";
	if (g_file_test(fullfilename.c_str(), G_FILE_TEST_IS_DIR)) {
		has_res = true;
	} else {
		fullfilename = dirname;
		fullfilename += G_DIR_SEPARATOR_S "res.rifo";
		if (g_file_test(fullfilename.c_str(), G_FILE_TEST_EXISTS)) {
			has_res = true;
		}
	}
	if (has_res) {
		storage = new ResourceStorage();
		bool failed = storage->load(dirname);
		if (failed) {
			delete storage;
			storage = NULL;
		}
	}
	g_free(dirname);

	g_print("bookname: %s , wordcount %lu\n", bookname.c_str(), wordcount);
	return true;
}

bool Dict::load_ifofile(const std::string& ifofilename, gulong &idxfilesize, glong &wordcount, glong &synwordcount)
{
	DictInfo dict_info;
	if (!dict_info.load_from_ifo_file(ifofilename, false))
		return false;
	if (dict_info.wordcount==0)
		return false;

	ifo_file_name=dict_info.ifo_file_name;
	bookname=dict_info.bookname;

	idxfilesize=dict_info.index_file_size;
	wordcount=dict_info.wordcount;
	synwordcount=dict_info.synwordcount;

	sametypesequence=dict_info.sametypesequence;
	dicttype=dict_info.dicttype;

	return true;
}

glong Dict::nsynarticles()
{
	if (syn_file.get() == NULL)
		return 0;
	return syn_file->wordcount;
}

bool Dict::GetWordPrev(glong idx, glong &pidx, bool isidx, int EnableCollationLevel, int servercollatefunc)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	if (idx==INVALID_INDEX) {
		pidx = is_file->wordcount-1;
		return true;
	}
	pidx = idx;
	gchar *cWord = g_strdup(is_file->getWord(pidx, EnableCollationLevel, servercollatefunc));
	const gchar *pWord;
	bool found=false;
	while (pidx>0) {
		pWord = is_file->getWord(pidx-1, EnableCollationLevel, servercollatefunc);
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

void Dict::GetWordNext(glong &idx, bool isidx, int EnableCollationLevel, int servercollatefunc)
{
	idxsyn_file *is_file;
	if (isidx)
		is_file = idx_file.get();
	else
		is_file = syn_file.get();
	gchar *cWord = g_strdup(is_file->getWord(idx, EnableCollationLevel, servercollatefunc));
	const gchar *pWord;
	bool found=false;
	while (idx < is_file->wordcount-1) {
		pWord = is_file->getWord(idx+1, EnableCollationLevel, servercollatefunc);
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
	while (idx2<is_file->wordcount-1) {
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

bool Dict::LookupSynonym(const char *str, glong &synidx, glong &synidx_suggest, int EnableCollationLevel, int servercollatefunc)
{
	if (syn_file.get() == NULL) {
		synidx = UNSET_INDEX;
		synidx_suggest = UNSET_INDEX;
		return false;
	}
	return syn_file->Lookup(str, synidx, synidx_suggest, EnableCollationLevel, servercollatefunc);
}

bool Dict::LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen)
{
	int iIndexCount=0;
	for (glong i=0; i<narticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_pattern_match_string(pspec, idx_file->getWord(i, 0, 0)))
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
		if (g_pattern_match_string(pspec, syn_file->getWord(i, 0, 0)))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

bool Dict::LookupWithRegex(GRegex *regex, glong *aIndex, int iBuffLen)
{
	int iIndexCount=0;
	for (glong i=0; i<narticles() && iIndexCount<iBuffLen-1; i++)
		// Need to deal with same word in index? But this will slow down processing in most case.
		if (g_regex_match(regex, idx_file->getWord(i, 0, 0), (GRegexMatchFlags)0, NULL))
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
		if (g_regex_match(regex, syn_file->getWord(i, 0, 0), (GRegexMatchFlags)0, NULL))
			aIndex[iIndexCount++]=i;
	aIndex[iIndexCount]= -1; // -1 is the end.
	return (iIndexCount>0);
}

//===================================================================
show_progress_t Libs::default_show_progress;

Libs::Libs(show_progress_t *sp, bool create, int enablelevel, int function)
{
#ifdef SD_SERVER_CODE
	root_info_item = NULL;
#endif
	set_show_progress(sp);
	CreateCacheFile = create;
	EnableCollationLevel = enablelevel;
	CollateFunction = (CollateFunctions)function;
	iMaxFuzzyDistance  = MAX_FUZZY_DISTANCE; //need to read from cfg.
	if (EnableCollationLevel == 0) {
	} else if (EnableCollationLevel == 1) {
		if (utf8_collate_init(CollateFunction))
			printf("Init collate function failed!\n");
	} else if (EnableCollationLevel == 2){
		if (utf8_collate_init_all())
			printf("Init collate functions failed!\n");
	}
}

Libs::~Libs()
{
#ifdef SD_SERVER_CODE
	if (root_info_item)
		delete root_info_item;
#endif
	for (std::vector<Dict *>::iterator p=oLib.begin(); p!=oLib.end(); ++p)
		delete *p;
	utf8_collate_end();
}

bool Libs::load_dict(const std::string& url, show_progress_t *sp)
{
	Dict *lib=new Dict;
	if (lib->load(url, CreateCacheFile, EnableCollationLevel,
		      CollateFunction, sp)) {
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
	filename = dir;
	filename += G_DIR_SEPARATOR_S "stardictd.xml";
	struct stat filestat;
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
			if (!dict_info.load_from_ifo_file(oLib[dict->id]->ifofilename(), false))
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
			oLib[(*i).index]->idx_file->collate_load(cltfuc);
			if (oLib[(*i).index]->syn_file.get() != NULL)
				oLib[(*i).index]->syn_file->collate_load(cltfuc);
		}
	}
}
#endif

#ifdef SD_CLIENT_CODE
bool Libs::find_lib_by_filename(const char *filename, size_t &iLib)
{
	for (std::vector<Dict *>::size_type i =0; i < oLib.size(); i++) {
		if (oLib[i]->ifofilename() == filename) {
			iLib = i;
			return true;
		}
	}
	return false;
}

void Libs::load(std::list<std::string> &load_list)
{
	for (std::list<std::string>::iterator i = load_list.begin(); i != load_list.end(); ++i) {
		load_dict(*i, show_progress);
	}
}

void Libs::reload(std::list<std::string> &load_list, int is_coll_enb, int collf)
{
	if (is_coll_enb == EnableCollationLevel && collf == CollateFunction) {
		std::vector<Dict *> prev(oLib);
		oLib.clear();
		for (std::list<std::string>::iterator i = load_list.begin(); i != load_list.end(); ++i) {
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
		EnableCollationLevel = is_coll_enb;
		CollateFunction = CollateFunctions(collf);
		if (EnableCollationLevel == 0) {
		} else if (EnableCollationLevel == 1) {
			if (utf8_collate_init(CollateFunction))
				printf("Init collate function failed!\n");
		} else if (EnableCollationLevel == 2) {
			if (utf8_collate_init_all())
				printf("Init collate functions failed!\n");
		}
		load(load_list);
	}
}
#endif

glong Libs::CltIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return cltidx;
	if (EnableCollationLevel == 1) {
		if (cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->idx_file->clt_file->GetOrigIndex(cltidx);
	}
	if (servercollatefunc == 0)
		return cltidx;
	if (cltidx == INVALID_INDEX)
		return cltidx;
	oLib[iLib]->idx_file->collate_load((CollateFunctions)(servercollatefunc-1));
	return oLib[iLib]->idx_file->clt_files[servercollatefunc-1]->GetOrigIndex(cltidx);
}

glong Libs::CltSynIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc)
{
	if (EnableCollationLevel == 0)
		return cltidx;
	if (EnableCollationLevel == 1) {
		if (cltidx == UNSET_INDEX || cltidx == INVALID_INDEX)
			return cltidx;
		return oLib[iLib]->syn_file->clt_file->GetOrigIndex(cltidx);
	}
	if (servercollatefunc == 0)
		return cltidx;
	if (cltidx == UNSET_INDEX || cltidx == INVALID_INDEX)
		return cltidx;
	oLib[iLib]->syn_file->collate_load((CollateFunctions)(servercollatefunc-1));
	return oLib[iLib]->syn_file->clt_files[servercollatefunc-1]->GetOrigIndex(cltidx);
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
				gint x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
				if (x > 0) {
					poCurrentWord = word;
				}
			}
		}
	}
	for (iLib=0; iLib<dictmask.size(); iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		if (iCurrent[iLib].synidx_suggest==UNSET_INDEX)
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
				gint x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
			gint x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
			gint x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
			oLib[iRealLib]->Lookup(sWord, iCurrent[iLib].idx, iCurrent[iLib].idx_suggest, EnableCollationLevel, servercollatefunc);
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
			x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
			oLib[iRealLib]->LookupSynonym(sWord, iCurrent[iLib].synidx, iCurrent[iLib].synidx_suggest, EnableCollationLevel, servercollatefunc);
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
			x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
	for (iLib=0;iLib<dictmask.size();iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->Lookup(sWord, iCurrent[iLib].idx, iCurrent[iLib].idx_suggest, EnableCollationLevel, servercollatefunc);
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
				x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
				if (x < 0 ) {
					poCurrentWord = word;
					iCurrentLib = iLib;
					iCurrentRealLib = iRealLib;
					isLib=true;
				}
			}
		}
	}
	for (iLib=0;iLib<dictmask.size();iLib++) {
		if (dictmask[iLib].type != InstantDictType_LOCAL)
			continue;
		iRealLib = dictmask[iLib].index;
		if (sWord) {
			oLib[iRealLib]->LookupSynonym(sWord, iCurrent[iLib].synidx, iCurrent[iLib].synidx_suggest, EnableCollationLevel, servercollatefunc);
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
				x = stardict_server_collate(poCurrentWord, word, EnableCollationLevel, CollateFunction, servercollatefunc);
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
			bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, EnableCollationLevel, servercollatefunc);
			if(bLookup)
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, EnableCollationLevel, servercollatefunc);
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
				bLookup = oLib[iLib]->LookupSynonym(casestr, iIndex, iIndex_suggest, EnableCollationLevel, servercollatefunc);
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
					if (stardict_casecmp(cword, sWord, EnableCollationLevel, CollateFunction, servercollatefunc)==0) {
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
					if (stardict_casecmp(cword, sWord, EnableCollationLevel, CollateFunction, servercollatefunc)==0) {
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

bool Libs::LookupSimilarWord(const gchar* sWord, glong & iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc)
{
	glong iIndex;
	bool bFound=false;
	gchar *casestr;

	if (!bFound) {
		// to lower case.
		casestr = g_utf8_strdown(sWord, -1);
		if (strcmp(casestr, sWord)) {
			if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
				bFound=true;
		}
		g_free(casestr);
		// to upper case.
		if (!bFound) {
			casestr = g_utf8_strup(sWord, -1);
			if (strcmp(casestr, sWord)) {
				if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
			}
			g_free(casestr);
		}
		if (!bFound) {
			iIndex = iWordIndex;
			glong pidx;
			const gchar *cword;
			do {
				if (GetWordPrev(iIndex, pidx, iLib, true, servercollatefunc)) {
					cword = poGetWord(pidx, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, EnableCollationLevel, CollateFunction, servercollatefunc)==0) {
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
					cword = poGetWord(iIndex, iLib, servercollatefunc);
					if (stardict_casecmp(cword, sWord, EnableCollationLevel, CollateFunction, servercollatefunc)==0) {
						bFound=true;
					}
				}
			}
		}
	}

	if (bIsPureEnglish(sWord)) {
		// If not Found , try other status of sWord.
		size_t iWordLen=strlen(sWord);
		bool isupcase;

		gchar *sNewWord = (gchar *)g_malloc(iWordLen + 1);

		//cut one char "s" or "d"
		if(!bFound && iWordLen>1) {
			isupcase = sWord[iWordLen-1]=='S' || !strncmp(&sWord[iWordLen-2],"ED",2);
			if (isupcase || sWord[iWordLen-1]=='s' || !strncmp(&sWord[iWordLen-2],"ed",2)) {
				strcpy(sNewWord,sWord);
				sNewWord[iWordLen-1]='\0'; // cut "s" or "d"
				if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
					if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
					if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-4]=sNewWord[iWordLen-5];  //restore
					}
				}
				if( !bFound ) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
					if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
					if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else {
						if (isupcase || g_ascii_isupper(sWord[0])) {
							casestr = g_ascii_strdown(sNewWord, -1);
							if (strcmp(casestr, sNewWord)) {
								if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
									bFound=true;
							}
							g_free(casestr);
						}
						if (!bFound)
							sNewWord[iWordLen-3]=sNewWord[iWordLen-4];  //restore
					}
				}
				if (!bFound) {
					if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
						bFound=true;
					else if (isupcase || g_ascii_isupper(sWord[0])) {
						casestr = g_ascii_strdown(sNewWord, -1);
						if (strcmp(casestr, sNewWord)) {
							if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if (oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
				if(oLib[iLib]->Lookup(sNewWord, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
					bFound=true;
				else if (isupcase || g_ascii_isupper(sWord[0])) {
					casestr = g_ascii_strdown(sNewWord, -1);
					if (strcmp(casestr, sNewWord)) {
						if(oLib[iLib]->Lookup(casestr, iIndex, idx_suggest, EnableCollationLevel, servercollatefunc))
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
	bool bFound = oLib[iLib]->Lookup(sWord, iWordIndex, idx_suggest, EnableCollationLevel, servercollatefunc);
	if (!bFound)
		bFound = LookupSimilarWord(sWord, iWordIndex, idx_suggest, iLib, servercollatefunc);
	return bFound;
}

bool Libs::SimpleLookupSynonymWord(const gchar* sWord, glong & iWordIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc)
{
	bool bFound = oLib[iLib]->LookupSynonym(sWord, iWordIndex, synidx_suggest, EnableCollationLevel, servercollatefunc);
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
	KMP_end();

	std::vector<InstantDictIndex>::size_type i;
	for (i=0; i<dictmask.size(); ++i)
		if (!reslist[i].empty())
			break;

	return i!=dictmask.size();
}

int Libs::GetStorageType(size_t iLib)
{
	if (oLib[iLib]->storage == NULL)
		return -1;
	return oLib[iLib]->storage->is_file_or_db;
}

const char *Libs::GetStorageFilePath(size_t iLib, const char *key)
{
	if (oLib[iLib]->storage == NULL)
		return NULL;
	return oLib[iLib]->storage->get_file_path(key);
}

const char *Libs::GetStorageFileContent(size_t iLib, const char *key)
{
	if (oLib[iLib]->storage == NULL)
		return NULL;
	return oLib[iLib]->storage->get_file_content(key);
}
