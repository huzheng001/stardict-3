#ifndef _NORMALIZE_TAGS_HPP_
#define _NORMALIZE_TAGS_HPP_

#include <glib.h>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

struct TagInfo {
	std::string  open_sig, close_sig;
	std::string  open_val, close_val;
	enum {
		tB,
		tI,
		tTranscription,
		tColor,
		tAbr,
		tSub,
		tSup,
		tComment,
		tDtrn,
		tExample,
		tKref,
		tBlockquote
	};

	int code;
	bool have_value;
  
	TagInfo(std::string op_sig, std::string cl_sig, std::string op_val, std::string cl_val,
		int c, bool h=false) 
		: open_sig(op_sig), close_sig(cl_sig), open_val(op_val),
		  close_val(cl_val), code(c), have_value(h) {}
	bool begin_from_this(const char *p) {
		return strncmp(p, open_sig.c_str(), open_sig.length())==0;
	}
	bool end_on_this(const char *p) {
		return strncmp(p, close_sig.c_str(), close_sig.length())==0;
	}
	bool have_such_code(int val) {
		return code==val;
	}
};

typedef std::vector<TagInfo> TagInfoList;

struct Tag {
	TagInfoList::iterator info;
	std::string::size_type p;
	std::string value;
	size_t timestamp_;
  
	Tag(TagInfoList::iterator info_, std::string::size_type p_, size_t t)
		: info(info_), p(p_), timestamp_(t)
		{
		}
  
	Tag(TagInfoList::iterator info_, std::string::size_type p_,
	    const std::string& val, size_t t):
		info(info_), p(p_), value(val), timestamp_(t)
		{
		}
  
	Tag(TagInfoList::iterator info_, std::string::size_type p_,
	    const char *beg, size_t len, size_t t):
		info(info_), p(p_), value(beg, len), timestamp_(t)
		{
		}
  
	bool have_such_code(int val) {
		return info->code==val;
	}
};

typedef std::vector<Tag> TagStack;

struct Section {
	Tag begin;
	std::string::size_type end;
  
	Section(Tag b, std::string::size_type e)
		: begin(b), end(e)
		{
		}
  
	Section(TagStack::reverse_iterator pb, std::string::size_type e) 
		: begin(*pb), end(e)
		{
		}

	friend bool operator<(const Section& lh, const Section& rh) {
		if (lh.begin.p != rh.begin.p)
			return lh.begin.p < rh.begin.p;
		if (lh.end != rh.end)
			return lh.end > rh.end;
		return lh.begin.timestamp_ < rh.begin.timestamp_;
	}
	friend bool operator==(const Section& lh, const Section& rh) {
		return lh.begin.p == rh.begin.p && 
			lh.begin.timestamp_ == rh.begin.timestamp_ &&
			lh.end == rh.end;
	}
};

extern void tag_value(const char *&p, std::string& val);

/**
 * Encapsulate mechanism to solve such task:
 * convert &lt;tag1&gt; aaa &lt;tag2&gt; bbb &lt;/tag1&gt; ccc &lt;/tag2&gt;
 * to normal XML.
 */
class NormalizeTags {
public:
	NormalizeTags(TagInfoList& taginfo_list_) : 
		taginfo_list(taginfo_list_), cur_timestamp_(0) 
		{
		}
	bool add_open_tag(std::string &resstr, const char *&p);
	bool add_close_tag(std::string &resstr, const char *&p);
	void operator()(std::string& resstr, std::string& datastr);
private:
	TagInfoList& taginfo_list;
	TagStack open_tags;
	std::vector<Section> sections;
	size_t cur_timestamp_;

	void add_section(const Section& sec);
};

#endif//!_NORMALIZE_TAGS_HPP_
