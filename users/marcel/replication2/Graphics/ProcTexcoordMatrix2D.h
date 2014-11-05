#ifndef __PROCTEXCOORDMATRIX2D_H__
#define __PROCTEXCOORDMATRIX2D_H__

#include "ProcTexcoordMatrix.h"
#include "ShapeBuilder.h"

class ProcTexcoordMatrix2D : public ProcTexcoordMatrix
{
public:
	ProcTexcoordMatrix2D();

	virtual Vec3 Generate(Vec3Arg position, Vec3Arg normal);
	void SetAxis(ShapeBuilder::AXIS axis);

private:
	ShapeBuilder::AXIS m_axis;
};

#endif
