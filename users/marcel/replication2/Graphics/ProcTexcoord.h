#ifndef __PROCTEXCOORD_H__
#define __PROCTEXCOORD_H__

#include "Vec3.h"

class ProcTexcoord
{
public:
	virtual Vec3 Generate(Vec3Arg position, Vec3Arg normal) = 0;
};

#endif
