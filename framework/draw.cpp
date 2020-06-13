/*
	Copyright (C) 2020 Marcel Smit
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

static void emitNormal(const int axis1, const int axis2)
{
	const int axis3 = 3 - axis1 - axis2;
	
	float normal[3];
	normal[axis1] = 0.f;
	normal[axis2] = 0.f;
	normal[axis3] = 1.f;
	
	gxNormal3fv(normal);
}

void drawPoint(float x, float y)
{
	gxBegin(GX_POINTS);
	{
		gxVertex2f(x, y);
	}
	gxEnd();
}

void drawLine(float x1, float y1, float x2, float y2)
{
	gxBegin(GX_LINES);
	{
		gxVertex2f(x1, y1);
		gxVertex2f(x2, y2);
	}
	gxEnd();
}

void drawRect(float x1, float y1, float x2, float y2)
{
	gxBegin(GX_QUADS);
	{
		emitNormal(0, 1);
		
		gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
		gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
		gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
		gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	}
	gxEnd();
}

void drawRectLine(float x1, float y1, float x2, float y2)
{
	gxBegin(GX_LINE_LOOP);
	{
		emitNormal(0, 1);
		
		gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
		gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
		gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
		gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	}
	gxEnd();
}

void drawCircle(float x, float y, float radius, int numSegments)
{
	gxBegin(GX_LINE_LOOP);
	{
		emitNormal(0, 1);
		
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle = i * (M_PI * 2.f / numSegments);

			gxVertex2f(
				x + cosf(angle) * radius,
				y + sinf(angle) * radius);
		}
	}
	gxEnd();
}

void fillCircle(float x, float y, float radius, int numSegments)
{
	gxBegin(GX_TRIANGLE_STRIP);
	{
		emitNormal(0, 1);
		
		for (int i = 0; i <= numSegments; ++i)
		{
			const int index = i < numSegments ? i : 0;
			
			const float angle = index * (M_PI * 2.f / numSegments);

			gxVertex2f(
				x,
				y);

			gxVertex2f(
				x + cosf(angle) * radius,
				y + sinf(angle) * radius);
		}
	}
	gxEnd();
}

void drawLine3d(int axis)
{
	gxBegin(GX_LINES);
	{
		gxTexCoord2f(0, 0); gxVertex3f(axis == 0 ? -1 : 0, axis == 1 ? -1 : 0, axis == 2 ? -1 : 0);
		gxTexCoord2f(1, 1); gxVertex3f(axis == 0 ? +1 : 0, axis == 1 ? +1 : 0, axis == 2 ? +1 : 0);
	}
	gxEnd();
}

void drawRect3d(int axis1, int axis2)
{
	const int axis3 = 3 - axis1 - axis2;
	
	gxBegin(GX_QUADS);
	{
		emitNormal(axis1, axis2);
		
		float xyz[3];
		
		xyz[axis1] = -1;
		xyz[axis2] = -1;
		xyz[axis3] = 0;
		gxTexCoord2f(0, 0); gxVertex3fv(xyz);
		
		xyz[axis1] = +1;
		gxTexCoord2f(1, 0); gxVertex3fv(xyz);
		
		xyz[axis2] = +1;
		gxTexCoord2f(1, 1); gxVertex3fv(xyz);
		
		xyz[axis1] = -1;
		gxTexCoord2f(0, 1); gxVertex3fv(xyz);
	}
	gxEnd();
}

void drawGrid3d(int resolution1, int resolution2, int axis1, int axis2)
{
	const int axis3 = 3 - axis1 - axis2;
	
	gxBegin(GX_QUADS);
	{
		emitNormal(axis1, axis2);
		
		for (int i = 0; i < resolution1; ++i)
		{
			for (int j = 0; j < resolution2; ++j)
			{
				const float u1 = (i + 0) / float(resolution1);
				const float u2 = (i + 1) / float(resolution1);
				const float v1 = (j + 0) / float(resolution2);
				const float v2 = (j + 1) / float(resolution2);
				
				const float x1 = (u1 - .5f) * 2.f;
				const float x2 = (u2 - .5f) * 2.f;
				const float y1 = (v1 - .5f) * 2.f;
				const float y2 = (v2 - .5f) * 2.f;
				
				float p[3];
				p[axis1] = x1;
				p[axis2] = y1;
				p[axis3] = 0.f;
				gxTexCoord2f(u1, v1); gxVertex3fv(p);
				
				p[axis1] = x2;
				gxTexCoord2f(u2, v1); gxVertex3fv(p);
				
				p[axis2] = y2;
				gxTexCoord2f(u2, v2); gxVertex3fv(p);
				
				p[axis1] = x1;
				gxTexCoord2f(u1, v2); gxVertex3fv(p);
			}
		}
	}
	gxEnd();
}

void drawGrid3dLine(int resolution1, int resolution2, int axis1, int axis2, bool optimized)
{
	const int axis3 = 3 - axis1 - axis2;
	
	if (optimized)
	{
		gxBegin(GX_LINES);
		{
			emitNormal(axis1, axis2);
			
			for (int i = 0; i <= resolution1; ++i)
			{
				const float u = i / float(resolution1);
				const float x = (u - .5f) * 2.f;
				
				float p[3];
				p[axis1] = x;
				p[axis2] = -1.f;
				p[axis3] = 0.f;
				gxTexCoord2f(u, 0.f); gxVertex3fv(p);
				
				p[axis2] = +1.f;
				gxTexCoord2f(u, 1.f); gxVertex3fv(p);
			}
			
			for (int j = 0; j <= resolution2; ++j)
			{
				const float v = j / float(resolution2);
				const float y = (v - .5f) * 2.f;
				
				float p[3];
				p[axis1] = -1.f;
				p[axis2] = y;
				p[axis3] = 0.f;
				gxTexCoord2f(0.f, v); gxVertex3fv(p);
				
				p[axis1] = +1.f;
				gxTexCoord2f(1.f, v); gxVertex3fv(p);
			}
		}
		gxEnd();
	}
	else
	{
		gxBegin(GX_LINES);
		{
			emitNormal(axis1, axis2);
			
			for (int i = 0; i < resolution1; ++i)
			{
				for (int j = 0; j < resolution2; ++j)
				{
					const float u1 = (i + 0) / float(resolution1);
					const float u2 = (i + 1) / float(resolution1);
					const float v1 = (j + 0) / float(resolution2);
					const float v2 = (j + 1) / float(resolution2);
					
					const float x1 = (u1 - .5f) * 2.f;
					const float x2 = (u2 - .5f) * 2.f;
					const float y1 = (v1 - .5f) * 2.f;
					const float y2 = (v2 - .5f) * 2.f;
					
					float p[3];
					p[axis1] = x1;
					p[axis2] = y1;
					p[axis3] = 0.f;
					gxTexCoord2f(u1, v1); gxVertex3fv(p);
					
					p[axis1] = x2;
					gxTexCoord2f(u2, v1); gxVertex3fv(p);
					gxTexCoord2f(u2, v1); gxVertex3fv(p);
					
					p[axis2] = y2;
					gxTexCoord2f(u2, v2); gxVertex3fv(p);
					gxTexCoord2f(u2, v2); gxVertex3fv(p);
					
					p[axis1] = x1;
					gxTexCoord2f(u1, v2); gxVertex3fv(p);
					gxTexCoord2f(u1, v2); gxVertex3fv(p);
					
					p[axis2] = y1;
					gxTexCoord2f(u1, v1); gxVertex3fv(p);
				}
			}
		}
		gxEnd();
	}
}

static bool s_isInCubeBatch = false;

void lineCube(Vec3Arg position, Vec3Arg size)
{
	const float vertices[8][3] =
	{
		{ position[0]-size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]+size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]+size[2] }
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
	
	gxBegin(GX_LINES);
	{
		for (int edge_idx = 0; edge_idx < 12; ++edge_idx)
		{
			const int * __restrict line = edges[edge_idx];
			
			for (int vertex_idx = 0; vertex_idx < 2; ++vertex_idx)
			{
				const float * __restrict vertex = vertices[line[vertex_idx]];
				
				gxVertex3fv(vertex);
			}
		}
	}
	gxEnd();
}

void fillCube(Vec3Arg position, Vec3Arg size)
{
	const float vertices[8][3] =
	{
		{ position[0]-size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]+size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]+size[2] }
	};

	const int faces[6][4] =
	{
		{ 0, 1, 2, 3 },
		{ 7, 6, 5, 4 },
		{ 3, 7, 4, 0 },
		{ 1, 5, 6, 2 },
		{ 4, 5, 1, 0 },
		{ 3, 2, 6, 7 }
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
			
			const int * __restrict face = faces[face_idx];
			
			for (int vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
			{
				const float * __restrict vertex = vertices[face[vertex_idx]];
				
				gxVertex3fv(vertex);
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

void fillCylinder(Vec3Arg position, const float radius, const float height, const int resolution, const float angleOffset, const bool smoothNormals)
{
	float * dx = (float*)alloca(resolution * sizeof(float));
	float * dz = (float*)alloca(resolution * sizeof(float));
	
	float * x = (float*)alloca(resolution * sizeof(float));
	float * z = (float*)alloca(resolution * sizeof(float));
	
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
	
#if true
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < resolution; ++i)
		{
			const int vertex1 = i;
			const int vertex2 = i + 1 < resolution ? i + 1 : 0;
			
			// emit quad
			
			if (smoothNormals)
			{
				gxNormal3f(dx[vertex1], 0.f, dz[vertex1]);
				gxVertex3f(x[vertex1], y2, z[vertex1]);
				gxVertex3f(x[vertex1], y1, z[vertex1]);
				
				gxNormal3f(dx[vertex2], 0.f, dz[vertex2]);
				gxVertex3f(x[vertex2], y1, z[vertex2]);
				gxVertex3f(x[vertex2], y2, z[vertex2]);
			}
			else
			{
				// calculate face normal
				
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
	}
	gxEnd();
	
	gxBegin(GX_TRIANGLES);
	{
		// emit bottom part
		
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
		
		// emit top part
		
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
#else
// todo : use a single triangle strip for everything

	gxBegin(GX_TRIANGLE_STRIP);
	{
		for (int i = 0; i <= resolution; ++i)
		{
			const int vertex = i < resolution ? i : 0;
			
			// emit vertical segment
			
			gxNormal3f(dx[vertex], 0.f, dz[vertex]);
			
			gxVertex3f(x[vertex], y1, z[vertex]);
			gxVertex3f(x[vertex], y2, z[vertex]);
		}
	}
	gxEnd();
	
	gxBegin(GX_TRIANGLE_STRIP);
	{
		// emit bottom part
		
		gxNormal3f(0, -1, 0);
		
		int index1 = 0;
		int index2 = resolution - 1;
		
		while (index1 < index2)
		{
			gxVertex3f(x[index1], y1, z[index1]);
			gxVertex3f(x[index2], y1, z[index2]);
			
			index1++;
			index2--;
		}
		
		// emit top part
		
		gxNormal3f(0, +1, 0);
		
		index1--;
		index2++;
		
		// note : the bottom and top will be connected through a degenerate
		//        triangle, so we don't need to start a new triangle strip here
		
		gxVertex3f(x[index2], y1, z[index2]);
		
		while (index1 >= 0)
		{
			gxVertex3f(x[index2], y2, z[index2]);
			gxVertex3f(x[index1], y2, z[index1]);
			
			index1--;
			index2++;
		}
	}
	gxEnd();
#endif
}

void fillHexagon(Vec3Arg position, const float radius, const float height, const float angleOffset, const bool smoothNormals)
{
	fillCylinder(position, radius, height, 5, angleOffset, smoothNormals);
}
