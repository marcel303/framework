#include "draw.h"
#include "framework.h"
#include "raycast.h"
#include "transformGizmos.h"

void TransformGizmo::show(const Mat4x4 & transform)
{
	if (state == kState_Hidden)
		state = kState_Visible;
	
	gizmoToWorld = transform;
}

void TransformGizmo::hide()
{
	state = kState_Hidden;
}

static int determineDragArrowProjectionAxis(const int axis, Vec3Arg ray_direction)
{
	int projection_axis = (axis + 1) % 3;
	
	if (ray_direction[projection_axis] == 0.f)
		projection_axis = (projection_axis + 1) % 3;
	
	Assert(ray_direction[projection_axis] != 0.f);
	
	return projection_axis;
}

bool TransformGizmo::tick(Vec3Arg ray_origin, Vec3Arg ray_direction, bool & inputIsCaptured)
{
	isInteractive = (inputIsCaptured == false);
	
	if (inputIsCaptured)
	{
		//
	}
	else if (state == kState_Hidden)
	{
		//
	}
	else if (state == kState_DragArrow)
	{
		inputIsCaptured = true;
		
		const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
		
		const Vec3 origin_gizmo = worldToGizmo * ray_origin;
		const Vec3 direction_gizmo = worldToGizmo.Mul3(ray_direction);
		
		const float t = - origin_gizmo[dragArrow.projection_axis] / direction_gizmo[dragArrow.projection_axis];
		const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
		
		//logDebug("initial_pos: %f, current_pos: %f", dragAxis.initialPosition[i], position_gizmo[i]);
		
		const Vec3 delta = position_gizmo - dragArrow.initialPosition;
		
		Vec3 drag;
		drag[dragArrow.axis] = delta[dragArrow.axis];
		
		gizmoToWorld = gizmoToWorld.Translate(drag[0], drag[1], drag[2]);
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			state = kState_Visible;
		}
	}
	else if (state == kState_DragPad)
	{
		inputIsCaptured = true;
		
		const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
		
		const Vec3 origin_gizmo = worldToGizmo * ray_origin;
		const Vec3 direction_gizmo = worldToGizmo.Mul3(ray_direction);
		
		const float t = - origin_gizmo[dragPad.projection_axis] / direction_gizmo[dragPad.projection_axis];
		const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
		
		//logDebug("initial_pos: %f, current_pos: %f", dragAxis.initialPosition[i], position_gizmo[i]);
		
		const Vec3 delta = position_gizmo - dragPad.initialPosition;
		
		Vec3 drag = delta;
		drag[dragPad.projection_axis] = 0.f;
		
		gizmoToWorld = gizmoToWorld.Translate(drag[0], drag[1], drag[2]);
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			state = kState_Visible;
		}
	}
	else if (state == kState_DragRing)
	{
		inputIsCaptured = true;
		
		const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
		// intersect the ray with the projection plane for the ring
		
		auto origin_gizmo = worldToGizmo * ray_origin;
		auto direction_gizmo = worldToGizmo.Mul3(ray_direction);
		
		// determine the intersection point with the plane, and check its angle relative to the center of the gizmo
		
		const float t = - origin_gizmo[dragRing.axis] / direction_gizmo[dragRing.axis];
		
		const Vec3 position_world = ray_origin + ray_direction * t;
		
		const float angle = calculateRingAngle(position_world, dragRing.axis);
		
		// determine how much rotation occurred and rotate to match
		
	#if 0 // todo : make smoothing amount a parameter
		const float delta = (dragRing.initialAngle - angle) * .1f;
	#else
		const float delta = dragRing.initialAngle - angle;
	#endif
		
		if (dragRing.axis == 0) gizmoToWorld = gizmoToWorld.RotateX(delta);
		if (dragRing.axis == 1) gizmoToWorld = gizmoToWorld.RotateY(delta);
		if (dragRing.axis == 2) gizmoToWorld = gizmoToWorld.RotateZ(delta);
		
		if (mouse.wentUp(BUTTON_LEFT))
		{
			state = kState_Visible;
		}
	}
	else if (state == kState_Visible)
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

				dragArrow = DragArrow();
				const int axis = intersectionResult.element - kElement_XAxis;
				dragArrow.axis = axis;
				
				//
				const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
				const Vec3 origin_gizmo = worldToGizmo * ray_origin;
				const Vec3 direction_gizmo = worldToGizmo.Mul3(ray_direction);
				dragArrow.projection_axis = determineDragArrowProjectionAxis(axis, direction_gizmo);
				const float t = - origin_gizmo[dragArrow.projection_axis] / direction_gizmo[dragArrow.projection_axis];
				const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
				dragArrow.initialPosition = position_gizmo;
			}
		}
		
		if (intersectionResult.element == kElement_XZPad ||
			intersectionResult.element == kElement_YXPad ||
			intersectionResult.element == kElement_ZYPad)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				inputIsCaptured = true;
				
				state = kState_DragPad;
				
				dragPad = DragPad();
				
				//
				const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
				const Vec3 origin_gizmo = worldToGizmo * ray_origin;
				const Vec3 direction_gizmo = worldToGizmo.Mul3(ray_direction);
				const int projection_axis =
					intersectionResult.element == kElement_XZPad ? 1 :
					intersectionResult.element == kElement_YXPad ? 2 :
					intersectionResult.element == kElement_ZYPad ? 0 :
					-1;
				const float t = - origin_gizmo[projection_axis] / direction_gizmo[projection_axis];
				const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
				dragPad.initialPosition = position_gizmo;
				dragPad.projection_axis = projection_axis;
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
				const Vec3 position_world = ray_origin + ray_direction * intersectionResult.t;
				dragRing.initialAngle = calculateRingAngle(position_world, dragRing.axis);
			}
		}
	}
	
	return
		state == kState_DragRing ||
		state == kState_DragArrow ||
		state == kState_DragPad;
}

static void drawRing(const Vec3 & position, const int axis, const float radius, const float tubeRadius)
{
	gxPushMatrix();
	gxTranslatef(position[0], position[1], position[2]);
	
	const int axis1 = (axis + 0) % 3;
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
	
	float normal[3];
	normal[axis1] = 1.f;
	normal[axis2] = 0.f;
	normal[axis3] = 0.f;
	
	gxBegin(GX_QUADS);
	{
		gxNormal3fv(normal);
		
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

void TransformGizmo::draw() const
{
	if (state == kState_Hidden)
		return;
	
	if (!isInteractive)
		pushColorPost(POST_RGB_TO_LUMI);
	
	pushBlend(BLEND_ALPHA); // fixme : draw gizmo in translucent pass or a separate pass where alpha blend is the default blend mode
	
	gxPushMatrix();
	gxMultMatrixf(gizmoToWorld.m_v);
	{
		const int opacity = 60;
		
		// draw axis arrows
		
		setColorForArrow(0);
		drawAxisArrow(Vec3(), 0, arrow_radius, arrow_length);
		
		setColorForArrow(1);
		drawAxisArrow(Vec3(), 1, arrow_radius, arrow_length);
		
		setColorForArrow(2);
		drawAxisArrow(Vec3(), 2, arrow_radius, arrow_length);
		
		// draw pads
		
		for (int i = 0; i < 3; ++i)
		{
			const Color pad_colors[3] =
			{
				Color(255, 40, 40, opacity),
				Color(40, 255, 40, opacity),
				Color(40, 40, 255, opacity)
			};
			
			const Color & pad_color = pad_colors[i];
			const Color pad_color_highlight(255, 255, 40, opacity);
			
			const int axis1 = (i + 0) % 3;
			const int axis2 = (i + 1) % 3;
			const int axis3 = (i + 2) % 3;
			
			Vec3 position;
			Vec3 size;
			
			position[axis1] = pad_offset;
			position[axis2] = 0.f;
			position[axis3] = pad_offset;
			
			size[axis1] = pad_size;
			size[axis2] = pad_thickness;
			size[axis3] = pad_size;
			
			const Color color =
				intersectionResult.element == (kElement_XZPad + i)
					? pad_color_highlight
					: pad_color;
			
			setColor(color);
			fillCube(
				position,
				size);
			setAlpha(255);
			lineCube(
				position,
				size);
		}
		
		setColorForRing(0);
		drawRing(Vec3(), 0, ring_radius, ring_tubeRadius);
		
		setColorForRing(1);
		drawRing(Vec3(), 1, ring_radius, ring_tubeRadius);
		
		setColorForRing(2);
		drawRing(Vec3(), 2, ring_radius, ring_tubeRadius);
	}
	gxPopMatrix();
	
	popBlend();
	
	if (!isInteractive)
		popColorPost();
}

TransformGizmo::IntersectionResult TransformGizmo::intersect(Vec3Arg origin_world, Vec3Arg direction_world) const
{
	IntersectionResult result;
	
	const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
	auto origin_gizmo = worldToGizmo * origin_world;
	auto direction_gizmo = worldToGizmo.Mul3(direction_world);
	
	float best_t = FLT_MAX;
	
	float t;
	
	if (enableTranslation)
	{
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

		for (int i = 0; i < 3; ++i)
		{
			// intersect the pads
			
			const int axis1 = (i + 0) % 3;
			const int axis2 = (i + 1) % 3;
			const int axis3 = (i + 2) % 3;
			
			const Element elements[3] =
			{
				kElement_XZPad,
				kElement_YXPad,
				kElement_ZYPad
			};
			
			{
				float min[3];
				min[axis1] = + pad_offset - pad_size;
				min[axis2] = - pad_thickness;
				min[axis3] = + pad_offset - pad_size;
				
				float max[3];
				max[axis1] = + pad_offset + pad_size;
				max[axis2] = + pad_thickness;
				max[axis3] = + pad_offset + pad_size;
				
				if (intersectBoundingBox3d(
					min, max,
					origin_gizmo[0],
					origin_gizmo[1],
					origin_gizmo[2],
					1.f / direction_gizmo[0],
					1.f / direction_gizmo[1],
					1.f / direction_gizmo[2], t) && t < best_t)
				{
					best_t = t;
					result.element = elements[i];
				}
			}
		}
	}

	if (enableRotation)
	{
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
	}
	
	result.t = best_t;
	
	return result;
}

float TransformGizmo::calculateRingAngle(Vec3Arg position_world, const int axis) const
{
	const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
	Vec3 position_gizmo = worldToGizmo * position_world;
	
	// we need to undo any scaling, to get 'reproducible' angles
	for (int i = 0; i < 3; ++i)
		position_gizmo[i] /= worldToGizmo.GetAxis(i).CalcSize();
	
	const int axis2 = (axis + 1) % 3;
	const int axis3 = (axis + 2) % 3;
	
	return atan2f(position_gizmo[axis3], position_gizmo[axis2]);
}

void TransformGizmo::setColorForArrow(const int axis) const
{
	const Color colors[3] = { colorRed, colorGreen, colorBlue };
	
	if (state == kState_DragArrow && dragArrow.axis == axis)
		setColor(colorWhite);
	else if (intersectionResult.element == kElement_XAxis + axis)
		setColor(colorYellow);
	else
		setColor(colors[axis]);
}

void TransformGizmo::setColorForRing(const int axis) const
{
	const Color colors[3] = { colorRed, colorGreen, colorBlue };
	
	if (state == kState_DragRing && dragRing.axis == axis)
		setColor(colorWhite);
	else if (intersectionResult.element == kElement_XRing + axis)
		setColor(colorYellow);
	else
		setColor(colors[axis]);
	setAlpha(100);
}
