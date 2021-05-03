#include "orientationGizmo.h"

#include "draw.h"
#include "raycast.h"

#include "framework.h" // Surface

#include "Calc.h" // mPI

void OrientationGizmo::init(const int in_sx, const int in_sy)
{
	sx = in_sx;
	sy = in_sy;
	
	camera.mode = Camera::kMode_FirstPerson;
	camera.firstPerson.position.Set(0, 0, -4);
	
	surface = new Surface();
	surface->init(sx, sy, SURFACE_RGBA8, true, false);
	surface->setClearColor(0, 0, 0, 0);
	
	orientation.fromAngleAxis(0.f, Vec3(0, 1, 0));
}

void OrientationGizmo::shut()
{
	surface->free();
	
	delete surface;
	surface = nullptr;
}

void OrientationGizmo::hitTest(
	Vec3Arg rayOrigin_world,
	Vec3Arg rayDirection_world,
	int & out_axis,
	bool & out_hitsBase) const
{
	const Mat4x4 orientationMatrix = orientation.toMatrix();
	
	out_axis = -1;
	out_hitsBase = false;
	
	float best_t = -1.f;
	
	for (int i = 0; i < 3; ++i)
	{
		const float coneRadius = .3f;
		const float coneHeight = 2.f;
		
		const Vec3 coneTip_world(0.f);
		
		Vec3 coneAxis_cone;
		coneAxis_cone[i] = 1.f;
		
		const Vec3 coneAxis_world = orientationMatrix.Mul3(coneAxis_cone);
		
		float t;
		bool hitsBase;
		if (intersectCone(
			rayOrigin_world,
			rayDirection_world,
			coneTip_world,
			coneAxis_world,
			coneRadius,
			coneHeight,
			t,
			hitsBase))
		{
			if (t < best_t || best_t < 0.f)
			{
				logDebug("hit at t=%.2f. base: %d", t, hitsBase?1:0);
				
				best_t = t;
				
				out_axis = i;
				out_hitsBase = hitsBase;
			}
		}
	}
}

void OrientationGizmo::tick(const float dt)
{
	if (animation.isActive())
	{
		animation.tick(dt, orientation);
	}
	else
	{
		Mat4x4 projectionMatrix;
		camera.calculateProjectionMatrix(sx, sy, projectionMatrix);
		
		Mat4x4 viewToWorld;
		camera.calculateWorldMatrix(viewToWorld);
		
		const Vec3 pointerOrigin_world = viewToWorld.GetTranslation();
		
		const Vec2 mousePosition_screen(
			mouse.x,
			mouse.y);
		const Vec2 mousePosition_clip(
			(mousePosition_screen[0] - x) / float(sx) * 2.f - 1.f,
			(mousePosition_screen[1] - y) / float(sy) * 2.f - 1.f);
		Vec2 mousePosition_view = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
		
		Vec3 pointerDirection_world = viewToWorld.Mul3(
			Vec3(
				+mousePosition_view[0],
				-mousePosition_view[1],
				1.f));
		pointerDirection_world = pointerDirection_world.CalcNormalized();
		
		// hit test
		
		hitTest(
			pointerOrigin_world,
			pointerDirection_world,
			hitTestResult.axis,
			hitTestResult.hitsBase);
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (hitTestResult.axis != -1)
			{
				if (hitTestResult.hitsBase)
				{
					if (hitTestResult.axis == 0) animation.begin(orientation, Quat(-Calc::mPI2, Vec3(0, 1, 0)));
					if (hitTestResult.axis == 1) animation.begin(orientation, Quat(-Calc::mPI2, Vec3(1, 0, 0)));
					if (hitTestResult.axis == 2) animation.begin(orientation, Quat(          0, Vec3(0, 1, 0)));
				}
				else
				{
					// manual drag
				}
			}
		}
	}
	
// todo : only redraw surface when dirty
	drawSurface();
}

void OrientationGizmo::drawSurface()
{
	pushSurface(surface, true);
	pushDepthTest(true, DEPTH_LESS);
	{
		camera.pushProjectionMatrix();
		camera.pushViewMatrix();
		{
			const Mat4x4 orientationMatrix = orientation.toMatrix();
			gxMultMatrixf(orientationMatrix.m_v);
			
			const Color colors[3] =
				{
					Color(255, 0, 0),
					Color(0, 255, 0),
					Color(0, 0, 255)
				};
				
			for (int i = 0; i < 3; ++i)
			{
				setColor(hitTestResult.axis == i ? colorWhite : colors[i]);
				fillCylinder(Vec3(), i, 0.f, .3f, 2.f, false);
			}
			
			//setColor(colorWhite);
			//fillCube(Vec3(0.f), Vec3(1.f));
		}
		camera.popViewMatrix();
		camera.popProjectionMatrix();
	}
	popDepthTest();
	popSurface();
}

void OrientationGizmo::draw() const
{
	gxSetTexture(surface->getTexture());
	{
		setColor(colorWhite);
		drawRect(x, y, x + sx, y + sy);
	}
	gxSetTexture(0);
	
	setColor(colorWhite);
	drawRectLine(x, y, x + sx, y + sy);
}
