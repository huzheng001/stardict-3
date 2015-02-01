#ifndef CHARSET_CONV_HPP
#define CHARSET_CONV_HPP

#include <iconv.h>
#include <string>

/**
 * Wrapper for iconv usage.
 */
class CharsetConv {
public:
	CharsetConv() { cd_ = iconv_t(-1); }
	void workwith(const char *from, const char *to);
	CharsetConv(const char *from, const char *to) {
		cd_ = iconv_t(-1);
		workwith(from, to);
	}
	~CharsetConv() { close(); }
	bool convert(const char *, std::string::size_type, std::string&) const;
	bool convert(const std::string& str, std::string& res) const {
		return convert(str.c_str(), str.length(), res);
	}
private:
	iconv_t cd_;

	void close() {
		if (cd_ != iconv_t(-1)) {
			iconv_close(cd_);
			cd_ = iconv_t(-1);
		}
	}
};

#endif//!CHARSET_CONV_HPP
