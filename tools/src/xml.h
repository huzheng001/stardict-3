#ifndef _XML_HPP_
#define _XML_HPP_

#include <string>
#include <expat.h>

namespace xml {
	extern void add_and_encode(std::string& str, char ch);
	extern void encode(const std::string& str, std::string& res);
	extern void decode(std::string& str);

	class Parser {
	public:
		Parser(XML_StartElementHandler on_start,
		       XML_EndElementHandler on_end,
		       XML_CharacterDataHandler on_char,
		       void *data);
		~Parser() { XML_ParserFree(xmlp); }
		void xml_error(const std::string& line) const;
		bool parse_line(const std::string& line) {
			if (XML_Parse(xmlp, line.c_str(), int(line.size()), 0) != XML_STATUS_OK) {
				xml_error(line);
				return false;
			}

			return true;
		}
		bool finish(const std::string& line) {
			if (XML_Parse(xmlp, line.c_str(), int(line.size()), 1) != XML_STATUS_OK) {
				xml_error(line);
				return false;
			}

			return true;
		}
	private:
		XML_Parser xmlp;
	};
};

#endif//!_XML_HPP_
