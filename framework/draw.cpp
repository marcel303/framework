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
	
	gxBegin(GX_QUADS);
	{
		for (int face_idx = 0; face_idx < 6; ++face_idx)
		{
			const float * normal = normals[face_idx];
			
			gxNormal3fv(normal);
			
			for (int vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
			{
				const float * vertex = vertices[faces[face_idx][vertex_idx]];
				
				gxVertex3f(
					position[0] + size[0] * vertex[0],
					position[1] + size[1] * vertex[1],
					position[2] + size[2] * vertex[2]);
			}
		}
	}
	gxEnd();
}
