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

#include "doctotext_c_api.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include "plain_text_extractor.h"

char* extract_plain_text(const char* file_name, int verbose_logging)
{
	#warning TODO: Add a possibility to set other parser than AUTO.
	#warning TODO: Add a possibility to set formatting style.
	#warning TODO: Add a possibility to set xml parsing mode.
	PlainTextExtractor extractor;
	#warning TODO: Add a possibility to set other log stream via C API.
	extractor.setVerboseLogging(verbose_logging);

	std::string text;
	if (!extractor.processFile(file_name, text))
		return NULL;

	char* result = (char*)malloc((text.length() + 1) * sizeof(char));
	strcpy(result, text.c_str());
	return result;
}
