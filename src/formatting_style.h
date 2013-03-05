#ifndef DOCTOTEXT_FORMATTING_STYLE_H
#define DOCTOTEXT_FORMATTING_STYLE_H

enum TableStyle { TABLE_STYLE_TABLE_LOOK, TABLE_STYLE_ONE_ROW, TABLE_STYLE_ONE_COL, };
enum UrlStyle { URL_STYLE_TEXT_ONLY, URL_STYLE_EXTENDED, };
enum ListStyle { LIST_STYLE_NUMBER, LIST_STYLE_DASH, LIST_STYLE_DOT, };

struct FormattingStyle
{
	TableStyle table_style;
	UrlStyle url_style;
	ListStyle list_style;
};

enum XmlParseMode { PARSE_XML, FIX_XML, STRIP_XML };

#endif
