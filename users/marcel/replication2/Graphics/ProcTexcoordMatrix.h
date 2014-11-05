#ifndef __PROCTEXCOORDMATRIX_H__
#define __PROCTEXCOORDMATRIX_H__

#include "Mat4x4.h"
#include "ProcTexcoord.h"

class ProcTexcoordMatrix : public ProcTexcoord
{
public:
	virtual Vec3 Generate(Vec3Arg position, Vec3Arg normal);
	void SetMatrix(const Mat4x4& matrix);

private:
	Mat4x4 m_matrix;
};

#endif
