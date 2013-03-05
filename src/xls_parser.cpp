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

#include "xls_parser.h"

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
	XLS_BOF = 0x809,
	XLS_BLANK = 0x201,
	XLS_CONTINUE = 0x3C,
	XLS_DATE_1904 = 0x22,
	XLS_FILEPASS = 0x2F,
	XLS_FORMAT = 0x41E,
	XLS_FORMULA = 0x06,
	XLS_INTEGER_CELL = 0x202, // Not in MS specification
	XLS_LABEL = 0x204,
	XLS_LABEL_SST = 0xFD,
	XLS_MULBLANK = 0xBE,
	XLS_MULRK = 0xBD,
	XLS_NUMBER = 0x203,
	XLS_RK = 0x27E,
	XLS_SST = 0xFC,
	XLS_STRING = 0x207,
	XLS_XF = 0xE0,
	XLS_EOF = 0x0A
};

struct XLSParser::Implementation
{
	bool m_error;
	std::string m_file_name;
	bool m_verbose_logging;
	std::ostream* m_log_stream;
	struct XFRecord
	{
		short int num_format_id;
	};
	std::vector<XFRecord> m_xf_records;
	double m_date_shift;
	std::vector<std::string> m_shared_string_table;
	std::vector<unsigned char> m_shared_string_table_buf;
	int m_prev_rec_type;
	int m_last_string_formula_row;
	int m_last_string_formula_col;
	std::set<int> m_defined_num_format_ids;
	int m_last_row;

	U16 getU16LittleEndian(std::vector<unsigned char>::const_iterator buffer)
	{
		return (unsigned short int)(*buffer) | ((unsigned short int)(*(buffer + 1)) << 8);
	}
	
	S32 getS32LittleEndian(std::vector<unsigned char>::const_iterator buffer)
	{
		return (long)(*buffer) | ((long)(*(buffer + 1)) << 8L) | ((long)(*(buffer + 2)) << 16L)|((long)(*(buffer + 3)) << 24L);
	}  

	class StandardDateFormats : public std::map<int, std::string>
	{
		public:
			StandardDateFormats()
			{
				insert(value_type(0x0E, "%m-%d-%y"));
				insert(value_type(0x0F, "%d-%b-%y"));
				insert(value_type(0x10, "%d-%b"));
				insert(value_type(0x11, "%b-%d"));
				insert(value_type(0x12, "%l:%M %p"));
				insert(value_type(0x13, "%l:%M:%S %p"));
				insert(value_type(0x14, "%H:%M"));
				insert(value_type(0x15, "%H:%M:%S"));
				insert(value_type(0x16, "%m-%d-%y %H:%M"));
				insert(value_type(0x2d, "%M:%S"));
				insert(value_type(0x2e, "%H:%M:%S"));
				insert(value_type(0x2f, "%M:%S"));
				insert(value_type(0xa4, "%m.%d.%Y %l:%M:%S %p"));
			}
	};

	bool oleEof(OLEStreamReader& reader)
	{
		return reader.tell() == reader.size();
	}

	std::string getStandardDateFormat(int xf_index)
	{
		static StandardDateFormats formats;
		if (xf_index >= m_xf_records.size())
		{
			*m_log_stream << "Incorrect format code " << xf_index << ".\n";
			return "";
		}
		int num_format_id = m_xf_records[xf_index].num_format_id;
		if (m_defined_num_format_ids.count(num_format_id))
			return "";
		StandardDateFormats::iterator i = formats.find(num_format_id);
		if (i == formats.end())
			return "";
		else
			return i->second;
	}	

	std::string xlsDateToString(double xls_date, std::string date_fmt)
	{
		time_t time = rint((xls_date - m_date_shift) * 86400);
		char buffer[128];
		strftime(buffer, 127, date_fmt.c_str(), gmtime(&time));  
		return buffer;
	}

	std::string formatXLSNumber(double number, short int xf_index)
	{
		std::string date_fmt = getStandardDateFormat(xf_index);
		if (date_fmt != "")
			return xlsDateToString(number, date_fmt);
		else
		{
			char buffer[128];
			sprintf(buffer,"%.12g",number);
			return buffer;
		}
	}

	std::string parseXNum(std::vector<unsigned char>::const_iterator src,int xf_index)
	{
		union
		{
			unsigned char xls_num[8];
			double num;
		} xnum_conv;
		#ifdef WORDS_BIGENDIAN
			std::reverse_copy(src, src + 8, xnum_conv.xls_num)
		#else
			std::copy(src, src + 8, xnum_conv.xls_num);
		#endif
		return formatXLSNumber(xnum_conv.num, xf_index);
	}

	std::string parseRkRec(std::vector<unsigned char>::const_iterator src, short int xf_index)
	{
		double number;
		if ((*src) & 0x02)
			number = (double)(getS32LittleEndian(src) >> 2);
		else
		{
			union
			{
				unsigned char xls_num[8];
				double num;
			} rk_num_conv;
			std::fill(rk_num_conv.xls_num, rk_num_conv.xls_num + 8, '\0');
		#ifdef WORDS_BIGENDIAN
			std:reverse_copy(src, src + 4, dconv.xls_num);
			rk_num_conv.xls_num[0] &= 0xfc;
		#else
			std::copy(src, src + 4, rk_num_conv.xls_num + 4);
			rk_num_conv.xls_num[3] &= 0xfc;
		#endif
			number = rk_num_conv.num;
		}
		if ((*src) & 0x01)
			number *= 0.01;
		return formatXLSNumber(number, xf_index);
	}

	std::string parseXLUnicodeString(std::vector<unsigned char>::const_iterator* src)
	{
		int flags = *(*src + 2);
		int count;
		#warning TODO: Is this ok to find if count is one or two long?
		if (flags & (~(1 | 4 | 8)))
		{
			count = **src;
			flags = *(*src+1);
			if (flags & (~(1 | 4 | 8)))
				return "";
			*src += 2;
		}
		else
		{
			count = getU16LittleEndian(*src);
			*src += 3;
		}
		int char_size = (flags & 0x01) ? 2 : 1;
		int after_text_block_len = 0;
		if (flags & 0x08) // rich text
		{
			after_text_block_len += 4*getU16LittleEndian(*src);
			*src += 2;
		}
		if (flags & 0x04) // asian
		{
			after_text_block_len += getS32LittleEndian(*src);
			*src += 4;
		}
		std::string dest;
		std::vector<unsigned char>::const_iterator s = *src;
		for (int i = 0; i < count; i++, s += char_size)
		{
			#warning TODO: Cleanup this hack
			if ((char_size == 1 && (*s == 1 || *s == 0)) ||
				(char_size == 2 && (*s == 1 || *s == 0) && *(s+1) != 4))
			{
				char_size = (*s & 0x01) ? 2 : 1;
				if (char_size == 2)
					s--;
				count++;
				continue;
			}
			if (char_size == 2)
			{
				gunichar uc =(unsigned short)getU16LittleEndian(s);
				gchar out[6];
				gint len = g_unichar_to_utf8(uc, out);
				out[len] = '\0';
				dest += std::string(out);
			}
			else
			{
				char* c = (char*)malloc(2 * sizeof(char));
				c[0] = *s;
				c[1] = '\0';
				std::string c2(1, *s);
				TextConverter tc("cp1251");
				#warning TODO: support other codepages than default 1251
				dest += ustring_to_string(tc.convert(c2));
			}
		}
		*src += count * char_size + after_text_block_len;
		return dest;
	}

	void parseSharedStringTable(const std::vector<unsigned char>& sst_buf)
	{
		int sst_size = getS32LittleEndian(sst_buf.begin() + 4);
		std::vector<unsigned char>::const_iterator src = sst_buf.begin() + 8;
		while (src < sst_buf.end() && m_shared_string_table.size() <= sst_size)
			m_shared_string_table.push_back(parseXLUnicodeString(&src));
	}	

	std::string cellText(int row, int col, const std::string& s)
	{
		std::string r;
		if (row > m_last_row)
		{
			r += "\n";
			m_last_row = row;
		}
		if (col > 0)
			r += "\t";
		r += s;
		return r;
	}

	bool processRecord(int rec_type, const std::vector<unsigned char>& rec, std::string& text)
	{
		if (m_verbose_logging)
			*m_log_stream << std::hex << "record=0x" << rec_type << std::endl;
		if (rec_type != XLS_CONTINUE && m_prev_rec_type == XLS_SST)
			parseSharedStringTable(m_shared_string_table_buf);
		switch (rec_type)
		{
			case XLS_BLANK:
			{
				int row = getU16LittleEndian(rec.begin());
				int col = getU16LittleEndian(rec.begin() + 2);
				text += cellText(row, col, "");
				break;
			}
			case XLS_BOF:
			{
				m_last_row = 0;
				#warning TODO: Check for stream type, ignore charts, or make it configurable
				#warning TODO: Mark beginning of sheet (configurable)
				break;
			}
			case XLS_CONTINUE:
			{
				if (m_prev_rec_type != XLS_SST)
					return true; // do not change m_prev_rec_type
				m_shared_string_table_buf.reserve(m_shared_string_table_buf.size() + rec.size());
				m_shared_string_table_buf.insert(m_shared_string_table_buf.end(), rec.begin(), rec.begin() + rec.size());
				return true;
			}
			case XLS_DATE_1904:
				m_date_shift = 24107.0; 
				break;
			case XLS_EOF:
			{
				text += "\n";
				#warning TODO: Mark end of sheet (configurable)
				break;
			}
			case XLS_FILEPASS:
			{
				*m_log_stream << "XLS file is encrypted.\n";
				return false;
			}
			case XLS_FORMAT:
			{
				int num_format_id = getU16LittleEndian(rec.begin());
				m_defined_num_format_ids.insert(num_format_id);
				break;
			}
			case XLS_FORMULA:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin());
				int col = getU16LittleEndian(rec.begin()+2);
				if (((unsigned char)rec[12] == 0xFF) && (unsigned char)rec[13] == 0xFF)
				{
					if (rec[6] == 0)
					{
						m_last_string_formula_row = row;
						m_last_string_formula_col = col;
					}
					else if (rec[6] == 1)
					{
						#warning TODO: check and test boolean formulas
						text += (rec[8] ? "TRUE" : "FALSE");
					}
					else if (rec[6] == 2)
						text += "ERROR";
				}
				else
				{
					int xf_index=getU16LittleEndian(rec.begin()+4);
					text += parseXNum(rec.begin() + 6,xf_index);
				}
				break;
			}
			case XLS_INTEGER_CELL:
			{
				int row = getU16LittleEndian(rec.begin());
				int col = getU16LittleEndian(rec.begin()+2);
				text += cellText(row, col, int2string(getU16LittleEndian(rec.begin() + 7)));
				break;
			}
			case XLS_LABEL:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin()); 
				int col = getU16LittleEndian(rec.begin() + 2);
				std::vector<unsigned char>::const_iterator src=rec.begin() + 6;
				text += cellText(row, col, parseXLUnicodeString(&src));
				break;
			}
			case XLS_LABEL_SST:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin()); 
				int col = getU16LittleEndian(rec.begin() + 2);
				int sst_index = getU16LittleEndian(rec.begin() + 6);
				if (sst_index >= m_shared_string_table.size() || sst_index < 0)
				{
					*m_log_stream << "Incorrect SST index.\n";
					return false;
				} else
					text += cellText(row, col, m_shared_string_table[sst_index]);
				break;
			}
			case XLS_MULBLANK:
			{
				int row = getU16LittleEndian(rec.begin());
				int start_col = getU16LittleEndian(rec.begin() + 2);
				int end_col=getU16LittleEndian(rec.begin() + rec.size() - 2);
				for (int c = start_col; c <= end_col; c++)
					text += cellText(row, c, "");
				break;
			}
			case XLS_MULRK:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin());
				int start_col = getU16LittleEndian(rec.begin() + 2);
				int end_col = getU16LittleEndian(rec.begin() + rec.size() - 2);
				for (int offset = 4, col = start_col; col <= end_col; offset += 6, col++)
				{
					int xf_index = getU16LittleEndian(rec.begin() + offset);
					text += cellText(row, col, parseRkRec(rec.begin() + offset + 2, xf_index));
				}
				break;
			}
			case XLS_NUMBER:
			case 0x03:
			case 0x103:
			case 0x303:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin());
				int col = getU16LittleEndian(rec.begin() + 2);
				text += cellText(row, col, parseXNum(rec.begin() + 6, getU16LittleEndian(rec.begin() + 4)));
				break;
			}
			case XLS_RK:
			{
				m_last_string_formula_row = -1;
				int row = getU16LittleEndian(rec.begin());
				int col = getU16LittleEndian(rec.begin() + 2);
				int xf_index = getU16LittleEndian(rec.begin() + 4);
				text += cellText(row, col, parseRkRec(rec.begin() + 6, xf_index));
				break;
			}
			case XLS_SST:
			{
				m_shared_string_table_buf.clear();
				m_shared_string_table.clear();
				m_shared_string_table_buf.reserve(rec.size());
				m_shared_string_table_buf.insert(m_shared_string_table_buf.end(), rec.begin(), rec.begin() + rec.size());
				break;
			}
			case XLS_STRING:
			{
				std::vector<unsigned char>::const_iterator src = rec.begin();
				if (m_last_string_formula_row < 0) {
					*m_log_stream << "String record without preceeding string formula.\n";
					break;
				}
				text += cellText(m_last_string_formula_row, m_last_string_formula_col, parseXLUnicodeString(&src));
				break;
			}
			case XLS_XF:
			case 0x43:
			{
				XFRecord xf_record;
				xf_record.num_format_id = getU16LittleEndian(rec.begin() + 2);
				m_xf_records.push_back(xf_record);
					break;
			} 
		}
		m_prev_rec_type = rec_type;
		return true;
	}  

	void parseXLS(OLEStreamReader& reader, std::string& text)
	{
		m_xf_records.clear();
		m_date_shift = 25569.0;
		m_shared_string_table.clear();
		m_shared_string_table_buf.clear();
		m_prev_rec_type = 0;
		int m_last_string_formula_row = -1;
		int m_last_string_formula_col = -1;
		m_defined_num_format_ids.clear();
		int m_last_row = 0;

		std::vector<unsigned char> rec;
		bool read_status = true;
		while (read_status)
		{
			int rec_type = reader.readU16();
			int rec_len = reader.readU16();
			enum BofRecordTypes
			{
				BOF_BIFF_2 = 0x009,
				BOF_BIFF_3 = 0x209,
				BOF_BIFF_4 = 0x0409,
				BOF_BIFF_5_AND_8 = XLS_BOF
			};
			if (rec_type == BOF_BIFF_2 || rec_type == BOF_BIFF_3 || rec_type == BOF_BIFF_4 || rec_type == BOF_BIFF_5_AND_8)
			{
				int bof_struct_size;
				#warning Has all BOF records size 8 or 16?
				if (rec_len == 8 || rec_len == 16)
				{
					switch (rec_type)
					{
						case BOF_BIFF_5_AND_8:
						{
							#warning TODO: Check, test, cleanup this
							long build_rel = reader.readU16();
							long build_year = reader.readU16();
							if(build_year > 5)
							{
								rec.resize(8);
								read_status = reader.read(&*rec.begin(), 8);
								/* biff version 8 */
								bof_struct_size = 16;
							}
							else
							{
								/* biff_version 7 */
								bof_struct_size = 8;
							}
							break;
						}
						case BOF_BIFF_3:
							bof_struct_size = 6;
							break;
						case BOF_BIFF_4:
							bof_struct_size = 6;
							break;
						default:
							bof_struct_size = 4;
					}
					rec.resize(rec_len - (bof_struct_size - 4));
					read_status = reader.read(&*rec.begin(), rec_len - (bof_struct_size - 4));
					break;
				}
				else
				{
					*m_log_stream << "Invalid BOF record.\n";
					return;
				} 
			}
			else
			{
				rec.resize(126);
				read_status = reader.read(&*rec.begin(), 126);
			}
		}
		if (oleEof(reader))
		{
			*m_log_stream << "No BOF record found.\n";
			return;
		}
		bool eof_rec_found = false;
		while (read_status)
		{
			int rec_type = reader.readU16();
			if (oleEof(reader))
			{
				processRecord(XLS_EOF, std::vector<unsigned char>(), text);
				return;
			}
			int rec_len = reader.readU16();
			if (rec_len > 0)
			{
				rec.resize(rec_len);
				read_status = reader.read(&*rec.begin(), rec_len);
			}
			else
				rec.clear();
			if (eof_rec_found)
			{
				if (rec_type != XLS_BOF)
					break;
			}
			if (!processRecord(rec_type, rec, text))
				return;
			if (rec_type == XLS_EOF)
				eof_rec_found = true;
			else
				eof_rec_found = false;	
		}
	}
};

XLSParser::XLSParser(const std::string& file_name)
{
	impl = new Implementation();
	impl->m_error = false;
	impl->m_file_name = file_name;
	impl->m_verbose_logging = false;
	impl->m_log_stream = &std::cerr;
}

XLSParser::~XLSParser()
{
	delete impl;
}

void XLSParser::setVerboseLogging(bool verbose)
{
	impl->m_verbose_logging = verbose;
}

void XLSParser::setLogStream(std::ostream& log_stream)
{
	impl->m_log_stream = &log_stream;
}

bool XLSParser::isXLS()
{
	impl->m_error = false;
	OLEStorage storage(impl->m_file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		return false;
	}
	OLEStreamReader* reader = storage.createStreamReader("Workbook");
	if (reader == NULL)
		reader = storage.createStreamReader("Book");
	if (reader == NULL)
		return false;
	delete reader;
	return true;
}

std::string XLSParser::plainText(const FormattingStyle& formatting)
{
	impl->m_error = false;
	OLEStorage storage(impl->m_file_name);
	if (!storage.open(OLEStorage::ReadOnly))
	{
		*impl->m_log_stream << "Error opening " << impl->m_file_name << " as OLE file.\n";
		impl->m_error = true;
		return "";
	}
	OLEStreamReader* reader = storage.createStreamReader("Workbook");
	if (reader == NULL)
		reader = storage.createStreamReader("Book");
	if (reader == NULL)
	{
		*impl->m_log_stream << "Error opening " << impl->m_file_name << " as OLE file.\n";
		impl->m_error = true;
		return "";
	}
	std::string text;
	impl->parseXLS(*reader, text);
	delete reader;
	return text;
}

bool XLSParser::error()
{
	return impl->m_error;
}
