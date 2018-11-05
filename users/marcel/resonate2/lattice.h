#pragma once

#include "constants.h"
#include <math.h>
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
		
		float calcMagnitude() const
		{
			const float result =
				sqrtf(
					x * x +
					y * y +
					z * z);
			
			return result;
		}
	};
	
	struct Vertex
	{
		Vector p;
		Vector p_init;
		
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
