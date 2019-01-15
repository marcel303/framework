#pragma once

#include "constants.h"
#include <math.h>
#include <vector>

static const int kNumVertices = 6 * kGridSize * kGridSize;

struct Lattice
{
	struct Vector
	{
		float x;
		float y;
		float z;
		float padding;
		
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
		
		float dot(const float in_x, const float in_y, const float in_z) const
		{
			return
				x * in_x +
				y * in_y +
				z * in_z;
		}
	};
	
	struct EdgeVertices
	{
		uint32_t vertex1;
		uint32_t vertex2;
	};

	struct Edge
	{
		float weight;
		
		// physics stuff
		float initialDistance;
	};

	Vector vertices_p[kNumVertices];
	Vector vertices_p_init[kNumVertices];
	Vector vertices_n[kNumVertices];
	Vector vertices_f[kNumVertices];      // physics stuff
	Vector vertices_v[kNumVertices];      // physics stuff
	
	std::vector<EdgeVertices> edgeVertices;
	std::vector<Edge> edges;
	
	void init();
	
	void finalize();
};

static inline int calcVertexIndex(const int cubeFaceIndex, const int x, const int y)
{
	return
		cubeFaceIndex * kGridSize * kGridSize +
		y * kGridSize +
		x;
}
