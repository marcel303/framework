#pragma once

#include "framework.h"

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
		float elevation = -25.f;
		float azimuth = -25.f;
		float distance = -10.f;
		
		void tick(const float dt, bool & inputIsCaptured);

		void calculateWorldMatrix(Mat4x4 & out_matrix) const;
		void calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const;
	};
	
	struct Ortho
	{
		OrthoSide side = kOrthoSide_Front;
		float scale = 1.f;
		Vec3 position;
		
		void tick(const float dt, bool & inputIsCaptured);
		
		void calculateWorldMatrix(Mat4x4 & out_matrix) const;
		void calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const;
	};
	
	struct FirstPerson
	{
		void tick(const float dt, bool & inputIsCaptured)
		{
			if (inputIsCaptured == false)
			{
				// todo
			}
		}
	};
	
	Mode mode = kMode_Orbit;
	
	Orbit orbit;
	Ortho ortho;
	FirstPerson firstPerson;
	
	void tick(const float dt, bool & inputIsCaptured);
	
	void calculateWorldMatrix(Mat4x4 & out_matrix) const;
	void calculateViewMatrix(Mat4x4 & out_matrix) const;

	void calculateProjectionMatrix(const int viewportSx, const int viewportSy, Mat4x4 & out_matrix) const;
	
	Mat4x4 calculateViewProjectionMatrix(const int viewportSx, const int viewportSy) const;

	void pushProjectionMatrix();
	void popProjectionMatrix();
	
	void pushViewMatrix() const;
	void popViewMatrix() const;
};
