#include <iostream>
#include "Exception.h"
#include "ImageLoader_Photoshop.h"
#include "Path.h"
//#include "StringEx.h"

int main (int argc, char * const argv[])
{
	try
	{
		if (argc < 2)
			throw ExceptionVA("missing filename");
		
		std::string fileName = argv[1];
		
		ImageLoader_Photoshop loader;
		
		Image image;
		
		loader.Load(image, fileName);
		
		//
		
		std::string dst = Path::StripExtension(fileName) + "_output.psd";
		
		image.SetSize(32, 32);

		loader.Save(image, dst);
		
		//
		
		loader.Load(image, dst);
		
		return 0;
	}
	catch (std::exception& e)
	{
		std::cout << e.what();
		
		return -1;
	}
}
