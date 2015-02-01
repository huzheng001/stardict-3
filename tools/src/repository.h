#ifndef __REPOSITORY_HPP__
#define __REPOSITORY_HPP__

#include <map>
#include <vector>
#include <string>

typedef std::vector<std::string> StringList;

template <class T>
class ICreator {
public:
	virtual ~ICreator() {}
	virtual T *create() const = 0;
};

/**
 * Template to writing factories to create parsers and generators.
 */
template <class T, typename ResType>
class CodecsRepo {
public:
	static ResType& get_instance();
	bool register_codec(const std::string& name, ICreator<T> *codec);
	T *create_codec(const std::string& name);
	const StringList& supported_formats();
	bool supported_format(const std::string& name);
protected:
	typedef std::map<std::string, ICreator<T> *> CodecsMap;
	CodecsMap codecs_;
	StringList names_list_;
};

template <class T, typename R>
const StringList& CodecsRepo<T, R>::supported_formats()
{
	if (!names_list_.empty())
		return names_list_;
	for (typename CodecsMap::const_iterator it = codecs_.begin();
	     it != codecs_.end(); ++it)
		names_list_.push_back(it->first);
	return names_list_;
}

template <class T, typename R>
bool CodecsRepo<T, R>::supported_format(const std::string& name)
{
	return codecs_.find(name) != codecs_.end();
}


template <class T, typename R>
R& CodecsRepo<T, R>::get_instance()
{
	static R repo;
	return repo;
}

template <class T, typename R>
T *CodecsRepo<T, R>::create_codec(const std::string& name)
{
	typename CodecsMap::const_iterator it = codecs_.find(name);
	if (it == codecs_.end())
		return NULL;
	return (*it->second).create();
}

template <class T, typename R>
bool CodecsRepo<T, R>::register_codec(const std::string& name, ICreator<T> *codec)
{	
	if (codecs_.find(name) != codecs_.end())
		return false;
	codecs_[name] = codec;
	return true;
}

#endif//!__REPOSITORY_HPP__
