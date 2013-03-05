#ifndef DOCTOTEXT_DOC_PARSER_H
#define DOCTOTEXT_DOC_PARSER_H

#include <string>

class FormattingStyle;

class DOCParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		DOCParser(const std::string& file_name);
		~DOCParser();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		bool isDOC();
		std::string plainText(const FormattingStyle& formatting);
		bool error();
};

#endif
