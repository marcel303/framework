#pragma once

#include "Vec3.h"

struct ShapeDefinition
{
	static const int kMaxPlanes = 64;
	
	struct Plane
	{
		Vec3 normal;
		float offset;
	};
	
	Plane planes[kMaxPlanes];
	int numPlanes;
	
	void loadFromFile(const char * filename);

	void makeRandomShape(const int in_numPlanes);
	
	float intersectRay_directional(Vec3Arg rayDirection, int & planeIndex) const;
	float intersectRay(Vec3Arg rayOrigin, Vec3Arg rayDirection) const;
};
