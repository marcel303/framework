#pragma once

#include "ui-capture.h"

#include "Mat4x4.h"
#include "Vec3.h"

#include <functional>

struct TransformGizmo
{
	enum State
	{
		kState_Hidden,
		kState_Visible,
		kState_DragArrow,
		kState_DragPad,
		kState_DragRing,
		kState_DragScale
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
		kElement_ZRing,
		kElement_XScale,
		kElement_YScale,
		kElement_ZScale
	};
	
	enum DrawPass
	{
		kDrawPass_DepthObscured,
		kDrawPass_Opaque,
		kDrawPass_Translucent
	};
	
	struct IntersectionResult
	{
		Element element = kElement_None;
		float t = 0.f;
	};
	
	struct ElementVisibility
	{
		float arrow[3] = { };
		float pad[3] = { };
		float ring[3] = { };
		float scale[3] = { };
	};
	
	State state = kState_Hidden;
	
	UiCaptureElem uiCaptureElem;
	
	Mat4x4 gizmoToWorld = Mat4x4(true);
	
	Vec3 rayOriginInGizmoSpace; // used for determining on which side to draw the pads
	
	Vec3 padPosition[3];
	
	Vec3 scalePosition[3];
	
	IntersectionResult intersectionResult;
	
	ElementVisibility elementVisibility;
	
	bool isInteractive = true;
	
	bool enableTranslation = true;
	bool enableRotation = true;
	bool enableScaling = true;
	
	float arrow_radius = .04f;
	float arrow_length = 1.f;
	float arrow_top_radius = arrow_radius * 1.4f;
	float arrow_top_length = arrow_radius * 3.f;
	float arrow_collision_radius = .08f;
	float arrow_smoothingAmount = .3f;
	
	float pad_offset = .5f;
	float pad_size = .3f;
	float pad_thickness = .02f;
	float pad_smoothingAmount = .3f;
	
	float ring_radius = 1.8f;
	float ring_tubeRadius = .1f;
	float ring_smoothingAmount = .3f;
	
	float scale_offset = 1.25f;
	float scale_size = .05f;
	float scale_smoothingAmount = .3f;
	
	std::function<void()> editingWillBegin;
	std::function<void()> editingDidEnd;
	
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
	
	struct DragScale
	{
		int axis = 0;
		float initialDistance = 0.f;
	} dragScale;
	
	struct
	{
		Vec3 nearestRingPos;
		Vec3 nearestRingPos_projected;
		bool hasNearestRingPos = false;
	} mutable debugging;
	
	void show(const Mat4x4 & transform);
	void hide();
	
	bool isActive() const;
	
	/**
	 * Handles interaction between the user's mouse (where the cursor position is translated to a
	 * world space ray with an origin and direction) and the gizmo. The gizmo supports translation
	 * by clicking on arrows and a pad, and rotation by clicking and dragging rings.
	 * @return True if the user is interacting with the object.
	 */
	bool tick(
		Vec3Arg pointer_origin,
		Vec3Arg pointer_direction,
		const bool pointer_isActive,
		const bool pointer_becameActive,
		bool & inputIsCaptured,
		Vec3Arg viewOrigin_world,
		Vec3Arg viewDirection_world,
		const float dt);
	void draw(const DrawPass drawPass) const;

private:
	void updatePadPositions(Vec3Arg rayOriginInGizmoSpace);
	void updateScalePositions(Vec3Arg rayOriginInGizmoSpace);
	void updateElementVisibility(Vec3Arg viewOrigin_world, Vec3Arg viewDirection_world);
	
	IntersectionResult intersect(Vec3Arg origin_world, Vec3Arg direction_world) const;
	static float calculateRingAngle(Vec3Arg position_world, const int axis, const Mat4x4 gizmoToWorld);
	
	void setColorForArrow(const int axis) const;
	void setColorForRing(const int axis, const bool isLine) const;
};
