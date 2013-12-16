#pragma once

#include <allegro.h>
#include <vector>
#include "Shape_Triangle.h"
#include "triangulate.h"
#include "types.h"

class Shape_Polygon
{
public:
	void Make_Rect(const Vec2F& p1, const Vec2F& p2)
	{
		Outline.push_back(Vec2F(p1[0], p1[1]));
		Outline.push_back(Vec2F(p2[0], p1[1]));
		Outline.push_back(Vec2F(p2[0], p2[1]));
		Outline.push_back(Vec2F(p1[0], p2[1]));
	}

	void Finalize()
	{
		for (size_t i = 0; i < Outline.size(); ++i)
		{
			OutlineT.push_back(Outline[i]);
		}

		// Convert outline into triangles.

		Vector2dVector input;

		for (size_t i = 0; i < Outline.size(); ++i)
			input.push_back(Outline[i]);

		Vector2dVector result;

		Triangulate::Process(input, result);

		int triangleCount = result.size() / 3;

		for (int i = 0; i < triangleCount; ++i)
		{
			Shape_Triangle triangle;

			triangle.Points[0] = result[i * 3 + 0];
			triangle.Points[1] = result[i * 3 + 1];
			triangle.Points[2] = result[i * 3 + 2];

			Triangles.push_back(triangle);
		}
	}

	std::vector<Vec2F> Outline;
	std::vector<Vec2F> OutlineT;
	std::vector<Shape_Triangle> Triangles;
};
