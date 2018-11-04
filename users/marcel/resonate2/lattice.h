#pragma once

#include "constants.h"
#include <vector>

struct Lattice
{
	struct Vector
	{
		float x;
		float y;
		float z;
		
		void set(const float in_x, const float in_y, const float in_z)
		{
			x = in_x;
			y = in_y;
			z = in_z;
		}
		
		void setZero()
		{
			x = y = z = 0.f;
		}
	};
	
	struct Vertex
	{
		Vector p;
		
		// physics stuff
		Vector f;
		Vector v;
	};
	
	struct Edge
	{
		int vertex1;
		int vertex2;
		float weight;
		
		// physics stuff
		float initialDistance;
	};
	
	Vertex vertices[6 * kTextureSize * kTextureSize];
	
	std::vector<Edge> edges;
	
	void init();
	
	void finalize();
};
