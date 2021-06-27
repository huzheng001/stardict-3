/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://xdxf.sourceforge.net
 * Copyright (C) 2005-2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#ifdef HAVE_LOCALE_H
#  include <clocale>
#endif

#include <glib/gi18n.h>
#include <glib.h>
#include <string>

#include "utils.h"
#include "resource.h"

#include "parser.h"

bool PipeParserDictOps::send_meta_info()
{
	out_ << "<meta_info>\n";
	for (StringMap::const_iterator p = dict_info_.begin();
	     p != dict_info_.end(); ++p)
		out_ << "<" << p->first << ">"
			  << p->second
			  << "</" << p->first << ">\n";

	out_ << "</meta_info>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::send_info()
{
	out_ <<
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<!DOCTYPE xdxf SYSTEM \"http://xdxf.sourceforge.net/xdxf_lousy.dtd\">\n"
		  << "<xdxf lang_from=\"" << dict_info_["lang_from"]
		  << "\" lang_to=\"" << dict_info_["lang_to"]
		  << "\" format=\"visual\">\n"
		"<full_name>" << dict_info_["full_name"] << "</full_name>\n"
		"<description>" << dict_info_["description"]
		  << "</description>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::abbrs_begin()
{
	out_ << "<abbreviations>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::abbrs_end()
{
	out_ << "</abbreviations>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::abbr(const StringList& keys,
			     const std::string& val)
{
	out_ << "<abr_def>";

	for (StringList::const_iterator p = keys.begin(); p != keys.end(); ++p)
		out_ << "<k>" << Strip(*p) << "</k>";

	out_ << "<v>" << val << "</v>"
		  << "</abr_def>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::article(const StringList& keys, const std::string& val,
				bool keys_in_article)
{
	out_ << "<ar>";
	if (!keys_in_article)
		for (StringList::const_iterator p = keys.begin(); p != keys.end(); ++p)
			out_ << "<k>" << Strip(*p) << "</k>\n";

	out_ << val << "</ar>\n";

	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::abbr(const std::string& key,
			     const std::string& val)
{
	out_ << "<abr_def>";

	out_ << "<k>" << Strip(key) << "</k>";

	out_ << "<v>" << val << "</v>"
		  << "</abr_def>\n";
	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::article(const std::string& key, const std::string& val,
				bool keys_in_article)
{
	out_ << "<ar>";
	if (!keys_in_article)
		out_ << "<k>" << Strip(key) << "</k>\n";

	out_ << val << "</ar>\n";

	if (!out_) {
		g_critical("Pipe write error\n");
		return false;
	}
	return true;
}

bool PipeParserDictOps::end()
{
	out_ << "</xdxf>\n";
	out_.flush();
	return !out_ ? false : true;
}

const std::string& ParserBase::format() const
{
	static std::string empty;

	StringMap::const_iterator it = parser_info_.find("format");

	if (it == parser_options_.end())
		return empty;

	return it->second;
}

ParserBase::ParserBase(bool generate_xdxf)
{
	generate_xdxf_ = generate_xdxf;
	std_dict_ops_.reset(new PipeParserDictOps(StdOut));
	dict_ops_ = std_dict_ops_.get();
}

bool ParserBase::parse_option(const std::string& optarg)
{
	std::vector<std::string> l = split(optarg, '=');
	if (l.size() != 2) {
		g_critical("Invalid usage of parser-option: didn't find '=' in option\n");
		return false;
	}
	StringMap::iterator opt_ptr = parser_options_.find(l[0]);
	if (opt_ptr == parser_options_.end()) {
		g_critical("Invalid parser option: %s\nPossible options:\n",
			      optarg.c_str());
		for (StringMap::iterator it = parser_options_.begin();
			it != parser_options_.end(); ++it)
		{
			g_critical(it->first.c_str());
			g_critical("\n");
		}
		return false;
	}
	opt_ptr->second = l[1];
	return true;
}

int ParserBase::run(int argc, char *argv[])
{
#ifdef HAVE_LOCALE_H
	setlocale(LC_ALL, "");
#endif
	gboolean show_version = FALSE, show_fmt = FALSE;
	glib::CharStr url;
	glib::CharStrArr parser_opts;
	gint verbose = 2;

	static GOptionEntry entries[] = {
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
		  _("print version information and exit"), NULL },	
		{ "is-your-format", 0, G_OPTION_FLAG_FILENAME, G_OPTION_ARG_STRING, get_addr(url),
		  _("test if file in format which accept this codec"), NULL },
		{ "input-format", 'i', 0, G_OPTION_ARG_NONE, &show_fmt,
		  _("output version information and exit"), NULL },
		{ "parser-option", 0, 0, G_OPTION_ARG_STRING_ARRAY,
		  get_addr(parser_opts), _("\"option_name=option_value\""),
		  NULL },
		{ "verbose", 0, 0, G_OPTION_ARG_INT, &verbose,
		  _("set level of verbosity"), NULL },
		{ NULL },
	};

	logger_.reset(new Logger);
	glib::OptionContext opt_cnt(g_option_context_new(_("file1 file2...")));
	g_option_context_add_main_entries(get_impl(opt_cnt), entries, NULL);
	g_option_context_set_help_enabled(get_impl(opt_cnt), TRUE);
	glib::Error err;
	if (!g_option_context_parse(get_impl(opt_cnt), &argc, &argv, get_addr(err))) {
		g_warning(_("Options parsing failed: %s\n"), err->message);
		return EXIT_FAILURE;
	}
	logger_->set_verbosity(verbose);


	if (show_version) {
		g_message(parser_info_["version"].c_str());
		g_message("\n");
		return EXIT_SUCCESS;
	}
	if (url)
		return is_my_format(get_impl(url)) ? EXIT_SUCCESS : EXIT_FAILURE;

	if (show_fmt) {
		g_message(parser_info_["format"].c_str());
		g_message("\n");
		return EXIT_SUCCESS;
	}

	if (parser_opts) {
		gchar **popts = get_impl(parser_opts);
		while (*popts) {
			if (!parse_option(*popts))
				return EXIT_FAILURE;
			++popts;
		}
	}

	if (1 == argc) {
		g_warning(_("%s: no input files\n"), argv[0]);
		return EXIT_FAILURE;
	}

	return do_run(argv[1]);
}

int ParserBase::run(const StringList& options, const std::string& url)
{
	for (StringList::const_iterator it = options.begin();
	     it != options.end(); ++it)
		if (!parse_option(*it))
			return EXIT_FAILURE;
	return do_run(url);
}

int ParserBase::do_run(const std::string& url)
{
	basename(url);
	int res = parse(url);
	if (res != EXIT_SUCCESS)
	  return res;
	if (generate_xdxf_)
		if (!dict_ops_->end())
			return EXIT_FAILURE;
	return res;
}

void ParserBase::basename(const std::string& url)
{
	std::string basename(url);
	std::string::size_type pos = basename.rfind(G_DIR_SEPARATOR);
	if (pos != std::string::npos)
		basename.erase(0, pos+1);

	if ((pos = basename.rfind('.')) != std::string::npos)
		basename.erase(pos, basename.length() - pos);
	set_dict_info("basename", basename);
}

void ParserBase::set_parser_info(const std::string& key, const std::string& val)
{
	parser_info_[key] = val;
}

bool ParserBase::meta_info()
{
	return dict_ops_->send_meta_info();
}

bool ParserBase::begin()
{
	return meta_info() && dict_ops_->send_info();
}

bool ParserBase::set_dict_info(const std::string& key, const std::string& val)
{
	return dict_ops_->set_dict_info(key, val);
}

bool ParserBase::abbrs_begin()
{
	return dict_ops_->abbrs_begin();
}

bool ParserBase::abbrs_end()
{
	return dict_ops_->abbrs_end();
}

bool ParserBase::abbr(const StringList& keys, const std::string& val)
{
	return dict_ops_->abbr(keys, val);
}

bool ParserBase::article(const StringList& keys, const std::string& val,
			 bool kia)
{
	return dict_ops_->article(keys, val, kia);
}

bool ParserBase::abbr(const std::string& key, const std::string& val)
{
	return dict_ops_->abbr(key, val);
}

bool ParserBase::article(const std::string& key, const std::string& val,
			 bool kia)
{
	return dict_ops_->article(key, val, kia);
}

int ParserBase::parse(const std::string& url)
{
	return EXIT_SUCCESS;
}

void ParserBase::remove_not_valid(std::string &str)
{
	std::string valid_data;
	char utf8buf[8];
	for (const char *p = str.c_str(); *p; p = g_utf8_next_char(p)) {
		gunichar ch = g_utf8_get_char(p);
		if (not_valid_chars.find(ch) == not_valid_chars.end()) {
			utf8buf[g_unichar_to_utf8(ch, utf8buf)] = '\0';
			valid_data += utf8buf;
		}/* else
			std::cerr<<_("Not valid character was removed, its code: ")<<ch<<std::endl;*/
	}
	str = valid_data;
}

ParserBase *ParsersRepo::create_suitable_parser(const std::string& url)
{
	CodecsMap::const_iterator it;
	for (it = codecs_.begin(); it != codecs_.end(); ++it) {
		ParserBase *res = (*it->second).create();
		if (res->is_my_format(url))
			return res;
		delete res;
	}

	return NULL;
}
