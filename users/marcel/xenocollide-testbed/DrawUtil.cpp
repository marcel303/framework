/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#include "StdAfx.h"

#include <gl/gl.h>
#include <gl/glu.h>

#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "opengl32.lib")
#pragma comment (lib, "glu32.lib")

void SetTransform(const Vector& pos, const Quat& q)
{
	Matrix transform(q);
	transform.SetTrans(pos);

	glMultMatrixf((GLfloat*) &transform);
}

void DrawSphere(const Vector& pos, const Quat& q, float radius, const Vector& c)
{
	glColor3f(c.X(), c.Y(), c.Z());
	glPushMatrix();
	SetTransform(pos, q);
	GLUquadricObj* quadric = gluNewQuadric();
	gluSphere(quadric, radius, 10, 10);
	gluDeleteQuadric(quadric);
	glPopMatrix();
}

