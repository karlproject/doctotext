/****************************************************************************
**
** DocToText - Converts DOC, XLS, PPT, RTF, ODF (ODT, ODS, ODP) and
**             OOXML (DOCX, XLSX, PPTX) documents to plain text
** Copyright (c) 2006-2012, SILVERCODERS(R)
** http://silvercoders.com
**
** Project homepage: http://silvercoders.com/en/products/doctotext
**
** This program may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file COPYING.GPL included in the
** packaging of this file.
**
** Please remember that any attempt to workaround the GNU General Public
** License using wrappers, pipes, client/server protocols, and so on
** is considered as license violation. If your program, published on license
** other than GNU General Public License version 2, calls some part of this
** code directly or indirectly, you have to buy commercial license.
** If you do not like our point of view, simply do not use the product.
**
** Licensees holding valid commercial license for this product
** may use this file in accordance with the license published by
** SILVERCODERS and appearing in the file COPYING.COM
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
**
*****************************************************************************/

#include "doctotext_unzip.h"

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include "unzip.h"
#include "zlib.h"

const int CASESENSITIVITY = 1;

struct DocToTextUnzip::Implementation
{
	std::string ArchiveFileName;
	std::ostream* m_log_stream;
	unzFile ArchiveFile;
	std::map<std::string, unz_file_pos> m_directory;
};

DocToTextUnzip::DocToTextUnzip(const std::string& archive_file_name)
{
	Impl = new Implementation();
	Impl->ArchiveFileName = archive_file_name;
	Impl->m_log_stream = &std::cerr;
}

DocToTextUnzip::~DocToTextUnzip()
{
	delete Impl;
}

void DocToTextUnzip::setLogStream(std::ostream& log_stream)
{
	Impl->m_log_stream = &log_stream;
}

static std::string unzip_command;

void DocToTextUnzip::setUnzipCommand(const std::string& command)
{
	unzip_command = command;
}

bool DocToTextUnzip::open()
{
	Impl->ArchiveFile = unzOpen(Impl->ArchiveFileName.c_str());
	if (Impl->ArchiveFile == NULL)
	{
		unzClose(Impl->ArchiveFile);
		return false;
	}
	return true;
}

void DocToTextUnzip::close()
{
	unzClose(Impl->ArchiveFile);
}

bool DocToTextUnzip::exists(const std::string& file_name) const
{
	return (unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY) == UNZ_OK);
}

bool DocToTextUnzip::read(const std::string& file_name, std::string* contents, int num_of_chars) const
{
	if (unzip_command != "")
	{
		std::string temp_dir = tempnam(NULL, NULL);
		std::string cmd = unzip_command;
		size_t d_pos = cmd.find("%d");
		if (d_pos == std::string::npos)
		{
			*Impl->m_log_stream << "Unzip command must contain %d symbol.\n";
			return false;
		}
		cmd.replace(d_pos, 2, temp_dir);
		size_t a_pos = cmd.find("%a");
		if (a_pos == std::string::npos)
		{
			*Impl->m_log_stream << "Unzip command must contain %a symbol.\n";
			return false;
		}
		cmd.replace(a_pos, 2, Impl->ArchiveFileName);
		size_t f_pos = cmd.find("%f");
		if (f_pos == std::string::npos)
		{
			*Impl->m_log_stream << "Unzip command must contain %f symbol.\n";
			return false;
		}
		#ifdef WIN32
			std::string fn = file_name;
			size_t b_pos;
			while ((b_pos = fn.find('/')) != std::string::npos)
				fn.replace(b_pos, 1, "\\");
			cmd.replace(f_pos, 2, fn);
		#else
			cmd.replace(f_pos, 2, file_name);
		#endif
		cmd = cmd + " >&2";
		#ifdef WIN32
			const std::string remove_cmd = "rmdir /S /Q " + temp_dir;
		#else
			const std::string remove_cmd = "rm -rf " + temp_dir;
		#endif
		*Impl->m_log_stream << "Executing " << cmd << "\n";
		if (system(cmd.c_str()) < 0)
			return false;
		FILE* f = fopen((temp_dir + "/" + file_name).c_str(), "r");
		if (f == NULL)
		{
			*Impl->m_log_stream << "Executing " << remove_cmd << "\n";
			system(remove_cmd.c_str());
			return false;
		}
		*contents = "";
		char buffer[1024 + 1];
		int res;
		while((res = fread(buffer, sizeof(char), (num_of_chars > 0 && num_of_chars < 1024) ? num_of_chars : 1024, f)) > 0)
		{
			buffer[res] = '\0';
			*contents += buffer;
			if (num_of_chars > 0)
				if (contents->length() >= num_of_chars)
				{
					*contents = contents->substr(0, num_of_chars);
					break;
				}
		}
		if (res < 0)
		{
			fclose(f);
			*Impl->m_log_stream << "Executing " << remove_cmd << "\n";
			system(remove_cmd.c_str());
			return false;
		}
		fclose(f);
		*Impl->m_log_stream << "Executing " << remove_cmd << "\n";
		if (system(remove_cmd.c_str()) != 0)
			return false;
		return true;
	}

	int res;
	if (Impl->m_directory.size() > 0)
	{
		std::map<std::string, unz_file_pos>::iterator i = Impl->m_directory.find(file_name);
		if (i == Impl->m_directory.end())
			return false;
		res = unzGoToFilePos(Impl->ArchiveFile, &i->second);
	}
	else
		res = unzLocateFile(Impl->ArchiveFile, file_name.c_str(), CASESENSITIVITY);
	if (res != UNZ_OK)
		return false;
	res = unzOpenCurrentFile(Impl->ArchiveFile);
	if (res != UNZ_OK)
		return false;
	*contents = "";
	char buffer[1024 + 1];
	while((res = unzReadCurrentFile(Impl->ArchiveFile, buffer, (num_of_chars > 0 && num_of_chars < 1024) ? num_of_chars : 1024)) > 0)
	{
		buffer[res] = '\0';
		*contents += buffer;
		if (num_of_chars > 0)
			if (contents->length() >= num_of_chars)
			{
				*contents = contents->substr(0, num_of_chars);
				break;
			}
	}
	if (res < 0)
	{
		unzCloseCurrentFile(Impl->ArchiveFile);
		return false;
	}
	unzCloseCurrentFile(Impl->ArchiveFile);
	return true;
}

bool DocToTextUnzip::loadDirectory()
{
	Impl->m_directory.clear();
	if (unzGoToFirstFile(Impl->ArchiveFile) != UNZ_OK)
		return false;
	for (;;)
	{
		char name[1024];
		if (unzGetCurrentFileInfo(Impl->ArchiveFile, NULL, name, 1024, NULL, 0, NULL, 0) != UNZ_OK)
			return false;
		unz_file_pos pos;
		if (unzGetFilePos(Impl->ArchiveFile, &pos) != UNZ_OK)
			return false;
		Impl->m_directory[name] = pos;
		int res = unzGoToNextFile(Impl->ArchiveFile);
		if (res == UNZ_END_OF_LIST_OF_FILE)
			break;
		if (res != UNZ_OK)
			return false;
	}
	return true;
}
