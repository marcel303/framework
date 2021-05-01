#pragma once

#include "componentDraw.h"

#include "framework.h"

#include <vector>

struct ComponentDraw_Framework : ComponentDraw
{
	struct Line
	{
		Vec3 from;
		Vec3 to;
		ComponentDrawColor color;
		
		Line(Vec3Arg in_from, Vec3Arg in_to, const ComponentDrawColor & in_color)
			: from(in_from)
			, to(in_to)
			, color(in_color)
		{
		}
	};
	
	std::vector<Line> lines;
	
	ComponentDrawColor currentColor;
	
	virtual ComponentDrawColor makeColor(const int r, const int g, const int b, const int a = 255) override
	{
		ComponentDrawColor result;
		result.rgba[0] = r / 255.f;
		result.rgba[1] = g / 255.f;
		result.rgba[2] = b / 255.f;
		result.rgba[3] = a / 255.f;
		return result;
	}
	
	virtual void color(const ComponentDrawColor & color) override
	{
		gxColor4fv(color.rgba);
		
		currentColor = color;
	}

	virtual void line(Vec3Arg from, Vec3Arg to) override
	{
		lines.emplace_back(from, to, currentColor);
	}
	
	virtual void line(
		float x1, float y1, float z1,
		float x2, float y2, float z2) override
	{
		lines.emplace_back(
			Vec3(x1, y1, z1),
			Vec3(x2, y2, z2),
			currentColor);
	}
	
	virtual void lineRect(float x1, float y1, float x2, float y2) override
	{
		lines.emplace_back(Vec3(x1, y1, 0), Vec3(x2, y1, 0), currentColor);
		lines.emplace_back(Vec3(x2, y1, 0), Vec3(x2, y2, 0), currentColor);
		lines.emplace_back(Vec3(x2, y2, 0), Vec3(x1, y2, 0), currentColor);
		lines.emplace_back(Vec3(x1, y2, 0), Vec3(x1, y1, 0), currentColor);
	}
	
	virtual void lineCircle(float x, float y, float radius) override
	{
		const int numSegments = 40;
		
		Vec3 * points = (Vec3*)alloca((numSegments + 1) * sizeof(Vec3));
		
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle = i * (M_PI * 2.f / numSegments);

			points[i].Set(
				x + cosf(angle) * radius,
				y + sinf(angle) * radius,
				0.f);
		}
		
		points[numSegments] = points[0];
		
		for (int i = 0; i < numSegments; ++i)
		{
			lines.emplace_back(points[i], points[i + 1], currentColor);
		}
	}
	
	virtual void lineCube(Vec3Arg position, Vec3Arg extents) override
	{
		const Vec3 vertices[8] =
		{
			{ position[0]-extents[0], position[1]-extents[1], position[2]-extents[2] },
			{ position[0]+extents[0], position[1]-extents[1], position[2]-extents[2] },
			{ position[0]+extents[0], position[1]+extents[1], position[2]-extents[2] },
			{ position[0]-extents[0], position[1]+extents[1], position[2]-extents[2] },
			{ position[0]-extents[0], position[1]-extents[1], position[2]+extents[2] },
			{ position[0]+extents[0], position[1]-extents[1], position[2]+extents[2] },
			{ position[0]+extents[0], position[1]+extents[1], position[2]+extents[2] },
			{ position[0]-extents[0], position[1]+extents[1], position[2]+extents[2] }
		};

		const int edges[12][2] =
		{
			// neg x -> pos x
			{ 0, 1 },
			{ 3, 2 },
			{ 4, 5 },
			{ 7, 6 },
			
			// neg y -> pos y
			{ 0, 3 },
			{ 1, 2 },
			{ 4, 7 },
			{ 5, 6 },
			
			// neg z -> pos z
			{ 0, 4 },
			{ 1, 5 },
			{ 2, 6 },
			{ 3, 7 },
		};
		
		for (int edge_idx = 0; edge_idx < 12; ++edge_idx)
		{
			const int * __restrict line = edges[edge_idx];
			
			const Vec3 & vertex1 = vertices[line[0]];
			const Vec3 & vertex2 = vertices[line[1]];
			
			lines.emplace_back(vertex1, vertex2, currentColor);
		}
	}
	
	virtual void pushMatrix() override
	{
		gxPushMatrix();
	}
	
	virtual void popMatrix() override
	{
		flush();
		
		gxPopMatrix();
	}
	
	virtual void multMatrix(const Mat4x4 & m) override
	{
		flush();
		
		gxMultMatrixf(m.m_v);
	}
	
	virtual void translate(Vec3Arg t) override
	{
		flush();
		
		gxTranslatef(t[0], t[1], t[2]);
	}
	
	virtual void rotate(float angle, Vec3Arg axis) override
	{
		flush();
		
		gxRotatef(angle, axis[0], axis[1], axis[2]);
	}
	
	void flush()
	{
		if (lines.size() > 0)
		{
			gxBegin(GX_LINES);
			{
				for (auto & line : lines)
				{
					gxColor4fv(line.color.rgba);
					gxVertex3fv(&line.from[0]);
					gxVertex3fv(&line.to[0]);
				}
			}
			gxEnd();
			
			lines.clear();
		}
	}
};
