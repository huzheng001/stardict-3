#ifndef LIB_CHARS_H_
#define LIB_CHARS_H_

#include <list>
#include <string>

#define key_forbidden_chars \
	"\n\r"
#define key_forbidden_chars_ex \
	key_forbidden_chars " \t"

extern int check_xml_string_chars(const char* str, std::list<const char*>& invalid_chars);
extern int check_xml_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars);
extern void fix_xml_string_chars(const char* src, std::string& dst);
extern void fix_xml_string_chars(const char* src, const size_t len, std::string& dst);

extern int check_stardict_string_chars(const char* str, std::list<const char*>& invalid_chars);
extern int check_stardict_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars);
extern void fix_stardict_string_chars(const char* src, std::string& dst);
extern void fix_stardict_string_chars(const char* src, const size_t len, std::string& dst);

extern int check_stardict_key_chars(const char* str);
extern void fix_stardict_key_chars(const char* str, std::string& dst);

#endif /* LIB_CHARS_H_ */
