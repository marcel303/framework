#include "Precompiled.h"
#include "CompiledShape.h"
#include "Path.h"
#include "StreamReader.h"
#include "StreamWriter.h"

/*

--------------------
VGC File Format
--------------------

[int32]    collision-circle-count
[int32]    collision-triangle-count
[int32]    tag-count
 
[float] pivot-x
[float] pivot-y

// todo: store pivot point or triangle/quad points
//       storing poly/texture coords allows reducing vertex
//       count by analyzing shape.

foreach collision-circle
	//[char[16]] name
	[float] position-x
	[float] position-y
	[float] radius
 
foreach collision-triangle
	//[char[16]] name
	foreach vertex[6]
		[float] position-x
		[float] position-y
		[float] texcoord-x
		[float] texcoord-y

foreach collision-rectangle
	[float] x1
	[float] y1
	[float] x2
	[float] y2
 
foreach tag
	[char[16]] name
	[float] position-x
	[float] position-y

texture = .vgc=.png
 
*/

namespace VRCS
{
	CompiledShape::CompiledShape()
	{
		m_Pivot[0] = 0.0f;
		m_Pivot[1] = 0.0f;
		
		m_HasSilhouette = false;
		m_HasSprite = false;
		
		m_CollisionCircles = 0;
		m_CollisionCircleCount = 0;
		
		m_CollisionTriangles = 0;
		m_CollisionTriangleCount = 0;
		
		m_CollisionHulls = 0;
		m_CollisionHullCount = 0;

		m_CollisionRectangles = 0;
		m_CollisionRectangleCount = 0;
		
		m_LogicTags = 0;
		m_LogicTagCount = 0;
	}
	
	CompiledShape::~CompiledShape()
	{
		Allocate(0, 0, 0, 0, 0);
	}
	
	void CompiledShape::Save(Stream* stream) const
	{
		StreamWriter writer(stream, false);
		
		// write header
		
		const int32_t collisionCircleCount = m_CollisionCircleCount;
		const int32_t collisionTriangleCount = m_CollisionTriangleCount;
		const int32_t collisionHullCount = m_CollisionHullCount;
		const int32_t collisionRectangleCount = m_CollisionRectangleCount;
		const int32_t logicTagCount = m_LogicTagCount;
		
		writer.WriteInt32(collisionCircleCount);
		writer.WriteInt32(collisionTriangleCount);
		writer.WriteInt32(collisionHullCount);
		writer.WriteInt32(collisionRectangleCount);
		writer.WriteInt32(logicTagCount);
		
		writer.WriteText_Binary(Path::GetFileName(m_TextureFileName));
		writer.WriteText_Binary(Path::GetFileName(m_SilhouetteFileName));
		
		writer.WriteFloat(m_Pivot[0]);
		writer.WriteFloat(m_Pivot[1]);
		
		writer.WriteInt32(m_HasSilhouette);
		writer.WriteInt32(m_HasSprite);
		
		// write bounding box
		
		writer.WriteInt32(m_BoundingBox.m_Min.x);
		writer.WriteInt32(m_BoundingBox.m_Min.y);
		writer.WriteInt32(m_BoundingBox.m_Max.x);
		writer.WriteInt32(m_BoundingBox.m_Max.y);
		
		// write collision circles
		
		for (int i = 0; i < m_CollisionCircleCount; ++i)
		{
			const Collision_Circle& circle = m_CollisionCircles[i];
			
			writer.WriteFloat(circle.m_Position[0]);
			writer.WriteFloat(circle.m_Position[1]);
			writer.WriteFloat(circle.m_Radius);
		}
		
		// write collision triangles
		
		for (int i = 0; i < m_CollisionTriangleCount; ++i)
		{
			const Collision_Triangle& triangle = m_CollisionTriangles[i];
			
			for (int j = 0; j < 6; ++j)
				writer.WriteFloat(triangle.m_Coords[j]);
		}
		
		// write collision hulls

		for (int i = 0; i < m_CollisionHullCount; ++i)
		{
			const Collision_Hull& hull = m_CollisionHulls[i];

			const int32_t lineCount = hull.m_LineCount;

			writer.WriteInt32(lineCount);

			for (int j = 0; j < lineCount * 2; ++j)
			{
				//printf("coord: %f\n", hull.m_Coords[j]);
				writer.WriteFloat(hull.m_Coords[j]);
			}
		}

		// write collision rectangles
		
		for (int i = 0; i < m_CollisionRectangleCount; ++i)
		{
			const Collision_Rectangle& rectangle = m_CollisionRectangles[i];
			
			writer.WriteFloat(rectangle.m_Position[0]);
			writer.WriteFloat(rectangle.m_Position[1]);
			writer.WriteFloat(rectangle.m_Size[0]);
			writer.WriteFloat(rectangle.m_Size[1]);
		}
		
		// write logic tags
		
		for (int i = 0; i < m_LogicTagCount; ++i)
		{
			const Logic_Tag& tag = m_LogicTags[i];
			
			writer.WriteText_Binary(tag.m_Name);
			
			writer.WriteFloat(tag.m_Position[0]);
			writer.WriteFloat(tag.m_Position[1]);
		}
		
#if VRCS_USE_GRAPHICSMESH
		// construct graphics mesh based on bounding box
		
		Graphics_Mesh mesh;
		
		mesh.Allocate(4, 6);
		mesh.m_Vertices[0].m_Position[0] = (float)m_BoundingBox.m_Min.x;
		mesh.m_Vertices[0].m_Position[1] = (float)m_BoundingBox.m_Min.y;
		mesh.m_Vertices[0].m_TexCoord[0] = 0.0f;
		mesh.m_Vertices[0].m_TexCoord[1] = 0.0f;
		mesh.m_Vertices[1].m_Position[0] = (float)m_BoundingBox.m_Max.x;
		mesh.m_Vertices[1].m_Position[1] = (float)m_BoundingBox.m_Min.y;
		mesh.m_Vertices[1].m_TexCoord[0] = 1.0f;
		mesh.m_Vertices[1].m_TexCoord[1] = 0.0f;
		mesh.m_Vertices[2].m_Position[0] = (float)m_BoundingBox.m_Max.x;
		mesh.m_Vertices[2].m_Position[1] = (float)m_BoundingBox.m_Max.y;
		mesh.m_Vertices[2].m_TexCoord[0] = 1.0f;
		mesh.m_Vertices[2].m_TexCoord[1] = 1.0f;
		mesh.m_Vertices[3].m_Position[0] = (float)m_BoundingBox.m_Min.x;
		mesh.m_Vertices[3].m_Position[1] = (float)m_BoundingBox.m_Max.y;
		mesh.m_Vertices[3].m_TexCoord[0] = 0.0f;
		mesh.m_Vertices[3].m_TexCoord[1] = 1.0f;
		
		mesh.m_Indices[0] = 0;
		mesh.m_Indices[1] = 1;
		mesh.m_Indices[2] = 2;
		mesh.m_Indices[3] = 0;
		mesh.m_Indices[4] = 2;
		mesh.m_Indices[5] = 3;
		
		// write graphics mesh
		
		writer.WriteInt32(mesh.m_VertexCount);
		writer.WriteInt32(mesh.m_IndexCount);
		
		// write vertices
		
		for (int i = 0; i < mesh.m_VertexCount; ++i)
		{
			writer.WriteFloat(mesh.m_Vertices[i].m_Position[0]);
			writer.WriteFloat(mesh.m_Vertices[i].m_Position[1]);
			writer.WriteFloat(mesh.m_Vertices[i].m_TexCoord[0]);
			writer.WriteFloat(mesh.m_Vertices[i].m_TexCoord[1]);
		}
		
		// write indices
		
		for (int i = 0; i < mesh.m_IndexCount; ++i)
			writer.WriteInt32(mesh.m_Indices[i]);
#endif
		
#if VRCS_USE_SPRITE
		// write graphics sprite
		
		writer.WriteInt32(m_GraphicsSprite.m_VertexCount);
		writer.WriteInt32(m_GraphicsSprite.m_IndexCount);
		
		// write vertices
		
		for (int i = 0; i < m_GraphicsSprite.m_VertexCount; ++i)
		{
			writer.WriteFloat(m_GraphicsSprite.m_Vertices[i].m_Position[0]);
			writer.WriteFloat(m_GraphicsSprite.m_Vertices[i].m_Position[1]);
			//writer.WriteFloat(m_GraphicsSprite.m_Vertices[i].m_Color);
		}
		
		// write indices
		
		for (int i = 0; i < m_GraphicsSprite.m_IndexCount; ++i)
			writer.WriteInt32(m_GraphicsSprite.m_Indices[i]);
#endif
	}
	
	void CompiledShape::Load(Stream* stream)
	{
		StreamReader reader(stream, false);
		
		// read header
		
		const int32_t collisionCircleCount = reader.ReadInt32();
		const int32_t collisionTriangleCount = reader.ReadInt32();
		const int32_t collisionHullCount = reader.ReadInt32();
		const int32_t collisionRectangleCount = reader.ReadInt32();
		const int32_t logicTagCount = reader.ReadInt32();
		
		m_TextureFileName = reader.ReadText_Binary();
		m_SilhouetteFileName = reader.ReadText_Binary();
		
		m_Pivot[0] = reader.ReadFloat();
		m_Pivot[1] = reader.ReadFloat();
		
		m_HasSilhouette = reader.ReadInt32();
		m_HasSprite = reader.ReadInt32();
		
		// read bounding box
		
		m_BoundingBox.m_Min.x = reader.ReadInt32();
		m_BoundingBox.m_Min.y = reader.ReadInt32();
		m_BoundingBox.m_Max.x = reader.ReadInt32();
		m_BoundingBox.m_Max.y = reader.ReadInt32();
		
		// allocate space
		
		Allocate(collisionCircleCount, collisionTriangleCount, collisionHullCount, collisionRectangleCount, logicTagCount);
		
		// read collision circles
		
		for (int i = 0; i < collisionCircleCount; ++i)
		{
			Collision_Circle circle;
			
			circle.m_Position[0] = reader.ReadFloat();
			circle.m_Position[1] = reader.ReadFloat();
			circle.m_Radius = reader.ReadFloat();
			
			m_CollisionCircles[i] = circle;
		}
		
		// read collision triangles
		
		for (int i = 0; i < collisionTriangleCount; ++i)
		{
			Collision_Triangle triangle;
			
			for (int j = 0; j < 6; ++j)
				triangle.m_Coords[j] = reader.ReadFloat();
			
			m_CollisionTriangles[i] = triangle;
		}
		
		// read collision hulls

		for (int i = 0; i < collisionHullCount; ++i)
		{
			Collision_Hull hull;

			hull.m_LineCount = reader.ReadInt32();

			for (int j = 0; j < hull.m_LineCount * 2; ++j)
				hull.m_Coords[j] = reader.ReadFloat();
			
			m_CollisionHulls[i] = hull;
		}

		// read collision rectangles
		
		for (int i = 0; i < collisionRectangleCount; ++i)
		{
			Collision_Rectangle rectangle;
			
			rectangle.m_Position[0] = reader.ReadFloat();
			rectangle.m_Position[1] = reader.ReadFloat();
			rectangle.m_Size[0] = reader.ReadFloat();
			rectangle.m_Size[1] = reader.ReadFloat();
			
			m_CollisionRectangles[i] = rectangle;
		}
		
		// read logic tags
		
		for (int i = 0; i < logicTagCount; ++i)
		{
			Logic_Tag tag;
			
			tag.m_Name = reader.ReadText_Binary();
			
			tag.m_Position[0] = reader.ReadFloat();
			tag.m_Position[1] = reader.ReadFloat();
			
			m_LogicTags[i] = tag;
		}
		
		// read graphics mesh
		
#if VRCS_USE_GRAPHICSMESH
		{
			const int vertexCount = reader.ReadInt32();
			const int indexCount = reader.ReadInt32();
			
			m_GraphicsMesh.Allocate(vertexCount, indexCount);
			
			// read vertices
			
			for (int i = 0; i < vertexCount; ++i)
			{
				m_GraphicsMesh.m_Vertices[i].m_Position[0] = reader.ReadFloat();
				m_GraphicsMesh.m_Vertices[i].m_Position[1] = reader.ReadFloat();
				m_GraphicsMesh.m_Vertices[i].m_TexCoord[0] = reader.ReadFloat();
				m_GraphicsMesh.m_Vertices[i].m_TexCoord[1] = reader.ReadFloat();
			}
			
			// read indices
			
			for (int i = 0; i < indexCount; ++i)
				m_GraphicsMesh.m_Indices[i] = reader.ReadInt32();
		}
#endif
		
#if VRCS_USE_SPRITE
		// read graphics sprite
		
		{
			const int vertexCount = reader.ReadInt32();
			const int indexCount = reader.ReadInt32();
			
			m_GraphicsSprite.Allocate(vertexCount, indexCount);
			
			// read vertices
			
			for (int i = 0; i < vertexCount; ++i)
			{
				m_GraphicsSprite.m_Vertices[i].m_Position[0] = reader.ReadFloat();
				m_GraphicsSprite.m_Vertices[i].m_Position[1] = reader.ReadFloat();
			}
			
			// read indices
			
			for (int i = 0; i < indexCount; ++i)
				m_GraphicsSprite.m_Indices[i] = reader.ReadInt32();
		}
#endif
	}
	
	void CompiledShape::Allocate(int collisionCircleCount, int collisionTriangleCount, int collisionHullCount, int collisionRectangleCount, int logicTagCount)
	{
		delete[] m_CollisionCircles;
		m_CollisionCircles = 0;
		m_CollisionCircleCount = 0;
		
		delete[] m_CollisionTriangles;
		m_CollisionTriangles = 0;
		m_CollisionTriangleCount = 0;
		
		delete[] m_CollisionHulls;
		m_CollisionHulls = 0;
		m_CollisionHullCount = 0;

		delete[] m_CollisionRectangles;
		m_CollisionRectangles = 0;
		m_CollisionRectangleCount = 0;
		
		delete[] m_LogicTags;
		m_LogicTags = 0;
		m_LogicTagCount = 0;
		
		if (collisionCircleCount > 0)
		{
			m_CollisionCircles = new Collision_Circle[collisionCircleCount];
			m_CollisionCircleCount = collisionCircleCount;
		}
		
		if (collisionTriangleCount > 0)
		{
			m_CollisionTriangles = new Collision_Triangle[collisionTriangleCount];
			m_CollisionTriangleCount = collisionTriangleCount;
		}

		if (collisionHullCount > 0)
		{
			m_CollisionHulls = new Collision_Hull[collisionHullCount];
			m_CollisionHullCount = collisionHullCount;
		}
		
		if (collisionRectangleCount > 0)
		{
			m_CollisionRectangles = new Collision_Rectangle[collisionRectangleCount];
			m_CollisionRectangleCount = collisionRectangleCount;
		}
		
		if (logicTagCount > 0)
		{
			m_LogicTags = new Logic_Tag[logicTagCount];
			m_LogicTagCount = logicTagCount;
		}
	}
};
