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

#include "doc_parser.h"
#include <fstream>
#include "rtf_parser.h"
#include "odf_ooxml_parser.h"
#include "misc.h"
#include "doctotext_unzip.h"
#include "plain_text_extractor.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "xls_parser.h"
#include "../version.h"
using namespace std;

static void version()
{
	printf("DocToText v%s\nConverts DOC, XLS, RTF, ODF and OOXML documents to plain text\nCopyright (c) 2006-2010 SILVERCODERS(R)\nhttp://silvercoders.com\n", VERSION);
}

static void help()
{
	version();
	printf("\nUsage: doctotext [OPTION]... [FILE]\n\n");
	printf("FILE\t\tname of file to convert\n\n");
	printf("Options:\n");
	printf("--rtf\ttry to parse as RTF document first.\n");
	printf("--odf\ttry to parse as ODF/OOXML document first.\n");
	printf("--ooxml\ttry to parse as ODF/OOXML document first.\n");
	printf("--xls\ttry to parse as XLS document first.\n");
	printf("--ppt\ttry to parse as PPT document first.\n");
	printf("--doc\ttry to parse as DOC document first.\n");
	printf("--fix-xml\ttry to fix corrupted xml files (ODF, OOXML)\n");
	printf("--strip-xml\tstrip xml tags instead of parsing them (ODF, OOXML)\n");
	printf("--unzip-cmd=[COMMAND]\tuse COMMAND to unzip files from archives (ODF, OOXML)\n"
		"\tinstead of build-in decompression library.\n"
		"\t%%d in the command is substituted with destination directory path\n"
		"\t%%a in the command is substituted with name of archive file\n"
		"\t%%f in the command is substituted with name of file to extract\n"
		"\tExample: --unzip-cmd=\"unzip -d %%d %%a %%f\"\n"
	);
	printf("--verbose\tturn on verbose logging\n");
	printf("--log-file=[PATH]\twrite logs to specified file.\n");
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		help();
		return EXIT_FAILURE;
	}

	std::string arg;
	std::string cmd;

	PlainTextExtractor::ParserType parser_type = PlainTextExtractor::PARSER_AUTO;
	XmlParseMode mode = PARSE_XML;

	FormattingStyle options;
	options.table_style = TABLE_STYLE_TABLE_LOOK;
	options.list_style = LIST_STYLE_NUMBER;
	options.url_style = URL_STYLE_EXTENDED;

	bool verbose = false;
	ofstream* log_stream = NULL;

	for(int i = 1 ; i < argc ; i ++)
	{
		arg = argv[i-1];

		if (arg.find("--rtf", 0) != -1)
			parser_type = PlainTextExtractor::PARSER_RTF;
		if (arg.find("--odf", 0) != -1 || arg.find("ooxml", 0) != -1)
			parser_type = PlainTextExtractor::PARSER_ODF_OOXML;
		if (arg.find("--xls", 0) != -1)
			parser_type = PlainTextExtractor::PARSER_XLS;
		if (arg.find("--ppt", 0) != -1)
			parser_type = PlainTextExtractor::PARSER_PPT;
		if (arg.find("--doc", 0) != -1)
			parser_type = PlainTextExtractor::PARSER_DOC;

		if(arg.find("table-style=", 0) != -1)
		{
			if(arg.find("one-row", arg.find("table-style=", 0) + 11) != -1)
			{
				options.table_style = TABLE_STYLE_ONE_ROW;
			}
			if(arg.find("one-col", arg.find("table-style=", 0) + 11) != -1)
			{
				options.table_style = TABLE_STYLE_ONE_COL;
			}
			if(arg.find("table-look", arg.find("table-style=", 0) + 11) != -1)
			{
				options.table_style = TABLE_STYLE_TABLE_LOOK;
			}
		}
		if(arg.find("url-style=", 0) != -1)
		{
			if(arg.find("text-only", arg.find("url-style=", 0) + 10) != -1)
			{
				options.url_style = URL_STYLE_TEXT_ONLY;
			}
			if(arg.find("extended", arg.find("url-style=", 0) + 10) != -1)
			{
				options.url_style = URL_STYLE_EXTENDED;
			}
		}
		if(arg.find("list-style=", 0) != -1)
		{
			if(arg.find("number", arg.find("list-style=", 0) + 11) != -1)
			{
				options.list_style = LIST_STYLE_NUMBER;
			}
			if(arg.find("dash", arg.find("list-style=", 0) + 11) != -1)
			{
				options.list_style = LIST_STYLE_DASH;
			}
			if(arg.find("dot", arg.find("list-style=", 0) + 11) != -1)
			{
				options.list_style = LIST_STYLE_DOT;
			}
		}
		if (arg.find("fix-xml", 0) != std::string::npos)
			mode = FIX_XML;
		if (arg.find("strip-xml", 0) != std::string::npos)
			mode = STRIP_XML;
		if (arg.find("unzip-cmd=", 0) != -1)
		{
			DocToTextUnzip::setUnzipCommand(arg.substr(arg.find("unzip-cmd=", 0) + 10));
		}
		if (arg.find("verbose", 0) != std::string::npos)
			verbose = true;
		if (arg.find("log-file=", 0) != std::string::npos)
			log_stream = new ofstream(arg.substr(arg.find("log-file=", 0) + 9).c_str());
	}

	PlainTextExtractor extractor(parser_type);
	if (verbose)
		extractor.setVerboseLogging(true);
	if (log_stream != NULL)
		extractor.setLogStream(*log_stream);
	if (mode != PARSE_XML)
		extractor.setXmlParseMode(mode);
	extractor.setFormattingStyle(options);
	string text;
	if (!extractor.processFile(argv[argc - 1], text))
		return EXIT_FAILURE;
	printf("%s\n", text.c_str());
	if (log_stream != NULL)
		delete log_stream;
	return EXIT_SUCCESS;
}
