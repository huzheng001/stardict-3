#ifndef LIB_XML_UTILS_H_
#define LIB_XML_UTILS_H_

#include <list>
#include <string>

extern int check_xml_string_chars(const char* str, std::list<const char*>& invalid_chars);
extern int check_xml_string_chars(const char* str, const size_t len, std::list<const char*>& invalid_chars);
extern void fix_xml_string_chars(const char* src, std::string& dst);
extern void fix_xml_string_chars(const char* src, const size_t len, std::string& dst);

#endif /* LIB_XML_UTILS_H_ */
