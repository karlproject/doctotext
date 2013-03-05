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

#include "plain_text_extractor.h"

#include <algorithm>
#include "doc_parser.h"
#include "ppt_parser.h"
#include "rtf_parser.h"
#include "odf_ooxml_parser.h"
#include <iostream>
#include <stdio.h>
#include "xls_parser.h"

struct PlainTextExtractor::Implementation
{
	ParserType m_parser_type;
	FormattingStyle m_formatting_style;
	XmlParseMode m_xml_parse_mode;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
};

PlainTextExtractor::PlainTextExtractor(ParserType parser_type)
{
	impl = new Implementation();
	impl->m_parser_type = parser_type;
	impl->m_formatting_style.table_style = TABLE_STYLE_TABLE_LOOK;
	impl->m_formatting_style.list_style = LIST_STYLE_NUMBER;
	impl->m_formatting_style.url_style = URL_STYLE_EXTENDED;
	impl->m_xml_parse_mode = PARSE_XML;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

PlainTextExtractor::~PlainTextExtractor()
{
	delete impl;
}

void PlainTextExtractor::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void PlainTextExtractor::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

void PlainTextExtractor::setFormattingStyle(const FormattingStyle& style)
{
	impl->m_formatting_style = style;
}

void PlainTextExtractor::setXmlParseMode(XmlParseMode mode)
{
	impl->m_xml_parse_mode = mode;
}

bool PlainTextExtractor::processFile(const std::string& file_name, std::string& text)
{
	ParserType parser_type = impl->m_parser_type;
	if (parser_type == PARSER_AUTO)
	{
		std::string ext = file_name.substr(file_name.find_last_of(".") + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		if (ext == "rtf")
			parser_type = PARSER_RTF;
		else if (ext == "odt" || ext == "ods" || ext == "odp" || ext == "docx" || ext == "xlsx" || ext == "pptx" || ext == "ppsx")
			parser_type = PARSER_ODF_OOXML;
		else if (ext == "xls")
			parser_type = PARSER_XLS;
		else if (ext == "doc")
			parser_type = PARSER_DOC;
		else if (ext == "ppt" || ext == "pps")
			parser_type = PARSER_PPT;
	}

	switch (parser_type)
	{
		case PARSER_RTF:
		{
			RTFParser rtf(file_name);
			if (impl->m_verbose_logging)
				rtf.setVerboseLogging(true);
			if (impl->m_log_stream != &std::cerr)
				rtf.setLogStream(*impl->m_log_stream);
			*impl->m_log_stream << "Using RTF parser.\n";
			text = rtf.plainText();
			return (!rtf.error());
		}
		case PARSER_ODF_OOXML:
		{
			ODFOOXMLParser odfooxml(file_name);
			if (impl->m_verbose_logging)
				odfooxml.setVerboseLogging(true);
			if (impl->m_log_stream != &std::cerr)
				odfooxml.setLogStream(*impl->m_log_stream);
			*impl->m_log_stream << "Using ODF/OOXML parser.\n";
			text = odfooxml.plainText(impl->m_xml_parse_mode, impl->m_formatting_style);
			return (!odfooxml.error());
		}
		case PARSER_XLS:
		{
			XLSParser xls(file_name);
			if (impl->m_verbose_logging)
				xls.setVerboseLogging(true);
			if (impl->m_log_stream != &std::cerr)
				xls.setLogStream(*impl->m_log_stream);
			*impl->m_log_stream << "Using XLS parser.\n";
			text = xls.plainText(impl->m_formatting_style);
			return (!xls.error());
		}
		case PARSER_DOC:
		{
			DOCParser doc(file_name);
			if (impl->m_verbose_logging)
				doc.setVerboseLogging(true);
			if (impl->m_log_stream != &std::cerr)
				doc.setLogStream(*impl->m_log_stream);
			*impl->m_log_stream << "Using DOC parser.\n";
			text = doc.plainText(impl->m_formatting_style);
			return (!doc.error());
		}
		case PARSER_PPT:
		{
			PPTParser ppt(file_name);
			if (impl->m_verbose_logging)
				ppt.setVerboseLogging(true);
			if (impl->m_log_stream != &std::cerr)
				ppt.setLogStream(*impl->m_log_stream);
			*impl->m_log_stream << "Using PPT parser.\n";
			text = ppt.plainText(impl->m_formatting_style);
			return (!ppt.error());
		}
	}

	RTFParser rtf(file_name);
	if (impl->m_verbose_logging)
		rtf.setVerboseLogging(true);
	if (impl->m_log_stream != &std::cerr)
		rtf.setLogStream(*impl->m_log_stream);
	bool is_rtf = rtf.isRTF();
	if (rtf.error())
		return false;
	if (is_rtf)
	{
		*impl->m_log_stream << "Using RTF parser.\n";
		text = rtf.plainText();
		return (!rtf.error());
	}

	ODFOOXMLParser odfooxml(file_name);
	if (impl->m_verbose_logging)
		odfooxml.setVerboseLogging(true);
	if (impl->m_log_stream != &std::cerr)
		odfooxml.setLogStream(*impl->m_log_stream);
	bool is_odfooxml = odfooxml.isODFOOXML();
	if (odfooxml.error())
		return false;
	if (is_odfooxml)
	{
		*impl->m_log_stream << "Using ODF/OOXML parser.\n";
		text = odfooxml.plainText(impl->m_xml_parse_mode, impl->m_formatting_style);
		return (!odfooxml.error());
	}

	XLSParser xls(file_name);
	if (impl->m_verbose_logging)
		xls.setVerboseLogging(true);
	if (impl->m_log_stream != &std::cerr)
		xls.setLogStream(*impl->m_log_stream);
	bool is_xls = xls.isXLS();
	if (xls.error())
		return false;
	if (is_xls)
	{
		*impl->m_log_stream << "Using XLS parser.\n";
		text = xls.plainText(impl->m_formatting_style);
		return (!xls.error());
	}

	DOCParser doc(file_name);
	if (impl->m_verbose_logging)
		doc.setVerboseLogging(true);
	if (impl->m_log_stream != &std::cerr)
		doc.setLogStream(*impl->m_log_stream);
	bool is_doc = doc.isDOC();
	if (doc.error())
		return false;
	if (is_doc)
	{
		*impl->m_log_stream << "Using DOC parser.\n";
		text = doc.plainText(impl->m_formatting_style);
		return (!doc.error());
	}

	PPTParser ppt(file_name);
	if (impl->m_verbose_logging)
		ppt.setVerboseLogging(true);
	if (impl->m_log_stream != &std::cerr)
		ppt.setLogStream(*impl->m_log_stream);
	bool is_ppt = ppt.isPPT();
	if (ppt.error())
		return false;
	if (is_ppt)
	{
		*impl->m_log_stream << "Using PPT parser.\n";
		text = ppt.plainText(impl->m_formatting_style);
		return (!ppt.error());
	}

	*impl->m_log_stream << "No maching parser found.\n";
	return false;
}
