#pragma once

#include <map>
#include <vector>
#include "Atlas_ImageInfo.h"
#include "IImageLoader.h"
#include "ResIndex.h"
#include "Stream.h"

#define ATLAS_EXTENSION ".tga"
//#define ATLAS_EXTENSION ".png"

class Atlas
{
public:
	Atlas();
	~Atlas();

	void Setup(const std::vector<Atlas_ImageInfo*>& images, int sx, int sy);
	void Compose();

	void AddImage(Atlas_ImageInfo* image);
	const Atlas_ImageInfo* GetImage(const std::string& name) const;
	const Atlas_ImageInfo* GetImage(int index) const;
	
	void UpdateImageInfo(Atlas_ImageInfo* image);
	void Shrink();
	//PointI CalcSize() const;
	
	void Save(const std::string& fileName, IImageLoader* loader) const;
	void Load(Stream* stream, IImageLoader* loader);

	std::vector<Atlas_ImageInfo*> m_Images;
	std::map<std::string, Atlas_ImageInfo*> m_ImagesByName; // todo: use hash map ?
	std::string m_TextureFileName;
	Image* m_Texture;
	ResIndex m_ImageIndex;
	const Atlas_ImageInfo** m_ImagesByIndex;
};
