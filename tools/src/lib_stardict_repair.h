#ifndef _LIB_STARDICT_REPAIR_H_
#define _LIB_STARDICT_REPAIR_H_

#include <string>

struct RepairOptions
{
	bool lot_of_memory;
	bool compress_dict;
	bool copy_res_store;
};

int stardict_repair(const std::string& ifofilepath, const std::string& outdirpath,
		RepairOptions options);

#endif // _LIB_STARDICT_REPAIR_H_
