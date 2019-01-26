/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

static bool s_isInCubeBatch = false;

void fillCube(Vec3Arg position, Vec3Arg size)
{
	const float vertices[8][3] =
	{
		{ -1, -1, -1 },
		{ +1, -1, -1 },
		{ +1, +1, -1 },
		{ -1, +1, -1 },
		{ -1, -1, +1 },
		{ +1, -1, +1 },
		{ +1, +1, +1 },
		{ -1, +1, +1 }
	};

	const int faces[6][4] =
	{
		{ 3, 2, 1, 0 },
		{ 4, 5, 6, 7 },
		{ 0, 4, 7, 3 },
		{ 2, 6, 5, 1 },
		{ 0, 1, 5, 4 },
		{ 7, 6, 2, 3 }
	};

	const float normals[6][3] =
	{
		{  0,  0, -1 },
		{  0,  0, +1 },
		{ -1,  0,  0 },
		{ +1,  0,  0 },
		{  0, -1,  0 },
		{  0, +1,  0 }
	};
	
	if (s_isInCubeBatch == false)
		gxBegin(GX_QUADS);
	
	{
		for (int face_idx = 0; face_idx < 6; ++face_idx)
		{
			const float * __restrict normal = normals[face_idx];
			
			gxNormal3fv(normal);
			
			for (int vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
			{
				const float * __restrict vertex = vertices[faces[face_idx][vertex_idx]];
				
				gxVertex3f(
					position[0] + size[0] * vertex[0],
					position[1] + size[1] * vertex[1],
					position[2] + size[2] * vertex[2]);
			}
		}
	}
	
	if (s_isInCubeBatch == false)
		gxEnd();
}

void beginCubeBatch()
{
	Assert(s_isInCubeBatch == false);
	
	gxBegin(GX_QUADS);
	s_isInCubeBatch = true;
}

void endCubeBatch()
{
	Assert(s_isInCubeBatch == true);
	
	gxEnd();
	s_isInCubeBatch = false;
}

void fillCylinder(Vec3Arg position, const float radius, const float height, const int resolution, const float angleOffset)
{
	float dx[resolution];
	float dz[resolution];
	
	float x[resolution];
	float z[resolution];
	
	for (int i = 0; i < resolution; ++i)
	{
		const float angle = angleOffset + (i + .5f) / float(resolution) * 2.f * float(M_PI);
		
		dx[i] = cosf(angle);
		dz[i] = sinf(angle);
		
		x[i] = position[0] + dx[i] * radius;
		z[i] = position[2] + dz[i] * radius;
	}
	
	const float y1 = position[1] - height;
	const float y2 = position[1] + height;
	
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < resolution; ++i)
		{
			const int vertex1 = i;
			const int vertex2 = i + 1 < resolution ? i + 1 : 0;
			
			// emit quad
			
			const float dxMid = dx[vertex1] + dx[vertex2];
			const float dzMid = dz[vertex1] + dz[vertex2];
			const float dLength = hypotf(dxMid, dzMid);
			
			const float nx = dxMid / dLength;
			const float nz = dzMid / dLength;
			
			gxNormal3f(nx, 0.f, nz);
			
			gxVertex3f(x[vertex1], y1, z[vertex1]);
			gxVertex3f(x[vertex2], y1, z[vertex2]);
			gxVertex3f(x[vertex2], y2, z[vertex2]);
			gxVertex3f(x[vertex1], y2, z[vertex1]);
		}
	}
	gxEnd();
	
	gxBegin(GX_TRIANGLES);
	{
		gxNormal3f(0, -1, 0);
		
		for (int i = 0; i < resolution; ++i)
		{
			const int vertex1 = 0;
			const int vertex2 = (i + 0) % resolution;
			const int vertex3 = (i + 1) % resolution;
			
			gxVertex3f(x[vertex1], y1, z[vertex1]);
			gxVertex3f(x[vertex2], y1, z[vertex2]);
			gxVertex3f(x[vertex3], y1, z[vertex3]);
		}
		
		gxNormal3f(0, +1, 0);
		
		for (int i = 0; i < resolution; ++i)
		{
			const int vertex1 = 0;
			const int vertex2 = (i + 0) % resolution;
			const int vertex3 = (i + 1) % resolution;
			
			gxVertex3f(x[vertex1], y2, z[vertex1]);
			gxVertex3f(x[vertex2], y2, z[vertex2]);
			gxVertex3f(x[vertex3], y2, z[vertex3]);
		}
	}
	gxEnd();
}

void fillHexagon(Vec3Arg position, const float radius, const float height, const float angleOffset)
{
	fillCylinder(position, radius, height, 5, angleOffset);
}
