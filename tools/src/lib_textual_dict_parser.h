#ifndef LIB_TEXTUAL_DICT_PARSER_H_
#define LIB_TEXTUAL_DICT_PARSER_H_

#include <string>

class common_dict_t;

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int parse_textual_dict(const std::string& xmlfilename, common_dict_t* norm_dict,
		bool show_xincludes);

#endif /* LIB_TEXTUAL_DICT_PARSER_H_ */
