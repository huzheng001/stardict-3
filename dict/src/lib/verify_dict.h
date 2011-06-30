#ifndef _VERIFY_DICT_H_
#define _VERIFY_DICT_H_

#include <list>
#include <string>

/* Verify dictionaries before loading them in StarDict.
 * Corrupted dictionaries may crash StarDict. */

class show_progress_t;

void filter_verify_dicts(const std::list<std::string>& dict_all_list,
	std::list<std::string>& dict_valid_list, show_progress_t *sp);

#endif // _VERIFY_DICT_H_
