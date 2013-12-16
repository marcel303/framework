#pragma once

#include "Types.h"

class Mat3x2
{
public:
	Mat3x2();
	void MakeIdentity();
	void MakeScaling(float sx, float sy);
	void MakeRotation(float angle);
	void MakeRotation_Fast(float angle);
	void MakeTranslation(const Vec2F& v);
	void MakeTransform(const Vec2F& v, float angle);

	void SetTrans(const Vec2F& v);
	
	Mat3x2 MultiplyWith(const Mat3x2& mat) const;
	
	Mat3x2 operator*(const Mat3x2& mat) const;
	Vec2F operator*(const Vec2F& v) const;

	float v[3][3];
};
