#include "draw.h"
#include "framework.h"
#include "raycast.h"
#include "transformGizmos.h"

void TranslationGizmo::show(const Mat4x4 & transform)
{
	if (state == kState_Hidden)
		state = kState_Visible;
	
	gizmoToWorld = transform;
}

void TranslationGizmo::hide()
{
	state = kState_Hidden;
}

void TranslationGizmo::tick(const Mat4x4 & projectionMatrix, const Mat4x4 & cameraToWorld, Vec3Arg ray_origin, Vec3Arg ray_direction, bool & inputIsCaptured)
{
	if (inputIsCaptured)
	{
		hide();
	}
	else if (state == kState_Hidden)
	{
		//
	}
	else if (state == kState_DragAxis)
	{
		const float dragSpeed = keyboard.isDown(SDLK_LSHIFT) ? 1.f : 10.f;
		
	#if 1
		int viewportSx;
		int viewportSy;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
		
		const Vec3 mouseDelta_clip(mouse.dx / float(viewportSx), -mouse.dy / float(viewportSy), 0.f);
		Vec3 mouseDelta_camera = projectionMatrix.CalcInv() * mouseDelta_clip;
		mouseDelta_camera[2] = 0.f;
		
	#else
	// fixme : use projection matrix instead ..
		int viewportSx;
		int viewportSy;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
		const float mouseScale = 2.f / viewportSy;
		
		const Vec3 mouseDelta_camera = Vec3(+mouse.dx * mouseScale, -mouse.dy * mouseScale, 0.f);
	#endif
		const Vec3 mouseDelta_world = cameraToWorld.Mul3(mouseDelta_camera);
		const Vec3 mouseDelta_gizmo = gizmoToWorld.CalcInv().Mul3(mouseDelta_world);
		
		for (int i = 0; i < 3; ++i)
		{
			if (dragAxis.active_axis[i])
			{
				Vec3 drag;
				drag[i] = mouseDelta_gizmo[i] * dragSpeed;
				
				gizmoToWorld = gizmoToWorld.Translate(drag[0], drag[1], drag[2]);
			}
		}
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			state = kState_Visible;
		}
		else
		{
			inputIsCaptured = true;
		}
	}
	else
	{
		intersectionResult = intersect(ray_origin, ray_direction);
		
		if (intersectionResult.element == kElement_XAxis ||
			intersectionResult.element == kElement_YAxis ||
			intersectionResult.element == kElement_ZAxis)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				inputIsCaptured = true;
				state = kState_DragAxis;
				dragAxis = DragAxis();
				const int axis_index = intersectionResult.element - kElement_XAxis;
				dragAxis.active_axis[axis_index] = true;
			}
		}
		
		if (intersectionResult.element == kElement_XZAxis)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				inputIsCaptured = true;
				
				state = kState_DragAxis;
				dragAxis = DragAxis();
				dragAxis.active_axis[0] = true;
				dragAxis.active_axis[2] = true;
			}
		}
	}
}

void TranslationGizmo::draw() const
{
	if (state == kState_Hidden)
		return;
	
	gxPushMatrix();
	gxMultMatrixf(gizmoToWorld.m_v);
	{
		// draw axis arrows
		
		setColorForAxis(0);
		drawAxisArrow(Vec3(), 0, radius, length);
		
		setColorForAxis(1);
		drawAxisArrow(Vec3(), 1, radius, length);
		
		setColorForAxis(2);
		drawAxisArrow(Vec3(), 2, radius, length);
		
		// draw pads
		
		const Color pad_color(100, 100, 100);
		const Color pad_color_highlight(200, 200, 200);
		
		setColor(intersectionResult.element == kElement_XZAxis ? pad_color_highlight : pad_color);
		fillCube(
			Vec3(pad_offset, 0.f, pad_offset),
			Vec3(pad_size, pad_thickness, pad_size));
	}
	gxPopMatrix();
}

TranslationGizmo::IntersectionResult TranslationGizmo::intersect(Vec3Arg origin_world, Vec3Arg direction_world) const
{
	IntersectionResult result;
	
	const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
	auto origin_gizmo = worldToGizmo * origin_world;
	auto direction_gizmo = worldToGizmo.Mul3(direction_world);
	
	float best_t = FLT_MAX;
	
	float t;
	
	for (int i = 0; i < 3; ++i)
	{
		const int axis1 = (i + 1) % 3;
		const int axis2 = (i + 2) % 3;
		
		if (intersectCircle(
			origin_gizmo[axis1],
			origin_gizmo[axis2],
			direction_gizmo[axis1],
			direction_gizmo[axis2],
			0.f, 0.f, radius, t) && t < best_t)
		{
			const Vec3 intersection_pos = origin_gizmo + direction_gizmo * t;
			
			Vec3 axis;
			axis[i] = 1.f;
			
			const float distance = intersection_pos * axis;
			
			if (distance >= 0.f && distance < length)
			{
				best_t = t;
				result.element = (Element)(kElement_XAxis + i);
			}
		}
	}
	
	{
		const float min[3] =
		{
			+ pad_offset - pad_size,
			- pad_thickness,
			+ pad_offset - pad_size
		};
		
		const float max[3] =
		{
			+ pad_offset + pad_size,
			+ pad_thickness,
			+ pad_offset + pad_size
		};
		
		if (intersectBoundingBox3d(
			min, max,
			origin_gizmo[0], origin_gizmo[1], origin_gizmo[2],
			1.f / direction_gizmo[0], 1.f / direction_gizmo[1], 1.f / direction_gizmo[2], t) && t < best_t)
		{
			result.element = kElement_XZAxis;
		}
	}
	
	return result;
}

void TranslationGizmo::setColorForAxis(const int axis) const
{
	const Color colors[3] = { colorRed, colorGreen, colorBlue };
	
	if (state == kState_DragAxis && dragAxis.active_axis[axis])
		setColor(colorWhite);
	else if (intersectionResult.element == kElement_XAxis + axis)
		setColor(colorYellow);
	else
		setColor(colors[axis]);
}
