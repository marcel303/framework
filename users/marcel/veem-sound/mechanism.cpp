#include "framework.h"
#include "mechanism.h"
#include <cmath>

void drawThickCircle(const float radius1, const float radius2, const int numSegments)
{
	const float angleStep = 2.f * M_PI / numSegments;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle1 = (i + 0) * angleStep;
			const float angle2 = (i + 1) * angleStep;
			
			const float dx1 = std::cos(angle1);
			const float dy1 = std::sin(angle1);
			const float dx2 = std::cos(angle2);
			const float dy2 = std::sin(angle2);
			
			const float x1 = dx1 * radius1;
			const float y1 = dy1 * radius1;
			const float x2 = dx2 * radius1;
			const float y2 = dy2 * radius1;
			const float x3 = dx2 * radius2;
			const float y3 = dy2 * radius2;
			const float x4 = dx1 * radius2;
			const float y4 = dy1 * radius2;
			
			gxVertex2f(x1, y1);
			gxVertex2f(x2, y2);
			gxVertex2f(x3, y3);
			gxVertex2f(x4, y4);
		}
	}
	gxEnd();
}

static void drawTubeCircle_failed1(const float radius1, const float radius2, const int numSegments1, const int numSegments2)
{
	const float angleStep1 = 2.f * M_PI / numSegments1;
	const float angleStep2 = 2.f * M_PI / numSegments2;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments1; ++i)
		{
			const float angle1_1 = (i + 0) * angleStep1;
			const float angle1_2 = (i + 1) * angleStep1;
			
			const float dx1 = std::cos(angle1_1);
			const float dy1 = std::sin(angle1_1);
			const float dx2 = std::cos(angle1_2);
			const float dy2 = std::sin(angle1_2);
			
			for (int j = 0; j < 10; ++j)
			{
				const float angle2_1 = (j + 0) * angleStep2;
				const float angle2_2 = (j + 1) * angleStep2;
				
				const float da1 = std::cos(angle2_1);
				const float dz1 = std::sin(angle2_1);
				const float da2 = std::cos(angle2_2);
				const float dz2 = std::sin(angle2_2);
			
				const float x1 = dx1 * radius1 + da1 * dx1 * radius2;
				const float y1 = dy1 * radius1 + da1 * dy1 * radius2;
				const float z1 = dz1 * radius2;
				
				const float x2 = dx1 * radius1 + da2 * dx1 * radius2;
				const float y2 = dy1 * radius1 + da2 * dy1 * radius2;
				const float z2 = dz2 * radius2;
				
				const float x3 = dx2 * radius1 + da2 * dx1 * radius2;
				const float y3 = dy2 * radius1 + da2 * dy1 * radius2;
				const float z3 = dz2 * radius2;
				
				const float x4 = dx2 * radius1 + da1 * dx1 * radius2;
				const float y4 = dy2 * radius1 + da1 * dy1 * radius2;
				const float z4 = dz1 * radius2;
				
				setLumif((x1 + y1 + z1 + 3.f) / 6.f);
				gxVertex3f(x1, y1, z1);
				gxVertex3f(x2, y2, z2);
				gxVertex3f(x3, y3, z3);
				gxVertex3f(x4, y4, z4);
			}
		}
	}
	gxEnd();
}

void drawTubeCircle(const double radius1, const double radius2, const int numSegments1, const int numSegments2)
{
	const double angleStep1 = 2.0 * M_PI / numSegments1;
	const double angleStep2 = 2.0 * M_PI / numSegments2;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments1; ++i)
		{
			const double outerAngle1 = (i + 0) * angleStep1;
			const double outerAngle2 = (i + 1) * angleStep1;
			
			const double dx1 = std::cos(outerAngle1);
			const double dy1 = std::sin(outerAngle1);
			const double dx2 = std::cos(outerAngle2);
			const double dy2 = std::sin(outerAngle2);
			
			for (int j = 0; j < 10; ++j)
			{
				const double innerAngle1 = (j + 0) * angleStep2;
				const double innerAngle2 = (j + 1) * angleStep2;
				
				const double da1 = std::cos(innerAngle1);
				const double dz1 = std::sin(innerAngle1);
				const double da2 = std::cos(innerAngle2);
				const double dz2 = std::sin(innerAngle2);
			
				const double x1 = dx1 * radius1 + da1 * dx1 * radius2;
				const double y1 = dy1 * radius1 + da1 * dy1 * radius2;
				const double z1 = dz1 * radius2;
				
				const double x2 = dx2 * radius1 + da1 * dx2 * radius2;
				const double y2 = dy2 * radius1 + da1 * dy2 * radius2;
				const double z2 = dz1 * radius2;
				
				const double x3 = dx2 * radius1 + da2 * dx2 * radius2;
				const double y3 = dy2 * radius1 + da2 * dy2 * radius2;
				const double z3 = dz2 * radius2;
				
				const double x4 = dx1 * radius1 + da2 * dx1 * radius2;
				const double y4 = dy1 * radius1 + da2 * dy1 * radius2;
				const double z4 = dz2 * radius2;
				
				//setLumif((x1 + y1 + z1 + 3.f) / 6.f);
				gxVertex3f(x1, y1, z1);
				gxVertex3f(x2, y2, z2);
				gxVertex3f(x3, y3, z3);
				gxVertex3f(x4, y4, z4);
			}
		}
	}
	gxEnd();
}

//

void Mechanism::tick(const float dt)
{
	xAngle += xAngleSpeed * dt;
	yAngle += yAngleSpeed * dt;
	zAngle += zAngleSpeed * dt;
	
	xAngle = std::fmod(xAngle, 360.0);
	yAngle = std::fmod(yAngle, 360.0);
	zAngle = std::fmod(zAngle, 360.0);
}

void Mechanism::evaluateMatrix(const int ringIndex, Mat4x4 & matrix, float & radius) const
{
	matrix.MakeIdentity();
	radius = kRadius1;
	
	if (ringIndex >= 1)
	{
		matrix = matrix.RotateX(-xAngle * M_PI / 180.0);
		radius = kRadius2;
	}
	
	if (ringIndex >= 2)
	{
		matrix = matrix.RotateY(-yAngle * M_PI / 180.0);
		radius = kRadius3;
	}
	
	if (ringIndex >= 3)
	{
		matrix = matrix.RotateX(-zAngle * M_PI / 180.0);
		radius = kRadius4;
	}
}

Vec3 Mechanism::evaluatePoint(const int ringIndex, const float angle) const
{
	Mat4x4 matrix;
	float radius;
	
	evaluateMatrix(ringIndex, matrix, radius);
	
	const Vec3 p(std::cos(angle), std::sin(angle), 0.f);
	
	return matrix * (p * radius);
}

void Mechanism::drawGizmo(const float radius, const float thickness) const
{
	drawCircle(0, 0, radius, 100);
	//drawThickCircle(radius - thickness, radius + thickness, 100);
	//drawTubeCircle_failed1(radius - thickness, radius + thickness, 100, 10);
	//drawTubeCircle_failed1(radius, thickness, 10, 4);
	drawTubeCircle(radius, thickness, 100, 5);
}

void Mechanism::draw_solid() const
{
	gxPushMatrix();
	{
		setLumi(200);
		drawGizmo(kRadius1, kThickness);
		drawLine(-kRadius1, 0, + (kRadius1 - kRadius2) - kRadius1, 0);
		drawLine(+kRadius1, 0, - (kRadius1 - kRadius2) + kRadius1, 0);
		gxRotatef(xAngle, 1, 0, 0);
		
		setLumi(180);
		drawGizmo(kRadius2, kThickness);
		drawLine(0, -kRadius2, 0, + (kRadius2 - kRadius3) - kRadius2);
		drawLine(0, +kRadius2, 0, - (kRadius2 - kRadius3) + kRadius2);
		gxRotatef(yAngle, 0, 1, 0);
		
		setLumi(160);
		drawGizmo(kRadius3, kThickness);
		drawLine(-kRadius3, 0, + (kRadius3 - kRadius4) - kRadius3, 0);
		drawLine(+kRadius3, 0, - (kRadius3 - kRadius4) + kRadius3, 0);
		gxRotatef(zAngle, 1, 0, 0);
		
		setLumi(140);
		drawGizmo(kRadius4, kThickness);
	}
	gxPopMatrix();
}
