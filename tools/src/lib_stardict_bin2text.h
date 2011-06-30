#ifndef LIB_STARDICT_BIN2TEXT_H_
#define LIB_STARDICT_BIN2TEXT_H_

#include <string>

extern int stardict_bin2text(const std::string& ifofilename, const std::string& xmlfilename,
		size_t chunk_size);

#endif /* LIB_STARDICT_BIN2TEXT_H_ */
