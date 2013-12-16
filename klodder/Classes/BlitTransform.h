#pragma once

#include "libgg_forward.h"

class BlitTransform
{
public:
	BlitTransform();
	
	void ToMatrix(Mat3x2& out_Matrix) const;
	
	float anchorX;
	float anchorY;
	float angle;
	float scale;
	float x;
	float y;
};
