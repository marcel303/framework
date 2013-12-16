#pragma once

#include "CompiledShape.h"
#include "Forward.h"

class VectorShape
{
public:
	enum DrawMode
	{
		DrawMode_Texture,
		DrawMode_Silhouette
	};
	
	void SetTextureAtlas(const TextureAtlas* textureAtlas, const char* texturePrefix);
	
	int FindTag(const char* name) const;
	Vec2F TransformTag(int tagIndex, float x, float y, float angle) const;
	
	VRCS::CompiledShape m_Shape;
	const TextureAtlas* m_TextureAtlas;
	const Atlas_ImageInfo* m_Texture;
	const Atlas_ImageInfo* m_Silhouette;
};
