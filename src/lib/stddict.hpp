#ifndef _STDDICT_HPP_
#define _STDDICT_HPP_

#include "stardict_libconfig.h"

#include <glib.h>
#include <string>
#include <vector>
#include <list>
#include <map>

#include "data.hpp"
#include "collation.h"
#include "dictmask.h"
#include "storage.h"

const int MAX_FUZZY_DISTANCE= 3; // at most MAX_FUZZY_DISTANCE-1 differences allowed when find similar words
const int MAX_MATCH_ITEM_PER_LIB=100;

extern gint stardict_casecmp(const gchar *s1, const gchar *s2, bool isClt, CollateFunctions func);
extern bool bIsPureEnglish(const gchar *str);
extern gint stardict_server_collate(const gchar *str1, const gchar *str2, int EnableCollationLevel, CollateFunctions func, int servercollatefunc);

class show_progress_t {
public:
	virtual ~show_progress_t() {}
	virtual void notify_about_start(const std::string& title) {}
	virtual void notify_about_work() {}
};

enum CacheFileType {
	CacheFileType_oft,
	CacheFileType_clt,
	CacheFileType_server_clt,
};

class cache_file {
public:
	guint32 *wordoffset;

	cache_file(CacheFileType _cachefiletype);
	~cache_file();
	bool load_cache(const std::string& url, const std::string& saveurl, CollateFunctions cltfunc, glong filedatasize);
	bool save_cache(const std::string& url, CollateFunctions cltfunc, gulong npages);
private:
	CacheFileType cachefiletype;
	MapFile *mf;
	bool get_cache_filename(const std::string& url, std::string &cachefilename, bool create, CollateFunctions cltfunc);
	MapFile* get_cache_loadfile(const gchar *filename, const std::string &url, const std::string &saveurl, CollateFunctions cltfunc, glong filedatasize, int next);
	FILE* get_cache_savefile(const gchar *filename, const std::string &url, int next, std::string &cfilename, CollateFunctions cltfunc);
};

class idxsyn_file;
class collation_file : public cache_file {
public:
	CollateFunctions CollateFunction;

	collation_file(idxsyn_file *_idx_file, CacheFileType _cachefiletype);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);
	const gchar *GetWord(glong idx);
	glong GetOrigIndex(glong cltidx);
private:
	idxsyn_file *idx_file;
};

class idxsyn_file {
public:
	glong wordcount;
	collation_file *clt_file;
	collation_file *clt_files[COLLATE_FUNC_NUMS];
	std::string url;
	std::string saveurl;

	idxsyn_file();
	const gchar *getWord(glong idx, int EnableCollationLevel, int servercollatefunc);
	bool Lookup(const char *str, glong &idx, glong &idx_suggest, int EnableCollationLevel, int servercollatefunc);
	virtual const gchar *get_key(glong idx) = 0;
	virtual bool lookup(const char *str, glong &idx, glong &idx_suggest) = 0;
	virtual ~idxsyn_file() {}
	void collate_sort(const std::string& url, const std::string& saveurl,
			  CollateFunctions collf, show_progress_t *sp);
	void collate_save_info(const std::string& _url, const std::string& _saveurl);
	void collate_load(CollateFunctions collf);
};

class index_file : public idxsyn_file {
public:
	guint32 wordentry_offset;
	guint32 wordentry_size;

	virtual bool load(const std::string& url, gulong wc, gulong fsize,
			  bool CreateCacheFile, int EnableCollationLevel,
			  CollateFunctions _CollateFunction, show_progress_t *sp) = 0;
	virtual void get_data(glong idx) = 0;
	virtual  const gchar *get_key_and_data(glong idx) = 0;
private:
	virtual bool lookup(const char *str, glong &idx, glong &idx_suggest) = 0;
};

class synonym_file : public idxsyn_file {
public:
	guint32 wordentry_index;

	synonym_file();
	~synonym_file();
	bool load(const std::string& url, gulong wc, bool CreateCacheFile,
		  int EnableCollationLevel, CollateFunctions _CollateFunction,
		  show_progress_t *sp);
private:
	const gchar *get_key(glong idx);
	bool lookup(const char *str, glong &idx, glong &idx_suggest);

	static const gint ENTR_PER_PAGE=32;
	gulong npages;

	cache_file oft_file;
	FILE *synfile;

	gchar wordentry_buf[256+sizeof(guint32)];
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
		guint32 index;
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

class Dict : public DictBase {
private:
	std::string ifo_file_name;
	std::string bookname;
	std::string dicttype;

	bool load_ifofile(const std::string& ifofilename, gulong &idxfilesize, glong &wordcount, glong &synwordcount);
public:
	std::auto_ptr<index_file> idx_file;
	std::auto_ptr<synonym_file> syn_file;
	ResourceStorage *storage;

	Dict();
	~Dict();
	bool load(const std::string&, bool CreateCacheFile, int EnableCollationLevel, CollateFunctions,
		  show_progress_t *);

	glong narticles() { return idx_file->wordcount; }
	glong nsynarticles();
	const std::string& dict_name() { return bookname; }
	const std::string& dict_type() { return dicttype; }
	const std::string& ifofilename() { return ifo_file_name; }

	gchar *get_data(glong index)
	{
		idx_file->get_data(index);
		return DictBase::GetWordData(idx_file->wordentry_offset, idx_file->wordentry_size);
	}
	void get_key_and_data(glong index, const gchar **key, guint32 *offset, guint32 *size)
	{
		*key = idx_file->get_key_and_data(index);
		*offset = idx_file->wordentry_offset;
		*size = idx_file->wordentry_size;
	}
	bool Lookup(const char *str, glong &idx, glong &idx_suggest, int EnableCollationLevel, int servercollatefunc)
	{
		return idx_file->Lookup(str, idx, idx_suggest, EnableCollationLevel, servercollatefunc);
	}
	bool LookupSynonym(const char *str, glong &synidx, glong &synidx_suggest, int EnableCollationLevel, int servercollatefunc);
	bool LookupWithRule(GPatternSpec *pspec, glong *aIndex, int iBuffLen);
	bool LookupWithRuleSynonym(GPatternSpec *pspec, glong *aIndex, int iBuffLen);
	bool LookupWithRegex(GRegex *regex, glong *aIndex, int iBuffLen);
	bool LookupWithRegexSynonym(GRegex *regex, glong *aIndex, int iBuffLen);
	gint GetOrigWordCount(glong& iWordIndex, bool isidx);
	bool GetWordPrev(glong iWordIndex, glong &pidx, bool isidx, int EnableCollationLevel, int servercollatefunc);
	void GetWordNext(glong &iWordIndex, bool isidx, int EnableCollationLevel, int servercollatefunc);
};

struct CurrentIndex {
	glong idx;
	glong idx_suggest;
	glong synidx;
	glong synidx_suggest;
};

class Libs {
public:
	static show_progress_t default_show_progress;
	int EnableCollationLevel;
	CollateFunctions CollateFunction;

	Libs(show_progress_t *sp, bool create, int enablelevel, int function);
	~Libs();
	void set_show_progress(show_progress_t *sp) {
		if (sp)
			show_progress = sp;
		else
			show_progress = &default_show_progress;
	}
	bool load_dict(const std::string& url, show_progress_t *sp);
#ifdef SD_SERVER_CODE
	void LoadFromXML();
	void SetServerDictMask(std::vector<InstantDictIndex> &dictmask, const char *dicts, int max, int level);
	void LoadCollateFile(std::vector<InstantDictIndex> &dictmask, CollateFunctions cltfuc);
	const std::string *get_dir_info(const char *path);
	const std::string *get_dict_info(const char *uid, bool is_short);
	const std::string &get_fromto_info();
	std::string get_dicts_list(const char *dicts, int max_dict_count, int userLevel);
	int get_dict_level(const char *uid);
#endif
#ifdef SD_CLIENT_CODE
	bool find_lib_by_filename(const char *filename, size_t &iLib);
	void load(std::list<std::string> &load_list);
	void reload(std::list<std::string> &load_list, int is_coll_enb, int collf);
#endif

	glong narticles(size_t idict) { return oLib[idict]->narticles(); }
	glong nsynarticles(size_t idict) { return oLib[idict]->nsynarticles(); }
	const std::string& dict_name(size_t idict) { return oLib[idict]->dict_name(); }
	const std::string& dict_type(size_t idict) { return oLib[idict]->dict_type(); }
	bool has_dict() { return !oLib.empty(); }

	const gchar * poGetWord(glong iIndex,size_t iLib, int servercollatefunc) {
		return oLib[iLib]->idx_file->getWord(iIndex, EnableCollationLevel, servercollatefunc);
	}
	const gchar * poGetOrigWord(glong iIndex,size_t iLib) {
		return oLib[iLib]->idx_file->getWord(iIndex, 0, 0);
	}
	const gchar * poGetSynonymWord(glong iSynonymIndex,size_t iLib, int servercollatefunc) {
		return oLib[iLib]->syn_file->getWord(iSynonymIndex, EnableCollationLevel, servercollatefunc);
	}
	const gchar * poGetOrigSynonymWord(glong iSynonymIndex,size_t iLib) {
		return oLib[iLib]->syn_file->getWord(iSynonymIndex, 0, 0);
	}
	glong poGetOrigSynonymWordIdx(glong iSynonymIndex, size_t iLib) {
		oLib[iLib]->syn_file->getWord(iSynonymIndex, 0, 0);
		return oLib[iLib]->syn_file->wordentry_index;
	}
	glong CltIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc);
	glong CltSynIndexToOrig(glong cltidx, size_t iLib, int servercollatefunc);
	gchar * poGetOrigWordData(glong iIndex,size_t iLib) {
		if (iIndex==INVALID_INDEX)
			return NULL;
		return oLib[iLib]->get_data(iIndex);
	}
	const gchar *GetSuggestWord(const gchar *sWord, CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc);
	const gchar *poGetCurrentWord(CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc);
	const gchar *poGetNextWord(const gchar *word, CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc);
	const gchar *poGetPreWord(const gchar *word, CurrentIndex *iCurrent, std::vector<InstantDictIndex> &dictmask, int servercollatefunc);
	bool LookupWord(const gchar* sWord, glong& iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc) {
		return oLib[iLib]->Lookup(sWord, iWordIndex, idx_suggest, EnableCollationLevel, servercollatefunc);
	}
	bool LookupSynonymWord(const gchar* sWord, glong& iSynonymIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc) {
		return oLib[iLib]->LookupSynonym(sWord, iSynonymIndex, synidx_suggest, EnableCollationLevel, servercollatefunc);
	}
	bool LookupSimilarWord(const gchar* sWord, glong &iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc);
	bool LookupSynonymSimilarWord(const gchar* sWord, glong &iSynonymWordIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc);
	bool SimpleLookupWord(const gchar* sWord, glong &iWordIndex, glong &idx_suggest, size_t iLib, int servercollatefunc);
	bool SimpleLookupSynonymWord(const gchar* sWord, glong &iWordIndex, glong &synidx_suggest, size_t iLib, int servercollatefunc);
	gint GetOrigWordCount(glong& iWordIndex, size_t iLib, bool isidx) {
		return oLib[iLib]->GetOrigWordCount(iWordIndex, isidx);
	}
	bool GetWordPrev(glong iWordIndex, glong &pidx, size_t iLib, bool isidx, int servercollatefunc) {
		return oLib[iLib]->GetWordPrev(iWordIndex, pidx, isidx, EnableCollationLevel, servercollatefunc);
	}
	void GetWordNext(glong &iWordIndex, size_t iLib, bool isidx, int servercollatefunc) {
		oLib[iLib]->GetWordNext(iWordIndex, isidx, EnableCollationLevel, servercollatefunc);
	}

	bool LookupWithFuzzy(const gchar *sWord, gchar *reslist[], gint reslist_size, std::vector<InstantDictIndex> &dictmask);
	gint LookupWithRule(const gchar *sWord, gchar *reslist[], std::vector<InstantDictIndex> &dictmask);
	gint LookupWithRegex(const gchar *sWord, gchar *reslist[], std::vector<InstantDictIndex> &dictmask);

	typedef void (*updateSearchDialog_func)(gpointer data, gdouble fraction);
	bool LookupData(const gchar *sWord, std::vector<gchar *> *reslist, updateSearchDialog_func func, gpointer data, bool *cancel, std::vector<InstantDictIndex> &dictmask);
	int GetStorageType(size_t iLib);
	const char *GetStorageFilePath(size_t iLib, const char *key);
	const char *GetStorageFileContent(size_t iLib, const char *key);
private:
	std::vector<Dict *> oLib; // word Libs.
	int iMaxFuzzyDistance;
	show_progress_t *show_progress;
	bool CreateCacheFile;

#ifdef SD_SERVER_CODE
	struct DictInfoItem;
	struct DictInfoDirItem {
		~DictInfoDirItem() {
			for (std::list<DictInfoItem *>::iterator i = info_item_list.begin(); i!= info_item_list.end(); ++i) {
				delete (*i);
			}
		}
		std::string info_string;
		std::string name;
		std::string dirname;
		unsigned int dictcount;
		std::list<DictInfoItem *> info_item_list;
	};
	struct DictInfoDictItem {
		std::string info_string;
		std::string short_info_string;
		std::string uid;
		std::string download;
		std::string from;
		std::string to;
		unsigned int level;
		unsigned int id;
	};
	struct DictInfoItem {
		~DictInfoItem() {
			if (isdir == 1)
				delete dir;
			else if (isdir == 0)
				delete dict;
		}
		int isdir;
		union {
			DictInfoDirItem *dir;
			DictInfoDictItem *dict;
			std::string *linkuid;
		};
	};
	DictInfoItem *root_info_item;
	std::map<std::string, DictInfoDictItem *> uidmap;
	void LoadXMLDir(const char *dir, DictInfoItem *info_item);
	void GenLinkDict(DictInfoItem *info_item);

	struct ParseUserData {
		Libs *oLibs;
		const char *dir;
		DictInfoItem *info_item;
		bool indict;
		std::string path;
		std::string uid;
		std::string level;
		std::string download;
		std::string from;
		std::string to;
		bool inlinkdict;
		std::string linkuid;
	};
	static void func_parse_start_element(GMarkupParseContext *context, const gchar *element_name, const gchar **attribute_names, const gchar **attribute_values, gpointer user_data, GError **error);
	static void func_parse_end_element(GMarkupParseContext *context, const gchar *element_name, gpointer user_data, GError **error);
	static void func_parse_text(GMarkupParseContext *context, const gchar *text, gsize text_len, gpointer user_data, GError **error);

	struct FromToInfo {
		std::string uid;
		std::string bookname;
	};
	struct FromTo {
		std::string to;
		std::list<FromToInfo> fromto_info;
	};
	std::string cache_fromto;
	void gen_fromto_info(struct DictInfoItem *info_item, std::map<std::string, std::list<FromTo> > &map_fromto);
#endif

	friend class DictLoader;
	friend class DictReLoader;
};


#endif//!_STDDICT_HPP_
