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

#include "ppt_parser.h"

#include <iostream>
#include <map>
#include <math.h>
#include "misc.h"
#include "olestream.h"
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include "textconverter.h"
#include "utilities.h"
#include <vector>

using namespace wvWare;

enum RecordType
{
	RT_CSTRING = 0xFBA,
	RT_DOCUMENT = 0x03E8,
	RT_DRAWING = 0x040C,
	RT_END_DOCUMENT_ATOM = 0x03EA,
	RT_LIST = 0x07D0,
	RT_MAIN_MASTER = 0x03F8,
	RT_SLIDE_BASE = 0x03EC, // Not in MS specification
	RT_SLIDE_LIST_WITH_TEXT = 0x0FF0,
	RT_SLIDE = 0x03EE,
	RT_TEXT_BYTES_ATOM = 0x0FA8,
	RT_TEXT_CHARS_ATOM = 0x0FA0,
	OFFICE_ART_CLIENT_TEXTBOX = 0xF00D,
	OFFICE_ART_DG_CONTAINER = 0xF002,
	OFFICE_ART_SPGR_CONTAINER = 0xF003,
	OFFICE_ART_SP_CONTAINER = 0xF004
};

struct PPTParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;

	U16 getU16LittleEndian(std::vector<unsigned char>::const_iterator buffer)
	{
		return (unsigned short int)(*buffer) | ((unsigned short int)(*(buffer + 1)) << 8);
	}

	U32 getU32LittleEndian(std::vector<unsigned char>::const_iterator buffer)
	{
		return (unsigned long)(*buffer) | ((unsigned long)(*(buffer +1 )) << 8L) | ((unsigned long)(*(buffer + 2)) << 16L) | ((unsigned long)(*(buffer + 3)) << 24L);
	}

	std::string unicodeCharToUTF8(int uc)
	{
		gchar out[6];
		gint len = g_unichar_to_utf8(uc, out);
		out[len] = '\0';
		return std::string(out);
	}

	void parseRecord(int rec_type, long rec_len, OLEStreamReader& reader, std::string& text)
	{
		switch(rec_type)
		{
			case RT_CSTRING:
			case RT_TEXT_CHARS_ATOM: 
			{
				if (m_verbose_logging)
					*m_log_stream << "RT_TextCharsAtom or RT_CString\n";
				std::vector<unsigned char> buf(2);
				long text_len = rec_len / 2;
				for (int i = 0; i < text_len; i++)
				{
					reader.read((U8*)&*buf.begin(), 2);
					U16 u = getU16LittleEndian(buf.begin());
					if (u != 0x0D)
						text += unicodeCharToUTF8(u);
					else
						text += '\n';
				}
				text += '\n';
				break;
			}
			case RT_DOCUMENT:
				if (m_verbose_logging)
					*m_log_stream << "RT_Document\n";
				break;
			case RT_DRAWING:
				if (m_verbose_logging)
					*m_log_stream << "RT_Drawing\n";
				break;
			case RT_END_DOCUMENT_ATOM:
				if (m_verbose_logging)
					*m_log_stream << "RT_DocumentEnd\n";
				reader.seek(rec_len, G_SEEK_CUR);
				break;
			case RT_LIST:
				if (m_verbose_logging)
					*m_log_stream << "RT_List\n";
				break;
			case RT_MAIN_MASTER:
				if (m_verbose_logging)
					*m_log_stream << "RT_MainMaster\n";
				#warning TODO: Make extracting text from main master slide configurable
				reader.seek(rec_len, G_SEEK_CUR);
				break;
			case RT_SLIDE:
				if (m_verbose_logging)
					*m_log_stream << "RT_Slide\n";
				break;
			case RT_SLIDE_BASE:
				break;
			case RT_SLIDE_LIST_WITH_TEXT:
				if (m_verbose_logging)
					*m_log_stream << "RT_SlideListWithText\n";
				break;
			case RT_TEXT_BYTES_ATOM:
			{
				if (m_verbose_logging)
					*m_log_stream << "RT_TextBytesAtom\n";
				std::vector<unsigned char> buf(2);
				for (int i = 0; i < rec_len; i++)
				{
					reader.read((U8*)&*buf.begin(), 1);
					if (buf[0] != 0x0D)
						text += unicodeCharToUTF8(buf[0]);
					else
						text += '\n';
				}
				text += '\n';
				break;
			}
			case OFFICE_ART_CLIENT_TEXTBOX:
				if (m_verbose_logging)
					*m_log_stream << "OfficeArtClientTextbox\n";
				break;
			case OFFICE_ART_DG_CONTAINER:
				if (m_verbose_logging)
					*m_log_stream << "OfficeArtDgContainer\n";
				break;
			case OFFICE_ART_SPGR_CONTAINER:
				if (m_verbose_logging)
					*m_log_stream << "OfficeArtSpgrContainer\n";
				break;
			case OFFICE_ART_SP_CONTAINER:
				if (m_verbose_logging)
					*m_log_stream << "OfficeArtSpContainer\n";
				break;
			default:
				reader.seek(rec_len, G_SEEK_CUR);
		}
	}

	bool oleEof(OLEStreamReader& reader)
	{
		return reader.tell() == reader.size();
	}

	void parsePPT(OLEStreamReader& reader, std::string& text)
	{
		std::vector<unsigned char> rec(8);
		bool read_status = true;
		std::stack<long> container_ends;
		while (read_status)
		{
			int pos = reader.tell();
			read_status = reader.read(&*rec.begin(), 8);
			if (oleEof(reader))
			{
				parseRecord(RT_END_DOCUMENT_ATOM, 0, reader, text);
				return;
			}
			if (rec.size() < 8)
				break;
			int rec_type = getU16LittleEndian(rec.begin() + 2);
			U32 rec_len = getU32LittleEndian(rec.begin() + 4);
			if (m_verbose_logging)
			{
				while (!container_ends.empty() && pos+rec_len-1 > container_ends.top())
					container_ends.pop();
				std::string indend;
				for (int i = 0; i < container_ends.size(); i++)
					indend += "\t";
				*m_log_stream << indend << "record=0x" << std::hex << rec_type << ", begin=0x" << pos << ", end=0x" << pos + rec_len - 1 << "\n";
				container_ends.push(pos + rec_len - 1);
			}
			parseRecord(rec_type, rec_len, reader, text);
		}
	}
};

PPTParser::PPTParser(const std::string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

PPTParser::~PPTParser()
{
	delete impl;
}

void PPTParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void PPTParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool PPTParser::isPPT()
{
	impl->m_error = false;
	OLEStorage storage(impl->m_file_name);
	if (!storage.open(OLEStorage::ReadOnly))
		return false;
	OLEStreamReader* reader = storage.createStreamReader("PowerPoint Document");
	if (reader == NULL)
		return false;
	delete reader;
	return true;
}

std::string PPTParser::plainText(const FormattingStyle& formatting)
{
	impl->m_error = false;
	OLEStorage storage(impl->m_file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		*impl->m_log_stream << "Error opening " << impl->m_file_name << " as OLE file.\n";
		impl->m_error = true;
		return "";
	}
	OLEStreamReader* reader = storage.createStreamReader("PowerPoint Document");
	if (reader == NULL)
	{
		*impl->m_log_stream << "Error opening " << impl->m_file_name << " as OLE file.\n";
		impl->m_error = true;
		return "";
	}
	std::string text;
	impl->parsePPT(*reader, text);
	delete reader;
	return text;
}

bool PPTParser::error()
{
	return impl->m_error;
}
