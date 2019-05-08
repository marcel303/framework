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
	else if (state == kState_DragArrow)
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
	else if (state == kState_DragRing)
	{
		const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
		// intersect the ray with the projection plane for the ring
		
		auto origin_gizmo = worldToGizmo * ray_origin;
		auto direction_gizmo = worldToGizmo.Mul3(ray_direction);
		
		// determine the intersection point with the plane, and check its angle relative to the center of the gizmo
		
		const float t = - origin_gizmo[dragRing.axis] / direction_gizmo[dragRing.axis];
		
		const Vec3 position_world = ray_origin + ray_direction * t;
		
		const float angle = calculateRingAngle(position_world, dragRing.axis);
		
		// determine how much rotation occurred and rotate to match
		
		const float delta = dragRing.initialAngle - angle;
		
		if (dragRing.axis == 0)
			gizmoToWorld = gizmoToWorld.RotateX(delta);
		if (dragRing.axis == 1)
			gizmoToWorld = gizmoToWorld.RotateY(delta);
		if (dragRing.axis == 2)
			gizmoToWorld = gizmoToWorld.RotateZ(delta);
		
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
				state = kState_DragArrow;
				
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
				
				state = kState_DragArrow;
				dragAxis = DragAxis();
				dragAxis.active_axis[0] = true;
				dragAxis.active_axis[2] = true;
			}
		}
		
		if (intersectionResult.element == kElement_XRing ||
			intersectionResult.element == kElement_YRing ||
			intersectionResult.element == kElement_ZRing)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				inputIsCaptured = true;
				state = kState_DragRing;
				
				dragRing = DragRing();
				dragRing.axis = intersectionResult.element - kElement_XRing;
				const Vec3 position = ray_origin + ray_direction * intersectionResult.t;
				dragRing.initialAngle = calculateRingAngle(position, dragRing.axis);
			}
		}
	}
}

static void drawRing(const Vec3 & position, const int axis, const float radius, const float tubeRadius)
{
	gxPushMatrix();
	gxTranslatef(position[0], position[1], position[2]);
	
	const int axis1 = axis;
	const int axis2 = (axis + 1) % 3;
	const int axis3 = (axis + 2) % 3;
	
	const float radius1 = radius - tubeRadius;
	const float radius2 = radius + tubeRadius;
	
	float coords[100][2][3];
	
	for (int i = 0; i < 100; ++i)
	{
		const float angle = 2.f * float(M_PI) * i / 100.f;
		
		const float c = cosf(angle);
		const float s = sinf(angle);
		
		coords[i][0][axis1] = 0.f;
		coords[i][0][axis2] = c * radius1;
		coords[i][0][axis3] = s * radius1;
		
		coords[i][1][axis1] = 0.f;
		coords[i][1][axis2] = c * radius2;
		coords[i][1][axis3] = s * radius2;
	}
	
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < 100; ++i)
		{
			const int i1 = i;
			const int i2 = i + 1 < 100 ? i + 1 : 0;
			
			gxVertex3fv(coords[i1][0]);
			gxVertex3fv(coords[i1][1]);
			gxVertex3fv(coords[i2][1]);
			gxVertex3fv(coords[i2][0]);
		}
	}
	gxEnd();
	
	gxPopMatrix();
}

void TranslationGizmo::draw() const
{
	if (state == kState_Hidden)
		return;
	
	gxPushMatrix();
	gxMultMatrixf(gizmoToWorld.m_v);
	{
		// draw axis arrows
		
		setColorForArrow(0);
		drawAxisArrow(Vec3(), 0, arrow_radius, arrow_length);
		
		setColorForArrow(1);
		drawAxisArrow(Vec3(), 1, arrow_radius, arrow_length);
		
		setColorForArrow(2);
		drawAxisArrow(Vec3(), 2, arrow_radius, arrow_length);
		
		// draw pads
		
		const Color pad_color(100, 100, 100);
		const Color pad_color_highlight(200, 200, 200);
		
		setColor(intersectionResult.element == kElement_XZAxis ? pad_color_highlight : pad_color);
		fillCube(
			Vec3(pad_offset, 0.f, pad_offset),
			Vec3(pad_size, pad_thickness, pad_size));
		
		setColorForRing(0);
		drawRing(Vec3(), 0, 2.f, .2f);
		
		setColorForRing(1);
		drawRing(Vec3(), 1, 2.f, .2f);
		
		setColorForRing(2);
		drawRing(Vec3(), 2, 2.f, .2f);
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
	
	// intersect the arrows
	
	for (int i = 0; i < 3; ++i)
	{
		const int axis1 = (i + 1) % 3;
		const int axis2 = (i + 2) % 3;
		
		if (intersectCircle(
			origin_gizmo[axis1],
			origin_gizmo[axis2],
			direction_gizmo[axis1],
			direction_gizmo[axis2],
			0.f, 0.f, arrow_radius, t) && t < best_t)
		{
			const Vec3 intersection_pos = origin_gizmo + direction_gizmo * t;
			
			Vec3 axis;
			axis[i] = 1.f;
			
			const float distance = intersection_pos * axis;
			
			if (distance >= 0.f && distance < arrow_length)
			{
				best_t = t;
				result.element = (Element)(kElement_XAxis + i);
			}
		}
	}
	
	// intersect the XZ pad
	
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
			best_t = t;
			result.element = kElement_XZAxis;
		}
	}

	// intersect the rings
	
	for (int i = 0; i < 3; ++i)
	{
		const float t = -origin_gizmo[i] / direction_gizmo[i];
		
		if (t >= 0.f && t < best_t)
		{
			const Vec3 intersectionPoint = origin_gizmo + direction_gizmo * t;
			const float distanceToCenterOfGizmo = intersectionPoint.CalcSize();
			
			if (distanceToCenterOfGizmo >= ring_radius - ring_tubeRadius &&
				distanceToCenterOfGizmo <= ring_radius + ring_tubeRadius)
			{
				best_t = t;
				result.element = (Element)(kElement_XRing + i);
			}
		}
	}
	
	result.t = best_t;
	
	return result;
}

float TranslationGizmo::calculateRingAngle(Vec3Arg position_world, const int axis) const
{
	const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
	const Vec3 position_gizmo = worldToGizmo * position_world;
	
	const int axis2 = (axis + 1) % 3;
	const int axis3 = (axis + 2) % 3;
	
	return atan2f(position_gizmo[axis3], position_gizmo[axis2]);
}

void TranslationGizmo::setColorForArrow(const int axis) const
{
	const Color colors[3] = { colorRed, colorGreen, colorBlue };
	
	if (state == kState_DragArrow && dragAxis.active_axis[axis])
		setColor(colorWhite);
	else if (intersectionResult.element == kElement_XAxis + axis)
		setColor(colorYellow);
	else
		setColor(colors[axis]);
}

void TranslationGizmo::setColorForRing(const int axis) const
{
	const Color colors[3] = { colorRed, colorGreen, colorBlue };
	
	if (state == kState_DragRing && dragRing.axis == axis)
		setColor(colorWhite);
	else if (intersectionResult.element == kElement_XRing + axis)
		setColor(colorYellow);
	else
		setColor(colors[axis]);
}
