#ifndef LIB_DICT_DATA_BLOCK_H_
#define LIB_DICT_DATA_BLOCK_H_

#include "lib_res_store.h"

/* extract result */
struct ext_result_t {
	ext_result_t(int extract, int content)
	:
		extract(extract),
		content(content)
	{

	}
	void append(const ext_result_t& right)
	{
		extract = (extract || right.extract) ? EXIT_FAILURE : EXIT_SUCCESS;
		content = (extract || content || right.content) ? EXIT_FAILURE : EXIT_SUCCESS;
	}
	int summary(void) const
	{
		return (extract || content) ? EXIT_FAILURE : EXIT_SUCCESS;
	}
	int extract:4; // field extraction result. Can we read the next field or not?
	int content:4; // field content result. Is content OK or not?
};

struct data_field_t
{
	data_field_t(void)
	:
		type_id(0)
	{
	}

	char type_id;
	/* for string data types, return string length,
	 * for binary data types, return data size */
	size_t get_size(void) const;
	/* for string data types, return a '\0'-terminated string. */
	const char* get_data(void) const;
	void set_data(const char* p, size_t size, bool add_null = false);
private:
	/* for string data types, like 'm', data ends with '\0' char,
	 * for binary data types, the vector contains only data. */
	std::vector<char> data;
};

typedef std::vector<data_field_t> data_field_vect_t;

class dictionary_data_block {
public:
	dictionary_data_block(void)
	:
		word(NULL),
		print_info(g_print),
		p_res_storage(NULL),
		fix_errors(false),
		fields(NULL),
		field_num(0)
	{

	}
	int load(const char* const data, size_t data_size,
		const std::string& sametypesequence, const char* word,
		data_field_vect_t* fields = NULL);
	void set_resource_storage(i_resource_storage* p_res_storage)
	{
		this->p_res_storage = p_res_storage;
	}
	void set_print_info(print_info_t print_info)
	{
		this->print_info = print_info;
	}
	void set_fix_errors(bool b)
	{
		fix_errors = b;
	}
	void set_word(const char* word)
	{
		this->word = word;
	}
	/* if you use this method directly, do not forget to set_word(). NULL as argument is OK. */
	int verify_field_content_r(const char* const data, guint32 size, resitem_vect_t *items = NULL);
private:
	int load_no_sametypesequence(const char* const data, size_t data_size);
	int load_sametypesequence(const char* const data, size_t data_size,
		const std::string& sametypesequence);
	ext_result_t load_field(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_upper(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_lower(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_sametypesequence_last_upper(const char type_id,
		const char*& p, size_t size_remain);
	ext_result_t load_field_sametypesequence_last_lower(const char type_id,
		const char*& p, size_t size_remain);
	int verify_field_content(const char type_id, const char* data, guint32 size);
	int verify_field_content_x(const char* data, guint32 size);

	const char* word;
	print_info_t print_info;
	i_resource_storage* p_res_storage; // may be NULL
	bool fix_errors;
	data_field_vect_t* fields;
	size_t field_num; // number of fields extracted
};


#endif /* LIB_DICT_DATA_BLOCK_H_ */
