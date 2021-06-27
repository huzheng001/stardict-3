/*
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <libxml/xmlreader.h>
#include <libxml/xinclude.h>
#include <cstring>
//#include <iostream>
#include <vector>
#include <algorithm>
#include "lib_stardict_text2bin.h"
#include "ifo_file.h"
#include "libcommon.h"
#include "lib_chars.h"
#include "lib_common_dict.h"
#include "lib_textual_dict_parser.h"
#include "lib_dict_verify.h"

#define parser_err \
	"Parser error."
#define duplicate_elem_err \
	"%u: duplicate element %s."
#define attribute_value_not_specified_err \
	"%u: attribute '%s' is not specified."
#define attribute_value_unknown_err \
	"%u: unknown value of the attribute '%s': %s."
#define element_blank_value \
	"%u: element '%s' has blank value."
#define attribute_content_type_absent_err \
	"%u: Content type attribute is not specified neither for the definition element, " \
	"nor for any element above."
#define attribute_value_empty_err \
	"%u: value of the attribute '%s' is an empty string."
#define save_into_temp_file_err \
	"Unable to save into a temporary file."
#define duplicate_key_err \
	"%u: duplicate key: %s."
#define duplicate_synonym_err \
		"%u: duplicate synonym: %s."
#define incorrect_dir_sep_err \
		"%u: Incorrect directory separator, use '/' char."
#define resource_list_empty_err \
		"%u: empty resource list."
#define article_key_list_empty_err \
		"%u: article: empty key list."
#define article_definition_list_empty_err \
		"%u: article: empty definition list."
#define missing_info_section_err \
		"missing info section."
#define article_list_empty \
	"article list empty."
#define unknown_content_type_str_err \
		"%u: unknown content type '%s'. It must be one ASCII char."
#define unknown_content_type_chr_err \
	"%u: unknown content type '%c'."
#define content_type_r_in_definition_err \
		"%u: use 'definition-r' element for content type 'r'."
#define element_invalid_utf8_value_err \
		"%u: element %s. Invalid utf-8 text value: %s."
#define article_key_forbidden_chars_err \
	"%u: key contains forbidden chars: %s."
#define article_synonym_forbidden_chars_err \
	"%u: synonym contains forbidden chars: %s."
#define normalization_failed_err \
		"%u: utf8 normalization failed. String: %s."
#define article_key_long_err \
		"%u: Key is too long: %s. Key length = %u, maximum length = %d."
#define article_synonym_long_err \
		"%u: Synonym is too long: %s. Synonym length = %u, maximum length = %d."
#define element_invalid_text_err \
		"%u: element '%s'. Invalid text: ''\n%s\n'''\n" \
		invalid_chars_in_textual_data_msg

#define missing_req_info_item_msg \
	"missing required info item %s."
#define parse_xml_done_msg \
	"xml parsing done."
#define allow_unknown_content_type_msg \
	"OK, allow unknown content type."
#define xinclude_process_msg \
		"%u: processing xinclude: %s"

namespace xml {
	/* ResourceWrapper needs an address of a function with external linkage.
	 * xmlFree is a global variable - a pointer to a function.
	 * Provide a wrapper function. */
	void stardict_xmlFree(void* p)
	{
		xmlFree(p);
	}
	typedef ResourceWrapper<xmlChar, void*, void, stardict_xmlFree> CharStr;
}

static const char* note_type_str(xmlReaderTypes type)
{
	return note_type_str(type);
}

static const char* note_type_str(int type)
{
	const char* types[] = {
		"NONE",
		"ELEMENT",
		"ATTRIBUTE",
		"TEXT",
		"CDATA",
		"ENTITY_REFERENCE",
		"ENTITY",
		"PROCESSING_INSTRUCTION",
		"COMMENT",
		"DOCUMENT",
		"DOCUMENT_TYPE",
		"DOCUMENT_FRAGMENT",
		"NOTATION",
		"WHITESPACE",
		"SIGNIFICANT_WHITESPACE",
		"END_ELEMENT",
		"END_ENTITY",
		"XML_DECLARATION"
	};
	if(type < 0 || type > XML_READER_TYPE_XML_DECLARATION)
		return "ERROR";
	return types[type];
}

#if 0
static void processNode(xmlTextReaderPtr reader) {
    xmlChar *name, *value;

    name = xmlTextReaderName(reader);
    if (name == NULL)
        name = xmlStrdup(BAD_CAST "--");
    value = xmlTextReaderValue(reader);
    xmlNodePtr node = xmlTextReaderCurrentNode(reader);

    printf("%hu: %d %s %s %d %d %d",
    		node ? node->line : 0,
            xmlTextReaderDepth(reader),
            note_type_str(xmlTextReaderNodeType(reader)),
            name,
            xmlTextReaderIsEmptyElement(reader),
            xmlTextReaderHasAttributes(reader),
            xmlTextReaderAttributeCount(reader)
            );
    xmlFree(name);
    if (value == NULL)
        printf("\n");
    else {
        printf(" '%s'\n", value);
        xmlFree(value);
    }
}
#endif

/* Implementation details
 *
 * Most of the elements have a special method that reads that particular element.
 * For example, we have read_article, read_contents, etc.
 * The method is invoked after the start tag of the element in read.
 * For example, after reading article start tag we invoke read_article method.
 * The method in question must read its element completely and return logical result:
 * EXIT_FAILURE or EXIT_SUCCESS.
 * A method may correct small errors, that does not effect the rest of the document.
 * If a method returns EXIT_SUCCESS, the following elements can be read without problems.
 * If a method returns EXIT_FAILURE, we should terminate the parsing process immediately.
 * */
class textual_dict_parser_t
{
public:
	textual_dict_parser_t(void);
	int parse(const std::string& xmlfilename, common_dict_t* norm_dict);
	void set_custom_include(bool b);
	bool get_custom_include(void) const;
private:
	enum ReadResult { rrEOF = -1, rrError = -2, rrEndElement = -3};
	typedef unsigned int line_number_t;
	int prepare_parser(void);
	void close_parser(void);
	void remove_reader(void);
	int read_all(void);
	int read_stardict(void);
	int read_info(void);
	int read_info_items(void);
	int read_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
		void (DictInfo:: *set_item)(const std::string& ));
	int read_contents(void);
	int read_article(void);
	int read_contents_r(article_def_t& article_def);
	TLoadResult read_att_type(char& type);
	int check_blank_info_items(void);
	int check_new_line_info_items(void);
	int check_mandatory_info_items(void);
	int check_article_key(const std::string& value, line_number_t article_item_line_number);
	int check_article_synonym(const std::string& value, line_number_t article_item_line_number);
	int read_xml_element(const char** exp_elems, const char* open_elem);
	int read_xml_end_element(const char* open_elem);
	int read_xml_text_item(const char* elem, std::string& value);
	int read_xml_text(const char* open_elem);
	int read_xml_attribute(const char* att_name, std::string& value);
	void check_blank_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
		const std::string& (DictInfo:: *get_item)(void) const, void (DictInfo:: *unset_item)(void));
	int check_new_line_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
			const std::string& (DictInfo:: *get_item)(void) const);
	char get_effective_content_type(void) const;
	void unexpected_node_type_element(const char** exp_elems);
	void unexpected_node_type_text(void);
	void unexpected_node_type_end_element(const char* open_elem);
	void unexpected_element(const char** exp_elems);
	void unexpected_eof(const char** exp_elems);
	void unexpected_eof(void);
	void unexpected_end_element(const char** exp_elems, const char* open_elem);
	void unmatched_end_element(const char* open_elem, const char* end_elem);
	void unexpected_empty_element(void);
	void unexpected_non_empty_element(void);
	line_number_t get_line_number(void);
	int next_node(void);
	std::string xmlfilename;
	xmlTextReaderPtr xml_reader;
	xmlXIncludeCtxtPtr xincctxt;
	static const int default_reader_options =
		XML_PARSE_NOENT | XML_PARSE_NOBLANKS | XML_PARSE_NOCDATA | XML_PARSE_XINCLUDE;
	int reader_options;
	struct elem_data_t
	{
		elem_data_t(const char* name, char content_type = 0)
		:
			name(name),
			content_type(content_type)
		{

		}
		const char* name;
		char content_type;
	private:
		elem_data_t(void);
	};
	/* stack of elements
	 * Each time new element start tag is read, name of the read element is added to the stack.
	 * When an end element is read, the top element of the stack is removed.
	 * Some elements (article, contents, definition) may have a content type attribute.
	 * The value of this attribute is one char that is assigned to content_type field.
	 * For other elements and when the attribute in question is not specified, content_type field
	 * must be = 0. */
	std::vector<elem_data_t> elem_stack;
	common_dict_t* norm_dict;
};

textual_dict_parser_t::textual_dict_parser_t(void)
:
	xml_reader(NULL),
	xincctxt(NULL),
	reader_options(default_reader_options),
	norm_dict(NULL)
{

}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::parse(const std::string& xmlfilename,
		common_dict_t* norm_dict)
{
	this->xmlfilename = xmlfilename;
	this->norm_dict = norm_dict;

	auto_executor_t<textual_dict_parser_t> auto_exec(*this, &textual_dict_parser_t::close_parser);
	if(prepare_parser())
		return EXIT_FAILURE;
	g_message("processing %s...", xmlfilename.c_str());
	if(read_all())
		return EXIT_FAILURE;
#if 0
	int ret = next_node();
	while (ret == 1) {
		processNode(xml_reader);
		ret = next_node();
	}
	if (ret != 0) {
		g_critical("%s : failed to parse.", xmlfilename.c_str());
		return EXIT_FAILURE;
	}
#endif
	g_message(parse_xml_done_msg);
	return EXIT_SUCCESS;
}

void textual_dict_parser_t::set_custom_include(bool b)
{
	if(b)
		reader_options &= ~XML_PARSE_XINCLUDE;
	else
		reader_options |= XML_PARSE_XINCLUDE;
}

bool textual_dict_parser_t::get_custom_include(void) const
{
	return (reader_options & XML_PARSE_XINCLUDE) == 0;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::prepare_parser(void)
{
	remove_reader();
	xml_reader = xmlReaderForFile(xmlfilename.c_str(), NULL,
		reader_options);
	if (!xml_reader) {
		g_critical(open_read_file_err, xmlfilename.c_str(), "");
		return EXIT_FAILURE;
	}
	if(!norm_dict) {
		g_critical("norm_dict == NULL.");
		return EXIT_FAILURE;
	}
	if(norm_dict->reset())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

void textual_dict_parser_t::close_parser(void)
{
	remove_reader();
	norm_dict = NULL;
	xmlfilename.clear();
}

void textual_dict_parser_t::remove_reader(void)
{
	if(xml_reader)
		xmlFreeTextReader(xml_reader);
	xml_reader = NULL;
	if(xincctxt)
		xmlXIncludeFreeContext(xincctxt);
	xincctxt = NULL;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_all(void)
{
	const char* const open_elem = "";
	static const char* exp_elems[] = {
		"stardict",
		NULL
	};
	int ret = read_xml_element(exp_elems, open_elem);
	if(ret == rrEOF) {
		unexpected_eof(exp_elems);
		return EXIT_FAILURE;
	}
	if(ret == rrError)
		return EXIT_FAILURE;
	if(ret == rrEndElement) {
		unexpected_end_element(exp_elems, open_elem);
		return EXIT_FAILURE;
	}
	if(xmlTextReaderIsEmptyElement(xml_reader)) {
		unexpected_empty_element();
		return EXIT_FAILURE;
	}
	g_assert(ret == 0);
	elem_stack.push_back(elem_data_t(exp_elems[0]));
	if(read_stardict())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/* read root stardict element
 * Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_stardict(void)
{
	static const char* exp_elems[] = {
		"info",
		"contents",
		"article",
		NULL
	};
	bool have_info = false;
	while(true) {
		int ret = read_xml_element(exp_elems, elem_stack.back().name);
		if(ret == rrEOF) {
			unexpected_eof(exp_elems);
			return EXIT_FAILURE;
		}
		if(ret == rrError)
			return EXIT_FAILURE;
		if(ret == rrEndElement) {
			elem_stack.pop_back();
			break;
		}
		if(xmlTextReaderIsEmptyElement(xml_reader)) {
			unexpected_empty_element();
			return EXIT_FAILURE;
		}
		elem_stack.push_back(elem_data_t(exp_elems[ret]));
		switch(ret) {
		case 0:
			if(have_info) {
				g_critical(duplicate_elem_err, get_line_number(), exp_elems[ret]);
				return EXIT_FAILURE;
			}
			if(read_info())
				return EXIT_FAILURE;
			have_info = true;
			break;
		case 1:
			if(read_contents())
				return EXIT_FAILURE;
			break;
		case 2:
			if(read_article())
				return EXIT_FAILURE;
			break;
		default:
			g_assert_not_reached();
		}
	}
	if(!have_info) {
		g_critical(missing_info_section_err);
		return EXIT_FAILURE;
	}
	if(norm_dict->articles.empty()) {
		g_critical(article_list_empty);
		return EXIT_FAILURE;
	}
	g_message("total articles: %lu.", static_cast<unsigned long>(norm_dict->articles.size()));
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_info(void)
{
	norm_dict->dict_info.clear();
	if(read_info_items())
		return EXIT_FAILURE;
	if(check_blank_info_items())
		return EXIT_FAILURE;
	if(check_new_line_info_items())
		return EXIT_FAILURE;
	if(check_mandatory_info_items())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_info_items(void)
{
	static const char* exp_elems[] = {
		"version",
		"bookname",
		"author",
		"email",
		"website",
		"description",
		"date",
		"dicttype",
		NULL
	};
	while(true) {
		int ret = read_xml_element(exp_elems, elem_stack.back().name);
		if(ret == rrEOF) {
			unexpected_eof(exp_elems);
			return EXIT_FAILURE;
		}
		if(ret == rrError)
			return EXIT_FAILURE;
		if(ret == rrEndElement) {
			elem_stack.pop_back();
			break;
		}
		if(xmlTextReaderIsEmptyElement(xml_reader)) {
			unexpected_empty_element();
			return EXIT_FAILURE;
		}
		// do not change elem_stack for leaf elements
		switch(ret) {
		case 0:
			if(read_info_item("version", &DictInfo::is_version, &DictInfo::set_version))
				return EXIT_FAILURE;
			break;
		case 1:
			if(read_info_item("bookname", &DictInfo::is_bookname, &DictInfo::set_bookname))
				return EXIT_FAILURE;
			break;
		case 2:
			if(read_info_item("author", &DictInfo::is_author, &DictInfo::set_author))
				return EXIT_FAILURE;
			break;
		case 3:
			if(read_info_item("email", &DictInfo::is_email, &DictInfo::set_email))
				return EXIT_FAILURE;
			break;
		case 4:
			if(read_info_item("website", &DictInfo::is_website, &DictInfo::set_website))
				return EXIT_FAILURE;
			break;
		case 5:
			if(read_info_item("description", &DictInfo::is_description, &DictInfo::set_description))
				return EXIT_FAILURE;
			break;
		case 6:
			if(read_info_item("date", &DictInfo::is_date, &DictInfo::set_date))
				return EXIT_FAILURE;
			break;
		case 7:
			if(read_info_item("dicttype", &DictInfo::is_dicttype, &DictInfo::set_dicttype))
				return EXIT_FAILURE;
			break;
		default:
			g_assert_not_reached();
		}
	}
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
	void (DictInfo:: *set_item)(const std::string& ))
{
	bool ignore = false;
	if((norm_dict->dict_info.*is_item)()) {
		g_warning(duplicate_elem_err, get_line_number(), elem);
		g_message(fixed_ignore_msg);
		ignore = true;
	}
	std::string value;
	if(read_xml_text_item(elem, value))
		return EXIT_FAILURE;
	if(value.empty()) {
		g_warning(element_blank_value, get_line_number(), elem);
		g_message(fixed_ignore_msg);
		return EXIT_SUCCESS;
	}
	if(!ignore) {
		(norm_dict->dict_info.*set_item)(value.c_str());
	}
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_contents(void)
{
	{
		char content_type;
		switch(read_att_type(content_type))
		{
		case lrOK:
			elem_stack.back().content_type = content_type;
			break;
		case lrError:
			return EXIT_FAILURE;
		case lrNotFound:
			break;
		}
	}
	static const char* exp_elems[] = {
		"contents",
		"article",
		NULL
	};
	while(true) {
		int ret = read_xml_element(exp_elems, elem_stack.back().name);
		if(ret == rrEOF) {
			unexpected_eof(exp_elems);
			return EXIT_FAILURE;
		}
		if(ret == rrError)
			return EXIT_FAILURE;
		if(ret == rrEndElement) {
			elem_stack.pop_back();
			break;
		}
		if(xmlTextReaderIsEmptyElement(xml_reader)) {
			unexpected_empty_element();
			return EXIT_FAILURE;
		}
		elem_stack.push_back(elem_data_t(exp_elems[ret]));
		switch(ret) {
		case 0:
			if(read_contents())
				return EXIT_FAILURE;
			break;
		case 1:
			if(read_article())
				return EXIT_FAILURE;
			break;
		default:
			g_assert_not_reached();
		}
	}
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_article(void)
{
	const line_number_t article_beg_line_number = get_line_number();
	{
		char content_type;
		switch(read_att_type(content_type))
		{
		case lrOK:
			elem_stack.back().content_type = content_type;
			break;
		case lrError:
			return EXIT_FAILURE;
		case lrNotFound:
			break;
		}
	}
	static const char* exp_elems[] = {
		"key",
		"synonym",
		"definition",
		"definition-r",
		NULL
	};
	article_data_t article;
	/* for simplicity we do not check the order of elements */
	while(true) {
		int ret = read_xml_element(exp_elems, elem_stack.back().name);
		if(ret == rrEOF) {
			unexpected_eof(exp_elems);
			return EXIT_FAILURE;
		}
		if(ret == rrError)
			return EXIT_FAILURE;
		if(ret == rrEndElement) {
			elem_stack.pop_back();
			break;
		}
		if(xmlTextReaderIsEmptyElement(xml_reader)) {
			unexpected_empty_element();
			return EXIT_FAILURE;
		}
		const line_number_t article_item_line_number = get_line_number();
		char content_type;
		if(ret == 2) { // read 'type' attribute of the 'definition' element
			switch(read_att_type(content_type))
			{
			case lrOK:
				break;
			case lrError:
				return EXIT_FAILURE;
			case lrNotFound:
				content_type = get_effective_content_type();
				break;
			}
			if(content_type == 0) {
				g_critical(attribute_content_type_absent_err,
					get_line_number());
				return EXIT_FAILURE;
			}
		}
		switch(ret) {
		case 0:
		case 1:
		case 2:
		{
			std::string value;
			if(read_xml_text_item(exp_elems[ret], value))
				return EXIT_FAILURE;
			if(value.empty()) {
				g_warning(element_blank_value, get_line_number(), exp_elems[ret]);
				g_message(fixed_ignore_msg);
				break;
			}
			if(ret == 0) {
				if(check_article_key(value, article_item_line_number))
					return EXIT_FAILURE;
				if(article.add_key(value)) {
					g_critical(duplicate_key_err, get_line_number(), value.c_str());
					return EXIT_FAILURE;
				}
			}
			if(ret == 1) {
				if(check_article_synonym(value, article_item_line_number))
					return EXIT_FAILURE;
				if(article.add_synonym(value)) {
					g_critical(duplicate_synonym_err, get_line_number(), value.c_str());
					return EXIT_FAILURE;
				}
			}
			if(ret == 2) {
				size_t size = value.length(), offset;
				if(norm_dict->write_data(value.c_str(), size, offset)) {
					g_critical(save_into_temp_file_err);
					return EXIT_FAILURE;
				}
				if(article.add_definition(article_def_t(content_type, offset, size)))
					return EXIT_FAILURE;
			}
			break;
		}
		case 3:
		{
			elem_stack.push_back(elem_data_t(exp_elems[ret]));
			article_def_t def;
			if(read_contents_r(def))
				return EXIT_FAILURE;
			if(article.add_definition(def))
				return EXIT_FAILURE;
			break;
		}
		default:
			g_assert_not_reached();
		}
	}
	if(article.key.empty()) {
		g_warning(article_key_list_empty_err, article_beg_line_number);
		g_message(fixed_ignore_msg);
		return EXIT_SUCCESS;
	}
	if(article.definitions.empty()) {
		g_warning(article_definition_list_empty_err, article_beg_line_number);
		g_message(fixed_ignore_msg);
		return EXIT_SUCCESS;
	}
	if(norm_dict->add_article(article))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_contents_r(article_def_t& article_def)
{
	const line_number_t contents_beg_line_number = get_line_number();
	article_def.type = 'r';
	static const char* exp_elems[] = {
		"resource",
		NULL
	};
	while(true) {
		int ret = read_xml_element(exp_elems, elem_stack.back().name);
		if(ret == rrEOF) {
			unexpected_eof(exp_elems);
			return EXIT_FAILURE;
		}
		if(ret == rrError)
			return EXIT_FAILURE;
		if(ret == rrEndElement) {
			elem_stack.pop_back();
			break;
		}
		if(!xmlTextReaderIsEmptyElement(xml_reader)) {
			unexpected_non_empty_element();
			return EXIT_FAILURE;
		}
		g_assert(ret == 0);
		const char* att_name = "key";
		std::string key;
		if(read_xml_attribute(att_name, key)) {
			g_critical(attribute_value_not_specified_err,
				get_line_number(), att_name);
			return EXIT_FAILURE;
		}
		if(key.empty()) {
			g_critical(attribute_value_empty_err,
				get_line_number(), att_name);
			return EXIT_FAILURE;
		}
		if(key.find_first_of('\\') != std::string::npos) {
			g_critical(incorrect_dir_sep_err, get_line_number());
			return EXIT_FAILURE;
		}
		att_name = "type";
		std::string type;
		if(read_xml_attribute(att_name, type)) {
			g_critical(attribute_value_not_specified_err,
				get_line_number(), att_name);
			return EXIT_FAILURE;
		}
		if(!is_known_resource_type(type.c_str())) {
			g_critical(attribute_value_unknown_err,
				get_line_number(), att_name, type.c_str());
			return EXIT_FAILURE;
		}
		article_def.resources.push_back(resource_t(type, key));
	}
	if(article_def.resources.empty()) {
		g_critical(resource_list_empty_err, contents_beg_line_number);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


TLoadResult textual_dict_parser_t::read_att_type(char& type)
{
	std::string value;
	static const char* const att_name = "type";
	if(read_xml_attribute(att_name, value))
		return lrNotFound;
	if(value.length() != 1) {
		g_critical(unknown_content_type_str_err,
			get_line_number(), value.c_str());
		return lrError;
	}
	type = value[0];
	g_assert(type != 0);
	if(strchr(known_type_ids, type) == NULL) {
		g_warning(unknown_content_type_chr_err, get_line_number(), type);
		g_message(allow_unknown_content_type_msg);
	}
	if(type == 'r') {
		g_critical(content_type_r_in_definition_err,
			get_line_number());
		return lrError;
	}
	return lrOK;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::check_blank_info_items(void)
{
	check_blank_info_item("version", &DictInfo::is_version,
		&DictInfo::get_version, &DictInfo::unset_version);
	check_blank_info_item("bookname", &DictInfo::is_bookname,
		&DictInfo::get_bookname, &DictInfo::unset_bookname);
	check_blank_info_item("author", &DictInfo::is_author,
		&DictInfo::get_author, &DictInfo::unset_author);
	check_blank_info_item("email", &DictInfo::is_email,
		&DictInfo::get_email, &DictInfo::unset_email);
	check_blank_info_item("website", &DictInfo::is_website,
		&DictInfo::get_website, &DictInfo::unset_website);
	check_blank_info_item("description", &DictInfo::is_description,
		&DictInfo::get_description, &DictInfo::unset_description);
	check_blank_info_item("date", &DictInfo::is_date,
		&DictInfo::get_date, &DictInfo::unset_date);
	check_blank_info_item("dicttype", &DictInfo::is_dicttype,
		&DictInfo::get_dicttype, &DictInfo::unset_dicttype);
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::check_new_line_info_items(void)
{
	if(check_new_line_info_item("version", &DictInfo::is_version,
			&DictInfo::get_version))
		return EXIT_FAILURE;
	if(check_new_line_info_item("bookname", &DictInfo::is_bookname,
			&DictInfo::get_bookname))
		return EXIT_FAILURE;
	if(check_new_line_info_item("author", &DictInfo::is_author,
			&DictInfo::get_author))
		return EXIT_FAILURE;
	if(check_new_line_info_item("email", &DictInfo::is_email,
			&DictInfo::get_email))
		return EXIT_FAILURE;
	if(check_new_line_info_item("website", &DictInfo::is_website,
			&DictInfo::get_website))
		return EXIT_FAILURE;
	if(check_new_line_info_item("date", &DictInfo::is_date,
			&DictInfo::get_date))
		return EXIT_FAILURE;
	if(check_new_line_info_item("dicttype", &DictInfo::is_dicttype,
			&DictInfo::get_dicttype))
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::check_mandatory_info_items(void)
{
	if(!norm_dict->dict_info.is_version()) {
		g_critical(missing_req_info_item_msg, "version");
		return EXIT_FAILURE;
	}
	if(!norm_dict->dict_info.is_bookname()) {
		g_critical(missing_req_info_item_msg, "bookname");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::check_article_key(const std::string& value, line_number_t article_item_line_number)
{
	if(value.find_first_of(key_forbidden_chars) != std::string::npos) {
		g_critical(article_key_forbidden_chars_err,
			article_item_line_number, value.c_str());
		return EXIT_FAILURE;
	}
	if(value.length() >= (size_t)MAX_INDEX_KEY_SIZE) {
		g_critical(article_key_long_err,
			get_line_number(), value.c_str(), static_cast<unsigned>(value.length()), MAX_INDEX_KEY_SIZE-1);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::check_article_synonym(const std::string& value, line_number_t article_item_line_number)
{
	if(value.find_first_of(key_forbidden_chars) != std::string::npos) {
		g_critical(article_synonym_forbidden_chars_err,
			article_item_line_number, value.c_str());
		return EXIT_FAILURE;
	}
	if(value.length() >= (size_t)MAX_INDEX_KEY_SIZE) {
		g_critical(article_synonym_long_err,
			get_line_number(), value.c_str(), static_cast<unsigned>(value.length()), MAX_INDEX_KEY_SIZE-1);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* Read the next element. It must be one of exp_elems.
 * parameters:
 * open_elem - expected end element, may be NULL
 *
 * return value:
 * >= 0 - index in the exp_elems array
 * rrEOF, rrError, rrEndElement
 * */
int textual_dict_parser_t::read_xml_element(const char** exp_elems, const char* open_elem)
{
	int ret = next_node();
	if(ret == 0)
		return rrEOF;
	if(ret < 0)
		return rrError;
	ret = xmlTextReaderNodeType(xml_reader);
	if(ret < 0) {
		g_critical(parser_err);
		return rrError;
	}
	const xmlReaderTypes node_type = static_cast<xmlReaderTypes>(ret);
	if(node_type == XML_READER_TYPE_END_ELEMENT) {
		xml::CharStr node_name(xmlTextReaderName(xml_reader));
		if(xmlStrEqual(get_impl(node_name), BAD_CAST open_elem))
			return rrEndElement;
		else {
			unmatched_end_element(open_elem, (const char*)get_impl(node_name));
			return rrError;
		}
	}
	if(node_type != XML_READER_TYPE_ELEMENT) {
		unexpected_node_type_element(exp_elems);
		return rrError;
	}
	xml::CharStr node_name(xmlTextReaderName(xml_reader));
	for(size_t i=0; exp_elems[i]; ++i)
		if(xmlStrEqual(get_impl(node_name), BAD_CAST exp_elems[i])) {
			return i;
		}
	unexpected_element(exp_elems);
	return rrError;
}

/* Read the next node. It must be end element.
 * parameters:
 * open_elem - opened element, != NULL
 *
 * return value:
 * rrEOF, rrError, rrEndElement
 * */
int textual_dict_parser_t::read_xml_end_element(const char* open_elem)
{
	int ret = next_node();
	if(ret == 0)
		return rrEOF;
	if(ret < 0)
		return rrError;
	ret = xmlTextReaderNodeType(xml_reader);
	if(ret < 0) {
		g_critical(parser_err);
		return rrError;
	}
	const xmlReaderTypes node_type = static_cast<xmlReaderTypes>(ret);
	if(node_type != XML_READER_TYPE_END_ELEMENT) {
		unexpected_node_type_end_element(open_elem);
		return rrError;
	}
	xml::CharStr node_name(xmlTextReaderName(xml_reader));
	if(!xmlStrEqual(get_impl(node_name), BAD_CAST open_elem)) {
		unmatched_end_element(open_elem, (const char*)get_impl(node_name));
		return rrError;
	}
	return rrEndElement;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS */
int textual_dict_parser_t::read_xml_text_item(const char* elem, std::string& value)
{
	value.clear();
	const line_number_t element_beg_line_number = get_line_number();
	int ret = read_xml_text(elem);
	if(ret == rrEOF) {
		unexpected_eof();
		return EXIT_FAILURE;
	}
	if(ret == rrError)
		return EXIT_FAILURE;
	if(ret == rrEndElement)
		return EXIT_SUCCESS;
	xml::CharStr tvalue(xmlTextReaderValue(xml_reader));
	if(!g_utf8_validate((const char*)get_impl(tvalue), -1, NULL)) {
		g_critical(element_invalid_utf8_value_err,
			element_beg_line_number, elem, get_impl(tvalue));
		return EXIT_FAILURE;
	}
	glib::CharStr t2value(g_utf8_normalize((const char*)get_impl(tvalue), -1, G_NORMALIZE_ALL_COMPOSE));
	if(!t2value) {
		g_critical(normalization_failed_err,
			element_beg_line_number, get_impl(tvalue));
		return EXIT_FAILURE;
	}
	std::string data_str;
	{	// check for invalid chars
		typedef std::list<const char*> str_list_t;
		str_list_t invalid_chars;
		if(check_stardict_string_chars(get_impl(t2value), invalid_chars)) {
			g_message(element_invalid_text_err,
				element_beg_line_number, elem, get_impl(t2value),
				print_char_codes(invalid_chars).c_str());
			fix_stardict_string_chars(get_impl(t2value), data_str);
			g_message(fixed_drop_invalid_char_msg);
		} else
			data_str.assign(get_impl(t2value));
	}
	{
		const char* beg;
		size_t len;
		trim_spaces(data_str.c_str(), beg, len);
		value.assign(beg, len);
	}
	// read end element
	ret = read_xml_end_element(elem);
	if(ret == rrEOF) {
		unexpected_eof();
		return EXIT_FAILURE;
	}
	if(ret == rrError)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

/* Read the next node. It must be text.
 * parameters:
 * open_elem - opened element, != NULL
 *
 *  return value:
 * 0 - text node is found
 * rrEOF, rrError, rrEndElement
 * */
int textual_dict_parser_t::read_xml_text(const char* open_elem)
{
	int ret = next_node();
	if(ret == 0)
		return rrEOF;
	if(ret < 0)
		return rrError;
	ret = xmlTextReaderNodeType(xml_reader);
	if(ret < 0) {
		g_critical(parser_err);
		return rrError;
	}
	const xmlReaderTypes node_type = static_cast<xmlReaderTypes>(ret);
	if(node_type == XML_READER_TYPE_END_ELEMENT) {
		xml::CharStr node_name(xmlTextReaderName(xml_reader));
		if(xmlStrEqual(get_impl(node_name), BAD_CAST open_elem))
			return rrEndElement;
		else {
			unmatched_end_element(open_elem, (const char*)get_impl(node_name));
			return rrError;
		}
	}
	if(node_type != XML_READER_TYPE_TEXT) {
		unexpected_node_type_text();
		return rrError;
	}
	return 0;
}

/* Return value:
 * EXIT_FAILURE or EXIT_SUCCESS
 *
 * EXIT_FAILURE - attribute is missing,
 * EXIT_SUCCESS - attribute value read successfully */
int textual_dict_parser_t::read_xml_attribute(const char* att_name, std::string& value)
{
	xmlChar * att_val = xmlTextReaderGetAttribute(xml_reader, (const xmlChar*) att_name);
	if(!att_val)
		return EXIT_FAILURE;
	value.assign((const char*) att_val);
	xmlFree(att_val);
	return EXIT_SUCCESS;
}

void textual_dict_parser_t::check_blank_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
		const std::string& (DictInfo:: *get_item)(void) const, void (DictInfo:: *unset_item)(void))
{
	if((norm_dict->dict_info.*is_item)() && (norm_dict->dict_info.*get_item)().empty()) {
		g_warning("info item %s is assigned an empty string.", elem);
		g_message(fixed_msg2 "Unset the item.");
		(norm_dict->dict_info.*unset_item)();
	}
}

int textual_dict_parser_t::check_new_line_info_item(const char* elem, bool (DictInfo::* is_item)(void) const,
		const std::string& (DictInfo:: *get_item)(void) const)
{
	if((norm_dict->dict_info.*is_item)() && (norm_dict->dict_info.*get_item)().find_first_of("\n\r") != std::string::npos) {
		g_critical("info item %s contains new line character.", elem);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

/* return effective content type for the current element.
 * return 0 if content type is not specified for all elements in the element stack. */
char textual_dict_parser_t::get_effective_content_type(void) const
{
	for(size_t i=elem_stack.size()-1; i>=0; --i)
		if(elem_stack[i].content_type)
			return elem_stack[i].content_type;
	return 0;
}

void textual_dict_parser_t::unexpected_node_type_element(const char** exp_elems)
{
	std::string buf;
	for(size_t i=0; exp_elems[i]; ++i) {
		buf += " ";
		buf += exp_elems[i];
	}
	g_warning("%u: Unexpected node type: %s, name: %s. Expecting elements:%s.",
		get_line_number(),
		note_type_str(xmlTextReaderNodeType(xml_reader)),
		xmlTextReaderName(xml_reader),
		buf.c_str()
		);
}

void textual_dict_parser_t::unexpected_node_type_text(void)
{
	g_warning("%u: Unexpected node type: %s, name: %s. Expecting text.",
		get_line_number(),
		note_type_str(xmlTextReaderNodeType(xml_reader)),
		xmlTextReaderName(xml_reader)
		);
}

void textual_dict_parser_t::unexpected_node_type_end_element(const char* open_elem)
{
	g_warning("%u: Unexpected node type: %s, name: %s. Expecting end element: %s.",
		get_line_number(),
		note_type_str(xmlTextReaderNodeType(xml_reader)),
		xmlTextReaderName(xml_reader),
		open_elem
		);
}

void textual_dict_parser_t::unexpected_element(const char** exp_elems)
{
	std::string buf;
	for(size_t i=0; exp_elems[i]; ++i) {
		buf += " ";
		buf += exp_elems[i];
	}
	g_warning("%u: Unexpected element: %s. Expecting elements:%s.",
		get_line_number(),
		xmlTextReaderName(xml_reader),
		buf.c_str()
		);
}

void textual_dict_parser_t::unexpected_eof(const char** exp_elems)
{
	std::string buf;
	for(size_t i=0; exp_elems[i]; ++i) {
		buf += " ";
		buf += exp_elems[i];
	}
	g_warning("Unexpected end of file. Expecting: %s.", buf.c_str());
}

void textual_dict_parser_t::unexpected_eof(void)
{
	g_warning("Unexpected end of file.");
}

void textual_dict_parser_t::unexpected_end_element(const char** exp_elems, const char* open_elem)
{
	std::string buf;
	for(size_t i=0; exp_elems[i]; ++i) {
		buf += " ";
		buf += exp_elems[i];
	}
	g_warning("%u: Unexpected end element %s. Expecting:%s.",
		get_line_number(),
		open_elem,
		buf.c_str());
}

void textual_dict_parser_t::unmatched_end_element(const char* open_elem, const char* end_elem)
{
	g_warning("%u: open element %s, end element %s.",
		get_line_number(),
		open_elem,
		end_elem
	);
}

void textual_dict_parser_t::unexpected_empty_element(void)
{
	g_warning("%u: Unexpected empty element: %s.",
		get_line_number(),
		xmlTextReaderName(xml_reader)
		);
}

void textual_dict_parser_t::unexpected_non_empty_element(void)
{
	g_warning("%u: Unexpected non-empty element: %s.",
		get_line_number(),
		xmlTextReaderName(xml_reader)
		);
}

textual_dict_parser_t::line_number_t textual_dict_parser_t::get_line_number(void)
{
	xmlNodePtr node = xmlTextReaderCurrentNode(xml_reader);
	return node ? node->line : 0;
}

/* Read next node, skipping comments and processing instructions.
 * Return value: see xmlTextReaderRead */
int textual_dict_parser_t::next_node(void)
{
	while(true) {
		int ret = xmlTextReaderRead(xml_reader);
		if(ret != 1)
			return ret;
		ret = xmlTextReaderNodeType(xml_reader);
		if(ret < 0)
			return -1;
		xmlReaderTypes reader_type = static_cast<xmlReaderTypes>(ret);
		if(reader_type == XML_READER_TYPE_COMMENT
			|| reader_type == XML_READER_TYPE_PROCESSING_INSTRUCTION) {
			continue;
		}
		if(get_custom_include()) {
			xmlNodePtr node = xmlTextReaderCurrentNode(xml_reader);
			if(!node)
				return -1;
			// see xmlreader.c, xmlTextReaderRead function, #ifdef LIBXML_XINCLUDE_ENABLED block
			if((node->type == XML_ELEMENT_NODE) &&
				(node->ns != NULL) &&
				((xmlStrEqual(node->ns->href, XINCLUDE_NS)) ||
					(xmlStrEqual(node->ns->href, XINCLUDE_OLD_NS)))) {
				if (xincctxt == NULL) {
					xincctxt = xmlXIncludeNewContext(xmlTextReaderCurrentDoc(xml_reader));
					xmlXIncludeSetFlags(xincctxt, reader_options);
				}
				const char* att_name = "href";
				std::string href;
				if(!read_xml_attribute(att_name, href)) {
					g_message(xinclude_process_msg,
						get_line_number(), href.c_str());
				} else {
					g_message(xinclude_process_msg,
						get_line_number(), href.c_str());
				}
				/*
				 * expand that node and process it
				 */
				if (xmlTextReaderExpand(xml_reader) == NULL)
					return -1;
				xmlXIncludeProcessNode(xincctxt, node);
				continue;
			}
		}
		break;
	}
	return 1;
}


int parse_textual_dict(const std::string& xmlfilename, common_dict_t* norm_dict,
		bool show_xincludes)
{
	textual_dict_parser_t parser;
	parser.set_custom_include(show_xincludes);
	return parser.parse(xmlfilename, norm_dict);
}
