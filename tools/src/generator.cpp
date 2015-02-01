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

#include <cstring>
#include <glib/gi18n.h>
#include <glib.h>

#include <numeric>

#include "utils.h"
#include "file.h"
#include "resource.h"
#include "xml.h"

#include "generator.h"

//#define DEBUG

GeneratorDictPipeOps::TagTable GeneratorDictPipeOps::tag_table_;


GeneratorDictPipeOps::GeneratorDictPipeOps(File &in, GeneratorBase& gen) :
	IGeneratorDictOps(gen), in_(in)
{
	meta_mode_ = mmNONE;
	skip_mode_ = false;

	if (!tag_table_.empty())
		return;

	tag_table_["full_name"] = FULL_NAME;
	tag_table_["description"] = DESCRIPTION;
	tag_table_["abr_def"] = ABR_DEF;
	tag_table_["ar"] = AR;
	tag_table_["k"] = K;
	tag_table_["v"] = V;
	tag_table_["opt"] = OPT;
	tag_table_["nu"] = NU;
	tag_table_["xdxf"] = XDXF;
	tag_table_["abbreviations"] = ABBREVIATIONS;
}

bool GeneratorDictPipeOps::get_meta_info()
{
	std::string line;

	xml::Parser meta_parser(on_meta_start, on_meta_end, on_meta_data, this);

	while (File::getline(in_, line) &&
	       line.find("</meta_info>") == std::string::npos) {
		line += '\n';
		if (!meta_parser.parse_line(line))
			return false;
	}

	if (!meta_parser.finish(line))
		return false;

	return true;
}

bool GeneratorDictPipeOps::get_info()
{
	std::string line;

	xml::Parser xdxf_parser(xml_start, xml_end, xml_char_data, this);

	while (File::getline(in_, line)) {
#ifdef DEBUG
		StdOut << "GeneratorDictPipeOps::get_info(): line: "
		       << line << "\n";
#endif
		line += '\n';
		if (!xdxf_parser.parse_line(line))
			return false;
	}
	if (!xdxf_parser.finish(line))
		return false;
	return true;
}

/* Parameters:
 enc_key - should keys be encoded or not */
GeneratorBase::GeneratorBase(bool enc_key)
{
	enc_key_ = enc_key;
	std_dict_ops_.reset(new GeneratorDictPipeOps(StdIn, *this));
	dict_ops_ = std_dict_ops_.get();
	dict_ops_->encode_keys(enc_key_);
}

int GeneratorBase::run(int argc, char *argv[])
{
#ifdef HAVE_LOCALE_H
	setlocale(LC_ALL, "");
#endif
	gint verbose = 2;
	gboolean show_version = FALSE, show_fmt = FALSE;
	glib::CharStr work_dir, basename;
	glib::CharStrArr generator_opts;

	static GOptionEntry entries[] = {
		{ "version", 'v', 0, G_OPTION_ARG_NONE, &show_version,
		  _("print version information and exit"), NULL },		
		{ "output-format", 'o', 0, G_OPTION_ARG_NONE, &show_fmt,
		  _("display supported output format and exit"), NULL },
		{ "work-dir", 'd', 0, G_OPTION_ARG_STRING, get_addr(work_dir),
		  _("use parameter as current directory"), NULL },
		{ "verbose", 0, 0, G_OPTION_ARG_INT, &verbose,
		  _("set level of verbosity"), NULL },
		{ "basename", 0, 0, G_OPTION_ARG_STRING, get_addr(basename),
		  _("base for name of newly create dictionary"), NULL },
		{ "generator-option", 0, 0, G_OPTION_ARG_STRING_ARRAY,
			get_addr(generator_opts), _("\"option_name=option_value\""),
			NULL },
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
		g_message(version_.c_str());
		g_message("\n");
		return EXIT_SUCCESS;
	}
	if (show_fmt) {
		g_message(format_.c_str());
		g_message("\n");
		return EXIT_SUCCESS;
	}

	if (basename)
		dict_ops_->set_basename(get_impl(basename));
	std::string str_workdir;
	if (work_dir)
		str_workdir = get_impl(work_dir);

	if (generator_opts) {
		gchar **popts = get_impl(generator_opts);
		while (*popts) {
			if (!parse_option(*popts))
				return EXIT_FAILURE;
			++popts;
		}
	}

	return run(argv[0], str_workdir);
}

int GeneratorBase::run(const std::string& appname, std::string& workdir)
{
	if (workdir.empty()) {
		std::string::size_type pos = appname.rfind(G_DIR_SEPARATOR);

		if (pos != std::string::npos)
			workdir.assign(appname, 0, pos);
		else
			workdir = ".";
	}

	if (!dict_ops_->get_meta_info())
		return EXIT_FAILURE;
	if (!on_prepare_generator(workdir, dict_ops_->get_dict_info("basename")))
		return EXIT_FAILURE;
	if (!dict_ops_->get_info())
		return EXIT_FAILURE;
	return generate();
}

int GeneratorBase::set_generator_options(const StringList& options)
{
	for (StringList::const_iterator it = options.begin();
	     it != options.end(); ++it)
		if (!parse_option(*it))
			return EXIT_FAILURE;
	return EXIT_SUCCESS;
}

bool GeneratorBase::parse_option(const std::string& optarg)
{
	std::vector<std::string> l = split(optarg, '=');
	if (l.size() != 2) {
		g_critical("Invalid usage of generator-option: didn't find '=' in option\n");
		return false;
	}
	StringMap::iterator opt_ptr = generator_options_.find(l[0]);
	if (opt_ptr == generator_options_.end()) {
		g_critical("Invalid generator option: %s\nPossible options:\n",
			      optarg.c_str());
		for (StringMap::iterator it = generator_options_.begin();
			it != generator_options_.end(); ++it)
		{
			g_critical(it->first.c_str()); 
			g_critical("\n");
		}
		return false;
	}
	opt_ptr->second = l[1];
	return true;
}

void GeneratorDictPipeOps::set_dict_info(const std::string& name, const std::string& val)
{
	dict_info_[name] = val;
	generator_.on_new_dict_info(name, val);
}

void XMLCALL GeneratorDictPipeOps::on_meta_start(void *user_arg,
						 const XML_Char *name,
						 const XML_Char **atts)
{
	GeneratorDictPipeOps *mg =
		static_cast<GeneratorDictPipeOps *>(user_arg);
	if (strcmp(name, "icon") == 0)
		mg->meta_mode_ = mmICON;
	else if (strcmp(name, "basename") == 0)
		mg->meta_mode_ = mmBASENAME;
}

void XMLCALL GeneratorDictPipeOps::on_meta_end(void *user_arg,
					       const XML_Char *name)
{
	GeneratorDictPipeOps *mg =
		static_cast<GeneratorDictPipeOps *>(user_arg);
	if (strcmp(name, "icon")==0 || strcmp(name, "basename")==0)
		mg->meta_mode_ = mmNONE;
}

void XMLCALL GeneratorDictPipeOps::on_meta_data(void *user_arg,
						const XML_Char *s, int len)
{
	GeneratorDictPipeOps *mg =
		static_cast<GeneratorDictPipeOps *>(user_arg);

	switch (mg->meta_mode_) {
	case mmICON:
		mg->dict_info_["icon"] += std::string(s, len);
		break;
	case mmBASENAME:
		mg->dict_info_["basename"] += std::string(s, len);
		break;
	default:
		/*nothing*/;
	}
}


void XMLCALL GeneratorDictPipeOps::xml_start(void *user_arg,
					     const XML_Char *name,
					     const XML_Char **atts)
{
#ifdef DEBUG
	StdErr << "xml_start: name=" << name << "\n";
#endif

	GeneratorDictPipeOps *gen =
		static_cast<GeneratorDictPipeOps *>(user_arg);
	TagTable::const_iterator it = tag_table_.find(name);

	if (it == tag_table_.end() && gen->state_stack_.empty())
		return;

	if (it == tag_table_.end()) {
		gen->data_ += std::string("<") + name;
		for (int i = 0; atts[i]; i += 2)
			gen->data_ += std::string(" ") + atts[i] +"=\"" +
				atts[i+1] + "\"";
		gen->data_ += ">";
	} else {
		switch (it->second) {
		case K:
			if (gen->state_stack_.top() == AR)
				gen->data_ += std::string("<")+name+">";
			gen->key_.parts_.push_back(std::string());
			break;
		case NU:
			gen->data_ += "<nu />";
			gen->skip_mode_ = !gen->skip_mode_;
			break;
		case V:
			gen->data_.clear();
			break;
		case OPT:
			if (!gen->skip_mode_) {
				gen->data_ += std::string("<") + name + ">";
				gen->key_.opts_.push_back(std::string());
			}
			break;
		case ABR_DEF:
		case AR:
			gen->data_.clear();
			break;
		case ABBREVIATIONS:
			gen->generator_.on_abbr_begin();
			break;
		case XDXF:

			for (int i = 0; atts[i]; i += 2)
				if (strcmp(atts[i], "lang_from") == 0) {
					if (atts[i + 1])
						gen->set_dict_info("lang_from", atts[i + 1]);
				} else if (strcmp(atts[i], "lang_to") == 0) {
					if (atts[i + 1])
						gen->set_dict_info("lang_to", atts[i + 1]);
				}
			break;
		default:
			/*nothing*/;
		}
		gen->state_stack_.push(it->second);
#ifdef DEBUG
		StdErr << "xml_start: tag added, data=" << gen->data_
			  << "\n";
#endif
	}
}

void XMLCALL GeneratorDictPipeOps::xml_end(void *userData, const XML_Char *name)
{
#ifdef DEBUG
	StdErr<<"xml_end: name="<<name<<"\n";
#endif

	GeneratorDictPipeOps *gen = static_cast<GeneratorDictPipeOps *>(userData);

	if (gen->state_stack_.empty())
		return;

	TagTable::const_iterator it = tag_table_.find(name);

	if (it == tag_table_.end() ||
	    gen->state_stack_.top() != it->second) {
		gen->data_ += std::string("</") + name + ">";
	} else {
		switch (gen->state_stack_.top()) {
		case FULL_NAME:
		case DESCRIPTION:
			gen->set_dict_info(it->first, gen->data_);
			gen->data_.clear();
			break;
		case OPT:
			if (!gen->skip_mode_) {
				gen->data_ += std::string("</") + name + ">";
				gen->key_.parts_.push_back(std::string());
			}
			break;
		case K:
			gen->data_ += std::string("</") + name + ">";
			gen->generate_keys(gen->keys_);
			gen->key_.clear();
			break;
		case AR:
		case ABR_DEF:
			gen->generator_.on_have_data(gen->keys_, gen->data_);
			gen->keys_.clear();
			gen->data_.clear();
			break;
		case ABBREVIATIONS:
			gen->generator_.on_abbr_end();
			break;
		default:
			/*nothing*/;
		}
		gen->state_stack_.pop();
	}
}

void XMLCALL GeneratorDictPipeOps::xml_char_data(void *userData,
						 const XML_Char *s, int len)
{
	GeneratorDictPipeOps *gen = static_cast<GeneratorDictPipeOps *>(userData);

	if (gen->state_stack_.empty() || gen->state_stack_.top() == XDXF ||
	    gen->state_stack_.top() == ABBREVIATIONS)
		return;

#ifdef DEBUG
	StdErr<<"xml_char_data, data="<<gen->data_<<"\n";
#endif

	std::string data;
	xml::encode(std::string(s, len), data);
	gen->data_ += data;
	switch (gen->state_stack_.top()) {
	case K:
		if (!gen->skip_mode_)
			gen->key_.parts_.back() += std::string(s, len);
		break;
	case OPT:
		if (!gen->skip_mode_)
			gen->key_.opts_.back() += std::string(s, len);
		break;
	default:
		/*nothing*/;
	}
}



void IGeneratorDictOps::sample(StringList& keys, StringList::size_type n)
{
	size_t index = key_.opts_.size() - n;
	if (index < key_.parts_.size())
		sample_data_.push_back(key_.parts_[index]);
#ifdef DEBUG
	StdErr << "n=" << n << "\n";
	for (std::list<std::string>::const_iterator it = sample_data_.begin();
	     it != sample_data_.end(); ++it)
		StdErr << "sample_data: " << *it << "\n";
#endif
	if (n == 0) {
		std::string res =
			std::accumulate(sample_data_.begin(), sample_data_.end(),
					std::string());
		keys.push_back(res);
		if (index < key_.parts_.size())
			sample_data_.pop_back();
#ifdef DEBUG
		StdErr << "key: " << res << "\n";
#endif
		return;
	}

	sample(keys, n - 1);

	sample_data_.push_back(key_.opts_[key_.opts_.size() - n]);
	sample(keys, n - 1);
	sample_data_.pop_back();
	sample_data_.pop_back();
}

void IGeneratorDictOps::generate_keys(StringList& keys)
{
	if (key_.parts_.empty()) {
		g_critical("Internal error, can not generate key list, there are no parts of key\n");
		return;
	}
	if (key_.parts_.back().empty())
		key_.parts_.pop_back();
	sample_data_.clear();
#ifdef DEBUG
	StdErr << "key_.parts.size() = " << key_.parts_.size() << "\n";
	StdErr << "key_.opts.size() = " << key_.opts_.size() << "\n";
#endif
	sample(keys, key_.opts_.size());

	for (StringList::iterator it = keys.begin(); it != keys.end(); ++it) {
		strip(*it);
		if (enc_keys_)
			xml::decode(*it);
	}
}
