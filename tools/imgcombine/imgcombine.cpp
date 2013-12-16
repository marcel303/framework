#include <vector>
#include "ImageLoader_FreeImage.h"
#include "Parse.h"

class Settings
{
public:
	std::vector<std::string> src;
	std::string dst;
	int gridSx;
	int gridSy;
	int cellSx;
	int cellSy;
};

Settings gSettings;

int main(int argc, char* argv[])
{
	gSettings.dst = argv[1];
	gSettings.gridSx = Parse::Int32(argv[2]);
	gSettings.gridSy = Parse::Int32(argv[3]);
	gSettings.cellSx = Parse::Int32(argv[4]);
	gSettings.cellSy = Parse::Int32(argv[5]);

	for (int i = 6; i < argc; ++i)
	{
		gSettings.src.push_back(argv[i]);
	}
	
	ImageLoader_FreeImage loader;
	
	Image image;
	
	image.SetSize(gSettings.gridSx * gSettings.cellSx, gSettings.gridSy * gSettings.cellSy);
	
	for (size_t i = 0; i < gSettings.src.size(); ++i)
	{
		int cellX = i % gSettings.gridSx;
		int cellY = i / gSettings.gridSx;
		
		std::string fileName = gSettings.src[i];
		
		Image temp;
		
		loader.Load(temp, fileName);
		
		temp.Blit(&image, cellX * gSettings.cellSx, cellY * gSettings.cellSy);
	}
	
	loader.Save(image, gSettings.dst);
	
	return 0;
}