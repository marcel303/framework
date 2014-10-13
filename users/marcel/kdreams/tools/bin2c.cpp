#include <stdio.h>

#ifndef WIN32
	#define sprintf_s(dst, dstSize, fmt, ...) sprintf(dst, fmt, __VA_ARGS__)
#endif

int main(int argc, const char * argv[])
{
	if (argc < 4)
	{
		printf("usage: bin2c src dst sym\n");
		printf("src = binary input file\n");
		printf("dst = name of the output file (excluding .c or .h extension)\n");
		printf("sym = symbolic name\n");
		return -1;
	}

	const char * src = argv[1];

	char dstc[1024];
	char dsth[1024];

	sprintf_s(dstc, 1024, "%s.c", argv[2]);
	sprintf_s(dsth, 1024, "%s.h", argv[2]);

	const char * sym = argv[3];

	FILE * ifile = fopen(src, "rb");
	FILE * cfile = fopen(dstc, "wt");
	FILE * hfile = fopen(dsth, "wt");
	
	if (!ifile || !cfile || !hfile)
	{
		printf("file error\n");
		return -1;
	}

	fseek(ifile, 0, SEEK_END);
	int len = ftell(ifile);
	fseek(ifile, 0, SEEK_SET);

	unsigned char * bytes = new unsigned char[len];

	if (fread(bytes, 1, len, ifile) != len)
	{
		printf("file error\n");
		return -1;
	}

	fprintf(hfile,
		"#pragma once\n"
		"extern char %s[%d];\n"
		"extern const unsigned int %s_len;\n",
		sym,
		len,
		sym);

	fprintf(cfile,
		"char %s[%d] = {",
		sym,
		len);
	for (int i = 0; i < len; ++i)
	{
		fprintf(cfile, "%c0x%x\n",
			i == 0 ? ' ' : ',',
			bytes[i]);
	}
	fprintf(cfile,
		"};\n"
		"const unsigned int %s_len = %d;\n",
		sym,
		len);

	fclose(ifile);
	fclose(cfile);
	fclose(hfile);

	return 0;
}
