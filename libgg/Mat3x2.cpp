#include "Calc.h"
#include "Mat3x2.h"

Mat3x2::Mat3x2()
{
	MakeIdentity();
}

void Mat3x2::MakeIdentity()
{
	v[0][0] = 1.0f;
	v[1][0] = 0.0f;
	v[2][0] = 0.0f;
	
	v[0][1] = 0.0f;
	v[1][1] = 1.0f;
	v[2][1] = 0.0f;
	
	v[0][2] = 0.0f;
	v[1][2] = 0.0f;
	v[2][2] = 1.0f;
}

void Mat3x2::MakeScaling(float sx, float sy)
{
	v[0][0] = sx;
	v[1][0] = 0.0f;
	v[2][0] = 0.0f;
	
	v[0][1] = 0.0f;
	v[1][1] = sy;
	v[2][1] = 0.0f;
	
	v[0][2] = 0.0f;
	v[1][2] = 0.0f;
	v[2][2] = 1.0f;
}

void Mat3x2::MakeRotation(float angle)
{
	MakeIdentity();

//	angle = -angle;
	
	v[0][0] = cosf(angle);
	v[1][0] = sinf(angle);
	v[2][0] = 0.0f;
	
	v[0][1] = cosf(angle + Calc::mPI2);
	v[1][1] = sinf(angle + Calc::mPI2);
	v[2][1] = 0.0f;
}

void Mat3x2::MakeTranslation(const Vec2F& _v)
{
	MakeIdentity();
	
	v[2][0] = _v[0];
	v[2][1] = _v[1];
}

void Mat3x2::MakeTransform(const Vec2F& _v, float angle)
{
	Mat3x2 r;
	Mat3x2 t;
	
	r.MakeRotation(angle);
	t.MakeTranslation(_v);
	
	*this = t.MultiplyWith(r);
//	*this = r.MultiplyWith(t);
}

void Mat3x2::SetTrans(const Vec2F& _v)
{
	v[2][0] = _v[0];
	v[2][1] = _v[1];
}

Mat3x2 Mat3x2::MultiplyWith(const Mat3x2& mat) const
{
	Mat3x2 r;

	for (int x = 0; x < 3; ++x)
	{
		for (int y = 0; y < 3; ++y)
		{
			float _v = 0.0f;

			for (int i = 0; i < 3; ++i)
				_v += v[i][y] * mat.v[x][i];

			r.v[x][y] = _v;
		}
	}

	return r;
}

Mat3x2 Mat3x2::operator*(const Mat3x2& mat) const
{
	return MultiplyWith(mat);
}

Vec2F Mat3x2::operator*(const Vec2F& _v) const
{
	Vec2F r;
	
	for (int i = 0; i < 2; ++i)
	{
		r[i] =
			v[0][i] * _v[0] +
			v[1][i] * _v[1] +
			v[2][i];
	}

	return r;
}
