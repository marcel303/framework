#pragma once

#include "Mat4x4.h"
#include "Vec3.h"

void drawTubeCircle(const double radius1, const double radius2, const int numSegments1, const int numSegments2);
void drawThickCircle(const float radius1, const float radius2, const int numSegments);

struct Mechanism
{
	const float increment = -.06f;
	
	const float kRadius1 = 1.00f;
	const float kRadius2 = kRadius1 + increment;
	const float kRadius3 = kRadius2 + increment;
	const float kRadius4 = kRadius3 + increment;

	const float kThickness = .02f;
	
	//
	
	double xAngle = 0.0;
	double yAngle = 0.0;
	double zAngle = 0.0;
	
	double xAngleSpeed = 0.0;
	double yAngleSpeed = 0.0;
	double zAngleSpeed = 0.0;
	
	void tick(const float dt);
	
	void evaluateMatrix(const int ringIndex, Mat4x4 & matrix, float & radius) const;
	Vec3 evaluatePoint(const int ringIndex, const float angle) const;
	
	void drawGizmo(const float radius, const float thickness);

	void draw_solid();
};