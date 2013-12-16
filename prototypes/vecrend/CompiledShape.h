#pragma once

#include <string>
#include "Stream.h"
#include "Types.h"

#define VRCS_USE_GRAPHICSMESH 1
#define VRCS_USE_SPRITE 1
#define VRCS_MAX_HULL_POINTS 6

namespace VRCS
{
	class Collision_Circle
	{
	public:
		float m_Position[2];
		float m_Radius;
	};

	class Collision_Triangle
	{
	public:
		float m_Coords[6];
	};

	class Collision_Hull
	{
	public:
		float m_Coords[2 * VRCS_MAX_HULL_POINTS];
		int m_LineCount;
	};

	class Logic_Tag
	{
	public:
		std::string m_Name;
		float m_Position[2];
	};
	
	class Collision_Rectangle
	{
	public:
		float m_Position[2];
		float m_Size[2];
	};

	class Graphics_MeshVertex
	{
	public:
		float m_Position[2];
		float m_TexCoord[2];
	};

#if VRCS_USE_GRAPHICSMESH
	class Graphics_Mesh
	{
	public:
		Graphics_Mesh()
		{
			m_Vertices = 0;
			m_VertexCount = 0;
			m_Indices = 0;
			m_IndexCount = 0;
		}
		
		~Graphics_Mesh()
		{
			Allocate(0, 0);
		}
		
		void Allocate(int vertexCount, int indexCount)
		{
			delete[] m_Vertices;
			m_Vertices = 0;
			delete[] m_Indices;
			m_Indices = 0;
			
			if (vertexCount > 0)
				m_Vertices = new Graphics_MeshVertex[vertexCount];
			if (indexCount > 0)
				m_Indices = new uint16_t[indexCount];
			
			m_VertexCount = vertexCount;
			m_IndexCount = indexCount;
		}
		
		Graphics_MeshVertex* m_Vertices;
		int m_VertexCount;
		uint16_t* m_Indices;
		int m_IndexCount;
	};
#endif
	
#if VRCS_USE_SPRITE
	// the sprite is more complex than mesh
	// sprite is used to draw filled area + stroke (for large bosses where atlas would be prohibitive)
	class Graphics_SpriteVertex
	{
	public:
		float m_Position[2];
	};
	
	class Graphics_Sprite
	{
	public:
		Graphics_Sprite()
		{
			m_Vertices = 0;
			m_VertexCount = 0;
			m_Indices = 0;
			m_IndexCount = 0;
		}
		
		~Graphics_Sprite()
		{
			Allocate(0, 0);
		}
		
		void Allocate(int vertexCount, int indexCount)
		{
			delete[] m_Vertices;
			m_Vertices = 0;
			delete[] m_Indices;
			m_Indices = 0;
			
			if (vertexCount > 0)
				m_Vertices = new Graphics_SpriteVertex[vertexCount];
			if (indexCount > 0)
				m_Indices = new uint16_t[indexCount];
			
			m_VertexCount = vertexCount;
			m_IndexCount = indexCount;
		}
		
		Graphics_SpriteVertex* m_Vertices;
		int m_VertexCount;
		uint16_t* m_Indices;
		int m_IndexCount;
	};
#endif
	
	class CompiledShape
	{
	public:
		CompiledShape();
		~CompiledShape();
		
		void Save(Stream* stream) const;
		void Load(Stream* stream);
		
		void Allocate(int collisionCircleCount, int collisionTriangleCount, int collisionHullCount, int collisionRectangleCount, int logicTagCount);
		
		std::string m_TextureFileName;
		std::string m_SilhouetteFileName;
		
		float m_Pivot[2];
		BBoxI m_BoundingBox;
		int m_HasSilhouette;
		int m_HasSprite;
		
		Collision_Circle* m_CollisionCircles;
		int m_CollisionCircleCount;
		
		Collision_Triangle* m_CollisionTriangles;
		int m_CollisionTriangleCount;
		
		Collision_Hull* m_CollisionHulls;
		int m_CollisionHullCount;

		Collision_Rectangle* m_CollisionRectangles;
		int m_CollisionRectangleCount;
		
#if VRCS_USE_GRAPHICSMESH
		Graphics_Mesh m_GraphicsMesh;
#endif
		
#if VRCS_USE_SPRITE
		Graphics_Sprite m_GraphicsSprite;
#endif
		
		Logic_Tag* m_LogicTags;
		int m_LogicTagCount;
	};
}
