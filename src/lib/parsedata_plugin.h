#ifndef _STARDICT_PARSEDATA_PLUGIN_H_
#define _STARDICT_PARSEDATA_PLUGIN_H_

#include <gtk/gtk.h>
#include <string>
#include <list>

enum ParseResultItemType {
	ParseResultItemType_mark,
	ParseResultItemType_link,
	ParseResultItemType_res,
	ParseResultItemType_widget,
	/* The _FormatBeg and _FormatEnd types should be used for formats that
	 * cannot be expressed in pango markup, for example, indentation. The part
	 * of the text undergoing formating may contain other formating
	 * specifications inside. */
	ParseResultItemType_FormatBeg,
	ParseResultItemType_FormatEnd,
};

enum ParseResultItemFormatType {
	 ParseResultItemFormatType_Indent,
/* May be extended with LeftMarginIndent, RightMarginIndent types with
 * explicitly specified indentation size.
 * */
};

struct ParseResultMarkItem {
	std::string pango;
};

struct LinkDesc {
	std::string::size_type pos_;
	std::string::size_type len_;
	std::string link_;
	LinkDesc(std::string::size_type pos, std::string::size_type len, const std::string &link):
		pos_(pos), len_(len), link_(link) {}
};

typedef std::list<LinkDesc> LinksPosList;

struct ParseResultLinkItem {
	std::string pango;
	LinksPosList links_list;
};

struct ParseResultResItem {
	std::string type;
	std::string key;
};

struct ParseResultWidgetItem {
	GtkWidget *widget;
};

struct ParseResultFormatBegItem {
	ParseResultItemFormatType type;
};

struct ParseResultFormatEndItem {
	ParseResultItemFormatType type;
};

struct ParseResultItem {
	ParseResultItemType type;
	union {
		ParseResultMarkItem *mark;
		ParseResultLinkItem *link;
		ParseResultResItem *res;
		ParseResultWidgetItem *widget;
		ParseResultFormatBegItem* format_beg;
		ParseResultFormatEndItem* format_end;
	};
};

struct ParseResult {
	~ParseResult();
	void clear();
	std::list<ParseResultItem> item_list;
};

struct StarDictParseDataPlugInObject{
	StarDictParseDataPlugInObject();

	typedef bool (*parse_func_t)(const char *p, unsigned int *parsed_size, ParseResult &result, const char *oword);
	parse_func_t parse_func;
};

#endif
