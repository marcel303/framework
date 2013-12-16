#include "FileStream.h"
#include "StreamReader.h"

bool Equal(std::string fileName1, std::string fileName2)
{
	FileStream stream1(fileName1.c_str(), OpenMode_Read);
	FileStream stream2(fileName2.c_str(), OpenMode_Read);
	
	if (stream1.Length_get() != stream2.Length_get())
		return false;
	
	StreamReader reader1(&stream1, false);
	StreamReader reader2(&stream2, false);
	
	while (!reader1.EOF_get() && !reader2.EOF_get())
	{
		std::string line1 = reader1.ReadLine();
		std::string line2 = reader2.ReadLine();
		
		if (line1 != line2)
			return false;
	}

	return true;
}

int main(int argc, char* const argv[])
{
	if (argc != 3)
	{
		printf("usage: gg-compare <file1> <file2>\n");
		printf("\n");
		printf("gg-compare will return 1 to stdout if both files are equal\n");
		printf("else, gg-compare will return 0\n");
		
		return -1;
	}
	
	std::string fileName1 = argv[1];
	std::string fileName2 = argv[2];
	
	bool equal = Equal(fileName1, fileName2);
	
	if (equal)
		printf("1");
	else
		printf("0");
	
	return 0;
}
