#ifndef DOCTOTEXT_RTF_PARSER_H
#define DOCTOTEXT_RTF_PARSER_H

#include <string>

class RTFParser
{
	private:
		struct Implementation;
		Implementation* impl;

	public:
		RTFParser(const std::string& file_name);
		~RTFParser();
		void setVerboseLogging(bool verbose);
		void setLogStream(std::ostream& log_stream);
		bool isRTF();
		std::string plainText();
		bool error();
};

#endif
