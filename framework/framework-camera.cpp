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

// todo : make fov a parameter or pass it when calculating projection matrix

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
		.Translate(origin[0], origin[1], origin[2])
		.RotateY(azimuth * float(M_PI) / 180.f)
		.RotateX(elevation * float(M_PI) / 180.f)
		.Translate(0, 0, distance);
}

void Camera::Orbit::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	out_matrix.MakePerspectiveLH(60.f * float(M_PI) / 180.f, viewportSy / float(viewportSx), .01f, 1000.f);
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
		.RotateX(elevation * float(M_PI) / 180.f);
}

void Camera::Ortho::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	const float sx = viewportSx / float(viewportSy);
	
	out_matrix.MakeOrthoLH(-sx * scale, +sx * scale, +1.f * scale, -1.f * scale, -1000.f, 1000.f);
}

//

void Camera::FirstPerson::tick(const float dt, bool & inputIsCaptured)
{
	float forwardSpeed = 0.f;
	float strafeSpeed = 0.f;
	float upSpeed = 0.f;
	
	float desiredHeight = height;
	float desiredLeanAngle = 0.f;
	
	if (inputIsCaptured == false)
	{
		// keyboard
		
		if (keyboard.isDown(SDLK_DOWN) || keyboard.isDown(SDLK_s))
			forwardSpeed -= 1.f;
		if (keyboard.isDown(SDLK_UP) || keyboard.isDown(SDLK_w))
			forwardSpeed += 1.f;
		if (keyboard.isDown(SDLK_LEFT) || keyboard.isDown(SDLK_a))
			strafeSpeed -= 1.f;
		if (keyboard.isDown(SDLK_RIGHT) || keyboard.isDown(SDLK_d))
			strafeSpeed += 1.f;
		
		if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
			desiredHeight *= 1.f - duckAmount;
		
		if (keyboard.isDown(SDLK_LEFTBRACKET))
			desiredLeanAngle = -15.f;
		if (keyboard.isDown(SDLK_RIGHTBRACKET))
			desiredLeanAngle = +15.f;
		
		// gamepad
		
		if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
		{
			strafeSpeed += gamepad[gamepadIndex].getAnalog(0, ANALOG_X);
			forwardSpeed -= gamepad[gamepadIndex].getAnalog(0, ANALOG_Y);
		
			yaw -= gamepad[gamepadIndex].getAnalog(1, ANALOG_X);
			pitch -= gamepad[gamepadIndex].getAnalog(1, ANALOG_Y);
		}
		
		// mouse + mouse smoothing
		
		if (mouseRotationEnabled)
		{
			mouseDx += mouse.dx;
			mouseDy += mouse.dy;
			
			const double retain = pow(mouseSmooth, dt * 100.0);
			
			const double newDx = mouseDx * retain;
			const double newDy = mouseDy * retain;
			
			const double thisDx = mouseDx - newDx;
			const double thisDy = mouseDy - newDy;
			
			mouseDx = newDx;
			mouseDy = newDy;
			
			yaw -= thisDx * mouseRotationSpeed;
			pitch -= thisDy * mouseRotationSpeed;
		}
	}
	else
	{
		mouseDx = 0.f;
		mouseDy = 0.f;
	}
	
	// apply rotation limit
	
	if (limitRotation)
	{
		pitch = clamp<float>(pitch, -90.f, +90.f);
	}
	
	// go from normalized input values to values directly used to add to position
	
	forwardSpeed *= movementSpeed * forwardSpeedMultiplier * dt;
	strafeSpeed *= movementSpeed * strafeSpeedMultiplier * dt;
	upSpeed *= movementSpeed * upSpeedMultiplier * dt;
	
	Mat4x4 worldMatrix;
	calculateWorldMatrix(worldMatrix);
	
	const Vec3 xAxis = worldMatrix.GetAxis(0);
	const Vec3 yAxis = worldMatrix.GetAxis(1);
	const Vec3 zAxis = worldMatrix.GetAxis(2);
	
	const Vec3 positionIncrement = xAxis * strafeSpeed + yAxis * upSpeed + zAxis * forwardSpeed;
	
	position = position + positionIncrement;
	
	// animate height
	
	const float heightRetain = powf(.02f, dt);
	
	if (currentHeight == 0.f)
		currentHeight = desiredHeight;
	else
		currentHeight = currentHeight * heightRetain + desiredHeight * (1.f - heightRetain);
	
	// animate lean
	
	const float leanRetain = powf(.02f, dt);
	
	leanAngle = leanAngle * leanRetain + desiredLeanAngle * (1.f - leanRetain);
	
	// animate bobbing
	
	currentBobbingPhase = fmodf(currentBobbingPhase + positionIncrement.CalcSize() * bobbingSpeed, 1.f);
}

void Camera::FirstPerson::calculateWorldMatrix(Mat4x4 & out_matrix) const
{
	out_matrix = Mat4x4(true)
		.Translate(0, currentHeight, 0)
		.Translate(0, sinf(currentBobbingPhase * 2.f * float(M_PI)) * bobbingAmount, 0)
		.Translate(position)
		.RotateZ(roll / 180.f * float(M_PI))
		.RotateY(yaw / 180.f * float(M_PI))
		.RotateX(pitch / 180.f * float(M_PI))
		.RotateZ(leanAngle / 180.f * float(M_PI));
}

void Camera::FirstPerson::calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const
{
	out_matrix.MakePerspectiveLH(60.f * float(M_PI) / 180.f, viewportSy / float(viewportSx), .01f, 1000.f);
}

//

void Camera::tick(const float dt, bool & inputIsCaptured, const bool movementIsLocked)
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
	
	if (movementIsLocked == false)
	{
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
		firstPerson.calculateWorldMatrix(out_matrix);
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
		firstPerson.calculateProjectionMatrix(viewportSx, viewportSy, out_matrix);
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
		firstPerson.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		firstPerson.calculateWorldMatrix(worldMatrix);
		break;
	}
	
	out_matrix = projectionMatrix * worldMatrix.CalcInv();
}

void Camera::pushProjectionMatrix() const
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

void Camera::popProjectionMatrix() const
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
		gxMultMatrixf(matrix.m_v);
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
