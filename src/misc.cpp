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

#include "misc.h"
#include <glib.h>
#include <iostream>
#include <stdio.h>

std::string ustring_to_string(const UString& s)
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

int prepareTable(std::vector<svector> &p_table)
{
	if(p_table.empty())
		return 0;
	for(int i = 0 ; i < p_table.size() ; i ++)
	{
		if(p_table.at(i).empty())
			continue;
		for(int j = 0 ; j < p_table.at(i).size() ; j ++)
		{
			if(p_table.at(i).at(j).length() < 2)
				continue;
			int k = p_table.at(i).at(j).find('\n', p_table.at(i).at(j).length() - 2);
			if(k >= 0)
				p_table.at(i).at(j).replace(k, 1, "");
		}
	}
	return 1;
}
int prepareList(std::vector<std::string> &p_list)
{
	if(p_list.empty())
		return 0;
	for(int i = 0 ; i < p_list.size() ; i ++)
	{
		if(p_list.at(i).empty())
			continue;
		if(p_list.at(i).length() < 2)
			continue;
		int k = p_list.at(i).find('\n', p_list.at(i).length() - 2);
		if(k >= 0)
			p_list.at(i).replace(k, 1, "");
	}
	return 1;
}



std::string formatTable(std::vector<svector>& mcols, const FormattingStyle& options)
{
	std::string table_out;

	prepareTable(mcols);

	for(int i = 0 ; i < mcols.size() ; i ++)
	{
		for(int j = 0 ; j < mcols.at(i).size() ; j ++)
		{
			table_out += mcols.at(i).at(j);
			switch(options.table_style)
			{
				case TABLE_STYLE_TABLE_LOOK:
					if(j + 1 == mcols.at(i).size())
					{
						table_out += "\n";
						break;
					}
					table_out += "\t";
					break;
				case TABLE_STYLE_ONE_ROW:
					table_out += " ";
					break;
				case TABLE_STYLE_ONE_COL:
					table_out += "\n";
					break;
				default:
					table_out += "\n";
					break;
			}
		}
	}
	return table_out;
}

std::string formatUrl(const std::string& mlink, const FormattingStyle& options)
{
	std::string u_url;
	int a = options.url_style;
	switch(a)
	{
		case URL_STYLE_TEXT_ONLY:
			u_url = "";
			break;
		case URL_STYLE_EXTENDED:
			u_url += "<";
			u_url += mlink;
			u_url += ">";
			break;
		default:
			u_url += "<";
			u_url += mlink;
			u_url += ">";
			break;
	}
	return u_url;
}

std::string formatList(std::vector<std::string>& mlist, const FormattingStyle& options)
{
	std::string list_out;
	char count[100] = "0";

	prepareList(mlist);

	for(int i = 0 ; i < mlist.size() ; i++)
	{
		sprintf(count, "%d", 1 + i);

		switch(options.list_style)
		{
			case LIST_STYLE_NUMBER:
				list_out = list_out + count + ". ";
				break;
			case LIST_STYLE_DASH:
				list_out = list_out + "- ";
				break;
			case LIST_STYLE_DOT:
				list_out = list_out + (char)0x2022;
				break;
			default:
				list_out = list_out + count + ". ";
				break;
		}
		list_out = list_out + mlist.at(i);
		list_out = list_out + "\n";
	}
	return list_out;
}
