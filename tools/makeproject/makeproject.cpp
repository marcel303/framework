#include <algorithm>
#include "generator.h"
#include "vc10.h"

int main(int argc, char * argv[])
{
	const char * src = nullptr;

	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-c"))
		{
			src = argv[i + 1];
			i += 2;
		}
		else
		{
			printf("error: unknown command line option: %s\n", argv[i]);
			exit(-1);
		}
	}

	std::string basePath = src;

	std::replace(basePath.begin(), basePath.end(), '\\', '/');
	size_t pos = basePath.find_last_of('/');
	if (pos == std::string::npos)
		basePath = "./";
	else
		basePath = basePath.substr(0, pos + 1);

	XMLDocument document;

	if (document.LoadFile(src) != XML_NO_ERROR)
	{
		printf("failed to load settings file\n");
		exit(-1);
	}

	NodeWrapper root(&document);

	Vc10Generator generator;

	generator.Parse(&document, basePath.c_str());

	generator.WriteProjectFile(root);

	return 0;
}
