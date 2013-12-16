#include "Atlas.h"
#include "Calc.h"
#include "Debugging.h"
#include "TextureAtlas.h"
#include "VectorShape.h"

void VectorShape::SetTextureAtlas(const TextureAtlas* textureAtlas, const char* texturePrefix)
{
	m_TextureAtlas = textureAtlas;
	m_Texture = textureAtlas->m_Atlas->GetImage(texturePrefix + m_Shape.m_TextureFileName);
	m_Silhouette = textureAtlas->m_Atlas->GetImage(texturePrefix + m_Shape.m_SilhouetteFileName);
	
	Assert(m_Texture != 0);
}

int VectorShape::FindTag(const char* name) const
{
	for (int i = 0; i < m_Shape.m_LogicTagCount; ++i)
		if (m_Shape.m_LogicTags[i].m_Name == name)
			return i;
	
	return -1;
}

Vec2F VectorShape::TransformTag(int tagIndex, float x, float y, float angle) const
{
	if (tagIndex < 0)
		return Vec2F(x, y);
	
	const float* position = m_Shape.m_LogicTags[tagIndex].m_Position;
	
	float axis[4];
	
	Calc::RotAxis_Fast(angle, axis);
	
	Vec2F result;
	
	result[0] = position[0] * axis[0] + position[1] * axis[1] + x;
	result[1] = position[0] * axis[2] + position[1] * axis[3] + y;
	
	return result;
}
