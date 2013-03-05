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

#include "misc.h"
#include <glib.h>
#include "fields.h"
#include "handlers.h"
#include "olestream.h"
#include "paragraphproperties.h"
#include "parser.h"
#include "parserfactory.h"
#include <stdio.h>
#include <unistd.h>
#include "ustring.h"
#include <vector>
#ifdef WIN32
	#include <windows.h>
#endif
#include "wvlog.h"
#include "xls_parser.h"
using namespace wvWare;

static UString string_to_ustring(const std::string s)
{
	UString r;
	for (int i = 0; i < s.length(); i++)
	{
		if (s[i] & 128)
		{
			gunichar* out = g_utf8_to_ucs4_fast(&s.c_str()[i], 2, NULL);
			r += UString(UChar(out[0]));
			g_free(out);
			i++;
		}
		else
			r += UString(s[i]);
	}
	return r;
}

struct CurrentTable
{
	bool in_table;
	std::string curr_cell_text;
	svector curr_row_cells;
	std::vector<svector> rows;
	CurrentTable() : in_table(false) {};
};

struct CurrentHeaderFooter
{
	bool in_header;
	bool in_footer;
	UString header;
	UString footer;
	CurrentHeaderFooter() : in_header(false), in_footer(false) {};
};

static std::list<std::string> obj_texts;
static std::list<std::string>::iterator obj_texts_iter;

class DocToTextTextHandler : public TextHandler
{
	private:
		UString* Text;
		CurrentHeaderFooter* m_curr_header_footer;
		CurrentTable* m_curr_table;
		FormattingStyle m_formatting;
		bool m_verbose_logging;
		std::ostream* m_log_stream;
		// constants are from MS Word Binary File Format Specification
		enum FieldType
		{
			FLT_NONE = 0,
			FLT_EMBEDDED_OBJECT = 58,
			FLT_HYPERLINK = 88
		};
		FieldType m_curr_field_type;
		UString m_curr_hyperlink_url;
		UString m_curr_hyperlink_text;

	public:
		DocToTextTextHandler(UString* text, CurrentHeaderFooter* curr_header_footer, CurrentTable* curr_table,
				const FormattingStyle& formatting, bool verbose_logging, std::ostream& log_stream)
			: m_curr_header_footer(curr_header_footer)
		{
			Text = text;
			m_curr_table = curr_table;
			m_formatting = formatting;
			m_curr_field_type = FLT_NONE;
			m_verbose_logging = verbose_logging;
			m_log_stream = &log_stream;
		}

		void sectionStart(SharedPtr<const Word97::SEP> sep)
		{
		}

		void sectionEnd()
		{
		}

		void pageBreak()
		{
		}

		void paragraphStart(SharedPtr<const ParagraphProperties>
			paragraphProperties)
		{
			if (m_curr_table->in_table && (!paragraphProperties->pap().fInTable))
			{
				m_curr_table->in_table = false;
				(*Text) += string_to_ustring(formatTable(m_curr_table->rows, m_formatting));
				m_curr_table->rows.clear();
			}
		}

		void paragraphEnd()
		{
			if (m_curr_table->in_table)
				m_curr_table->curr_cell_text += "\n";
			else if(m_curr_header_footer->in_header)
				m_curr_header_footer->header += UString("\n");
			else if (m_curr_header_footer->in_footer)
				m_curr_header_footer->footer += UString("\n");
			else
				(*Text) += UString("\n");
		}

		void runOfText (const UString &text, SharedPtr< const Word97::CHP > chp)
		{
			if (m_curr_field_type == FLT_HYPERLINK)
			{
				if (m_curr_hyperlink_url.isEmpty())
				{
					m_curr_hyperlink_url = text;
					if (m_curr_hyperlink_url.substr(0, 12) == " HYPERLINK \"")
						m_curr_hyperlink_url = m_curr_hyperlink_url.substr(12);
					if (m_curr_hyperlink_url[m_curr_hyperlink_url.length() - 1] == '"')
						m_curr_hyperlink_url = m_curr_hyperlink_url.substr(0, m_curr_hyperlink_url.length() - 1);
				}
				else
					m_curr_hyperlink_text += text;
			}
			else if (m_curr_field_type == FLT_EMBEDDED_OBJECT)
			{
				// do nothing
			}
			else if (m_curr_table->in_table)
				m_curr_table->curr_cell_text += ustring_to_string(text);
			else if (m_curr_header_footer->in_header)
				m_curr_header_footer->header += text;
			else if (m_curr_header_footer->in_footer)
				m_curr_header_footer->footer += text;
			else
				(*Text) += text;
		}

		void specialCharacter(SpecialCharacter character,
			SharedPtr<const Word97::CHP> chp)
		{
		}

		void footnoteFound (FootnoteData::Type type, UChar character,
			SharedPtr<const Word97::CHP> chp,
			const FootnoteFunctor &parseFootnote)
		{
		}

		void footnoteAutoNumber(SharedPtr<const Word97::CHP> chp)
		{
		}

		void fieldStart(const FLD *fld,
			SharedPtr<const Word97::CHP> chp)
		{
			
			if (fld->flt == FLT_HYPERLINK)
				m_curr_field_type = FLT_HYPERLINK;
			else if (fld->flt == FLT_EMBEDDED_OBJECT)
			{
				if (m_verbose_logging)
					*m_log_stream << "Embedded OLE object reference found.\n";
				m_curr_field_type = FLT_EMBEDDED_OBJECT;
				if (obj_texts_iter == obj_texts.end())
					*m_log_stream << "Unexpected OLE object reference.\n";
				else
				{
					(*Text) += string_to_ustring(*obj_texts_iter);
					obj_texts_iter++;
				}
			}
		}

		void fieldSeparator(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
		}

		void fieldEnd(const FLD* fld,
			SharedPtr<const Word97::CHP> chp)
		{
			if (m_curr_field_type == FLT_HYPERLINK)
			{
				m_curr_field_type = FLT_NONE;
				UString link_str = UString(formatUrl(ustring_to_string(m_curr_hyperlink_url), m_formatting).c_str()) + m_curr_hyperlink_text;
				m_curr_hyperlink_url = "";
				m_curr_hyperlink_text = "";
				if (m_curr_table->in_table)
					m_curr_table->curr_cell_text += ustring_to_string(link_str);
				else if (m_curr_header_footer->in_header)
					m_curr_header_footer->header += link_str;
				else if (m_curr_header_footer->in_footer)
					m_curr_header_footer->footer += link_str;
				else
					(*Text) += link_str;
			}
			else if (m_curr_field_type == FLT_EMBEDDED_OBJECT)
				m_curr_field_type = FLT_NONE;
		}
};

class DocToTextTableHandler : public TableHandler
{
	private:
		UString* Text;
		CurrentTable* m_curr_table;

	public:
		DocToTextTableHandler(UString* text, CurrentTable* curr_table)
		{
			Text = text;
			m_curr_table = curr_table;
		}

		void tableRowStart(SharedPtr<const Word97::TAP> tap)
		{
			m_curr_table->in_table = true;
		}

		void tableRowEnd()
		{
			m_curr_table->rows.push_back(m_curr_table->curr_row_cells);
			m_curr_table->curr_row_cells.clear();
		}

		void tableCellStart()
		{
		}

		void tableCellEnd()
		{
			m_curr_table->curr_row_cells.push_back(m_curr_table->curr_cell_text);
			m_curr_table->curr_cell_text = "";
		}
};

class DocToTextSubDocumentHandler : public SubDocumentHandler
{
	private:
		CurrentHeaderFooter* m_curr_header_footer;
	
	public:
		DocToTextSubDocumentHandler(CurrentHeaderFooter* curr_header_footer)
			: m_curr_header_footer(curr_header_footer)
		{
		}

		virtual void headerStart(HeaderData::Type type)
		{
			switch (type)
			{
				case HeaderData::HeaderOdd:
				case HeaderData::HeaderEven:
				case HeaderData::HeaderFirst:
					m_curr_header_footer->in_header = true;
					break;
				case HeaderData::FooterOdd:
				case HeaderData::FooterEven:
				case HeaderData::FooterFirst:
					m_curr_header_footer->in_footer = true;
					break;
			}
		}

		virtual void headerEnd()
		{
			m_curr_header_footer->in_header = false;
			m_curr_header_footer->in_footer = false;
		}
};

struct DOCParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
	std::streambuf* m_cerr_buf_backup;

	void modifyCerr()
	{
		if (m_verbose_logging)
		{
			if (m_log_stream != std::cerr)
				m_cerr_buf_backup = std::cerr.rdbuf(m_log_stream->rdbuf());
		}
		else
			std::cerr.setstate(std::ios::failbit);
	}

	void restoreCerr()
	{
		if (m_verbose_logging)
		{
			if (m_log_stream != std::cerr)
				std::cerr.rdbuf(m_cerr_buf_backup);
		}
		else
			std::cerr.clear();
	}

	bool copyOleField(OLEStorage& in_storage, OLEStorage& out_storage, const std::string field_name)
	{
		OLEStreamReader* reader = in_storage.createStreamReader(field_name);
		if (!reader)
		{
			*m_log_stream << "Error opening " << field_name << " OLE field for reading.\n";
			delete reader;
			return false;
		}
		OLEStreamWriter* writer = out_storage.createStreamWriter(field_name);
		if (!writer)
		{
			*m_log_stream << "Error opening " << field_name << " OLE field for writting.\n";
			delete reader;
			delete writer;
			return false;
		}
		const size_t buflen = 1024;
		char buffer[buflen];
		size_t remaining = reader->size();
		size_t length;
		while (remaining)
		{
			length = remaining > buflen ? buflen : remaining;
			remaining -= length;
			if (reader->read(reinterpret_cast<guint8*>(buffer), length))
				writer->write(reinterpret_cast<guint8*>(buffer), length);
		}
		delete reader;
		delete writer;
		return true;
	}
};

DOCParser::DOCParser(const std::string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

DOCParser::~DOCParser()
{
	delete impl;
}

void DOCParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void DOCParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool DOCParser::isDOC()
{
	impl->m_error = false;
	impl->modifyCerr();
	SharedPtr<Parser> parser = ParserFactory::createParser(impl->m_file_name.c_str());
	impl->restoreCerr();
	if (!parser || !parser->isOk())
	{
		*impl->m_log_stream << "Creating parser failed.\n";
		impl->m_error = true;
		return false;
	}
	return true;
}

std::string DOCParser::plainText(const FormattingStyle& formatting)
{
	impl->m_error = false;
	obj_texts.clear();
	if (impl->m_verbose_logging)
		*impl->m_log_stream << "Opening " << impl->m_file_name << " as OLE file to parse all embedded objects in supported formats.\n";
	OLEStorage storage(impl->m_file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		*impl->m_log_stream << "Error opening " << impl->m_file_name << " as OLE file.\n";
		impl->m_error = true;
		return "";
	}
	if (storage.enterDirectory("ObjectPool"))
	{
		if (impl->m_verbose_logging)
			*impl->m_log_stream << "ObjectPool found, embedded OLE objects probably exist.\n";
		std::list<std::string> obj_list = storage.listDirectory();
		obj_list.sort();
		for (std::list<std::string>::iterator i = obj_list.begin(); i != obj_list.end(); i++)
		{
			if (impl->m_verbose_logging)
				*impl->m_log_stream << "OLE object entry found: " << *i << "\n";
			std::string obj_text;
			if (storage.enterDirectory(*i))
			{
				std::list<std::string> obj_list = storage.listDirectory();
				if (find(obj_list.begin(), obj_list.end(), "Workbook") != obj_list.end())
				{
					*impl->m_log_stream << "Embedded MS Excel workbook detected.\n";
					std::string tmp_xls_fn = tmpnam(NULL);
					#ifdef WIN32
					// tmpname() is broken on win32 - always returns path in root directory
					char tmp_path[256];
					GetTempPathA(256, tmp_path);
					tmp_xls_fn = std::string(tmp_path) + "\\" + tmp_xls_fn;
					#endif
					OLEStorage xls_storage(tmp_xls_fn);
					if (xls_storage.open(OLEStorage::WriteOnly))
					{
						bool copy_error = false;
						for (std::list<std::string>::iterator i = obj_list.begin(); i != obj_list.end(); i++)
						{
							if (impl->m_verbose_logging)
								*impl->m_log_stream << "Copying OLE object entry: " << *i << "\n";
							if (!impl->copyOleField(storage, xls_storage, *i))
							{
								copy_error = true;
								break;
							}
						}
						xls_storage.close();
						if (!copy_error)
						{
							XLSParser xls(tmp_xls_fn);
							bool is_xls = xls.isXLS();
							*impl->m_log_stream << "Using XLS parser.\n";
							std::string xls_text = xls.plainText(formatting);
							if (!xls.error())
								obj_text = xls_text;
						}
						unlink(tmp_xls_fn.c_str());
					}
					else
						*impl->m_log_stream << "Error opening temporary OLE storage.\n";
				}
				storage.leaveDirectory();
			}
			obj_texts.push_back(obj_text);
		}
	}
	else
	{
		if (impl->m_verbose_logging)
			*impl->m_log_stream << "No ObjectPool found, embedded OLE objects probably do not exist.\n";
	}
	storage.close();
	obj_texts_iter = obj_texts.begin();
	impl->modifyCerr();
	SharedPtr<Parser> parser = ParserFactory::createParser(impl->m_file_name.c_str());
	impl->restoreCerr();
	if (!parser || !parser->isOk())
	{
		*impl->m_log_stream << "Creating parser failed.\n";
		impl->m_error = true;
		return "";
	}
	UString text;
	CurrentHeaderFooter curr_header_footer;
	CurrentTable curr_table;
	DocToTextTextHandler text_handler(&text, &curr_header_footer, &curr_table, formatting,
		impl->m_verbose_logging, *impl->m_log_stream);
	parser->setTextHandler(&text_handler);
	DocToTextTableHandler table_handler(&text, &curr_table);
	parser->setTableHandler(&table_handler);
	DocToTextSubDocumentHandler sub_document_handler(&curr_header_footer);
	parser->setSubDocumentHandler(&sub_document_handler);
	impl->modifyCerr();
	bool res = parser->parse();
	impl->restoreCerr();
	if (!res)
	{
		*impl->m_log_stream << "Parsing document failed.\n";
		impl->m_error = true;
		return "";
	}
	if (curr_header_footer.header != "")
		text = curr_header_footer.header + UString("\n") + text;
	if (curr_header_footer.footer != "")
		text += UString("\n") + curr_header_footer.footer;
	return ustring_to_string(text);
}

bool DOCParser::error()
{
	return impl->m_error;
}
