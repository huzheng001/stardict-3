#ifndef XML_STR_H_
#define XML_STR_H_

#include <string>

/* xml strings in utf8 encoding 
 * calculate string lengths, next, previous characters, etc ignoring xml tags 
 * */

size_t xml_utf8_strlen(const char *str);
void xml_utf8_decode(const char *str, std::string& decoded);
size_t xml_utf8_get_index_at_offset(const std::string& str, size_t char_offset);
const char* xml_utf8_offset_to_pointer(const char *str, size_t char_offset);
const char* xml_utf8_end_of_char(const char *str);

#endif /* XML_STR_H_ */
