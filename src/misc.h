#ifndef DOCTOTEXT_MISC_H
#define DOCTOTEXT_MISC_H

#include "formatting_style.h"
#include <string>
#include <vector>
#include <ustring.h>
using namespace wvWare;

typedef std::vector<std::string> svector;

std::string formatTable(std::vector<svector>& mcols, const FormattingStyle& options);
std::string formatUrl(const std::string& mlink, const FormattingStyle& options);
std::string formatList(std::vector<std::string>& mlist, const FormattingStyle& options);

std::string ustring_to_string(const UString& s);

#endif
