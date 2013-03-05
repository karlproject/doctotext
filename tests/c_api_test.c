#include "../build/doctotext_c_api.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
	char* file_name;
	int verbose = 0;
	if (argc == 2)
		file_name = argv[1];
	else if (argc == 3 && strcmp(argv[1], "--verbose") == 0)
	{
		verbose = 1;
		file_name = argv[2];
	}
	else
		return 1;
	char* text = extract_plain_text(file_name, verbose);
	printf("%s\n", text);
	free(text);
	return 0;
}
