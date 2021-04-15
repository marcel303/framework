#include "draw.h"
#include "framework.h"
#include "raycast.h"
#include "transformGizmos.h"

static const float kMinArrowVisibility = .01f; // cos of angle (rad) at which the arrows will start to disappear
static const float kMinPadVisibility = .12f; // cos of angle (rad) at which the pads will start to disappear
static const float kMinRingVisibility = .06f; // cos of angle (rad) at which the arrows will start to disappear

void TransformGizmo::show(const Mat4x4 & transform)
{
	if (state == kState_Hidden)
		state = kState_Visible;
	
	gizmoToWorld = transform;
}

void TransformGizmo::hide()
{
	if (isActive())
	{
		if (editingDidEnd != nullptr)
			editingDidEnd();
	}
	
	state = kState_Hidden;
}

bool TransformGizmo::isActive() const
{
	return
		state != kState_Hidden &&
		state != kState_Visible;
}

static int determineDragArrowProjectionAxis(const int axis, Vec3Arg ray_direction)
{
	int projection_axis = (axis + 1) % 3;
	
	if (ray_direction[projection_axis] == 0.f)
		projection_axis = (projection_axis + 1) % 3;
	
	Assert(ray_direction[projection_axis] != 0.f);
	
	return projection_axis;
}

bool TransformGizmo::tick(
	Vec3Arg pointer_origin,
	Vec3Arg pointer_direction,
	const bool pointer_isActive,
	const bool pointer_becameActive,
	bool & inputIsCaptured,
	Vec3Arg viewOrigin_world,
	Vec3Arg viewDirection_world)
{
	isInteractive = isActive() || (inputIsCaptured == false);
	
	const Mat4x4 worldToGizmo = gizmoToWorld.CalcInv();
	
	rayOriginInGizmoSpace = worldToGizmo.Mul4(pointer_origin);
	
	updatePadPositions(rayOriginInGizmoSpace);
	
	updateElementVisibility(viewOrigin_world, viewDirection_world);
	
	if (state == kState_Hidden)
	{
		//
	}
	else if (state == kState_DragArrow)
	{
		Assert(inputIsCaptured && uiCaptureElem.hasCapture);
		Verify(uiCaptureElem.capture());
		
		const Vec3 origin_gizmo = worldToGizmo * pointer_origin;
		const Vec3 direction_gizmo = worldToGizmo.Mul3(pointer_direction);
		
		const float t = - origin_gizmo[dragArrow.projection_axis] / direction_gizmo[dragArrow.projection_axis];
		const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
		
		//logDebug("initial_pos: %f, current_pos: %f", dragAxis.initialPosition[i], position_gizmo[i]);
		
		const Vec3 delta = position_gizmo - dragArrow.initialPosition;
		
		Vec3 drag;
		drag[dragArrow.axis] = delta[dragArrow.axis];
		
		gizmoToWorld = gizmoToWorld.Translate(drag[0], drag[1], drag[2]);
		
		if (!pointer_isActive)
		{
			state = kState_Visible;
			
			if (editingDidEnd != nullptr)
				editingDidEnd();
		}
	}
	else if (state == kState_DragPad)
	{
		Assert(inputIsCaptured && uiCaptureElem.hasCapture);
		Verify(uiCaptureElem.capture());
		
		const Vec3 origin_gizmo = worldToGizmo * pointer_origin;
		const Vec3 direction_gizmo = worldToGizmo.Mul3(pointer_direction);
		
		const float t = - origin_gizmo[dragPad.projection_axis] / direction_gizmo[dragPad.projection_axis];
		const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
		
		//logDebug("initial_pos: %f, current_pos: %f", dragAxis.initialPosition[i], position_gizmo[i]);
		
		const Vec3 delta = position_gizmo - dragPad.initialPosition;
		
		Vec3 drag = delta;
		drag[dragPad.projection_axis] = 0.f;
		
		gizmoToWorld = gizmoToWorld.Translate(drag[0], drag[1], drag[2]);
		
		if (!pointer_isActive)
		{
			state = kState_Visible;
			
			if (editingDidEnd != nullptr)
				editingDidEnd();
		}
	}
	else if (state == kState_DragRing)
	{
		Assert(inputIsCaptured && uiCaptureElem.hasCapture);
		Verify(uiCaptureElem.capture());
	
		// intersect the ray with the projection plane for the ring
		
		auto origin_gizmo = worldToGizmo * pointer_origin;
		auto direction_gizmo = worldToGizmo.Mul3(pointer_direction);
		
		// determine the intersection point with the plane, and check its angle relative to the center of the gizmo
		
		const float t = - origin_gizmo[dragRing.axis] / direction_gizmo[dragRing.axis];
		
		const Vec3 position_world = pointer_origin + pointer_direction * t;
		
		const float angle_a = calculateRingAngle(position_world, dragRing.axis);
		const float angle = t < 0.f ? angle_a + float(M_PI) : angle_a;
		
		// determine how much rotation occurred and rotate to match
		
	#if 0 // todo : make smoothing amount a parameter
		const float delta = (dragRing.initialAngle - angle) * .1f;
	#else
		const float delta = dragRing.initialAngle - angle;
	#endif
		
		if (dragRing.axis == 0) gizmoToWorld = gizmoToWorld.RotateX(delta);
		if (dragRing.axis == 1) gizmoToWorld = gizmoToWorld.RotateY(delta);
		if (dragRing.axis == 2) gizmoToWorld = gizmoToWorld.RotateZ(delta);
		
		if (!pointer_isActive)
		{
			state = kState_Visible;
			
			if (editingDidEnd != nullptr)
				editingDidEnd();
		}
	}
	else if (state == kState_Visible)
	{
		intersectionResult =
			isInteractive
			? intersect(pointer_origin, pointer_direction)
			: IntersectionResult();
		
		if (intersectionResult.element == kElement_XAxis ||
			intersectionResult.element == kElement_YAxis ||
			intersectionResult.element == kElement_ZAxis)
		{
			if (pointer_becameActive && inputIsCaptured == false)
			{
				Verify(uiCaptureElem.capture());
				inputIsCaptured = true;
				
				state = kState_DragArrow;

				dragArrow = DragArrow();
				const int axis = intersectionResult.element - kElement_XAxis;
				dragArrow.axis = axis;
				
				//
				
				const Vec3 origin_gizmo = worldToGizmo * pointer_origin;
				const Vec3 direction_gizmo = worldToGizmo.Mul3(pointer_direction);
				dragArrow.projection_axis = determineDragArrowProjectionAxis(axis, direction_gizmo);
				const float t = - origin_gizmo[dragArrow.projection_axis] / direction_gizmo[dragArrow.projection_axis];
				const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
				dragArrow.initialPosition = position_gizmo;
				
				//
				
				if (editingWillBegin != nullptr)
					editingWillBegin();
			}
		}
		
		if (intersectionResult.element == kElement_XZPad ||
			intersectionResult.element == kElement_YXPad ||
			intersectionResult.element == kElement_ZYPad)
		{
			if (pointer_becameActive && inputIsCaptured == false)
			{
				Verify(uiCaptureElem.capture());
				inputIsCaptured = true;
				
				state = kState_DragPad;
				
				dragPad = DragPad();
				
				//
				
				const Vec3 origin_gizmo = worldToGizmo * pointer_origin;
				const Vec3 direction_gizmo = worldToGizmo.Mul3(pointer_direction);
				const int projection_axis =
					intersectionResult.element == kElement_XZPad ? 1 :
					intersectionResult.element == kElement_YXPad ? 2 :
					intersectionResult.element == kElement_ZYPad ? 0 :
					-1;
				const float t = - origin_gizmo[projection_axis] / direction_gizmo[projection_axis];
				const Vec3 position_gizmo = origin_gizmo + direction_gizmo * t;
				dragPad.initialPosition = position_gizmo;
				dragPad.projection_axis = projection_axis;
				
				//
				
				if (editingWillBegin != nullptr)
					editingWillBegin();
			}
		}
		
		if (intersectionResult.element == kElement_XRing ||
			intersectionResult.element == kElement_YRing ||
			intersectionResult.element == kElement_ZRing)
		{
			if (pointer_becameActive && inputIsCaptured == false)
			{
				Verify(uiCaptureElem.capture());
				inputIsCaptured = true;
				
				state = kState_DragRing;
				
				dragRing = DragRing();
				dragRing.axis = intersectionResult.element - kElement_XRing;
				const Vec3 position_world = pointer_origin + pointer_direction * intersectionResult.t;
				dragRing.initialAngle = calculateRingAngle(position_world, dragRing.axis);
				
				if (editingWillBegin != nullptr)
					editingWillBegin();
			}
		}
	}
	
	Assert(
		state == kState_Hidden ||
		state == kState_Visible ||
		(inputIsCaptured && uiCaptureElem.hasCapture));
	
	return
		state == kState_DragRing ||
		state == kState_DragArrow ||
		state == kState_DragPad;
}

static void drawRingLine(const Vec3 & position, const int axis, const float radius)
{
	gxPushMatrix();
	gxTranslatef(position[0], position[1], position[2]);
	
	const int axis1 = (axis + 0) % 3;
	const int axis2 = (axis + 1) % 3;
	const int axis3 = (axis + 2) % 3;
	
	float coords[100][3];
	
	for (int i = 0; i < 100; ++i)
	{
		const float angle = 2.f * float(M_PI) * i / 100.f;
		
		const float c = cosf(angle);
		const float s = sinf(angle);
		
		coords[i][axis1] = 0.f;
		coords[i][axis2] = c * radius;
		coords[i][axis3] = s * radius;
	}
	
	float normal[3];
	normal[axis1] = 1.f;
	normal[axis2] = 0.f;
	normal[axis3] = 0.f;
	
	gxBegin(GX_LINE_LOOP);
	{
		gxNormal3fv(normal);
		
		for (int i = 0; i < 100; ++i)
			gxVertex3fv(coords[i]);
	}
	gxEnd();
	
	gxPopMatrix();
}

static void drawRingFill(const Vec3 & position, const int axis, const float radius, const float tubeRadius)
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
	
	pushCullMode(CULL_NONE, CULL_CCW);
	
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
	
	popCullMode();
	
	gxPopMatrix();
}

void TransformGizmo::drawOpaque(const bool depthObscuredPass) const
{
	if (state == kState_Hidden)
		return;
	
	if (!isInteractive || depthObscuredPass)
		pushColorPost(POST_RGB_TO_LUMI);
	
	gxPushMatrix();
	gxMultMatrixf(gizmoToWorld.m_v);
	{
		// draw axis arrows
		
		for (int i = 0; i < 3; ++i)
		{
			if (elementVisibility.arrow[i] < kMinArrowVisibility)
				continue;
				
			setColorForArrow(i);
			drawAxisArrow(Vec3(), i, arrow_radius, arrow_length, arrow_top_radius, arrow_top_length, rayOriginInGizmoSpace[i] < 0.f);
		}
		
		// draw pads
		
		for (int i = 0; i < 3; ++i)
		{
			if (elementVisibility.pad[i] < kMinPadVisibility)
				continue;;
				
			const Color pad_colors[3] =
			{
				Color(255, 40, 40),
				Color(40, 255, 40),
				Color(40, 40, 255)
			};
			
			const Color & pad_color = pad_colors[i];
			const Color pad_color_highlight(255, 255, 255);
			
			const int axis1 = (i + 0) % 3;
			const int axis2 = (i + 1) % 3;
			const int axis3 = (i + 2) % 3;
			
			Vec3 position;
			Vec3 size;
			
			position = padPosition[i];
		
			size[axis1] = pad_size;
			size[axis2] = pad_thickness;
			size[axis3] = pad_size;
			
			const Color color =
				intersectionResult.element == (kElement_XZPad + i)
					? pad_color_highlight
					: pad_color;
			
			setColor(color);
			lineCube(
				position,
				size);
		}
		
		for (int i = 0; i < 3; ++i)
		{
			if (elementVisibility.ring[i] < kMinRingVisibility)
				continue;
				
			setColorForRing(i);
			drawRingLine(Vec3(), i, ring_radius);
		}
		
	#if defined(DEBUG)
		if (debugging.hasNearestRingPos)
		{
			setColor(colorWhite);
			fillCube(debugging.nearestRingPos, Vec3(.01f));
			fillCube(debugging.nearestRingPos_projected, Vec3(.01f));
		}
	#endif
	}
	gxPopMatrix();
	
	if (!isInteractive || depthObscuredPass)
		popColorPost();
}

void TransformGizmo::drawTranslucent() const
{
	if (state == kState_Hidden)
		return;
	
	if (!isInteractive)
		pushColorPost(POST_RGB_TO_LUMI);
	
	gxPushMatrix();
	gxMultMatrixf(gizmoToWorld.m_v);
	{
		const int base_opacity = 191;
		
		// draw pads
		
		for (int i = 0; i < 3; ++i)
		{
		// todo : float colors
			const float fadeValue = powf(saturate<float>(lerp<float>(0.f, 1.f, (elementVisibility.pad[i] - kMinPadVisibility) / (1.f - kMinPadVisibility))), .5f);
			const int opacity = fadeValue * base_opacity;
			
			const Color pad_colors[3] =
			{
				Color(255, 40, 40, opacity),
				Color(40, 255, 40, opacity),
				Color(40, 40, 255, opacity)
			};
			
			const Color & pad_color = pad_colors[i];
			const Color pad_color_highlight = pad_color;
			
			const int axis1 = (i + 0) % 3;
			const int axis2 = (i + 1) % 3;
			const int axis3 = (i + 2) % 3;
			
			Vec3 position;
			Vec3 size;
			
			position[axis1] = pad_offset;
			position[axis2] = 0.f;
			position[axis3] = pad_offset;
			
			if (rayOriginInGizmoSpace[axis1] < 0.f)
				position[axis1] = -position[axis1];
			if (rayOriginInGizmoSpace[axis3] < 0.f)
				position[axis3] = -position[axis3];
			
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
		}
		
		for (int i = 0; i < 3; ++i)
		{
			if (intersectionResult.element == kElement_XRing + i)
			{
				setColorForRing(i);
				drawRingFill(Vec3(), i, ring_radius, ring_tubeRadius);
			}
		}
	}
	gxPopMatrix();
	
	if (!isInteractive)
		popColorPost();
}

void TransformGizmo::updatePadPositions(Vec3Arg rayOriginInGizmoSpace)
{
	for (int i = 0; i < 3; ++i)
	{
		const int axis1 = (i + 0) % 3;
		const int axis2 = (i + 1) % 3;
		const int axis3 = (i + 2) % 3;
		
		Vec3 & position = padPosition[i];
		
		position[axis1] = pad_offset;
		position[axis2] = 0.f;
		position[axis3] = pad_offset;
		
		if (rayOriginInGizmoSpace[axis1] < 0.f)
			position[axis1] = -position[axis1];
		if (rayOriginInGizmoSpace[axis3] < 0.f)
			position[axis3] = -position[axis3];
	}
}

void TransformGizmo::updateElementVisibility(Vec3Arg viewOrigin_world, Vec3Arg viewDirection_world)
{
	const Vec3 axis[3] =
		{
			gizmoToWorld.GetAxis(0).CalcNormalized(),
			gizmoToWorld.GetAxis(1).CalcNormalized(),
			gizmoToWorld.GetAxis(2).CalcNormalized()
		};
		
	const Vec3 viewToGizmo_world = (gizmoToWorld.GetTranslation() - viewOrigin_world).CalcNormalized();
	
	// update arrow visibility
	
	for (int i = 0; i < 3; ++i)
	{
		elementVisibility.arrow[i] = 1.f - fabsf(viewToGizmo_world * axis[i]);
	}
	
	// update pad visibility
	
	for (int i = 0; i < 3; ++i)
	{
		const int normalAxis = (i + 1) % 3;
		
		elementVisibility.pad[i] = fabsf(viewToGizmo_world * axis[normalAxis]);
	}
	
	// update ring visibility
	
	for (int i = 0; i < 3; ++i)
	{
		elementVisibility.ring[i] = fabsf(viewToGizmo_world * axis[i]);
	}
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
			if (elementVisibility.arrow[i] < kMinArrowVisibility)
				continue;
				
			const int axis1 = (i + 1) % 3;
			const int axis2 = (i + 2) % 3;
			
			if (intersectCircle(
				origin_gizmo[axis1],
				origin_gizmo[axis2],
				direction_gizmo[axis1],
				direction_gizmo[axis2],
				0.f, 0.f, arrow_collision_radius, t) && t < best_t)
			{
				const Vec3 intersection_pos = origin_gizmo + direction_gizmo * t;
				
				Vec3 axis;
				axis[i] = rayOriginInGizmoSpace[i] < 0.f ? -1.f : +1.f;
				
				const float distance = intersection_pos * axis;
				
				if (distance >= 0.f && distance < arrow_length + arrow_top_length)
				{
					best_t = t;
					result.element = (Element)(kElement_XAxis + i);
				}
			}
		}

		for (int i = 0; i < 3; ++i)
		{
			// intersect the pads
			
			const Element elements[3] =
			{
				kElement_XZPad,
				kElement_YXPad,
				kElement_ZYPad
			};
			
			if (elementVisibility.pad[i] < kMinPadVisibility)
				continue;;

			const int axis1 = (i + 0) % 3;
			const int axis2 = (i + 1) % 3;
			const int axis3 = (i + 2) % 3;
			
			Vec3 position;
			Vec3 size;
			
			position = padPosition[i];
			
			size[axis1] = pad_size;
			size[axis2] = pad_thickness;
			size[axis3] = pad_size;
			
			const Vec3 min = position - size;
			const Vec3 max = position + size;
		
			if (intersectBoundingBox3d(
				&min[0], &max[0],
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

	if (enableRotation)
	{
		// intersect the rings

	#if 1
		// works well when facing the ring head on. difficulty picking when viewing the ring at a grazing angle
		
		debugging.hasNearestRingPos = false;
		
		for (int i = 0; i < 3; ++i)
		{
			if (elementVisibility.ring[i] < kMinRingVisibility)
				continue;
				
			const int axis1 = (i + 1) % 3;
			const int axis2 = (i + 2) % 3;
			
			const float t_plane = - origin_gizmo[i] / direction_gizmo[i];
			const Vec3 p_plane = origin_gizmo + direction_gizmo * t_plane;
			const Vec3 nearestPoint = p_plane.CalcNormalized() * ring_radius;
			
			float sphere_t1, sphere_t2;
			
			if (intersectSphere(
				origin_gizmo[0],
				origin_gizmo[1],
				origin_gizmo[2],
				direction_gizmo[0],
				direction_gizmo[1],
				direction_gizmo[2],
				nearestPoint[0],
				nearestPoint[1],
				nearestPoint[2],
				ring_tubeRadius,
				sphere_t1,
				sphere_t2))
			{
				const float sphere_t = fminf(fmaxf(0.f, sphere_t1), sphere_t2);
				
				if (sphere_t2 >= 0.f && sphere_t < best_t)
				{
					best_t = sphere_t;
					result.element = (Element)(kElement_XRing + i);
					
				#if defined(DEBUG)
					debugging.hasNearestRingPos        = true;
					debugging.nearestRingPos           = origin_gizmo + direction_gizmo * sphere_t;
					debugging.nearestRingPos_projected = p_plane;//nearestPoint;
				#endif
				}
			}
		}
	#elif 1
		// works well both facing a ring head on and at a grazing angle, but misses the intersection when the initial
		// circle intersection fails, which happens at the 'extremes' of the circle from a visual perspective
		debugging.hasNearestRingPos = false;
		
		for (int i = 0; i < 3; ++i)
		{
			const int axis1 = (i + 1) % 3;
			const int axis2 = (i + 2) % 3;
			
			float t_circle[2];
			
			if (intersectCircle2(
				origin_gizmo[axis1],
				origin_gizmo[axis2],
				direction_gizmo[axis1],
				direction_gizmo[axis2],
				0.f,
				0.f,
				ring_radius,
				t_circle[0],
				t_circle[1]))
			{
				Vec3 origin_gizmo_projected;
				origin_gizmo_projected[axis1] = origin_gizmo[axis1];
				origin_gizmo_projected[axis2] = origin_gizmo[axis2];
				
				Vec3 direction_gizmo_projected;
				direction_gizmo_projected[axis1] = direction_gizmo[axis1];
				direction_gizmo_projected[axis2] = direction_gizmo[axis2];
				
				for (int c = 0; c < 2; ++c)
				{
					const Vec3 intersectionPos_projected = origin_gizmo_projected + direction_gizmo_projected * t_circle[c];
					
					float sphere_t1, sphere_t2;
					
					if (intersectSphere(
						origin_gizmo[0],
						origin_gizmo[1],
						origin_gizmo[2],
						direction_gizmo[0],
						direction_gizmo[1],
						direction_gizmo[2],
						intersectionPos_projected[0],
						intersectionPos_projected[1],
						intersectionPos_projected[2],
						ring_tubeRadius,
						sphere_t1,
						sphere_t2))
					{
						const float sphere_t = fminf(fmaxf(0.f, sphere_t1), sphere_t2);
						
						if (sphere_t2 >= 0.f && sphere_t < best_t)
						{
							best_t = sphere_t;
							result.element = (Element)(kElement_XRing + i);
							
						#if defined(DEBUG)
							debugging.hasNearestRingPos        = true;
							debugging.nearestRingPos           = origin_gizmo + direction_gizmo * sphere_t;
							debugging.nearestRingPos_projected = intersectionPos_projected;
						#endif
						}
					}
				}
			}
		}
	#else
		// has a lot of issues at grazing angles
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
	#endif
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
		setColor(colorYellow);
	/*else if (intersectionResult.element == kElement_XRing + axis)
		setColor(colorYellow);*/
	else
		setColor(colors[axis]);
	setAlpha(100);
}
