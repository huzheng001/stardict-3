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

#include <string>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <glib/gstdio.h>
#include <glib.h>
#include "lib_stardict_bin2text.h"
#include "lib_binary_dict_parser.h"
#include "libcommon.h"
#include "lib_common_dict.h"
#include "lib_binary_parser_unify.h"

/* escape text that will be placed in xml comment.
 * "--" is replaced with "- -". */
static std::string escape_xml_comment(const std::string& str)
{
	const size_t len = str.length();
	std::string res;
	res.reserve(len);
	std::string::size_type beg=0, end;
	while(true) {
		end = str.find("--", beg);
		if(end == std::string::npos) {
			res.append(str, beg, len-beg);
			break;
		} else {
			res.append(str, beg, end - beg);
			/* Replace only the first minus.
			 * Why? Consider the case when minuses form a sequence: "-------------". */
			res.append("- ");
			beg = end + 1;
		}
	}
	return res;
}

class textual_dict_gen_t
{
public:
	textual_dict_gen_t(void);
	int generate(const std::string& xmlfilenamemain, common_dict_t *parsed_norm_dict);

	void set_chunk_size(size_t size)
	{
		chunk_size = size;
	}
private:
	int prepare_output_files(void);
	void close_output_files(void);
	void clear(void);
	int generate_header(void);
	int generate_footer(void);
	int generate_info(void);
	int generate_contents(void);
	int generate_article(const article_data_t& article);
	int generate_field_CDATA(const char type_id, const char* data);
	int generate_field_escaped(const char type_id, const char* data);
	int generate_field_unescaped(const char type_id, const char* data);
	int generate_field_r(const resource_vect_t& resources);
	int xmlfilemain_puts(const glib::CharStr& str);
	int xmlfilemain_puts(const char* str);
	int contents_puts(const glib::CharStr& str);
	int contents_puts(const char* str);
	std::string get_chunk_file_name(size_t chunk_num) const;
	int prepare_chunk_file(void);
	int generate_chunk_file_header(void);
	int generate_chunk_file_footer(void);
	bool use_chunk_file(void) const
	{
		return chunk_size > 0;
	}
	bool is_chunk_file_full(void) const;
	int generate_chunk_include(size_t first_article_num);
	common_dict_t *parsed_norm_dict;
	std::string xmlfilenamemain;
	clib::File xmlfilemain;
	clib::File xmlfilechunk;
	size_t chunk_size;
	size_t cur_chunk_num;
};

textual_dict_gen_t::textual_dict_gen_t(void)
:
	parsed_norm_dict(NULL),
	chunk_size(0)
{

}

int textual_dict_gen_t::generate(const std::string& xmlfilenamemain,
	common_dict_t *parsed_norm_dict)
{
	this->xmlfilenamemain = xmlfilenamemain;
	this->parsed_norm_dict = parsed_norm_dict;
	cur_chunk_num = 0;
	auto_executor_t<textual_dict_gen_t> auto_exec(*this, &textual_dict_gen_t::clear);
	if(prepare_output_files())
		return EXIT_FAILURE;
	if(generate_header())
		return EXIT_FAILURE;
	if(generate_info())
		return EXIT_FAILURE;
	if(generate_contents())
		return EXIT_FAILURE;
	if(generate_footer())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::prepare_output_files(void)
{
	close_output_files();
	xmlfilemain.reset(g_fopen(xmlfilenamemain.c_str(), "wb"));
	if(!xmlfilemain) {
		g_critical(open_write_file_err, xmlfilenamemain.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

void textual_dict_gen_t::close_output_files(void)
{
	xmlfilemain.reset(NULL);
	xmlfilechunk.reset(NULL);
}

void textual_dict_gen_t::clear(void)
{
	close_output_files();
	xmlfilenamemain.clear();
	parsed_norm_dict = NULL;
}

int textual_dict_gen_t::generate_header(void)
{
	if(0 > xmlfilemain_puts(UTF8_BOM
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<stardict xmlns:xi=\"http://www.w3.org/2003/XInclude\">\n")) {
		g_critical(write_file_err, xmlfilenamemain.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_footer(void)
{
	xmlfilemain_puts("</stardict>\n");
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_info(void)
{
	const DictInfo& dict_info = parsed_norm_dict->dict_info;

	/* print all field here even if they are blank */
	glib::CharStr temp(g_markup_printf_escaped(
		"<info>\n"
		"<version>%s</version>\n"
		"<bookname>%s</bookname>\n"
		"<author>%s</author>\n"
		"<email>%s</email>\n"
		"<website>%s</website>\n"
		"<description>%s</description>\n"
		"<date>%s</date>\n"
		"<dicttype>%s</dicttype>\n"
		"</info>\n\n\n"
		, dict_info.get_version().c_str()
		, dict_info.get_bookname().c_str()
		, dict_info.get_author().c_str()
		, dict_info.get_email().c_str()
		, dict_info.get_website().c_str()
		, dict_info.get_description().c_str()
		, dict_info.get_date().c_str()
		, dict_info.get_dicttype().c_str()
	));
	xmlfilemain_puts(get_impl(temp));
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_contents(void)
{
	for(size_t i=0; i<parsed_norm_dict->articles.size(); ++i) {
		if(use_chunk_file() && !xmlfilechunk) {
			if(prepare_chunk_file())
				return EXIT_FAILURE;
			if(generate_chunk_file_header())
				return EXIT_FAILURE;
			if(generate_chunk_include(i))
				return EXIT_FAILURE;
		}
		generate_article(parsed_norm_dict->articles[i]);
		if(use_chunk_file() && is_chunk_file_full()) {
			if(generate_chunk_file_footer())
				return EXIT_FAILURE;
			xmlfilechunk.reset(NULL);
			++cur_chunk_num;
		}
	}
	if(use_chunk_file() && !!xmlfilechunk) {
		if(generate_chunk_file_footer())
			return EXIT_FAILURE;
		xmlfilechunk.reset(NULL);
	}
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_article(const article_data_t& article)
{
	contents_puts("<article>\n");
	glib::CharStr temp;
	temp.reset(g_markup_printf_escaped("<key>%s</key>\n", article.key.c_str()));
	contents_puts(temp);
	for(size_t i=0; i<article.synonyms.size(); ++i) {
		temp.reset(g_markup_printf_escaped("<synonym>%s</synonym>\n", article.synonyms[i].c_str()));
		contents_puts(temp);
	}
	std::vector<char> buf;
	for(size_t i=0; i<article.definitions.size(); ++i) {
		const article_def_t& def = article.definitions[i];
		const char type_id = def.type;
		if(g_ascii_islower(type_id)) {
			if(type_id == 'r') {
				generate_field_r(def.resources);
			} else {
				buf.resize(def.size + 1);
				if(parsed_norm_dict->read_data(&buf[0], def.size, def.offset))
					return EXIT_FAILURE;
				buf[def.size] = '\0';
				generate_field_CDATA(type_id, &buf[0]);
			}
		} else {
			g_message("Index item %s, type = '%c'. Binary data types are not supported presently. Skipping.",
				article.key.c_str(), type_id);
		}
	}
	contents_puts("</article>\n\n");
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_field_CDATA(const char type_id, const char* data)
{
	std::string contents;
	contents += "\n<definition type=\"";
	contents += type_id;
	contents += "\">\n";
	const char* beg, * end;
	beg = data;
	while(*beg) {
		end = strstr(beg, "]]>");
		if(!end)
			end = strchr(beg, '\0');
		contents.append("<![CDATA[");
		contents.append(beg, end - beg);
		contents.append("]]>");
		if(*end == '\0')
			break;
		contents.append("]]&gt;");
		beg = end + 3;
		// ...]]>]]&gt;<![CDATA[...
	}
	contents += "\n</definition>\n";
	return contents_puts(contents.c_str());
}

int textual_dict_gen_t::generate_field_escaped(const char type_id, const char* data)
{
	static const char* const definition_pattern =
		"\n<definition type=\"%c\">\n"
		"%s\n"
		"</definition>\n";
	glib::CharStr temp;
	temp.reset(g_markup_printf_escaped(definition_pattern
		, type_id
		, data
	));
	return contents_puts(temp);
}

int textual_dict_gen_t::generate_field_unescaped(const char type_id, const char* data)
{
	static const char* const definition_pattern =
		"\n<definition type=\"%c\">\n"
		"%s\n"
		"</definition>\n";
	glib::CharStr temp;
	temp.reset(g_strdup_printf(definition_pattern
		, type_id
		, data
	));
	return contents_puts(temp);
}

int textual_dict_gen_t::generate_field_r(const resource_vect_t& resources)
{
	glib::CharStr temp;
	contents_puts("\n<definition-r>\n");
	for(size_t i=0; i<resources.size(); ++i) {
		temp.reset(g_markup_printf_escaped("<resource type=\"%s\" key=\"%s\"/>\n"
			, resources[i].type.c_str()
			, resources[i].key.c_str()
		));
		contents_puts(temp);
	}
	contents_puts("</definition-r>\n");
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::xmlfilemain_puts(const glib::CharStr& str)
{
	int res = fputs(get_impl(str), get_impl(xmlfilemain));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int textual_dict_gen_t::xmlfilemain_puts(const char* str)
{
	int res = fputs(str, get_impl(xmlfilemain));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int textual_dict_gen_t::contents_puts(const glib::CharStr& str)
{
	int res = fputs(get_impl(str), get_impl(use_chunk_file() ? xmlfilechunk : xmlfilemain));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int textual_dict_gen_t::contents_puts(const char* str)
{
	int res = fputs(str, get_impl(use_chunk_file() ? xmlfilechunk : xmlfilemain));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

std::string textual_dict_gen_t::get_chunk_file_name(size_t chunk_num) const
{
	const std::string& filename = xmlfilenamemain;
	std::string basefilename, ext;
	std::string::size_type pos = filename.find_last_of("." G_DIR_SEPARATOR_S);
	if(pos != std::string::npos && filename[pos] == '.') {
		basefilename = filename.substr(0, pos);
		ext = filename.substr(pos);
	} else {
		basefilename = filename;
		ext = ".xml";
	}
	std::stringstream sstr;
	sstr << "-part" << std::setfill('0') << std::setw(10) << chunk_num;
	return basefilename + sstr.str() + ext;
}

int textual_dict_gen_t::prepare_chunk_file(void)
{
	const std::string chuck_file_name = get_chunk_file_name(cur_chunk_num);
	xmlfilechunk.reset(g_fopen(chuck_file_name.c_str(), "wb"));
	if(!xmlfilechunk) {
		g_critical(open_write_file_err, chuck_file_name.c_str());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_chunk_file_header(void)
{
	const char * str = UTF8_BOM "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
	"<contents>\n";
	int res = fputs(str, get_impl(xmlfilechunk));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

int textual_dict_gen_t::generate_chunk_file_footer(void)
{
	const char * str = "</contents>\n";
	int res = fputs(str, get_impl(xmlfilechunk));
	return (res < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* returns true if size of the chunk file is off the limit. */
bool textual_dict_gen_t::is_chunk_file_full(void) const
{
	return static_cast<size_t>(ftell(get_impl(xmlfilechunk))) >= chunk_size;
}

int textual_dict_gen_t::generate_chunk_include(size_t first_article_num)
{
	{	// comments
		std::stringstream str;
		str << "part: " << cur_chunk_num << "\n"
			<< "first key: " << parsed_norm_dict->articles[first_article_num].key;
		std::string temp = std::string("\n<!-- ") + escape_xml_comment(str.str()) + " -->\n";
		if(xmlfilemain_puts(temp.c_str()))
			return EXIT_FAILURE;
	}
	{	// xi:include
		std::string chunk_file_name(get_chunk_file_name(cur_chunk_num));
		std::string::size_type pos = chunk_file_name.find_last_of(G_DIR_SEPARATOR);
		if(pos != std::string::npos)
			chunk_file_name.erase(0, pos + 1);
		// chunk_file_name now contains only file name
		glib::CharStr temp(
			g_markup_printf_escaped("<xi:include href=\"%s\"/>\n", chunk_file_name.c_str())
		);
		if(xmlfilemain_puts(get_impl(temp)))
			return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int stardict_bin2text(const std::string& ifofilename, const std::string& xmlfilename,
		size_t chunk_size)
{
	if(!is_path_end_with(ifofilename, ".ifo")) {
		g_critical(unsupported_file_type_err, ifofilename.c_str());
		return EXIT_FAILURE;
	}
	// do not check for xmlfilename extension
	binary_dict_parser_t parser;
	parser.set_fix_errors(true);
	if(VERIF_RESULT_FATAL <= parser.load(ifofilename)) {
		g_critical("Dictionary load failed.");
		return EXIT_FAILURE;
	}
	g_message("Dictionary loaded.");
	common_dict_t parsed_norm_dict;
	if(convert_to_parsed_dict(parsed_norm_dict, parser))
		return EXIT_FAILURE;
	g_message("Generating xml file...");
	textual_dict_gen_t generator;
	generator.set_chunk_size(chunk_size);
	if(generator.generate(xmlfilename, &parsed_norm_dict)) {
		g_critical("Generating xml file failed.");
		return EXIT_FAILURE;
	}
	g_message("xml file's created.");
	return EXIT_SUCCESS;
}
