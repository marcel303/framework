#include "AtlasBuilderV2.h"
#include "FileStream.h"
#include "FontCompiler.h"
#include "Path.h"
#include "StreamWriter.h"
#include "StringEx.h"

#warning todo: get content scale from argument list; required to do proper sx/sy calculation for glyphs

#define ALPHA_TRESHOLD 15

static bool Minimize(Image& image, int x1, int y1, int x2, int y2, int& out_X1, int& out_X2);
static void MinimizeY(Image& image, int& out_Y1, int& out_Y2);

void FontCompiler::Create(const std::string& srcImage, const std::string& dst, int downScale, int spaceSx, int range1, int range2, bool shrinkHeight, int atlasSx, int atlasSy, bool shrinkAtlas, Font& out_Font, IImageLoader* loader)
{
	// load image
	
	Image image_full;
	
	loader->Load(image_full, srcImage);
	
	Image image;
	
	image_full.DemultiplyAlpha();
	image_full.DownscaleTo(image, downScale);
	
	int sx = image.m_Sx;
	int sy = image.m_Sy;
	
	int areaSx = sx / 16;
	int areaSy = sy / 16;
	
	out_Font.m_SpaceSx = spaceSx;
	
	// generate glyphs
	
	for (int gy = 0; gy < 16; ++gy)
	{
		for (int gx = 0; gx < 16; ++gx)
		{
			int index = gx + gy * 16;
			
			if (index < range1 || index > range2)
				continue;
			
			int x1 = (gx + 0) * areaSx - 0;
			int y1 = (gy + 0) * areaSy - 0;
			int x2 = (gx + 1) * areaSx - 1;
			int y2 = (gy + 1) * areaSy - 1;
			
			if (!Minimize(image, x1, y1, x2, y2, x1, x2))
				continue;
			
			int glyphSx = x2 - x1 + 1;
			int glyphSy = y2 - y1 + 1;
			
			//
			
			FontGlyph* glyph = new FontGlyph();
						
			// export glyph image
			
			Image glyphImage;
			
			glyphImage.SetSize(glyphSx, glyphSy);
			
			image.Blit(&glyphImage, x1, y1, glyphSx, glyphSy, 0, 0);
			
			//
			
			std::string shapeName = Path::StripExtension(dst) + String::Format("_c%d.png", index);
			
			//printf("dst: %s, shapeName: %s\n", dst.c_str(), shapeName.c_str());
			
			glyph->Setup(shapeName, glyphSx * downScale / 2, glyphSy * downScale / 2, glyphImage);
			
			out_Font.Glyph_set(index, glyph);
		}
	}
	
	if (shrinkHeight)
	{
		// Calculate font height
		
		int minY = -1;
		int maxY = -1;
		
		for (int i = 0; i < 256; ++i)
		{
			FontGlyph* glyph = out_Font.Glyph_get(i);
			
			if (!glyph)
				continue;
			
			int y1;
			int y2;
			
			MinimizeY(glyph->m_Image, y1, y2);
				
			if (y1 < minY || minY < 0)
				minY = y1;
			if (y2 > maxY || maxY < 0)
				maxY = y2;
		}
		
		if (minY >= 0)
		{
			//LOG_DBG("resize y: %d, %d\n", minY, maxY);
			
			for (int i = 0; i < 256; ++i)
			{
				FontGlyph* glyph = out_Font.Glyph_get(i);
				
				if (!glyph)
					continue;
				
				Image& image = glyph->m_Image;
				
				int x1 = 0;
				int x2 = image.m_Sx - 1;
				int y1 = minY;
				int y2 = maxY;
				
				int glyphSx = x2 - x1 + 1;
				int glyphSy = y2 - y1 + 1;
				
				Image glyphImage;
				
				glyphImage.SetSize(glyphSx, glyphSy);
				
				image.Blit(&glyphImage, x1, y1, glyphSx, glyphSy, 0, 0);
				
				glyph->Setup(glyph->m_ShapeName, glyphSx * downScale / 2, glyphSy * downScale / 2, glyphImage);
			}
		}
	}
	
	// Write images to disk
	
	ExportImages(out_Font, loader);
	
	// Create texture atlas from generated glyphs
	
	AtlasBuilderV2 builder;
	
	builder.Setup(atlasSx, atlasSy, 1);
	
	std::vector<Atlas_ImageInfo*> images;
	
	for (int i = 0; i < 256; ++i)
	{
		FontGlyph* glyph = out_Font.Glyph_get(i);
		
		if (!glyph)
			continue;
		
		// todo: don't load from disk.. :/
		
		Atlas_ImageInfo* image = new Atlas_ImageInfo(glyph->m_ShapeName, Path::GetBaseName(glyph->m_ShapeName), 1, loader);
		
		glyph->m_AtlasImage = image;
		
		images.push_back(image);
	}
	
	// Generate atlas and export it to disk
	
	Atlas* atlas = builder.Create(images);
	
	if (shrinkAtlas)
		atlas->Shrink();
	
	std::string dstFileName = dst + ".png";
	
	loader->Save(*atlas->m_Texture, dstFileName);
	
	out_Font.m_TextureFileName = dstFileName;
	
	// Copy atlas details into glyphs
	
	for (int i = 0; i < 256; ++i)
	{
		FontGlyph* glyph = out_Font.Glyph_get(i);
		
		if (!glyph)
			continue;
		
		glyph->m_Min[0] = glyph->m_AtlasImage->m_TexCoord[0];
		glyph->m_Min[1] = glyph->m_AtlasImage->m_TexCoord[1];
		glyph->m_Max[0] = glyph->m_AtlasImage->m_TexCoord2[0];
		glyph->m_Max[1] = glyph->m_AtlasImage->m_TexCoord2[1];
	}
	
	delete atlas;
}

void FontCompiler::ExportImages(Font& font, IImageLoader* loader)
{
	for (int i = 0; i < 256; ++i)
	{
		FontGlyph* glyph = font.Glyph_get(i);
		
		if (!glyph)
			continue;
		
		std::string fileName = glyph->m_ShapeName;
		
		//printf("save: %s\n", glyph->m_ShapeName.c_str());
		
		loader->Save(glyph->m_Image, fileName);
	}
}

/*
void FontCompiler::ExportResourceList(Font& font, std::string fileName)
{
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Write);
	
	StreamWriter writer(&stream, false);
	
	for (int i = 0; i < 256; ++i)
	{
		FontGlyph* glyph = font.Glyph_get(i);
		
		if (!glyph)
			continue;
		
		writer.WriteText(String::Format("texture %s %s\n", Path::GetBaseName(glyph->m_ShapeName).c_str(), Path::GetBaseName(glyph->m_ShapeName).c_str()));
	}
	
	writer.WriteText(String::Format("font %s %s\n", Path::GetBaseName(fileName).c_str(), Path::GetBaseName(fileName).c_str()));
}*/

void FontCompiler::Compile(const Font& font, CompiledFont& out_Font)
{
	out_Font.m_TextureFileName = Path::GetFileName(font.m_TextureFileName).c_str();
	
	out_Font.m_SpaceSx = font.m_SpaceSx;
	
	for (int i = 0; i < 256; ++i)
	{
		const FontGlyph* glyph = font.Glyph_get(i);
		
		if (!glyph)
			continue;
		
		CompiledFont_Glyph& compiledGlyph = out_Font.m_Glyphs[i];
		
		compiledGlyph.Setup(glyph->m_SizeX, glyph->m_SizeY, glyph->m_Min, glyph->m_Max);
	}
	
	out_Font.CalculateHeight();
}

//

static bool IsEmpty(Image& image, int x, int y1, int y2)
{
	// Returns true if an entire column in a vertical line segment is empty
	
	for (int y = y1; y <= y2; ++y)
	{
		ImagePixel pixel = image.GetPixel(x, y);
		
		if (pixel.a > ALPHA_TRESHOLD)
			return false;
	}
	
	return true;
}

static bool IsEmptyY(Image& image, int x1, int x2, int y)
{
	// Returns true if an entire row in a horizontal line segment is empty
	
	for (int x = x1; x <= x2; ++x)
	{
		ImagePixel pixel = image.GetPixel(x, y);
		
		if (pixel.a > ALPHA_TRESHOLD)
			return false;
	}
	
	return true;
}

static bool Minimize(Image& image, int x1, int y1, int x2, int y2, int& out_X1, int& out_X2)
{
	// Reduce the width of the rectangular area to encompass solid pixels
	
	out_X1 = x1;
	out_X2 = x2;
	
	for (int x = x1; x <= x2; ++x)
	{
		if (!IsEmpty(image, x, y1, y2))
			break;
		else
			out_X1 = x;
	}
	
	for (int x = x2; x >= x1; --x)
	{
		if (!IsEmpty(image, x, y1, y2))
			break;
		else
			out_X2 = x;
	}
	
	return out_X1 <= out_X2;
}

static void MinimizeY(Image& image, int& out_Y1, int& out_Y2)
{
	// Reduce height of the rectangular area to encompass solid pixels
	
	int y1 = 0;
	int y2 = image.m_Sy - 1;
	
	out_Y1 = 0;
	out_Y2 = image.m_Sy - 1;
	
	int x1 = 0;
	int x2 = image.m_Sx - 1;
	
	for (int y = y1; y <= y2; ++y)
	{
		if (!IsEmptyY(image, x1, x2, y))
			break;
		else
			out_Y1 = y;
	}
	
	for (int y = y2; y >= y1; --y)
	{
		if (!IsEmptyY(image, x1, x2, y))
			break;
		else
			out_Y2 = y;
	}
}
