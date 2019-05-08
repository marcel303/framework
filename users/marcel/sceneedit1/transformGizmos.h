#pragma once

struct TranslationGizmo
{
	enum State
	{
		kState_Hidden,
		kState_Visible,
		kState_DragArrow,
		kState_DragRing
	};
	
	enum Element
	{
		kElement_None,
		kElement_XAxis,
		kElement_YAxis,
		kElement_ZAxis,
		kElement_XZAxis,
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
	
	IntersectionResult intersectionResult;
	
	float arrow_radius = .08f;
	float arrow_length = 1.f;
	
	float pad_offset = .5f;
	float pad_size = .3f;
	float pad_thickness = .04f;
	
	float ring_radius = 2.f;
	float ring_tubeRadius = .2f;
	
	struct DragAxis
	{
		bool active_axis[3] = { };
	} dragAxis;
	
	struct DragRing
	{
		int axis_index = 0;
	} dragRing;
	
	void show(const Mat4x4 & transform);
	void hide();
	
	void tick(const Mat4x4 & projectionMatrix, const Mat4x4 & cameraToWorld, Vec3Arg ray_origin, Vec3Arg ray_direction, bool & inputIsCaptured);
	
	void draw() const;

private:
	IntersectionResult intersect(Vec3Arg origin_world, Vec3Arg direction_world) const;
	
	void setColorForArrow(const int axis) const;
	void setColorForRing(const int axis) const;
};
