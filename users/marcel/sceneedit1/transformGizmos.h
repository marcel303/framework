#pragma once

struct TranslationGizmo
{
	enum State
	{
		kState_Hidden,
		kState_Visible,
		kState_DragAxis
	};
	
	enum Element
	{
		kElement_None,
		kElement_XAxis,
		kElement_YAxis,
		kElement_ZAxis
	};
	
	struct IntersectionResult
	{
		Element element = kElement_None;
	};
	
	State state = kState_Hidden;
	
	Mat4x4 gizmoToWorld = Mat4x4(true);
	
	IntersectionResult intersectionResult;
	
	float radius = .08f;
	float length = 1.f;
	
	struct DragAxis
	{
		int axis = -1;
	} dragAxis;
	
	void show(const Mat4x4 & transform);
	void hide();
	
	void tick(const Mat4x4 & projectionMatrix, const Mat4x4 & cameraToWorld, Vec3Arg ray_origin, Vec3Arg ray_direction, bool & inputIsCaptured);
	
	void draw() const;

private:
	IntersectionResult intersect(Vec3Arg origin_world, Vec3Arg direction_world) const;
	
	void setColorForAxis(const int axis) const;
};
