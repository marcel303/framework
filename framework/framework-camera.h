#pragma once

struct Camera
{
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
		
		void tick(const float dt, bool & inputIsCaptured)
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
		
		void calculateTransform(Mat4x4 & out_transform, const int viewportSx, const int viewportSy) const
		{
			Mat4x4 perspective;
			perspective.MakePerspectiveLH(60.f * float(M_PI) / 180.f, viewportSy / float(viewportSx), .01f, 100.f);
			
			const Mat4x4 transform = Mat4x4(true)
				.RotateY(azimuth * float(M_PI) / 180.f)
				.RotateX(elevation * float(M_PI) / 180.f)
				.Translate(0, 0, distance);
			
			out_transform = perspective * transform.CalcInv();
		}
	};
	
	struct Ortho
	{
		OrthoSide side = kOrthoSide_Front;
		float scale = 1.f;
		Vec3 position;
		
		void tick(const float dt, bool & inputIsCaptured)
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
			
				const Mat4x4 worldTransform = calculateWorldTransform();
				
				Vec3 direction;
				
				if (keyboard.isDown(SDLK_LEFT))
				{
					direction -= worldTransform.GetAxis(0);
					inputIsCaptured = true;
				}
				if (keyboard.isDown(SDLK_RIGHT))
				{
					direction += worldTransform.GetAxis(0);
					inputIsCaptured = true;
				}
				if (keyboard.isDown(SDLK_DOWN))
				{
					direction -= worldTransform.GetAxis(1);
					inputIsCaptured = true;
				}
				if (keyboard.isDown(SDLK_UP))
				{
					direction += worldTransform.GetAxis(1);
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
		
		Mat4x4 calculateWorldTransform() const
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
			
			return Mat4x4(true)
				.Translate(position)
				.RotateY(azimuth * float(M_PI) / 180.f)
				.RotateX(elevation * float(M_PI) / 180.f)
				.Scale(scale, scale, scale);
		}
		
		void calculateTransform(Mat4x4 & out_transform, const int viewportSx, const int viewportSy) const
		{
			const float sx = viewportSx / float(viewportSy);
			
			Mat4x4 projection;
			projection.MakeOrthoLH(-sx, +sx, +1.f, -1.f, .01f, 100.f);
			
			const Mat4x4 transform = calculateWorldTransform();
			const Mat4x4 transform_inv = transform.CalcInv();
			
			out_transform = projection * transform_inv;
		}
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
	
	void tick(const float dt, bool & inputIsCaptured)
	{
		if (inputIsCaptured == false)
		{
			if (keyboard.wentDown(SDLK_1))
			{
				setMode(Camera::kMode_Orbit);
				inputIsCaptured = true;
			}
			if (keyboard.wentDown(SDLK_2))
			{
				setMode(Camera::kMode_Ortho);
				inputIsCaptured = true;
			}
			if (keyboard.wentDown(SDLK_3))
			{
				setMode(Camera::kMode_FirstPerson);
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
	
	void setMode(const Mode in_mode)
	{
		mode = in_mode;
	}
	
	Mat4x4 getViewProjectionMatrix(const int viewportSx, const int viewportSy) const
	{
		Mat4x4 matrix;
		
		switch (mode)
		{
		case kMode_Orbit:
			orbit.calculateTransform(matrix, viewportSx, viewportSy);
			break;
		case kMode_Ortho:
			ortho.calculateTransform(matrix, viewportSx, viewportSy);
			break;
		case kMode_FirstPerson:
			matrix.MakeIdentity();
			break;
		}
		
		return matrix;
	}
	
	void pushViewProjectionMatrix() const
	{
		int viewportSx = 0;
		int viewportSy = 0;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
		
		const Mat4x4 matrix = getViewProjectionMatrix(viewportSx, viewportSy);
		
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxLoadIdentity();
			gxMultMatrixf(matrix.m_v);
		}
		gxMatrixMode(restoreMatrixMode);
	}

	void popViewProjectionMatrix() const
	{
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
		}
		gxMatrixMode(restoreMatrixMode);
	}
};
