#ifndef LIB_STARDICT_TEXT2BIN_H_
#define LIB_STARDICT_TEXT2BIN_H_

#include <string>

extern int stardict_text2bin(const std::string& xmlfilename, const std::string& ifofilename,
		bool show_xincludes, bool use_same_type_sequence);

#endif /* LIB_STARDICT_TEXT2BIN_H_ */
