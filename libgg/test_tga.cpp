#include <stdlib.h>
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "ImageLoader_Tga.h"

int main(int argc, const char* argv[])
{
	if (argc < 2)
		exit(-1);

	const char* fileName = argv[1];

	Image image;
	
	ImageLoader_Tga loader;
	loader.Load(image, fileName);
	loader.Save(image, "test.tga");
	
//	ImageLoader_FreeImage loader2;
//	loader2.Save(image, "test.png");

	return 0;
}

