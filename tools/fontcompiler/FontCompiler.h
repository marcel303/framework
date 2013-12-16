#pragma once

#include "CompiledFont.h"
#include "Font.h"
#include "IImageLoader.h"

class FontCompiler
{
public:
	static void Create(const std::string& srcImage, const std::string& dst, int downScale, int spaceSx, int range1, int range2, bool shrinkHeight, int atlasSx, int atlasSy, bool shrinkAtlas, Font& out_Font, IImageLoader* loader);
	static void ExportImages(Font& font, IImageLoader* loader);
//	static void ExportResourceList(Font& font, std::string fileName);
	static void Compile(const Font& font, CompiledFont& out_Font);
};
