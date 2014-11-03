#include "ProcTexcoordMatrix2D.h"

ProcTexcoordMatrix2D::ProcTexcoordMatrix2D() : ProcTexcoordMatrix()
{
	m_axis = ShapeBuilder::AXIS_X;

	Mat4x4 matrix;

	matrix.MakeIdentity();

	SetMatrix(matrix);
}

Vec3 ProcTexcoordMatrix2D::Generate(Vec3Arg position, Vec3Arg normal)
{
	Vec3 position2D;
	position2D[0] = position[(m_axis + 1) % 3];
	position2D[1] = position[(m_axis + 2) % 3];
	position2D[2] = position[(m_axis + 3) % 3];
	return ProcTexcoordMatrix::Generate(position2D, normal);
}

void ProcTexcoordMatrix2D::SetAxis(ShapeBuilder::AXIS axis)
{
	m_axis = axis;
}