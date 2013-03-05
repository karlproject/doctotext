#ifndef DOCTOTEXT_UNZIP_H
#define DOCTOTEXT_UNZIP_H

#include <string>

class DocToTextUnzip
{
	private:
		struct Implementation;
		Implementation* Impl;

	public:
		DocToTextUnzip(const std::string& archive_file_name);
		~DocToTextUnzip();
		void setLogStream(std::ostream& log_stream);
		static void setUnzipCommand(const std::string& command);
		bool open();
		void close();
		bool exists(const std::string& file_name) const;
		bool read(const std::string& file_name, std::string* contents, int num_of_chars = 0) const;
		/**
			Load and cache zip file directory. Speed up locating files dramatically. Use before multiple read() calls.
		**/
		bool loadDirectory();
};

#endif
