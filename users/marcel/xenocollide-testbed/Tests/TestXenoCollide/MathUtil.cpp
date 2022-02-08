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

#include <stdlib.h>

#include "Math/Math.h"

#include "Constraint.h"
#include "MathUtil.h"
#include "RenderPolytope.h"

float32 RandFloat(float32 min, float32 max)
{
	int32 randInt = rand();
	float32 r = (float32) randInt / (float32) RAND_MAX;
	return r * (max - min) + min;
}


float32 Det(const Vector& v0, const Vector& v1, const Vector& v2)
{
	float32 det =
		v0.X() * ( v1.Y() * v2.Z() - v1.Z() * v2.Y() ) +
		v0.Y() * ( v1.Z() * v2.X() - v1.X() * v2.Z() ) +
		v0.Z() * ( v1.X() * v2.Y() - v1.Y() * v2.X() );
	return det;
}

void ComputeMassProperties(Body* body, RenderPolytope* model, float32 density)
{
	Vector diag(0, 0, 0);
	Vector offDiag(0, 0, 0);
	Vector weightedCenterOfMass(0, 0, 0);
	float32 volume = 0;
	float32 mass = 0;

	// Iterate through the faces
	for (int32 faceIndex = 0; faceIndex < model->mFaceCount; faceIndex++)
	{
		RenderPolytope::RenderFace& face = model->mFaces[faceIndex];

		// Iterate through the tris in the face
		for (int32 triIndex = 0; triIndex < face.mVertCount-2; triIndex++)
		{
			Vector v0 = model->mVerts[ face.mVertList[0] ];
			Vector v1 = model->mVerts[ face.mVertList[triIndex+1] ];
			Vector v2 = model->mVerts[ face.mVertList[triIndex+2] ];

			float32 det = Det(v0, v1, v2);

			// Volume
			float32 tetVolume = det / 6.0f;
			volume += tetVolume;

			// Mass
			float32 tetMass = tetVolume * density;
			mass += tetMass;

			// Center of Mass
			Vector tetCenterOfMass = (v0 + v1 + v2) / 4.0f; // Note: includes origin (0, 0, 0) as fourth vertex
			weightedCenterOfMass += tetMass * tetCenterOfMass;

			// Inertia Tensor
			for (int32 i = 0; i < 3; i++)
			{
				int32 j = (i + 1) % 3;
				int32 k = (i + 2) % 3;

				diag(i) += det * ( v0(i)*v1(i) + v1(i)*v2(i) + v2(i)*v0(i) + v0(i)*v0(i) + v1(i)*v1(i) + v2(i)*v2(i) ) / 60.0f;

				offDiag(i) += det * (
					v0(j)*v1(k) + v1(j)*v2(k) + v2(j)*v0(k) +
					v0(j)*v2(k) + v1(j)*v0(k) + v2(j)*v1(k) +
					2*v0(j)*v0(k) + 2*v1(j)*v1(k) + 2*v2(j)*v2(k) ) / 120.0f;
			}
		}
	}
		
	Vector centerOfMass = weightedCenterOfMass / mass;

	diag *= density;
	offDiag *= density;

	Matrix I;
	I.BuildIdentity();
	I(0,0) = diag(1) + diag(2);
	I(1,1) = diag(2) + diag(0);
	I(2,2) = diag(0) + diag(1);
	I(1,2) = I(2,1) = -offDiag(0);
	I(0,2) = I(2,0) = -offDiag(1);
	I(0,1) = I(1,0) = -offDiag(2);

	///
	// Move inertia tensor to be relative to center of mass (rather than origin)

	// Translate intertia to center of mass
	float32 x = centerOfMass.X();
	float32 y = centerOfMass.Y();
	float32 z = centerOfMass.Z();

	I(0,0) -= mass*(y*y + z*z);
	I(0,1) -= mass*(-x*y);
	I(0,2) -= mass*(-x*z);
	I(1,1) -= mass*(x*x + z*z);
	I(1,2) -= mass*(-y*z);
	I(2,2) -= mass*(x*x + y*y);

	// Symmetry
	I(1,0) = I(0,1);
	I(2,0) = I(0,2);
	I(2,1) = I(1,2);

	body->com = centerOfMass;
	body->inv_m = 1.0f / mass;
	body->I = I;
	GeneralInverse4x4(&body->inv_I, I);
}
