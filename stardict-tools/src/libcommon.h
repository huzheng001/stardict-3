#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <glib.h>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

typedef void (*print_info_t)(const char *info, ...);

extern gint stardict_strcmp(const gchar *s1, const gchar *s2);
extern bool file_name_to_utf8(const std::string& str, std::string& out);
extern bool utf8_to_file_name(const std::string& str, std::string& out);
#ifdef _WIN32
typedef std::basic_string<TCHAR> std_win_string;
extern bool utf8_to_windows(const std::string& str_utf8, std_win_string& out);
extern bool windows_to_utf8(const std_win_string& str, std::string& out_utf8);
#endif
#define DB_DIR_SEPARATOR '/'
#define DB_DIR_SEPARATOR_S "/"
/* functions to convert directory separator characters 
 * 1. file system separator character = G_DIR_SEPARATOR
 * 2. database separator character = DB_DIR_SEPARATOR */
#if DB_DIR_SEPARATOR == G_DIR_SEPARATOR
inline std::string dir_separator_fs_to_db(const std::string& path)
{
	return path;
}
inline std::string dir_separator_db_to_fs(const std::string& path)
{
	return path;
}
#else
extern std::string dir_separator_fs_to_db(const std::string& path);
extern std::string dir_separator_db_to_fs(const std::string& path);
#endif

enum TLoadResult { lrOK, lrError, lrNotFound };

int unpack_zlib(const char* arch_file_name, const char* out_file_name, print_info_t print_info);

/* allows to create a temp file, remove the temp file when the object is destroyed. */
class TempFile
{
public:
	explicit TempFile(print_info_t print_info = g_print)
		: print_info(print_info)
	{
	}
	~TempFile(void)
	{
		clear();
	}
	const std::string& create_temp_file(void);
	const std::string& get_file_name(void) const
	{
		return file_name;
	}
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
private:
	void clear(void);
	std::string file_name;
	print_info_t print_info;
};

template <class T>
class auto_executor_t
{
public:
	typedef void (T::*method_t)(void);
	auto_executor_t(T& obj, method_t method)
	:
		obj(obj),
		method(method)
	{

	}
	~auto_executor_t(void)
	{
		(obj.*method)();
	}
private:
	T& obj;
	method_t method;
};

/* Create a new temporary file. Return file name in file name encoding.
Return an empty string if file cannot be created. */
std::string create_temp_file(void);

extern const char* known_resource_types[];

bool is_known_resource_type(const char* str);

void trim_spaces(const char* const src, const char*& new_beg, size_t& new_len);

#define UTF8_BOM "\xEF\xBB\xBF"

#define key_forbidden_chars \
	"\n\r"
#define known_type_ids \
	"mtygxkwhnr"

#define read_file_err \
	"Error reading file %s.\n"
#define write_file_err \
	"Error writing file %s.\n"
#define open_read_file_err \
	"Unable open file %s for reading\n"
#define open_write_file_err \
	"Unable open file %s for writing\n"
#define create_temp_file_err \
		"Unable to create a temporary file: %s.\n"
#define incorrect_arg_err \
		"Incorrect argument.\n"
#define index_file_truncated_err \
	"Index file is truncated, last record is truncated.\n"
#define incorrect_data_block_size_err \
	"Index item %s. Fields do not fit into the data block, incorrect data block size.\n"
#define empty_field_err \
	"Index item %s. Empty field in definition data block. Type ID '%c'.\n"
#define invalid_utf8_field_err \
	"Index item %s. Invalid field. Type id = '%c'. Invalid utf8 string: %s\n"
#define invalid_char_value_err \
	"Invalid Char value %lu.\n"
#define invalid_field_content_err \
	"Index item %s. Type id '%c'. Invalid field content: %s\n"
#define syn_file_truncated_err \
	"Synonyms file is truncated, last record is truncated.\n"
#define unknown_type_id_err \
	"Index item %s. Unknown type identifier '%c'.\n"
#define empty_word_err \
	"Empty word in index.\n"
#define long_word_err \
	"Index item %s. wordlen>=%d, wordlen = %d.\n"
#define word_begin_space_err \
	"Warning: Index item %s. word begins with space.\n"
#define word_end_space_err \
	"Warning: Index item %s. word ends with space.\n"
#define word_forbidden_chars_err \
	"Warning: Index item %s. word contains forbidden characters.\n"
#define word_invalid_utf8_err \
	"Index item %s. Invalid utf8 string.\n"
#define word_invalid_char_value_err \
	"Index item %s. Invalid Char value %lu.\n"
#define wrong_word_order_err \
	"Wrong order, first word = %s, second word = %s\n"
#define fixed_ignore_field_msg \
	"fixed. ignore the field.\n"
#define fixed_accept_unknown_field_msg \
	"fixed. accept unknown field type.\n"
#define fixed_ignore_line_msg \
	"fixed. ignore the line.\n"
#define fixed_ignore_file_tail_msg \
	"fixed. ignore the tail of the file.\n"
#define fixed_ignore_syn_file_msg \
	"fixed. ignore the .syn file.\n"
#define fixed_ignore_word_msg \
	"fixed. ignore the word.\n"
#define fixed_drop_invalid_char_msg \
	"fixed. Dropping invalid chars.\n"
#define fixed_ignore_msg \
	"fixed. ignore.\n"
#define fixed_msg \
	"fixed\n"


/* Maximum size of word in index. strlen(word) < MAX_INDEX_KEY_SIZE.
 * See doc/StarDictFileFormat. */
const int MAX_INDEX_KEY_SIZE=256;

#endif

