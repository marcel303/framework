#pragma once

#include "Calc.h"
#include "Mat4x4.h"
#include "Types.h"

#define SPGFX_INTERLEAVED

typedef struct SpriteCoord
{
	float x;
	float y;
#ifdef PSP
	float z;
#endif
} SpriteCoord;

typedef struct SpriteColor
{
	union
	{
		uint8_t v[4];
		uint32_t rgba;
	};
} SpriteColor;

inline SpriteColor SpriteColor_Make(int r, int g, int b, int a)
{
	SpriteColor result;
	
	result.v[0] = r;
	result.v[1] = g;
	result.v[2] = b,
	result.v[3] = a;
	
	return result;
}

inline SpriteColor SpriteColor_MakeF(float r, float g, float b, float a)
{
	SpriteColor result;
	
	result.v[0] = (uint8_t)(r * 255.0f);
	result.v[1] = (uint8_t)(g * 255.0f);
	result.v[2] = (uint8_t)(b * 255.0f);
	result.v[3] = (uint8_t)(a * 255.0f);
	
	return result;
}

inline SpriteColor SpriteColor_Modulate(const SpriteColor& c1, const SpriteColor& c2)
{
	SpriteColor result;
	
	result.v[0] = (c1.v[0] * c2.v[0]) >> 8;
	result.v[1] = (c1.v[1] * c2.v[1]) >> 8;
	result.v[2] = (c1.v[2] * c2.v[2]) >> 8;
	result.v[3] = (c1.v[3] * c2.v[3]) >> 8;
	
	return result;
}

inline SpriteColor SpriteColor_Modulate(const SpriteColor& c, float t)
{
	SpriteColor result;
	
	const int temp = (int)(t * 255.0f);
	
	result.v[0] = (c.v[0] * temp) >> 8;
	result.v[1] = (c.v[1] * temp) >> 8;
	result.v[2] = (c.v[2] * temp) >> 8;
	result.v[3] = (c.v[3] * temp) >> 8;
	
	return result;
}

// add with saturation

inline SpriteColor SpriteColor_AddSat(const SpriteColor& c1, const SpriteColor& c2)
{
	SpriteColor result;
	
	for (int i = 0; i < 4; ++i)
	{
		const int v = c1.v[i] + c2.v[i];
		
		if (v > 255)
			result.v[i] = 255;
		else
			result.v[i] = v;
	}
	
	return result;
}

inline SpriteColor SpriteColor_AddSatF(const SpriteColor& c1, const SpriteColor& c2, float t)
{
	SpriteColor result;
	
	for (int i = 0; i < 4; ++i)
	{
		const int v = c1.v[i] + (int)(c2.v[i] * t);
		
		if (v > 255)
			result.v[i] = 255;
		else
			result.v[i] = v;
	}
	
	return result;
}

inline SpriteColor SpriteColor_Blend(SpriteColor from, SpriteColor to, int t)
{
	SpriteColor result;
	
	for (int i = 0; i < 4; ++i)
		result.v[i] = (from.v[i] * (255 - t) + to.v[i] * t) >> 8;
	
	return result;
}

inline SpriteColor SpriteColor_BlendF(SpriteColor from, SpriteColor to, float t)
{
	SpriteColor result;
	
	for (int i = 0; i < 4; ++i)
		result.v[i] = (uint8_t)(from.v[i] * (1.0f - t) + to.v[i] * t);
	
	return result;
}

inline SpriteColor SpriteColor_Scale(SpriteColor color, int s)
{
	return SpriteColor_Make(
		(color.v[0] * s) >> 8,
		(color.v[1] * s) >> 8,
		(color.v[2] * s) >> 8,
		color.v[3]);
}

inline SpriteColor SpriteColor_ScaleF(SpriteColor color, float s)
{
	return SpriteColor_Scale(color, (int)(s * 255.0f));
}

namespace SpriteColors
{
	const static SpriteColor Black = SpriteColor_Make(0, 0, 0, 255);
	const static SpriteColor White = SpriteColor_Make(255, 255, 255, 255);
	
	extern SpriteColor Red;
	extern SpriteColor Green;
	extern SpriteColor Blue;
	
	extern SpriteColor HitEffect;
	extern SpriteColor HitEffect_Boss;
}

typedef struct SpriteTexCoord
{
	float u;
	float v;
} SpriteTexCoord;

class SpriteVertex
{
public:
	inline SpriteVertex()
	{
#ifdef PSP
		m_Coord.z = 0.0f;
#endif
	}
	
	inline SpriteVertex(float x, float y, SpriteColor c, float u, float v)
	{
		m_Coord.x = x;
		m_Coord.y = y;
#ifdef PSP
		m_Coord.z = 0.0f;
#endif
		m_Color = c;
		m_TexCoord.u = u;
		m_TexCoord.v = v;
	}
	
#ifdef PSP
	SpriteTexCoord m_TexCoord;
	SpriteColor m_Color;
	SpriteCoord m_Coord;
#else
	SpriteCoord m_Coord;
	SpriteColor m_Color;
	SpriteTexCoord m_TexCoord;
#endif
};

typedef uint16_t SpriteIndex;

class Sprite
{
public:
	Sprite();
	~Sprite();
	
	void Allocate(int vertexCount, int indexCount);
	
	int m_VertexCount;
	SpriteVertex* m_Vertices;
	SpriteIndex* m_Indices;
	int m_IndexCount;
};

enum SpriteGfxRenderTime
{
	SpriteGfxRenderTime_Undefined,
	SpriteGfxRenderTime_OnFlush,
	SpriteGfxRenderTime_OnFrameEnd
};

class SpriteGfx
{
public:
	SpriteGfx();
	SpriteGfx(int vertexCount, int indexCount, SpriteGfxRenderTime renderTime);	
	~SpriteGfx();
	void Initialize();
	
	void Setup(int vertexCount, int indexCount, SpriteGfxRenderTime renderTime);
	
	void Reserve(int vertexCount, int indexCount);
	
	void Flush();
	void Reset(bool resetDrawBase);
	void FrameEnd();
	
	inline void WriteBegin()
	{
		// set base vertex index for indexed rendering
		
		m_BaseVertexIndex = m_WriteVertexIndex;
	}
	
	inline void WriteEnd()
	{
		// nop
	}
	
	inline void WriteVertex(const SpriteVertex& vertex)
	{
#ifdef SPGFX_INTERLEAVED
		m_Vertices[m_WriteVertexIndex] = vertex;
#else
		m_Coords[m_WriteVertexIndex] = vertex.m_Coord;
		m_Colors[m_WriteVertexIndex] = vertex.m_Color;
		m_TexCoords[m_WriteVertexIndex] = vertex.m_TexCoord;
#endif
		
		m_WriteVertexIndex++;
	}
	
	inline void WriteVertex(float x, float y, int rgba, float u, float v)
	{
#ifdef SPGFX_INTERLEAVED
		SpriteVertex& vertex = m_Vertices[m_WriteVertexIndex];
		
		vertex.m_Coord.x = x;
		vertex.m_Coord.y = y;
#ifdef PSP
		vertex.m_Coord.z = 0.0f;
#endif
		vertex.m_Color.rgba = rgba;
		vertex.m_TexCoord.u = u;
		vertex.m_TexCoord.v = v;
#else
		m_Coords[m_WriteVertexIndex].x = x;
		m_Coords[m_WriteVertexIndex].y = y;
#ifdef PSP
		m_Coords[m_WriteVertexIndex].z = 0.0f;
#endif
		m_Colors[m_WriteVertexIndex].rgba = rgba;
		m_TexCoords[m_WriteVertexIndex].u = u;
		m_TexCoords[m_WriteVertexIndex].v = v;
#endif
		
		m_WriteVertexIndex++;
	}
	
	inline void WriteIndex(SpriteIndex vertexIndex)
	{
		vertexIndex += m_BaseVertexIndex - m_DrawVertexBaseIndex;
		
		m_Indices[m_WriteIndexIndex] = vertexIndex;
		
		m_WriteIndexIndex++;

		m_DrawIndexCount++;
	}
	
	inline void WriteIndex3(SpriteIndex vertexIndex1, SpriteIndex vertexIndex2, SpriteIndex vertexIndex3)
	{
		const register int base = m_BaseVertexIndex - m_DrawVertexBaseIndex;
		
		SpriteIndex* index = m_Indices + m_WriteIndexIndex;
		
		index[0] = + vertexIndex1 + base;
		index[1] = + vertexIndex2 + base;
		index[2] = + vertexIndex3 + base;
		
		m_WriteIndexIndex += 3;
		m_DrawIndexCount += 3;
	}
	
	inline void WriteIndexN(const SpriteIndex* indexArray, int indexCount)
	{
		const register int base = + m_BaseVertexIndex - m_DrawVertexBaseIndex;
		
		SpriteIndex* index = m_Indices + m_WriteIndexIndex;
		
		for (int i = 0; i < indexCount; ++i)
			index[i] = indexArray[i] + base;
		
		m_WriteIndexIndex += indexCount;
		m_DrawIndexCount += indexCount;
	}
	
	void WriteSprite(const Sprite& sprite);	
	void WriteSprite(const Sprite& sprite, float x, float y, float angle, float scaleX, float scaleY);
	void WriteSprite(const Vec3* positionArray, const SpriteColor* colorArray, const SpriteTexCoord* texCoordArray, int vertexCount, const SpriteIndex* indexArray, int indexCount, const Mat4x4& mat);
	
	void DBG_Render();

private:
	void Render();

	inline XBOOL HasCapacity(int vertexCount, int indexCount) const
	{
		if (m_WriteVertexIndex + vertexCount > m_VertexCount)
			return XFALSE;
		if (m_WriteIndexIndex + indexCount > m_IndexCount)
			return XFALSE;
		
		return XTRUE;
	}
	
	// Buffers
	int m_VertexCount;
	int m_IndexCount;
#ifdef SPGFX_INTERLEAVED
	SpriteVertex* m_Vertices;
#else
	SpriteCoord* m_Coords;
	SpriteColor* m_Colors;
	SpriteTexCoord* m_TexCoords;
#endif
	SpriteIndex* m_Indices;

	// Rendering
	SpriteGfxRenderTime m_RenderTime;
	int m_BaseVertexIndex;
	int m_WriteVertexIndex;
	int m_WriteIndexIndex;

	// Rendering - Draw Call
	int m_DrawVertexBaseIndex;
	int m_DrawVertexCount;
	int m_DrawIndexBaseIndex;
	int m_DrawIndexCount;
};
