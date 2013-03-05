#ifndef DOCTOTEXT_PPT_PARSER_H
#define DOCTOTEXT_PPT_PARSER_H

#include <string>

struct FormattingStyle;

class PPTParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		PPTParser(const std::string& file_name);
		~PPTParser();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		bool isPPT();
		std::string plainText(const FormattingStyle& formatting);
		bool error();
};

#endif
