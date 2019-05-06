/*
	Copyright (C) 2019 Marcel Smit
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
#include "framework-camera.h"

void Camera::Orbit::tick(const float dt, bool & inputIsCaptured)
{
	if (inputIsCaptured == false)
	{
		// rotation
		
		const float rotationSpeed = 45.f;
		
		if (keyboard.isDown(SDLK_UP))
		{
			elevation -= rotationSpeed * dt;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_DOWN))
		{
			elevation += rotationSpeed * dt;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_LEFT))
		{
			azimuth -= rotationSpeed * dt;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_RIGHT))
		{
			azimuth += rotationSpeed * dt;
			inputIsCaptured = true;
		}
		
		// distance
		
		if (keyboard.isDown(SDLK_EQUALS))
		{
			distance /= powf(1.5f, dt);
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_MINUS))
		{
			distance *= powf(1.5f, dt);
			inputIsCaptured = true;
		}
		
		// transform reset
		
		if (keyboard.wentDown(SDLK_o) && keyboard.isDown(SDLK_LGUI))
		{
			*this = Orbit();
			inputIsCaptured = true;
		}
	}
}

void Camera::Orbit::calculateWorldMatrix(Mat4x4 & out_matrix) const
{
	out_matrix = Mat4x4(true)
		.RotateY(azimuth * float(M_PI) / 180.f)
		.RotateX(elevation * float(M_PI) / 180.f)
		.Translate(0, 0, distance);
}

void Camera::Orbit::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	out_matrix.MakePerspectiveLH(60.f * float(M_PI) / 180.f, viewportSy / float(viewportSx), .01f, 100.f);
}

//

void Camera::Ortho::tick(const float dt, bool & inputIsCaptured)
{
	if (inputIsCaptured == false)
	{
		// side
		
		if (keyboard.isDown(SDLK_f))
		{
			side = kOrthoSide_Front;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_b))
		{
			side = kOrthoSide_Back;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_s))
		{
			side = kOrthoSide_Side;
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_t))
		{
			side = kOrthoSide_Top;
			inputIsCaptured = true;
		}
		
		// scale
	
		if (keyboard.isDown(SDLK_EQUALS))
		{
			scale /= powf(1.5f, dt);
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_MINUS))
		{
			scale *= powf(1.5f, dt);
			inputIsCaptured = true;
		}
		
		// position movement
	
		Mat4x4 cameraToWorld;
		calculateWorldMatrix(cameraToWorld);
		
		Vec3 direction;
		
		if (keyboard.isDown(SDLK_LEFT))
		{
			direction -= cameraToWorld.GetAxis(0);
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_RIGHT))
		{
			direction += cameraToWorld.GetAxis(0);
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_DOWN))
		{
			direction -= cameraToWorld.GetAxis(1);
			inputIsCaptured = true;
		}
		if (keyboard.isDown(SDLK_UP))
		{
			direction += cameraToWorld.GetAxis(1);
			inputIsCaptured = true;
		}
		
		const float speed = 1.f;
		
		position += direction * speed * dt;
		
		// transform reset
		
		if (keyboard.wentDown(SDLK_o) && keyboard.isDown(SDLK_LGUI))
		{
			position.SetZero();
			inputIsCaptured = true;
		}
	}
}

void Camera::Ortho::calculateWorldMatrix(Mat4x4 & out_matrix) const
{
	float elevation = 0.f;
	float azimuth = 0.f;
	
	switch (side)
	{
	case kOrthoSide_Front:
		elevation = 0.f;
		azimuth = 0.f;
		break;
	case kOrthoSide_Back:
		elevation = 0.f;
		azimuth = 180.f;
		break;
	case kOrthoSide_Side:
		elevation = 0.f;
		azimuth = 90.f;
		break;
	case kOrthoSide_Top:
		elevation = -90.f;
		azimuth = 0.f;
		break;
	}
	
	out_matrix = Mat4x4(true)
		.Translate(position)
		.RotateY(azimuth * float(M_PI) / 180.f)
		.RotateX(elevation * float(M_PI) / 180.f)
		.Scale(scale, scale, scale);
}

void Camera::Ortho::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	const float sx = viewportSx / float(viewportSy);
	
	out_matrix.MakeOrthoLH(-sx, +sx, +1.f, -1.f, .01f, 100.f);
}

//

void Camera::tick(const float dt, bool & inputIsCaptured)
{
	if (inputIsCaptured == false)
	{
		if (keyboard.wentDown(SDLK_1))
		{
			mode = Camera::kMode_Orbit;
			inputIsCaptured = true;
		}
		if (keyboard.wentDown(SDLK_2))
		{
			mode = Camera::kMode_Ortho;
			inputIsCaptured = true;
		}
		if (keyboard.wentDown(SDLK_3))
		{
			mode = Camera::kMode_FirstPerson;
			inputIsCaptured = true;
		}
	}
	
	switch (mode)
	{
	case kMode_Orbit:
		orbit.tick(dt, inputIsCaptured);
		break;
	case kMode_Ortho:
		ortho.tick(dt, inputIsCaptured);
		break;
	case kMode_FirstPerson:
		firstPerson.tick(dt, inputIsCaptured);
		break;
	}
}

void Camera::calculateWorldMatrix(Mat4x4 & out_matrix) const
{
	switch (mode)
	{
	case kMode_Orbit:
		orbit.calculateWorldMatrix(out_matrix);
		break;
	case kMode_Ortho:
		ortho.calculateWorldMatrix(out_matrix);
		break;
	case kMode_FirstPerson:
		out_matrix.MakeIdentity();
		break;
	}
}

void Camera::calculateViewMatrix(Mat4x4 & out_matrix) const
{
	Mat4x4 worldMatrix;
	calculateWorldMatrix(worldMatrix);
	out_matrix = worldMatrix.CalcInv();
}

void Camera::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	switch (mode)
	{
	case kMode_Orbit:
		orbit.calculateProjectionMatrix(viewportSx, viewportSy, out_matrix);
		break;
	case kMode_Ortho:
		ortho.calculateProjectionMatrix(viewportSx, viewportSy, out_matrix);
		break;
	case kMode_FirstPerson:
		out_matrix.MakeIdentity();
		break;
	}
}

void Camera::calculateViewProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	Mat4x4 projectionMatrix;
	Mat4x4 worldMatrix;
	
	switch (mode)
	{
	case kMode_Orbit:
		orbit.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		orbit.calculateWorldMatrix(worldMatrix);
		break;
	case kMode_Ortho:
		ortho.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		ortho.calculateWorldMatrix(worldMatrix);
		break;
	case kMode_FirstPerson:
		projectionMatrix.MakeIdentity();
		worldMatrix.MakeIdentity();
		break;
	}
	
	out_matrix = projectionMatrix * worldMatrix.CalcInv();
}

void Camera::pushProjectionMatrix()
{
	int viewportSx = 0;
	int viewportSy = 0;
	framework.getCurrentViewportSize(viewportSx, viewportSy);
	
	Mat4x4 matrix;
	calculateProjectionMatrix(viewportSx, viewportSy, matrix);
	
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(matrix.m_v);
	}
	gxMatrixMode(restoreMatrixMode);
}

void Camera::popProjectionMatrix()
{
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_PROJECTION);
		gxPopMatrix();
	}
	gxMatrixMode(restoreMatrixMode);
}

void Camera::pushViewMatrix() const
{
	Mat4x4 matrix;
	calculateViewMatrix(matrix);
	
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		gxLoadMatrixf(matrix.m_v);
	}
	gxMatrixMode(restoreMatrixMode);
}

void Camera::popViewMatrix() const
{
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPopMatrix();
	}
	gxMatrixMode(restoreMatrixMode);
}
