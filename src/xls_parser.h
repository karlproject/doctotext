#ifndef DOCTOTEXT_XLS_PARSER_H
#define DOCTOTEXT_XLS_PARSER_H

#include <string>

struct FormattingStyle;

class XLSParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		XLSParser(const std::string& file_name);
		~XLSParser();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		bool isXLS();
		std::string plainText(const FormattingStyle& formatting);
		bool error();
};

#endif
