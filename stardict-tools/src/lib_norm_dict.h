#ifndef STARDICT_NORM_DICT_H_
#define STARDICT_NORM_DICT_H_

#include <string>
#include "libcommon.h"
#include "ifo_file.hpp"

struct worditem_t {
	std::string word;
	guint32 offset;
	guint32 size;
};

struct synitem_t {
	std::string word;
	guint32 index;
};

class norm_dict
{
public:
	int load(const std::string& ifofilename, print_info_t print_info,
			i_resource_storage* p_res_storage = NULL);
private:
	int prepare_idx_file(void);
	int prepare_dict_file(void);
	int load_ifo_file(void);
	int load_idx_file(void);
	int load_syn_file(void);
	int load_dict_file(void);
	void verify_data_blocks_overlapping(void);

	std::string basefilename;
	std::string ifofilename;
	std::string idxfilename; // may be archive
	std::string idxfilename_orig; // file to read, uncompressed
	std::string dictfilename;
	std::string dictfilename_orig;
	std::string synfilename;
	/* This dictionary has a syn file. */
	bool have_synfile;
	DictInfo dict_info;
	TempFile idxtemp;
	TempFile dicttemp;
	guint32 dictfilesize;
	std::vector<worditem_t> index;
	std::vector<synitem_t> synindex;
	print_info_t print_info;
	i_resource_storage* p_res_storage;
};


#endif /* STARDICT_NORM_DICT_H_ */
