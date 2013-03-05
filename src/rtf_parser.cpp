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

#include "rtf_parser.h"

#include <glib.h>

#include <iostream>
#include <map>
#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <textconverter.h>
#include <ustring.h>
using namespace wvWare;

struct RTFParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
};

RTFParser::RTFParser(const std::string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

RTFParser::~RTFParser()
{
	delete impl;
}

void RTFParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void RTFParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool RTFParser::isRTF()
{
	impl->m_error = false;
	FILE* f = fopen(impl->m_file_name.c_str(), "r");
	if (f == NULL)
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name << ".\n";
		impl->m_error = true;
		return false;
	}
	char buf[6];
	int res = fread(buf, sizeof(char), 5, f);
	if (res == 0)
	{
		fclose(f);
		*impl->m_log_stream << "Error reading signature from file " << impl->m_file_name << ".\n";
		impl->m_error = true;
		return false;
	}
	if (res != 5)
		return false;
	fclose(f);
	buf[5] = '\0';
	return (strcmp(buf, "{\\rtf") == 0);
}

#define RTFNAMEMAXLEN 32
#define RTFARGSMAXLEN 64

static long parseNumber(FILE* f)
{
	int ch;
	int count = 0;
	char buf[RTFARGSMAXLEN + 1];
	
	while (isdigit(ch = fgetc(f)) || ch == '-')
	{
		if(feof(f))
			return -1;
		buf[count++] = (char)ch;
	}
	ungetc(ch, f);
	buf[count] = '\0';
	return strtol(buf, (char **)NULL, 10);
}

int parseCharCode(FILE *f)
{
	int ch;
	int count = 0;
	char buf[RTFARGSMAXLEN + 1];

	for (int i = 0; i < 2; i++)
	{
		if (isdigit(ch = fgetc(f)) || (ch >= 'a' && ch <= 'f'))
		{
			if(feof(f))
				return -1;
			buf[count++] = (char)ch;
		}
		else
			ungetc(ch, f);
	}

	buf[count]='\0';
	return strtol(buf, (char **)NULL, 16);
}

enum RTFCommand
{
	RTF_CODEPAGE,
	RTF_FONT_CHARSET,
	RTF_UC,
	RTF_UNICODE_CHAR,
	RTF_CHAR,
	RTF_PARA,
	RTF_TABLE_START,
	RTF_TABLE_END,
	RTF_ROW,
	RTF_CELL,
	RTF_UNKNOWN,
	RTF_OVERLAY,
	RTF_PICT,
	RTF_F,
	RTF_AUTHOR,
	RTF_FONTTBL,
	RTF_INFO,
	RTF_STYLESHEET,
	RTF_COLORTBL,
	RTF_LISTOVERRIDETABLE,
	RTF_LISTTABLE,
	RTF_RSIDTBL,
	RTF_GENERATOR,
	RTF_DATAFIELD,
	RTF_LANG,
	RTF_LINE,
	RTF_PARD,
	RTF_TAB,
	RTF_SPEC_CHAR,
	RTF_EMDASH,
	RTF_ENDASH,
	RTF_EMSPACE,
	RTF_ENSPACE,
 	RTF_BULLET, 
 	RTF_LQUOTE,
	RTF_RQUOTE,
	RTF_LDBLQUOTE,
	RTF_RDBLQUOTE,
	RTF_ZWNONJOINER,
	RTF_FLDINST,
	RTF_FLDRSLT,
};

struct RTFStringCommand
{
	const char* name;
	RTFCommand cmd;
};

RTFStringCommand rtf_commands[] =
{
	{ "uc", RTF_UC},
	{ "ansicpg", RTF_CODEPAGE},
	{ "pard", RTF_PARD},
	{ "par", RTF_PARA},
	{ "cell", RTF_CELL},
	{ "row", RTF_ROW},
	{ "overlay", RTF_OVERLAY}, 
	{ "pict" ,RTF_PICT},
	{ "author", RTF_AUTHOR},
	{ "f", RTF_F}, 
	{ "fonttbl", RTF_FONTTBL}, 
	{ "info", RTF_INFO}, 
	{ "stylesheet", RTF_STYLESHEET},
	{ "colortbl", RTF_COLORTBL},
	{ "line", RTF_LINE},
	{ "listtable", RTF_LISTTABLE},
	{ "listoverridetable", RTF_LISTOVERRIDETABLE},
	{ "rsidtbl", RTF_RSIDTBL}, 
	{ "generator", RTF_GENERATOR}, 
	{ "datafield", RTF_DATAFIELD}, 
	{ "lang", RTF_LANG}, 
	{ "tab", RTF_TAB}, 
	{ "emdash", RTF_EMDASH},
	{ "endash", RTF_ENDASH},
	{ "emspace", RTF_EMDASH},
	{ "enspace", RTF_ENDASH},
	{ "bullet", RTF_BULLET}, 
	{ "lquote", RTF_LQUOTE},
	{ "rquote", RTF_RQUOTE},
	{ "ldblquote", RTF_LDBLQUOTE},
	{ "rdblquote", RTF_RDBLQUOTE},
	{ "zwnj", RTF_ZWNONJOINER},
	{ "u", RTF_UNICODE_CHAR},
	{ "fldinst", RTF_FLDINST},
	{ "fldrslt", RTF_FLDRSLT},
	{ "fcharset", RTF_FONT_CHARSET },
};

struct RTFGroup
{
	int uc;
	//int codepage;
};

struct RTFParserState
{
	std::stack<RTFGroup> groups;
	long int last_font_ref_num;
	std::map<long int, std::string> font_table;
};

static RTFCommand commandNameToEnum(char* name)
{
	for (int i = 0; i < sizeof(rtf_commands) / sizeof(RTFStringCommand); i++)
	{
		if (strcmp(name, rtf_commands[i].name) == 0)
			return rtf_commands[i].cmd;
	}
	return RTF_UNKNOWN;
}

static bool parseCommand(FILE* f, RTFCommand& cmd, long int& arg, bool verbose, std::ostream& log_stream)
{
	char name[RTFNAMEMAXLEN + 1];

	int ch = fgetc(f);
	if (isalpha(ch))
	{
		int name_count = 1;
		name[0] = (char)ch;
		while (isalpha(ch = fgetc(f)) && name_count < RTFNAMEMAXLEN)
		{
			if(feof(f))
				return false;
			name[name_count++] = (char)ch;
		}
		name[name_count] = '\0';
		cmd = commandNameToEnum(name);
		ungetc(ch, f);
		if (isdigit(ch) || ch == '-' )
			arg = parseNumber(f);
		else
			arg = 0;
		ch = fgetc(f);
		if(!(ch == ' ' || ch == '\t'))
			ungetc(ch, f);
	}
	else
	{
		name[0] = (char)ch;
		name[1] = '\0';
		if (ch == '\'')
		{
			cmd = RTF_CHAR;
			arg = parseCharCode(f);
			if(feof(f))
				return false;
		}
		else
		{
			cmd = RTF_SPEC_CHAR;
			arg = ch;
		}
	}
	if (verbose)
		log_stream << "[cmd: " << name << " (" << arg << ")]\n";
	return true;
}

static std::string codepage_to_encoding(int codepage)
{
	char encoding[7];
	snprintf(encoding, 7, "CP%i", codepage);
	return encoding;
}

static std::string win_charset_to_encoding(long int win_charset)
{
	long int codepage = 1250;
	switch (win_charset)
	{
		case 0: codepage = 1250; break; // ANSI_CHARSET
		case 1: codepage = 1250; break; // DEFAULT_CHARSET
		//case 2: codepage = ; break; // SYMBOL_CHARSET not supported yet.
		case 77: codepage = 10000; break; // MAC_CHARSET
		case 128: codepage = 932; break; // SHIFTJIS_CHARSET
		case 129: codepage = 949; break; // HANGEUL_CHARSET
		case 130: codepage = 1361; break; // JOHAB_CHARSET
		case 134: codepage = 936; break; // GB2312_CHARSET
		case 136: codepage = 950; break; // CHINESEBIG5_CHARSET
		case 161: codepage = 1253; break; // GREEK_CHARSET
		case 162: codepage = 1254; break; // TURKISH_CHARSET
		case 163: codepage = 1258; break; // VIETNAMESE_CHARSET
		case 177: codepage = 1255; break; // HEBREW_CHARSET
		case 178: codepage = 1256; break; // ARABIC_CHARSET / ARABICSIMPLIFIED_CHARSET
		case 186: codepage = 1257; break; // BALTIC_CHARSET
		case 204: codepage = 1251; break; // RUSSIAN_CHARSET / CYRILLIC_CHARSET
		case 222: codepage = 874; break; // THAI_CHARSET
		case 238: codepage = 1250; break; // EASTEUROPE_CHARSET / 
		case 255: codepage = 850; break; // OEM_CHARSET
	}
	return codepage_to_encoding(codepage);
}

static void execCommand(FILE* f, UString& text, int& skip, RTFParserState& state, RTFCommand cmd, long int arg,
	TextConverter*& converter, bool verbose, std::ostream& log_stream)
{
	switch (cmd)
	{
		case RTF_SPEC_CHAR:
			if (arg == '*' && skip == 0)
				skip = state.groups.size() - 1;
			else if (arg == '\r' || arg == '\n') // the same as \para command
				text += UString("\n");
			else if (arg == '~')
				text += UString((UChar)0xA0); /* No-break space */
			else if (arg == '-')
				text += UString((UChar)0xAD); /* Optional hyphen */
			break;
		case RTF_EMDASH:
			text += UString((UChar)0x2014);
			break;
		case RTF_ENDASH: 
			text += UString((UChar)0x2013);
			break;
		case RTF_BULLET: 
			text += UString((UChar)0x2022);
			break;
		case RTF_LQUOTE:
			text += UString((UChar)0x2018);
			break;
		case RTF_RQUOTE:
			text += UString((UChar)0x2019);
			break;
		case RTF_LDBLQUOTE:
			text += UString((UChar)0x201C);
			break;
		case RTF_RDBLQUOTE:
			text += UString((UChar)0x201D);
			break;
		case RTF_ZWNONJOINER:
			text += UString((UChar)0xfeff);
			break;
		case RTF_EMSPACE:
		case RTF_ENSPACE:
			text += UString(' ');
			break;
		case RTF_CHAR:
			if (skip == 0)
			{
				if (converter != NULL)
					text += converter->convert((const char*)&arg, sizeof(arg));
				else
					text += UString((UChar)arg);
			}
			break;
		case RTF_UC:
			state.groups.top().uc = arg;
			break;
		case RTF_TAB:
			text += UString((UChar)0x0009);
			break;
		case RTF_UNICODE_CHAR:
			if (arg < 0)
				break;
			if (skip == 0)
				text += UString((UChar)arg);
			for (int i = 0; i < state.groups.top().uc; i++)
			{
				char ch = fgetc(f);
				if (ch == '\\')
				{
					RTFCommand tmp_cmd;
					long int tmp_arg;
					parseCommand(f, tmp_cmd, tmp_arg, verbose, log_stream);
				}
			}
			break;
		case RTF_PARA:
			text += UString("\n");
			break;
		case RTF_CELL:
			text += UString("\n");
			break;
		case RTF_FLDINST:
			text += UString("<");
			skip = 0;
			break;
		case RTF_FLDRSLT:
			text+=UString(">");
			break;
		case RTF_PICT:
		case RTF_FONTTBL:
		case RTF_INFO:
		case RTF_COLORTBL:
		case RTF_STYLESHEET:
		case RTF_LISTTABLE:
		case RTF_LISTOVERRIDETABLE:
		case RTF_RSIDTBL:
		case RTF_GENERATOR:
		case RTF_DATAFIELD:
			if (skip == 0)
				skip = state.groups.size() - 1;
			break;
		case RTF_LANG:
			break;
		case RTF_LINE:
			text += UString("\n");
			break;
		case RTF_CODEPAGE:
			if (verbose)
				log_stream << "Initializing converter for codepage " << arg << "\n";
			converter = new TextConverter(codepage_to_encoding(arg));
			if (converter->isOk())
			{
				if (verbose)
					log_stream << "Converter initialized.\n";
			}
			else
			{
				log_stream << "Converter initialization ERROR!\n";
				delete converter;
				converter = NULL;
			}
			break;
		case RTF_FONT_CHARSET:
			if (verbose)
				log_stream << "Setting win charset " << arg << " for font number " << state.last_font_ref_num << "\n";
			state.font_table[state.last_font_ref_num] = win_charset_to_encoding(arg);
			break;
		case RTF_F:
			if (state.font_table.find(arg) != state.font_table.end())
			{
				if (verbose)
					log_stream << "Font number " << arg << " referenced. Setting converter for encoding " << state.font_table[arg] << "\n";
				if (converter != NULL)
					converter->setFromCode(state.font_table[arg]);
			}
			else
				state.last_font_ref_num = arg;
			break;
	}
}

static std::string ustring_to_string(const UString& s)
{
	std::string r;
	for (int i = 0; i < s.length(); i++)
	{
		gchar out[6];
		gint len = g_unichar_to_utf8(s[i].unicode(), out);
		out[len] = '\0';
		r += out;
	}
	return r;
}

std::string RTFParser::plainText()
{
	impl->m_error = false;
	UString text;
	TextConverter* converter = NULL;
	FILE* f = fopen(impl->m_file_name.c_str(), "r");
	if (f == NULL)
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name << ".\n";
		impl->m_error = true;
		return "";
	}
	int ch;
	RTFParserState state;
	state.groups.push(RTFGroup());
	state.groups.top().uc = 1;
	state.last_font_ref_num = 0;
	int skip = 0;
	while ((ch = fgetc(f)) != EOF)
	{
		switch (ch)
		{
			case '\\':
				RTFCommand cmd;
				long int arg;
				if (!parseCommand(f, cmd, arg, impl->m_verbose_logging, *impl->m_log_stream))
					break;
				execCommand(f, text, skip, state, cmd, arg, converter, impl->m_verbose_logging, *impl->m_log_stream);
				break;
			case '{':
				state.groups.push(state.groups.top());
				break;
			case '}':
				state.groups.pop();
				if (skip > state.groups.size() - 1)
					skip = 0;
				break;
			default:
				if (skip == 0 && ch != '\n' && ch != '\r')
				{
					if (converter != NULL)
						text += converter->convert((const char*)&ch, sizeof(ch));
					else
						text += UString((UChar)ch);
				}
		}
	}
	fclose(f);
	if (converter != NULL)
		delete converter;
	return ustring_to_string(text);
}

bool RTFParser::error()
{
	return impl->m_error;
}
