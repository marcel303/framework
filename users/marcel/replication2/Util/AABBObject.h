#ifndef AABBOBJECT_H
#define AABBOBJECT_H
#pragma once

#include "AABB.h"

class AABBObject
{
public:
	virtual AABB CalcAABB() const = 0;
};

#endif
