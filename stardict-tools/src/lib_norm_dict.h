#ifndef STARDICT_NORM_DICT_H_
#define STARDICT_NORM_DICT_H_

#include <string>
#include <vector>
#include "libcommon.h"
#include "ifo_file.hpp"
#include "resourcewrap.hpp"
#include "lib_dict_data_block.h"
#include "lib_res_store.h"

struct worditem_t {
	std::string word;
	guint32 offset;
	guint32 size;
};

struct synitem_t {
	std::string word;
	guint32 index;
};

class i_resource_storage;

class norm_dict
{
public:
	typedef std::vector<worditem_t> worditem_vect_t;
	typedef std::vector<synitem_t> synitem_vect_t;

	norm_dict(void);
	int load(const std::string& ifofilename, print_info_t print_info,
			i_resource_storage* p_res_storage = NULL);
	void set_fix_errors(bool b)
	{
		fix_errors = b;
	}
	bool get_fix_errors(void) const
	{
		return fix_errors;
	}
	const worditem_vect_t& get_worditems(void) const
	{
		return index;
	}
	const synitem_vect_t& get_synitems(void) const
	{
		return synindex;
	}
	const DictInfo& get_dict_info(void) const
	{
		return dict_info;
	}
	int get_data_fields(guint32 offset, guint32 size, data_field_vect_t& fields) const;

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
	std::string idxfilename; // file to read, uncompressed
	std::string idxfilename_orig; // may be archive
	std::string dictfilename;
	std::string dictfilename_orig;
	std::string synfilename;
	DictInfo dict_info;
	TempFile idxtemp;
	TempFile dicttemp;
	clib::File dictfile;
	guint32 dictfilesize;
	std::vector<worditem_t> index;
	std::vector<synitem_t> synindex;
	print_info_t print_info;
	i_resource_storage* p_res_storage;
	/* fix errors if possible. We never change the files we read,
	 * all fixes effect only in-memory data structures.
	 * If an error is fixed, we do not return failure status,
	 * but an error message is printed nevertheless. */
	bool fix_errors;
};


#endif /* STARDICT_NORM_DICT_H_ */
