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

#pragma once

#include "Mat4x4.h"
#include "Vec3.h"

class Camera
{
public:
	enum Mode
	{
		kMode_Orbit,
		kMode_Ortho,
		kMode_FirstPerson
	};
	
	enum OrthoSide
	{
		kOrthoSide_Front,
		kOrthoSide_Back,
		kOrthoSide_Side,
		kOrthoSide_Top
	};
	
	struct Orbit
	{
		Vec3 origin;
		
		float elevation = -25.f;
		float azimuth = -25.f;
		float distance = -10.f;
		
		float fov = 60.f;
		float nearZ = .01f;
		float farZ = 1000.f;
		
		int gamepadIndex = -1;
		
		void tick(const float dt, bool & inputIsCaptured);

		void calculateWorldMatrix(Mat4x4 & out_matrix) const;
		void calculateProjectionMatrix(const float viewportSx, const float viewportSy, Mat4x4 & out_matrix) const;
	};
	
	struct Ortho
	{
		OrthoSide side = kOrthoSide_Front;
		float scale = 1.f;
		Vec3 position;
		
		float zRange = 1000.f;
		
		float speed = 1.f;
		
		int gamepadIndex = -1;
		
		void tick(const float dt, bool & inputIsCaptured);
		
		void calculateWorldMatrix(Mat4x4 & out_matrix) const;
		void calculateProjectionMatrix(const float viewportSx, const float gviewportSy, Mat4x4 & out_matrix) const;
	};
	
	struct FirstPerson
	{
	private:
		double mouseDx = 0.0;
		double mouseDy = 0.0;
		float currentHeight = 0.f;
		float currentBobbingPhase = 0.f;
		
	public:
		Vec3 position;
		float height = 1.8f;
		float duckAmount = .5f;
		float bobbingAmount = 0.f;
		float bobbingSpeed = 1.f;
		
		float yaw = 0.f;
		float pitch = 0.f;
		float roll = 0.f;
		
		double mouseSmooth = 0.75;
		float mouseRotationSpeed = 1.f;
		bool mouseRotationEnabled = true;
		bool limitRotation = false;
		float movementSpeed = 1.f;
		float forwardSpeedMultiplier = 1.f;
		float strafeSpeedMultiplier = 1.f;
		float upSpeedMultiplier = 1.f;
		float leanAngle = 0.f;
		
		float fov = 60.f;
		float nearZ = .01f;
		float farZ = 1000.f;
	
		int gamepadIndex = -1;
		
		void tick(const float dt, bool & inputIsCaptured);
		
		void calculateWorldMatrix(Mat4x4 & out_matrix) const;
		void calculateProjectionMatrix(const float viewportSx, const float viewportSy, Mat4x4 & out_matrix) const;
	};
	
public:
	Mode mode = kMode_Orbit;
	
	Orbit orbit;
	Ortho ortho;
	FirstPerson firstPerson;
	
	int gamepadIndex = -1;
	
public:
	void tick(const float dt, bool & inputIsCaptured, const bool movementIsLocked);
	
	void calculateWorldMatrix(Mat4x4 & out_matrix) const;
	void calculateViewMatrix(Mat4x4 & out_matrix) const;

	void calculateProjectionMatrix(const float viewportSx, const float viewportSy, Mat4x4 & out_matrix) const;
	
	void calculateViewProjectionMatrix(const float viewportSx, const float viewportSy, Mat4x4 & out_matrix) const;

	void pushProjectionMatrix() const;
	void popProjectionMatrix() const;
	
	void pushViewMatrix() const;
	void popViewMatrix() const;
};
