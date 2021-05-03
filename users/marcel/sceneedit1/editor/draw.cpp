#include "draw.h"
#include "framework.h"

void fillCylinder(
	const Vec3 & position,
	const int axis,
	const float radius1,
	const float radius2,
	const float length,
	const bool mirrored)
{
	gxPushMatrix();
	gxTranslatef(position[0], position[1], position[2]);
	
	const int axis1 = axis;
	const int axis2 = (axis + 1) % 3;
	const int axis3 = (axis + 2) % 3;
	
	const float sign = mirrored ? -1.f : +1.f;

	float coords[2][100][3];
	float normals[100][3];
	
	for (int i = 0; i < 100; ++i)
	{
		const float angle = 2.f * float(M_PI) * i / 100.f;
		
		const float c = cosf(angle) * sign;
		const float s = sinf(angle);
		
		coords[0][i][axis1] = 0.f;
		coords[0][i][axis2] = c * radius1;
		coords[0][i][axis3] = s * radius1;
		
		coords[1][i][axis1] = length * sign;
		coords[1][i][axis2] = c * radius2;
		coords[1][i][axis3] = s * radius2;
		
		normals[i][axis1] = 0.f;
		normals[i][axis2] = c;
		normals[i][axis3] = s;
	}
	
	// begin cap
	
	gxBegin(GX_TRIANGLE_FAN);
	{
		float normal[3];
		normal[axis1] = -sign;
		normal[axis2] = 0.f;
		normal[axis3] = 0.f;
		gxNormal3fv(normal);
		
		for (int i = 0; i < 100; ++i)
		{
			gxVertex3fv(coords[0][i]);
		}
	}
	gxEnd();
	
	// end cap
	
	gxBegin(GX_TRIANGLE_FAN);
	{
		float normal[3];
		normal[axis1] = +sign;
		normal[axis2] = 0.f;
		normal[axis3] = 0.f;
		gxNormal3fv(normal);
		
		for (int i = 0; i < 100; ++i)
		{
			gxVertex3fv(coords[1][i]);
		}
	}
	gxEnd();
	
	// middle section
	
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < 100; ++i)
		{
			const int index1 = i;
			const int index2 = i + 1 == 100 ? 0 : i + 1;
			
			gxNormal3fv(normals[index1]);
			gxVertex3fv(coords[0][index1]);
			gxVertex3fv(coords[1][index1]);
			
			gxNormal3fv(normals[index2]);
			gxVertex3fv(coords[1][index2]);
			gxVertex3fv(coords[0][index2]);
		}
	}
	gxEnd();
	
	gxPopMatrix();
}

void drawAxisArrow(
	const Vec3 & position,
	const int axis, const float radius, const float length,
	const float top_radius, const float top_length,
	const bool mirrored)
{
	// draw a cylinder with a cone on top
	
	fillCylinder(position, axis, radius, radius, length, mirrored);
	
	Vec3 offset;
	offset[axis] = length * (mirrored ? -1.f : +1.f);
	fillCylinder(position + offset, axis, top_radius, 0.f, top_length, mirrored);
}

void drawSelectionBox(
	const Mat4x4 & objectToWorld,
	const Vec3 & min,
	const Vec3 & max,
	const Color & color)
{
	setColor(color);
	gxBegin(GX_LINES);
	{
		const float length = .2f;
		
		const Vec3 direction_x = objectToWorld.Mul3(Vec3(length, 0, 0));
		const Vec3 direction_y = objectToWorld.Mul3(Vec3(0, length, 0));
		const Vec3 direction_z = objectToWorld.Mul3(Vec3(0, 0, length));
		
		const Vec3 points[2] = { min, max };
		
		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 2; ++y)
			{
				for (int z = 0; z < 2; ++z)
				{
					const Vec3 point1 = objectToWorld * Vec3(points[x][0], points[y][1], points[z][2]);
					const Vec3 point2_x = point1 + (x == 0 ? direction_x : -direction_x);
					const Vec3 point2_y = point1 + (y == 0 ? direction_y : -direction_y);
					const Vec3 point2_z = point1 + (z == 0 ? direction_z : -direction_z);
					
					gxVertex3fv(&point1[0]); gxVertex3fv(&point2_x[0]);
					gxVertex3fv(&point1[0]); gxVertex3fv(&point2_y[0]);
					gxVertex3fv(&point1[0]); gxVertex3fv(&point2_z[0]);
				}
			}
		}
	}
	gxEnd();
}
