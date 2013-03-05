#ifndef DOCTOTEXT_ODFOOXML_PARSER_H
#define DOCTOTEXT_ODFOOXML_PARSER_H

#include "formatting_style.h"
#include <string>
#include <ustring.h>
using namespace wvWare;

class ODFOOXMLParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		ODFOOXMLParser(const std::string &file_name);
		~ODFOOXMLParser();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		bool isODFOOXML();
		std::string plainText(XmlParseMode mode, FormattingStyle options);
		bool error();
};

#endif

 
