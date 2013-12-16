#include <string.h>
#include "SpriteGfx.h"
#include "SpriteRenderer.h"

//

namespace SpriteColors
{
//	SpriteColor Black = SpriteColor_Make(0, 0, 0, 255);
//	SpriteColor White = SpriteColor_Make(255, 255, 255, 255);
	
	SpriteColor Red = SpriteColor_Make(255, 0, 0, 255);
	SpriteColor Green = SpriteColor_Make(0, 255, 0, 255);
	SpriteColor Blue = SpriteColor_Make(0, 0, 255, 255);
	
	SpriteColor HitEffect = SpriteColor_Make(255, 191, 127, 255);
	SpriteColor HitEffect_Boss = SpriteColor_Make(255, 127, 0, 255);
};

//

Sprite::Sprite()
{
	m_VertexCount = 0;
	m_Vertices = 0;
	m_IndexCount = 0;
	m_Indices = 0;
}

Sprite::~Sprite()
{
	Allocate(0, 0);
}

void Sprite::Allocate(int vertexCount, int indexCount)
{
	if (vertexCount >= 65536)
		throw ExceptionVA("vertex count >= 65536");

	delete[] m_Vertices;
	m_Vertices = 0;
	m_VertexCount = 0;
	delete[] m_Indices;
	m_Indices = 0;
	m_IndexCount = 0;
	
	if (vertexCount > 0)
	{
		m_Vertices = new SpriteVertex[vertexCount];
		m_VertexCount = vertexCount;
	}
	
	if (indexCount > 0)
	{
		m_Indices = new SpriteIndex[indexCount];
		m_IndexCount = indexCount;
	}
}

//

SpriteGfx::SpriteGfx()
{
	Initialize();
}

SpriteGfx::SpriteGfx(int vertexCount, int indexCount, SpriteGfxRenderTime renderTime)
{		
	Initialize();
	
	Setup(vertexCount, indexCount, renderTime);
}

SpriteGfx::~SpriteGfx()
{
	Setup(0, 0, SpriteGfxRenderTime_Undefined);
}

void SpriteGfx::Initialize()
{
#ifdef SPGFX_INTERLEAVED
	m_Vertices = 0;
#else	
	m_Coords = 0;
	m_Colors = 0;
	m_TexCoords = 0;
#endif
	m_Indices = 0;

	m_RenderTime = SpriteGfxRenderTime_Undefined;
	m_BaseVertexIndex = 0;
	m_WriteVertexIndex = 0;
	m_WriteIndexIndex = 0;

	m_DrawVertexBaseIndex = 0;
	m_DrawVertexCount = 0;
	m_DrawIndexBaseIndex = 0;
	m_DrawIndexCount = 0;
}

void SpriteGfx::Setup(int vertexCount, int indexCount, SpriteGfxRenderTime renderTime)
{
#ifdef SPGFX_INTERLEAVED
	delete[] m_Vertices;
	m_Vertices = 0;
#else
	delete[] m_Coords;
	m_Coords = 0;
	delete[] m_Colors;
	m_Colors = 0;
	delete[] m_TexCoords;
	m_TexCoords = 0;
#endif
	delete[] m_Indices;
	m_Indices = 0;

	m_RenderTime = renderTime;
	m_VertexCount = vertexCount;
	m_IndexCount = indexCount;
	
	if (m_VertexCount > 0)
	{
#ifdef SPGFX_INTERLEAVED
		m_Vertices = new SpriteVertex[m_VertexCount];
#else
		m_Coords = new SpriteCoord[m_VertexCount];
		m_Colors = new SpriteColor[m_VertexCount];
		m_TexCoords = new SpriteTexCoord[m_VertexCount];
#endif
	}
	
	if (m_IndexCount > 0)
	{
		m_Indices = new SpriteIndex[m_IndexCount];
	}
	
	Reset(true);
}

void SpriteGfx::Reserve(int vertexCount, int indexCount)
{
	if (!HasCapacity(vertexCount, indexCount))
	{
		Flush();
	}
}

void SpriteGfx::Flush()
{
	if (m_DrawIndexCount == 0)
		return;
	
	if (m_RenderTime == SpriteGfxRenderTime_OnFlush)
	{
		// render
	
		Render();

		// reset render state

		Reset(true);
	}

	if (m_RenderTime == SpriteGfxRenderTime_OnFrameEnd)
	{
		// render
	
		Render();

		// reset render state

		Reset(false);
	}

//	g_PerfCount.Increment_Count(PC_RENDER_FLUSH, 1);
}

void SpriteGfx::Reset(bool resetDrawBase)
{
	if (resetDrawBase == false)
	{
		m_DrawVertexBaseIndex = m_WriteVertexIndex;
		m_DrawIndexBaseIndex = m_WriteIndexIndex;
	}
	else
	{
		m_WriteVertexIndex = 0;
		m_WriteIndexIndex = 0;
	}

	m_BaseVertexIndex = 0;

	m_DrawVertexCount = 0;
	m_DrawIndexCount = 0;

	if (resetDrawBase)
	{
		m_DrawVertexBaseIndex = 0;
		m_DrawIndexBaseIndex = 0;
	}
}

void SpriteGfx::FrameEnd()
{
	if (m_RenderTime == SpriteGfxRenderTime_OnFrameEnd)
	{
		// render
	
		//Render();

		Reset(true);
	}
}

void SpriteGfx::WriteSprite(const Sprite& sprite)
{
	Reserve(sprite.m_VertexCount, sprite.m_IndexCount);
	WriteBegin();
	
#ifndef SPGFX_INTERLEAVED
	// iterate over vertices
	
	for (int i = 0; i < sprite.m_VertexCount; ++i)
	{
		const SpriteVertex& vertex = sprite.m_Vertices[i];
		
		// write vertex
		
		WriteVertex(vertex);
	}
#else
	memcpy(m_Vertices + m_WriteVertexIndex, sprite.m_Vertices, sizeof(SpriteVertex) * sprite.m_VertexCount);
	
	m_WriteVertexIndex += sprite.m_VertexCount;
#endif
	
	// iterate over indices
	
	WriteIndexN(sprite.m_Indices, sprite.m_IndexCount);
	/*
	for (int i = 0; i < sprite.m_IndexCount; ++i)
	{
		const SpriteIndex index = sprite.m_Indices[i];
		
		// write index
		
		WriteIndex(index);
	}*/
	
	WriteEnd();
}

void SpriteGfx::WriteSprite(const Sprite& sprite, float x, float y, float angle, float scaleX, float scaleY)
{
	float axis[4];

	Calc::RotAxis_Fast(angle, axis);
	
	axis[0] *= scaleX;
	axis[1] *= scaleY;
	axis[2] *= scaleX;
	axis[3] *= scaleY;

	Reserve(sprite.m_VertexCount, sprite.m_IndexCount);
	WriteBegin();

	// iterate over vertices
	
	for (int i = 0; i < sprite.m_VertexCount; ++i)
	{
		const SpriteVertex& vertex = sprite.m_Vertices[i];
		
		// transform vertex
		
		const float tx = x + vertex.m_Coord.x * axis[0] + vertex.m_Coord.y * axis[1];
		const float ty = y + vertex.m_Coord.x * axis[2] + vertex.m_Coord.y * axis[3];
		
		// write vertex
		
		WriteVertex(tx, ty, vertex.m_Color.rgba, vertex.m_TexCoord.u, vertex.m_TexCoord.v);
	}
	
	// iterate over indices
	
	WriteIndexN(sprite.m_Indices, sprite.m_IndexCount);
	/*
	for (int i = 0; i < sprite.m_IndexCount; ++i)
	{
		const SpriteIndex index = sprite.m_Indices[i];
		
		// write index
		
		WriteIndex(index);
	}*/
	
	WriteEnd();
}

void SpriteGfx::WriteSprite(const Vec3* positionArray, const SpriteColor* colorArray, const SpriteTexCoord* texCoordArray, int vertexCount, const SpriteIndex* indexArray, int indexCount, const Mat4x4& mat)
{
	Reserve(vertexCount, indexCount);
	WriteBegin();

	// iterate over vertices
	
	for (int i = 0; i < vertexCount; ++i)
	{
		// transform vertex
		
		Vec3 position = mat.Mul4(positionArray[i]);
		
		// write vertex
		
		WriteVertex(position[0], position[1], colorArray[i].rgba, texCoordArray[i].u, texCoordArray[i].v);
	}
	
	// write indices
	
	WriteIndexN(indexArray, indexCount);
	
	WriteEnd();
}

void SpriteGfx::DBG_Render()
{
	Render();
}

void SpriteGfx::Render()
{
#ifdef DEBUG
	// validate index buffer

	for (int i = 0; i < m_WriteIndexIndex; ++i)
	{
		if (m_Indices[i] >= m_WriteVertexIndex)
			throw ExceptionVA("index out of bounds");
	}
#endif

#ifdef SPGFX_INTERLEAVED
	SpriteRenderer::DrawTriangles_VCT_Index(
		&m_Vertices[m_DrawVertexBaseIndex].m_Coord.x,
		sizeof(SpriteVertex),
		&m_Vertices[m_DrawVertexBaseIndex].m_Color.rgba,
		sizeof(SpriteVertex),
		&m_Vertices[m_DrawVertexBaseIndex].m_TexCoord.u,
		sizeof(SpriteVertex),
		&m_Indices[m_DrawIndexBaseIndex],
		m_DrawIndexCount);

	//LOG_DBG("sprite render: %d indices", m_WriteIndexIndex);
#else
	SpriteRenderer::DrawTriangles_VCT_Index(
		&m_Vertices[m_DrawVertexBaseIndex].m_Coord.x,
		sizeof(float) * 2,
		&m_Vertices[m_DrawVertexBaseIndex].m_Color.rgba,
		sizeof(SpriteColor),
		&m_Vertices[m_DrawVertexBaseIndex].m_TexCoord.u,
		sizeof(float) * 2,
		&m_Indices[m_DrawIndexBaseIndex],
		m_WriteIndexIndex);
#endif
}
