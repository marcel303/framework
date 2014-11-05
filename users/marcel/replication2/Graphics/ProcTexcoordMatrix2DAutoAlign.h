#ifndef __PROCTEXCOORDMATRIX2DAUTOALIGN_H__
#define __PROCTEXCOORDMATRIX2DAUTOALIGN_H__

#include "ProcTexcoordMatrix2D.h"

class ProcTexcoordMatrix2DAutoAlign : public ProcTexcoordMatrix2D
{
public:
	virtual Vec3 Generate(Vec3Arg position, Vec3Arg normal);
};

#endif
