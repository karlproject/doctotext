#ifndef DOCTOTEXT_PLAIN_TEXT_EXTRACTOR_H
#define DOCTOTEXT_PLAIN_TEXT_EXTRACTOR_H

#include "formatting_style.h"
#include <string>

class PlainTextExtractor
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		enum ParserType { PARSER_AUTO, PARSER_RTF, PARSER_ODF_OOXML, PARSER_XLS, PARSER_DOC, PARSER_PPT };
		PlainTextExtractor(ParserType parser_type = PARSER_AUTO);
		~PlainTextExtractor();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		void setFormattingStyle(const FormattingStyle& style);
		void setXmlParseMode(XmlParseMode mode);
		bool processFile(const std::string& file_name, std::string& text);
};

#endif
