#pragma once

#include "framework-camera.h"

#include "Quat.h"

#include <math.h> // fminf

class Surface;

struct OrientationGizmo
{
	int x = 0;
	int y = 0;
	
	int sx = 0;
	int sy = 0;
	
	Camera camera;
	
	Surface * surface = nullptr;
	
	Quat orientation;
	
	struct Animation
	{
		Quat from;
		Quat to;
		
		float progress = 1.f;
		
		void begin(const Quat & in_from, const Quat & in_to)
		{
			from = in_from;
			to = in_to;
			
			progress = 0.f;
		}
		
		void tick(const float dt, Quat & out_orientation)
		{
			if (isActive())
			{
				const float duration = .3f;
				
				progress = fminf(1.f, progress + dt / duration);
				
				if (progress == 1.f)
					out_orientation = to;
				else
					out_orientation = from.slerp(to, progress);
			}
		}
		
		bool isActive()
		{
			return progress != 1.f;
		}
	} animation;
	
	struct HitTestResult
	{
		int axis = -1;
		bool hitsBase = false;
	} hitTestResult;
	
	void init(const int in_sx, const int in_sy);
	void shut();
	
	void hitTest(
		Vec3Arg rayOrigin_world,
		Vec3Arg rayDirection_world,
		int & out_axis,
		bool & out_hitsBase) const;
	
	void tick(const float dt);
	
	void drawSurface();
	
	void draw() const;
};

