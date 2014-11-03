#include "ProcTexcoordMatrix.h"

Vec3 ProcTexcoordMatrix::Generate(Vec3Arg position, Vec3Arg normal)
{
	return m_matrix * position;
}

void ProcTexcoordMatrix::SetMatrix(const Mat4x4& matrix)
{
	m_matrix = matrix;
}