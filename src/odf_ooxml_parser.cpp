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

#include "odf_ooxml_parser.h"

#include "xml_fixer.h"
#include "doctotext_unzip.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include <map>
#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

const int CASESENSITIVITY = 1;

static string int_to_str(int i)
{
	ostringstream s;
	s << i;
	return s.str();
}

static int str_to_int(const string& s)
{
	istringstream ss(s);
	int i;
	ss >> i;
	return i;
}

enum ODFOOXMLCommand
{
	ODFOOXML_TEXT,
	ODFOOXML_PARA,
	ODFOOXML_TAB,
	ODFOOXML_URL,
	ODFOOXML_LIST,
	ODFOOXML_TABLE,
	ODFOOXML_ROW,
	ODF_CELL,
	OOXML_ATTR,
	OOXML_CELL,
	ODFOOXML_UNKNOWN,
};

struct ODFOOXMLParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
	vector<string> SharedStrings;
	void execCommand(xmlNodePtr &Body, std::string &text, FormattingStyle options, ODFOOXMLCommand cmd, bool* children_processed);
	std::string returnText(xmlNodePtr Body, FormattingStyle options);
	bool extractText(const string& xml_contents, XmlParseMode mode, FormattingStyle options, string* text);
};

ODFOOXMLParser::ODFOOXMLParser(const string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

ODFOOXMLParser::~ODFOOXMLParser()
{
	delete impl;
}

void ODFOOXMLParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void ODFOOXMLParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

string locate_main_file(const DocToTextUnzip& zipfile, std::ostream& log_stream)
{
	if (zipfile.exists("content.xml"))
		return "content.xml";
	if (zipfile.exists("word/document.xml"))
		return "word/document.xml";
	if (zipfile.exists("xl/workbook.xml"))
		return "xl/workbook.xml";
	if (zipfile.exists("ppt/presentation.xml"))
		return "ppt/presentation.xml";
	log_stream << "Error - no content.xml, no word/document.xml and no ppt/presentation.xml" << endl;
	return "";
}

bool ODFOOXMLParser::isODFOOXML()
{
	impl->m_error = false;
	FILE* f = fopen(impl->m_file_name.c_str(), "r");
	if (f == NULL)
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name << ".\n";
		impl->m_error = true;
		return false;
	}
	fclose(f);
	DocToTextUnzip zipfile(impl->m_file_name.c_str());
	if (impl->m_log_stream != &std::cerr)
		zipfile.setLogStream(*impl->m_log_stream);
	if (!zipfile.open())
		return false;
	string main_file_name = locate_main_file(zipfile, *impl->m_log_stream);
	if (main_file_name == "")
	{
		zipfile.close();
		return false;
	}
	string contents;
	if (!zipfile.read(main_file_name, &contents, 1))
	{
		zipfile.close();
		return false;
	}
	zipfile.close();
	return true;
}

xmlNodePtr getAttribute(xmlAttrPtr Node, string AttributeName)
{
	if (Node == NULL)
		return NULL;
	xmlNodePtr interesting_node = NULL;
	if( ! xmlStrcmp(Node -> name, (const xmlChar*)AttributeName.c_str()))
		return Node -> children;
	else
		if((Node -> next != NULL) && (interesting_node == NULL))
			interesting_node = getAttribute(Node -> next, AttributeName);
	return interesting_node;
}

struct ODFOOXMLStringCommand
{
	ODFOOXMLCommand cmd;
	const char* name;
};

ODFOOXMLStringCommand odf_ooxml_commands [] =
{
	{ODFOOXML_TEXT, "text"},
	{ODFOOXML_PARA, "p"},
	{ODFOOXML_TAB, "tab"},
	{ODFOOXML_URL, "a"},
	{ODFOOXML_LIST, "list"},
	{ODFOOXML_TABLE, "table"},
	{ODFOOXML_ROW, "table-row"},
	{ ODF_CELL, "table-cell"},
	{ OOXML_ATTR, "attrName" }, // pptx
	{ OOXML_CELL, "c" }, // xlsx
};

static ODFOOXMLCommand odfOOXMLCommandNameToEnum(char *name)
{
	for(int i = 0 ; i < sizeof(odf_ooxml_commands) / sizeof(ODFOOXMLStringCommand) ; i ++)
	{
		if(strcmp(name, odf_ooxml_commands[i].name) == 0)
			return odf_ooxml_commands[i].cmd;
	}
	return ODFOOXML_UNKNOWN;
}

void ODFOOXMLParser::Implementation::execCommand(xmlNodePtr &Body, std::string &text, FormattingStyle options, ODFOOXMLCommand cmd, bool* children_processed)
{
	if (m_verbose_logging)
		*m_log_stream << "Command: " << cmd << endl;
	*children_processed = false;
	xmlNodePtr tmp;
	xmlNodePtr row;
	xmlNodePtr cell;
	std::string mlink;
	std::vector<std::string> list_vector;
	svector cell_vector;
	std::vector<svector> row_vector;
	switch(cmd)
	{
		case ODFOOXML_TEXT:
			if(Body -> content != NULL)
				text += (char*) Body -> content;
			break;
		case ODFOOXML_PARA:
			text += returnText(Body->children, options) + '\n';
			*children_processed = true;
			break;
		case ODFOOXML_TAB:
			text += "\t";
			break;
		case ODFOOXML_URL:
			tmp = getAttribute(Body -> properties, "href");
			if(tmp != NULL)
			{
				mlink = (char*) tmp -> content;
				text += formatUrl(mlink, options);
			}
			tmp = Body;
			text += returnText(tmp -> children, options);
			*children_processed = true;
			break;
		case ODFOOXML_LIST:
			tmp = Body -> children;
			while(tmp != NULL)
			{
				if(tmp -> children != NULL)
				{
					list_vector.push_back(returnText(tmp -> children, options));
				}
				tmp = tmp -> next;
			}

			text += formatList(list_vector, options);
			list_vector.clear();
			*children_processed = true;
			break;
		case ODFOOXML_TABLE:
			row = Body -> children;
			cell = NULL;

			while(row != NULL)
			{
				if( ! xmlStrcmp(row -> name, (const xmlChar*)"table-row"))
				{
					cell = row -> children;
					cell_vector.clear();

					while(cell != NULL)
					{
						if( ! xmlStrcmp(cell -> name, (const xmlChar*)"table-cell"))
						{
							cell_vector.push_back(returnText(cell -> children, options));
						}
						cell = cell -> next;
					}
					row_vector.push_back(cell_vector);
				}
				row = row -> next;
			}

			text += formatTable(row_vector, options);
			for(int i = 0 ; i < row_vector.size() ; i ++)
			{
				row_vector.at(i).clear();
			}
			row_vector.clear();
			*children_processed = true;
			break;
		case ODFOOXML_ROW:
			break;
		case ODF_CELL:
			break;
		case OOXML_ATTR:
			*children_processed = true;
			break;
		case OOXML_CELL:
			tmp = getAttribute(Body->properties, "t");
			if (tmp != NULL && string((char*)tmp->content) == "s")
			{
				tmp = Body;
				int shared_string_index = str_to_int(returnText(tmp->children, options));
				if (shared_string_index < SharedStrings.size())
					text += SharedStrings[shared_string_index];
			}
			else
				text += returnText(Body->children, options);
			if (Body->next != NULL)
				text += "\t";
			else
				text += "\n";
			*children_processed = true;
			break;
	}
}

std::string ODFOOXMLParser::Implementation::returnText(xmlNodePtr Body, FormattingStyle options)
{
	string text;
	xmlNodePtr t_next = Body;
	xmlNodePtr t_child;
	while(t_next != NULL)
	{
		bool children_processed;
		execCommand(t_next, text, options, odfOOXMLCommandNameToEnum((char*)t_next->name), &children_processed);
		if((t_next != NULL) && (t_next->children != NULL) && (!children_processed))
		{
			t_child = t_next -> children;
			text += returnText(t_child, options);
		}
		if(t_next != NULL)
		{
			t_next = t_next -> next;
		}
	}
	return text;
}

bool ODFOOXMLParser::Implementation::extractText(const string& xml_contents, XmlParseMode mode, FormattingStyle options, string* text)
{
	if (mode == STRIP_XML)
	{
		*text = "";
		bool in_tag = false;
		for (int i = 0; i < xml_contents.length(); i++)
		{
			char ch = xml_contents[i];
			if (ch == '<')
				in_tag = true;
			else if (ch == '>')
				in_tag = false;
			else if (!in_tag)
				*text += ch;
		}
		return true;
	}
	std::string xml;
	if (mode == FIX_XML)
	{
		DocToTextXmlFixer xml_fixer;
		xml = xml_fixer.fix(xml_contents);
	}
	else
		 xml = xml_contents;
	xmlDocPtr doc = xmlParseMemory(xml.c_str(), xml.length());
	if (doc == NULL)
		return false;
	xmlNodePtr root_elem = xmlDocGetRootElement(doc);
	*text = returnText(root_elem, options);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	xmlMemoryDump();
	return true;
}

string ODFOOXMLParser::plainText(XmlParseMode mode, FormattingStyle options)
{
	DocToTextUnzip zipfile(impl->m_file_name.c_str());
	if (impl->m_log_stream != &std::cerr)
		zipfile.setLogStream(*impl->m_log_stream);
	if (!zipfile.open())
	{
		*impl->m_log_stream << "Error opening file " << impl->m_file_name.c_str() << endl;
		return "";
	}
	string main_file_name = locate_main_file(zipfile, *impl->m_log_stream);
	if (main_file_name == "")
	{
		zipfile.close();
		return "";
	}
	string content;
	string text;
	if (main_file_name == "ppt/presentation.xml")
	{
		if (!zipfile.loadDirectory())
		{
			*impl->m_log_stream << "Error loading zip directory of file " << impl->m_file_name.c_str() << endl;
			return "";
		}
		for (int i = 1; zipfile.read("ppt/slides/slide" + int_to_str(i) + ".xml", &content) && i < 2500; i++)
		{
			string slide_text;
			if (!impl->extractText(content, mode, options, &slide_text))
			{
				*impl->m_log_stream << "parser error" << endl;
				zipfile.close();
				return "";
			}
			text += slide_text;
		}
	}
	else if (main_file_name == "xl/workbook.xml")
	{
		if (!zipfile.read("xl/sharedStrings.xml", &content))
		{
			*impl->m_log_stream << "Error reading xl/sharedStrings.xml" << endl;
			zipfile.close();
			return "";
		}
		std::string xml;
		if (mode == FIX_XML)
		{
			DocToTextXmlFixer xml_fixer;
			xml = xml_fixer.fix(content);
		}
		else if (mode == PARSE_XML)
			xml = content;
		else
		{
			*impl->m_log_stream << "XML stripping not possible for xlsx format" << endl;
			zipfile.close();
			return "";
		}
		xmlDocPtr doc = xmlParseMemory(xml.c_str(), xml.length());
		if (doc == NULL)
		{
			*impl->m_log_stream << "Error parsing xl/sharedStrings.xml" << endl;
			zipfile.close();
			return "";
		}
		xmlNodePtr root_elem = xmlDocGetRootElement(doc);
		xmlNodePtr elem = root_elem->children;
		while (elem != NULL)
		{
			if (string((char*)elem->name) == "si")
				impl->SharedStrings.push_back(impl->returnText(elem->children, options));
			elem = elem->next;
		}
		xmlFreeDoc(doc);
		xmlCleanupParser();
		xmlMemoryDump();
		for (int i = 1; zipfile.read("xl/worksheets/sheet" + int_to_str(i) + ".xml", &content); i++)
		{
			string sheet_text;
			if (!impl->extractText(content, mode, options, &sheet_text))
			{
				*impl->m_log_stream << "parser error" << endl;
				zipfile.close();
				return "";
			}
			text += sheet_text;
		}
	}
	else
	{
		if (!zipfile.read(main_file_name, &content))
		{
			*impl->m_log_stream << "Error reading " << main_file_name << endl;
			zipfile.close();
			return "";
		}
		if (!impl->extractText(content, mode, options, &text))
		{
			*impl->m_log_stream << "parser error" << endl;
			zipfile.close();
			return "";
		}
	}
	zipfile.close();
	return text;
}

bool ODFOOXMLParser::error(void)
{
	return impl->m_error;
}
