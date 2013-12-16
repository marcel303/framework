#include "Precompiled.h"
#include <map>
#include "ShapeCompiler.h"
#include "Triangulate.h"

//#include <allegro.h> // todo: remove

namespace VRCS
{
	class MeshVertex
	{
	public:
		bool operator<(const MeshVertex& v) const
		{
			for (int i = 0; i < 2; ++i)
			{
				if (m_Position[i] < v.m_Position[i])
					return true;
				if (m_Position[i] > v.m_Position[i])
					return false;
			}
			for (int i = 0; i < 3; ++i)
			{
				if (m_Color[i] < v.m_Color[i])
					return true;
				if (m_Color[i] > v.m_Color[i])
					return false;
			}

			return false;
		}

		Vec2F m_Position;
		float m_Color[3];
	};

	class MeshBuilder
	{
	public:
		MeshBuilder()
		{
			m_Color = 1.0f;
		}

		void WriteVertex(const Vec2F& pos)
		{
			//printf("vertex: %f, %f\n", pos[0], pos[1]);

			MeshVertex vertex;

			vertex.m_Position = pos;
			vertex.m_Color[0] = m_Color;
			vertex.m_Color[1] = m_Color;
			vertex.m_Color[2] = m_Color;

			m_Vertices.push_back(vertex);
		}
		
		void WriteColor(float color)
		{
			m_Color = color;
		}
		
		void Optimize()
		{
#if 0
			allegro_init();
			set_color_depth(32);
			set_gfx_mode(GFX_AUTODETECT_WINDOWED, 640, 480, 0, 0);
			install_keyboard();
			for (size_t i = 0; i < m_Vertices.size() / 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					int index1 = i * 3 + (j + 0) % 3;
					int index2 = i * 3 + (j + 1) % 3;

					MeshVertex& v1 = m_Vertices[index1];
					MeshVertex& v2 = m_Vertices[index2];

					line(screen, 
						v1.m_Position[0],
						v1.m_Position[1],
						v2.m_Position[0],
						v2.m_Position[1],
						makecol(255, 255, 255));
				}
			}

			readkey();
#endif

			std::vector<MeshVertex> vertices;
			std::map<MeshVertex, size_t> vertexMap;
			std::vector<size_t> indices;

			for (size_t i = 0; i < m_Vertices.size(); ++i)
			{
				const MeshVertex& v = m_Vertices[i];

				std::map<MeshVertex, size_t>::iterator itr = vertexMap.find(v);

				if (itr == vertexMap.end())
				{
					size_t index = vertices.size();

					vertexMap[v] = index;
					vertices.push_back(v);
					indices.push_back(index);
				}
				else
				{
					indices.push_back(itr->second);
				}
			}

			m_Vertices = vertices;
			m_Indices = indices;
		}

		std::vector<MeshVertex> m_Vertices;
		std::vector<size_t> m_Indices;

	private:
		float m_Color;
	};
	
	class SegInfo
	{
	public:
		Vec2F m_Pos[2];
		Vec2F m_Norm;
	};
	
	class VertInfo
	{
	public:
		SegInfo m_Seg[2];
		Vec2F m_Pos;
		Vec2F m_Att[2];
	};
	
	void ShapeCompiler::Compile(Shape shape, std::string texFileName, std::string silFileName, CompiledShape& out_Shape)
	{		
		out_Shape.m_TextureFileName = texFileName;
		out_Shape.m_SilhouetteFileName = silFileName;
		
		out_Shape.m_HasSilhouette = shape.m_HasShadow;
		out_Shape.m_HasSprite = shape.m_HasSprite;

		// center shape around pivot

		PointI pivot(0, 0);

		Tag* tag = shape.FindTag("Pivot");

		if (tag)
		{
//			printf("using pivot: %d, %d\n", tag->x, tag->y);

			pivot = PointI(tag->x, tag->y);

			shape.Offset(-tag->x, -tag->y);
		}

		shape.Finalize(1);
		
		//
		
		out_Shape.m_BoundingBox = shape.m_BBWithStroke;
		
		// sort items by type
		
		std::vector<ShapeItem*> circles;
		std::vector<ShapeItem*> polygons;
		std::vector<ShapeItem*> rectangles;
		std::vector<ShapeItem*> tags;
		
		for (size_t i = 0; i < shape.m_Items.size(); ++i)
		{
			ShapeItem& item = shape.m_Items[i];
			
			if (item.m_Type == ShapeType_Circle)
				circles.push_back(&item);
			if (item.m_Type == ShapeType_Poly)
				polygons.push_back(&item);
			if (item.m_Type == ShapeType_Rectangle)
				rectangles.push_back(&item);
			if (item.m_Type == ShapeType_Tag)
				tags.push_back(&item);
		}
		
		std::vector<Collision_Circle> collision_circles;
		std::vector<Collision_Triangle> collision_triangles;
		std::vector<Collision_Hull> collision_hulls;
		std::vector<Collision_Rectangle> collision_rectangles;
		std::vector<Logic_Tag> logic_tags;
		
		// collision circles
		
		for (size_t i = 0; i < circles.size(); ++i)
		{
			ShapeItem* item = circles[i];
			
			if (!item->m_Collision)
				continue;
			
			Collision_Circle circle;
			
			circle.m_Position[0] = (float)item->m_Circle.x;
			circle.m_Position[1] = (float)item->m_Circle.y;
			circle.m_Radius = (float)item->m_Circle.r;
			
			collision_circles.push_back(circle);
		}
		
		// collision triangles
		
		for (size_t i = 0; i < polygons.size(); ++i)
		{
			ShapeItem* item = polygons[i];
			
			item->m_Poly.m_Hull.Construct(item->m_Poly.m_Poly.m_Points);
			
			if (!item->m_Collision)
				continue;
			//printf("convex: %d\n", (int)item->m_Poly.m_Hull.IsConvex());
			if (item->m_Poly.m_Hull.IsConvex())
				continue;
			
			throw ExceptionVA("collision poly not convex");

			//printf("warning: triangulating collision shape\n");

			// triangulate

			std::vector<PointF> points;
			std::vector<PointF> triangles;

			for (size_t i = 0; i < item->m_Poly.m_Poly.m_Points.size(); ++i)
			{
				PointF point(
					(float)item->m_Poly.m_Poly.m_Points[i].x,
					(float)item->m_Poly.m_Poly.m_Points[i].y);

				points.push_back(point);
			}

			Triangulate::Process(points, triangles);

			for (size_t i = 0; i < triangles.size() / 3; ++i)
			{
				Collision_Triangle triangle;
				
				triangle.m_Coords[0] = triangles[i * 3 + 0][0];
				triangle.m_Coords[1] = triangles[i * 3 + 0][1];
				triangle.m_Coords[2] = triangles[i * 3 + 1][0];
				triangle.m_Coords[3] = triangles[i * 3 + 1][1];
				triangle.m_Coords[4] = triangles[i * 3 + 2][0];
				triangle.m_Coords[5] = triangles[i * 3 + 2][1];
				
				collision_triangles.push_back(triangle);
			}
		}

		// collision hulls

		for (size_t i = 0; i < polygons.size(); ++i)
		{
			ShapeItem* item = polygons[i];
			
			if (!item->m_Collision)
				continue;
			if (!item->m_Poly.m_Hull.IsConvex())
				continue;
			
			if (item->m_Poly.m_Poly.m_Points.size() > VRCS_MAX_HULL_POINTS)
			{
				printf("hull_line_count: %d\n", (int)item->m_Poly.m_Hull.m_Lines.size());
				for (size_t j = 0; j < item->m_Poly.m_Hull.m_Lines.size(); ++j)
					printf("hull_line_plane: %f, %f @ %f\n",
						   item->m_Poly.m_Hull.m_Lines[j].m_Plane.m_Normal[0],
						   item->m_Poly.m_Hull.m_Lines[j].m_Plane.m_Normal[1],
						   item->m_Poly.m_Hull.m_Lines[j].m_Plane.m_Distance);
				
				throw ExceptionVA("poly exceeds max hull point count");
			}
			
			Collision_Hull hull;
			
			hull.m_LineCount = item->m_Poly.m_Poly.m_Points.size();
			
			for (int j = 0; j < hull.m_LineCount; ++j)
			{
				hull.m_Coords[j * 2 + 0] = static_cast<float>(item->m_Poly.m_Poly.m_Points[j].x);
				hull.m_Coords[j * 2 + 1] = static_cast<float>(item->m_Poly.m_Poly.m_Points[j].y);
			}
			
			collision_hulls.push_back(hull);
		}
		
		// collision rectangles
		
		for (size_t i = 0; i < rectangles.size(); ++i)
		{
			ShapeItem* item = rectangles[i];
			
			if (!item->m_Collision)
				continue;
			
			Collision_Rectangle rectangle;
			
			rectangle.m_Position[0] = (float)item->m_Rectangle.m_Location.x;
			rectangle.m_Position[1] = (float)item->m_Rectangle.m_Location.y;
			rectangle.m_Size[0] = (float)item->m_Rectangle.m_Size.x;
			rectangle.m_Size[1] = (float)item->m_Rectangle.m_Size.y;
			
			collision_rectangles.push_back(rectangle);
		}
		
		// logic tags
		
		for (size_t i = 0; i < tags.size(); ++i)
		{
			ShapeItem* item = tags[i];
			
			Logic_Tag tag;
			
			tag.m_Name = item->m_Name;
			tag.m_Position[0] = (float)item->m_Tag.x;
			tag.m_Position[1] = (float)item->m_Tag.y;
			
			logic_tags.push_back(tag);
		}
		
		//
		
		out_Shape.Allocate(collision_circles.size(), collision_triangles.size(), collision_hulls.size(), collision_rectangles.size(), tags.size());
		
		//
		
		for (size_t i = 0; i < collision_circles.size(); ++i)
			out_Shape.m_CollisionCircles[i] = collision_circles[i];
		for (size_t i = 0; i < collision_triangles.size(); ++i)
			out_Shape.m_CollisionTriangles[i] = collision_triangles[i];
		for (size_t i = 0; i < collision_hulls.size(); ++i)
			out_Shape.m_CollisionHulls[i] = collision_hulls[i];
		for (size_t i = 0; i < collision_rectangles.size(); ++i)
			out_Shape.m_CollisionRectangles[i] = collision_rectangles[i];
		for (size_t i = 0; i < logic_tags.size(); ++i)
			out_Shape.m_LogicTags[i] = logic_tags[i];
		
		// sprite
		
		if (shape.m_HasSprite)
		{
			MeshBuilder builder;
		
#if 1
			// fill
			
			for (size_t i = 0; i < polygons.size(); ++i)
			{
				ShapeItem* item = polygons[i];
				
				if (!item->m_Visible)
					continue;			
				
				// triangulate
	
				std::vector<PointF> points;
				std::vector<PointF> triangles;
	
				for (size_t i = 0; i < item->m_Poly.m_Poly.m_Points.size(); ++i)
				{
					PointF point(
						(float)item->m_Poly.m_Poly.m_Points[i].x,
						(float)item->m_Poly.m_Poly.m_Points[i].y);
	
					points.push_back(point);
				}
	
				Triangulate::Process(points, triangles);
				
				builder.WriteColor(item->m_FillOpacity);
				
				for (size_t i = 0; i < triangles.size() / 3; ++i)
				{
					builder.WriteVertex(triangles[i * 3 + 0]);
					builder.WriteVertex(triangles[i * 3 + 1]);
					builder.WriteVertex(triangles[i * 3 + 2]);
				}
			}
#endif

#if 0
			// line
			
			for (size_t i = 0; i < polygons.size(); ++i)
			{
				ShapeItem* item = polygons[i];
				
				if (!item->m_Visible)
					continue;
				
				// generate segment infos
				
				std::vector<SegInfo> segs;
				
				for (int i = 0; i < (int)item->m_Poly.m_Poly.m_Points.size(); ++i)
				{
					int index1 = (i + 0) % (int)item->m_Poly.m_Poly.m_Points.size();
					int index2 = (i + 1) % (int)item->m_Poly.m_Poly.m_Points.size();
					
					Vec2F p1((float)item->m_Poly.m_Poly.m_Points[index1].x, (float)item->m_Poly.m_Poly.m_Points[index1].y);
					Vec2F p2((float)item->m_Poly.m_Poly.m_Points[index2].x, (float)item->m_Poly.m_Poly.m_Points[index2].y);
					
					SegInfo seg;
					seg.m_Pos[0] = p1;
					seg.m_Pos[1] = p2;
					Vec2F delta = p2 - p1;
					delta.Normalize();
					seg.m_Norm[0] = -delta[1];
					seg.m_Norm[1] = +delta[0];
					
					segs.push_back(seg);
				}
				
				// generate vertex infos
				
				std::vector<VertInfo> verts;
				
				for (size_t i = 0; i < segs.size(); ++i)
				{
					size_t index1 = (i + 0) % segs.size();
					size_t index2 = (i + 1) % segs.size();
					
					VertInfo vert;
					vert.m_Seg[0] = segs[index1];
					vert.m_Seg[1] = segs[index2];
					
					Vec2F norm = segs[index1].m_Norm + segs[index2].m_Norm;
					norm.Normalize();
					vert.m_Pos = segs[index1].m_Pos[1];
					vert.m_Att[0] = vert.m_Pos - norm * 8.0f;
					vert.m_Att[1] = vert.m_Pos;

					verts.push_back(vert);
				}
				
				// generate mesh
				
				builder.WriteColor(1.0f);
					
				for (size_t i = 0; i < verts.size(); ++i)
				{
					size_t index1 = (i + 0) % verts.size();
					size_t index2 = (i + 1) % verts.size();
					
					builder.WriteVertex(verts[index1].m_Att[0]);
					builder.WriteVertex(verts[index2].m_Att[0]);
					builder.WriteVertex(verts[index2].m_Att[1]);
					
					builder.WriteVertex(verts[index1].m_Att[0]);
					builder.WriteVertex(verts[index2].m_Att[1]);
					builder.WriteVertex(verts[index1].m_Att[1]);
				}
			}
#endif
			
#if VRCS_USE_SPRITE
			builder.Optimize();
			
			int vertexCount = (int)builder.m_Vertices.size();
			int indexCount = (int)builder.m_Indices.size();

			out_Shape.m_GraphicsSprite.Allocate(vertexCount, indexCount);

			for (int i = 0; i < vertexCount; ++i)
			{
				out_Shape.m_GraphicsSprite.m_Vertices[i].m_Position[0] = builder.m_Vertices[i].m_Position[0];
				out_Shape.m_GraphicsSprite.m_Vertices[i].m_Position[1] = builder.m_Vertices[i].m_Position[1];
			}

			for (int i = 0; i < indexCount; ++i)
			{
				out_Shape.m_GraphicsSprite.m_Indices[i] = (int)builder.m_Indices[i];
			}
#endif
		}
	}
};
