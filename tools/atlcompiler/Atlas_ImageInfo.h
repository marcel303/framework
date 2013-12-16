#pragma once

#include <string>
#include "IImageLoader.h"
#include "Image.h"
#include "Types.h"

class Atlas_ImageInfo
{
public:
	Atlas_ImageInfo(const std::string& fileName, const std::string& name, int imageSx, int imageSy, float texCoordX, float texCoordY, float texSx, float texSy)
	{
		Initialize();
		
		m_FileName = fileName;
		m_Name = name;
		
		SetupAtlas(imageSx, imageSy, texCoordX, texCoordY, texSx, texSy);
	}
	
	Atlas_ImageInfo(const std::string& fileName, const std::string& name, int borderSize, IImageLoader* loader)
	{
		Initialize();
		
		m_FileName = fileName;
		m_Name = name;
		m_BorderSize = borderSize;

		Load(m_FileName, loader);
	}
	
	void SetupAtlas(int imageSx, int imageSy, float texCoordX, float texCoordY, float texSx, float texSy)
	{
		m_ImageSize[0] = imageSx;
		m_ImageSize[1] = imageSy;
		m_TexCoord[0] = texCoordX;
		m_TexCoord[1] = texCoordY;
		m_TexSize[0] = texSx;
		m_TexSize[1] = texSy;
		m_TexCoord2[0] = texCoordX + texSx;
		m_TexCoord2[1] = texCoordY + texSy;
	}
	
	void Initialize()
	{
		m_Image = 0;
		m_ImageSize = PointI(0, 0);
		m_BorderSize = 0;
		m_Size = PointI(0, 0);
		m_Position = PointI(0, 0);
		m_Offset = PointI(0, 0);
		
		m_TexCoord[0] = 0.0f;
		m_TexCoord[1] = 0.0f;
		m_TexCoord2[0] = 0.0f;
		m_TexCoord2[1] = 0.0f;
		m_TexSize[0] = 0.0f;
		m_TexSize[1] = 0.0f;
	}

	void Load(const std::string& fileName, IImageLoader* loader)
	{
		Image* image = new Image();
		loader->Load(*image, fileName.c_str());
		m_Image = CreateBorder(image, m_BorderSize);
		m_ImageSize = PointI(image->m_Sx, image->m_Sy);
		delete image;
		image = 0;

		m_Offset = PointI(m_BorderSize, m_BorderSize);

		m_Size = PointI(
			m_ImageSize[0] + m_BorderSize * 2,
			m_ImageSize[1] + m_BorderSize * 2);
	}

	//
	
	Image* CreateBorder(const Image* image, int borderSize)
	{
		Image* result = new Image();
		
		result->SetSize(
			image->m_Sx + borderSize * 2,
			image->m_Sy + borderSize * 2);

		// middle

		image->Blit(result, borderSize, borderSize);

		// corners

		result->RectFill(0, 0, borderSize - 1, borderSize - 1, image->GetPixel(0, 0));
		result->RectFill(
			result->m_Sx - 1 - borderSize + 1,
			0,
			result->m_Sx - 1,
			borderSize - 1,
			image->GetPixel(image->m_Sx - 1, 0));
		result->RectFill(
			result->m_Sx - 1 - borderSize + 1,
			result->m_Sy - 1 - borderSize + 1,
			result->m_Sx - 1,
			result->m_Sy - 1,
			image->GetPixel(image->m_Sx - 1, image->m_Sy - 1));
		result->RectFill(
			0,
			result->m_Sy - 1 - borderSize + 1,
			borderSize - 1,
			result->m_Sy - 1,
			image->GetPixel(0, image->m_Sy - 1));

		// sides

		for (int i = 0; i < borderSize; ++i)
		{
			image->Blit(result,
				0,
				0,
				1, 
				image->m_Sy, 
				i,
				borderSize);

			image->Blit(result,
				image->m_Sx - 1, 
				0, 
				1,
				image->m_Sy,
				result->m_Sx - 1 - i, 
				borderSize);

			image->Blit(result, 
				0,
				0,
				image->m_Sx,
				1,
				borderSize,
				i);

			image->Blit(result,
				0, 
				image->m_Sy - 1,
				image->m_Sx,
				1,
				borderSize, 
				result->m_Sy - 1 - i);
		}

		return result;
	}

	static bool CompareBySize(const Atlas_ImageInfo* image1, const Atlas_ImageInfo* image2)
	{
#if 0
		int area1 = image1->m_Size[0] * image1->m_Size[1];
		int area2 = image2->m_Size[0] * image2->m_Size[1];

		if (area1 * 2 / 3 > area2)
			return true;

#endif

#if 1
		if (image1->m_Size[1] > image2->m_Size[1])
			return true;
		if (image1->m_Size[1] < image2->m_Size[1])
			return false;

		if (image1->m_Size[0] > image2->m_Size[0])
			return true;
		if (image1->m_Size[0] < image2->m_Size[0])
			return false;
#endif

		return false;
	}
	
	static bool CompareByName(const Atlas_ImageInfo* image1, const Atlas_ImageInfo* image2)
	{
		return image1->m_Name < image2->m_Name;
	}
	
	inline Vec2F ImageSizeF_get() const
	{
		return Vec2F((float)m_ImageSize[0], (float)m_ImageSize[1]);
	}

	std::string m_FileName;
	std::string m_Name;
	Image* m_Image;
	PointI m_ImageSize;
	//Point m_GlyphSize;
	int m_BorderSize;
	PointI m_Size;
	PointI m_Position;
	PointI m_Offset;
	float m_TexCoord[2];
	float m_TexSize[2];
	float m_TexCoord2[2];
};
