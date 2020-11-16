/*
 * This file part of makedict - convertor from any dictionary format to any
 * http://sdcv.sourceforge.net
 * Copyright (C) 2005 Evgeniy <dushistov@mail.ru>
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

#ifndef GENERATOR_HPP
#define GENERATOR_HPP

#include <list>
#include <stack>
#include <string>
#include <map>
#include <memory>

#include "file.h"
#include "log.h"
#include "repository.h"
#include "utils.h"
#include "xml.h"

typedef std::map<std::string, std::string> StringMap;
typedef std::vector<std::string> StringList;

class GeneratorBase;

/**
 * Interface for exchange information between dictionary parsers
 * and generators.
 */
class IGeneratorDictOps {
public:
	IGeneratorDictOps(GeneratorBase& generator) : 
		generator_(generator), enc_keys_(false) {}
	virtual ~IGeneratorDictOps() {}

	virtual bool get_meta_info() = 0;
	virtual bool get_info() = 0;
	virtual void set_basename(const std::string&) = 0;
	virtual const std::string& get_dict_info(const std::string&) const = 0;
	void encode_keys(bool enc_keys) { enc_keys_ = enc_keys; }
protected:
	struct Key {
		std::vector<std::string> parts_;
		std::vector<std::string> opts_;
		void clear() { parts_.clear(); opts_.clear(); }
	} key_;
	GeneratorBase& generator_;
	bool enc_keys_;

	void generate_keys(StringList& keys);
private:	
	std::list<std::string> sample_data_;

	void sample(StringList& keys, std::vector<std::string>::size_type n);
};

/**
 * Implementation of IGeneratorDictOps to pass internal
 * represantation of dictionary through pipe.
 */
class GeneratorDictPipeOps : public IGeneratorDictOps {
public:
	GeneratorDictPipeOps(File& in, GeneratorBase& generator);
	bool get_meta_info();
	bool get_info();
	void set_dict_info(const std::string& name, const std::string& val);
	const std::string& get_dict_info(const std::string& name) const {
		static std::string empty;

		StringMap::const_iterator it = dict_info_.find(name);
		if (it == dict_info_.end())
			return empty;
		return it->second;
	}
	void set_basename(const std::string& val) {
		dict_info_["basename"] = val;
	}
private:
	enum {mmNONE, mmICON, mmBASENAME} meta_mode_;
	StringMap dict_info_;
	enum StateType { XDXF, ABBREVIATIONS, AR, ABR_DEF,
			 FULL_NAME, DESCRIPTION, K, V, OPT, NU
	};
	typedef std::map<std::string, StateType> TagTable;
	static TagTable tag_table_;
	std::stack<StateType> state_stack_;
	std::string data_;
	StringList keys_;
	File &in_;
	bool skip_mode_;

	static void XMLCALL on_meta_start(void *, const XML_Char *,
					  const XML_Char **);
	static void XMLCALL on_meta_end(void *, const XML_Char *);
	static void XMLCALL on_meta_data(void *, const XML_Char *, int);

	static void XMLCALL xml_start(void *, const XML_Char *,
				      const XML_Char **);
	static void XMLCALL xml_end(void *, const XML_Char *);
	static void XMLCALL xml_char_data(void *, const XML_Char *, int);
};

/**
 * Base class for write dictionary generators.
 */
class GeneratorBase {
public:
	GeneratorBase(bool enc_key);
	virtual ~GeneratorBase() {}
	int run(int argc, char *argv[]);
	int run(const std::string& appname, std::string& workdir);

	virtual bool on_abbr_begin() { return true; }
	virtual bool on_abbr_end() { return true; }
	virtual bool on_have_data(const StringList&, const std::string&) { return true; }
	virtual bool on_new_dict_info(const std::string&, const std::string&) { return true; }
	virtual bool on_prepare_generator(const std::string&,
					  const std::string&) = 0;
	virtual int generate() = 0;
	const std::string& format() const { return format_; }
	void reset_ops(IGeneratorDictOps *dict_ops) { 
		if (dict_ops)
			dict_ops_ = dict_ops; 
		else
			dict_ops_ = std_dict_ops_.get();
		dict_ops_->encode_keys(enc_key_);
	}
	int set_generator_options(const StringList& options);
protected:
	void set_format(const std::string& val)	{ format_ = val; }
	void set_version(const std::string& val) { version_ = val; }	

	const std::string& get_dict_info(const std::string& name) const {
		return dict_ops_->get_dict_info(name);
	}
private:
	bool parse_option(const std::string& optarg);
protected:
	StringMap generator_options_;
private:
	std::string format_;
	std::string version_;
	bool enc_key_;
	std::unique_ptr<IGeneratorDictOps> std_dict_ops_;
	IGeneratorDictOps *dict_ops_;
	std::unique_ptr<Logger> logger_;
};

class GeneratorsRepo : public CodecsRepo<GeneratorBase, GeneratorsRepo> {};

#define REGISTER_GENERATOR(Class,name) \
	class Class##name##Registrator : public ICreator<GeneratorBase> { \
	public: \
		Class##name##Registrator() { \
			GeneratorsRepo::get_instance().register_codec(#name, this); \
		} \
		GeneratorBase *create() const { \
			return new Class; \
		} \
	} Class##name##_registrator

#endif//!GENERATOR_HPP
