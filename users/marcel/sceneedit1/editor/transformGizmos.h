#pragma once

#include "Mat4x4.h"
#include "Vec3.h"

struct TransformGizmo
{
	enum State
	{
		kState_Hidden,
		kState_Visible,
		kState_DragArrow,
		kState_DragPad,
		kState_DragRing
	};
	
	enum Element
	{
		kElement_None,
		kElement_XAxis,
		kElement_YAxis,
		kElement_ZAxis,
		kElement_XZPad,
		kElement_YXPad,
		kElement_ZYPad,
		kElement_XRing,
		kElement_YRing,
		kElement_ZRing
	};
	
	struct IntersectionResult
	{
		Element element = kElement_None;
		float t = 0.f;
	};
	
	State state = kState_Hidden;
	
	Mat4x4 gizmoToWorld = Mat4x4(true);
	
	Vec3 rayOriginInGizmoSpace; // used for determining on which side to draw the pads
	
	IntersectionResult intersectionResult;
	
	bool isInteractive = true;
	
	bool enableTranslation = true;
	bool enableRotation = true;
	
	float arrow_radius = .08f;
	float arrow_length = 1.f;
	
	float pad_offset = .5f;
	float pad_size = .3f;
	float pad_thickness = .02f;
	
	float ring_radius = 1.8f;
	float ring_tubeRadius = .1f;
	
	struct DragArrow
	{
		int axis = 0;
		int projection_axis = 0;
		Vec3 initialPosition;
	} dragArrow;
	
	struct DragPad
	{
		Vec3 initialPosition;
		int projection_axis = 0;
	} dragPad;
	
	struct DragRing
	{
		int axis = 0;
		float initialAngle = 0.f;
	} dragRing;
	
	void show(const Mat4x4 & transform);
	void hide();
	
	/**
	 * Handles interaction between the user's mouse (where the cursor position is translated to a
	 * world space ray with an origin and direction) and the gizmo. The gizmo supports translation
	 * by clicking on arrows and a pad, and rotation by clicking and dragging rings.
	 * @return True if the user is interacting with the object.
	 */
	bool tick(Vec3Arg ray_origin, Vec3Arg ray_direction, bool & inputIsCaptured);
	void draw() const;

private:
	IntersectionResult intersect(Vec3Arg origin_world, Vec3Arg direction_world) const;
	float calculateRingAngle(Vec3Arg position_world, const int axis) const;
	
	void setColorForArrow(const int axis) const;
	void setColorForRing(const int axis) const;
};