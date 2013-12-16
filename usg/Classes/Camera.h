#pragma once

#include "Types.h"

class Camera
{
public:
	Camera();
	
	void Setup(int viewSx, int viewSy, Vec2F min, Vec2F max);
	
	void Move(const Vec2F& delta);
	void Focus(const Vec2F& location);
	void AdjustSize(const Vec2F& newSize);
	void Shrink(float amount);
	void Zoom(float zoom);
	
	void Clip();
	void ApplyGL(bool tilt3D);
	
	Vec2F ViewToWorld(const Vec2F& point) const;
	Vec2F WorldToView(const Vec2F& point) const;
	
	float Zoom_get() const;
	Vec2F Position_get() const;
	
	RectF m_Area;
	Vec2F m_ViewSize;
	Vec2F m_WorldSize;
	Vec2F m_WorldPos;
};
