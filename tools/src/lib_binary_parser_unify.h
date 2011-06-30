#ifndef LIB_BINARY_PARSER_UNIFY_H_
#define LIB_BINARY_PARSER_UNIFY_H_

#include "lib_common_dict.h"
#include "lib_binary_dict_parser.h"
#include "libcommon.h"

/* convert binary dictionary parser results into common dictionary format */
int convert_to_parsed_dict(common_dict_t& parsed_norm_dict, const binary_dict_parser_t& norm_dict);

#endif /* LIB_BINARY_PARSER_UNIFY_H_ */
