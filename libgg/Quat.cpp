#include <cmath>
#include "Debugging.h"
#include "Quat.h"

Quat::Quat()
{
	m_w = 1.f;
}

Quat::Quat(const Quat & quat)
	: m_xyz(quat.m_xyz)
	, m_w(quat.m_w)
{
}

Quat::Quat(float x, float y, float z, float w)
	: m_xyz(x, y, z)
	, m_w(w)
{
}

float Quat::calcSize() const
{
	return std::sqrt(m_xyz * m_xyz + m_w * m_w);
}

void Quat::normalize()
{
	// calculate the length of the quaternion and divde each component by this length to create a unit quaternion

	const float size = calcSize();
	
	Assert(size != 0.0f);

	m_w /= size;
	m_xyz /= size;
}

Quat Quat::calcConjugate() const
{
	Quat result;
	
	// mirror along the real axis

	result.m_xyz = -m_xyz;
	result.m_w = m_w;
	
	return result;
}

void Quat::fromAxisAngle(Vec3 axis, float angle)
{
	axis.Normalize();
	
	// setup 'axis'

	const float sinAngle = std::sin(angle / 2.f);
	
	m_xyz[0] = axis[0] * sinAngle;
	m_xyz[1] = axis[1] * sinAngle;
	m_xyz[2] = axis[2] * sinAngle;
		
	// setup 'angle'

	m_w = std::cos(angle / 2.0f);
}

void Quat::fromMatrix(const Mat4x4 & matrix)
{
	const float eps = 0.001f;

	// we need four cases, 1 special (and preferred) and 3 backups, to extract
	// the axis angle pair. We need the backups in case the rotation is aligned
	// in special ways.

	if (1.0f + matrix(0, 0) + matrix(1, 1) + matrix(2, 2) > eps)
	{
		const float s = 0.5f / sqrt(1.0f + matrix(0, 0) + matrix(1, 1) + matrix(2, 2));
		
		m_xyz[0] = (matrix(1, 2) - matrix(2, 1)) * s; 
		m_xyz[1] = (matrix(2, 0) - matrix(0, 2)) * s;
		m_xyz[2] = (matrix(0, 1) - matrix(1, 0)) * s;

		m_w = 0.25f / s;
	}
	else
	{ 
		int max = 0;
		
		for (int i = 1; i < 3; ++i)
			if (matrix(i, i) > matrix(max, max))
				max = i;
		
		int a0 = max;
		int a1 = a0 == 2 ? 0 : a0 + 1;
		int a2 = a1 == 2 ? 0 : a1 + 1;
		
		const float s = sqrt(1.0f + matrix(a0, a0) - matrix(a1, a1) - matrix(a2, a2));
		
		const float sRcp = s != 0.f ? 1.f / s : 0.f;
		
		m_xyz[a0] = s / 2.f;		
		m_xyz[a1] = (matrix(a1, a0) + matrix(a0, a1)) * sRcp / 2.f;
		m_xyz[a2] = (matrix(a2, a0) + matrix(a0, a2)) * sRcp / 2.f;
		m_w       = (matrix(a2, a1) - matrix(a1, a2)) * sRcp / 2.f;
	}
	
	normalize();
}

Mat4x4 Quat::toMatrix() const
{
	Mat4x4 matrix;
	
	toMatrix3x3(matrix);
	
	matrix(0, 3) = 0.f;
	matrix(1, 3) = 0.f;
	matrix(2, 3) = 0.f;
	
	matrix(3, 0) = 0.f;
	matrix(3, 1) = 0.f;
	matrix(3, 2) = 0.f;
	matrix(3, 3) = 1.f;
	
	return matrix;
}

void Quat::toMatrix3x3(Mat4x4 & matrix) const
{
	// calculate 3x3 rotation part
	
	const float xx = m_xyz[0] * m_xyz[0];
	const float yy = m_xyz[1] * m_xyz[1];
	const float zz = m_xyz[2] * m_xyz[2];
	
	const float xy = m_xyz[0] * m_xyz[1];
	const float yz = m_xyz[1] * m_xyz[2];
	const float zx = m_xyz[0] * m_xyz[2];
	
	const float xw = m_xyz[0] * m_w;
	const float yw = m_xyz[1] * m_w;
	const float zw = m_xyz[2] * m_w;
	
	matrix(0, 0) = 1.0f + 2.f * (- yy - zz);
	matrix(1, 0) =        2.f * (+ xy - zw);
	matrix(2, 0) =        2.f * (+ zx + yw);
	
	matrix(0, 1) =       2.f * (+ xy + zw);
	matrix(1, 1) = 1.f + 2.f * (- xx - zz);
	matrix(2, 1) =       2.f * (+ yz - xw);
	
	matrix(0, 2) =       2.f * (+ zx - yw);
	matrix(1, 2) =       2.f * (+ yz + xw);
	matrix(2, 2) = 1.f + 2.f * (- xx - yy);
}

void Quat::toAxisAngle(Vec3 & out_axis, float & out_angle) const
{
	// calculations are easier when dealing with a unit length quaternion,
	// so create a temp quaternion and normalize it

	Quat temp = (*this);
	
	temp.normalize();

	// calculate axis & angle.

	out_angle = std::acos(temp.m_w) * 2.0f;
	
	const float sinAngle = std::sin(out_angle / 2.0f);
	
	if (sinAngle != 0.0f)
	{
		// we have an axis! calculate it

		out_axis[0] = temp.m_xyz[0] / sinAngle;
		out_axis[1] = temp.m_xyz[1] / sinAngle;
		out_axis[2] = temp.m_xyz[2] / sinAngle;
	}
	else
	{
		// whatever. select an arbitrary axis

		out_axis[0] = 1.0f;
		out_axis[1] = 0.0f;
		out_axis[2] = 0.0f;
	}
}

float Quat::calcDot(const Quat & quat) const
{
	return m_w * quat.m_w + m_xyz * quat.m_xyz;
}

void Quat::makeIdentity()
{
	m_xyz = Vec3(0.0f, 0.0f, 0.0f);
	m_w = 1.0f;
}

Quat Quat::calcInverse() const
{
	// calculate the inverse, which is basically the opposite rotation combined and the inverse length

	Quat result = calcConjugate();
	
	const float size = calcSize();
	
	if (size != 0.0f)
		result /= size;
		
	return result;
}

Quat Quat::calcExp() const
{
	const float eps = 0.00001f;
	
	Quat result;
	
	const float x = m_xyz[0];
	const float y = m_xyz[1];
	const float z = m_xyz[2];
	
	const float fAngle ( std::sqrt(x*x+y*y+z*z) );
	const float fSin = std::sin(fAngle);

	result.m_w = std::cos(fAngle);

	if ( std::abs(fSin) >= eps )
	{
		float fCoeff = fSin / fAngle;
		
		result.m_xyz[0] = fCoeff*x;
		result.m_xyz[1] = fCoeff*y;
		result.m_xyz[2] = fCoeff*z;
	}
	else
	{
		result.m_xyz[0] = x;
		result.m_xyz[1] = y;
		result.m_xyz[2] = z;
	}
	
	return result;
}

Quat Quat::calcLog() const
{
	const float eps = 0.00001f;
	
	Quat result;
	
	const float x = m_xyz[0];
	const float y = m_xyz[1];
	const float z = m_xyz[2];
	
	result.m_w = 0.0;

	if (std::abs(m_w) < 1.0)
	{
		const float fAngle ( std::acos(m_w) );
		const float fSin = std::sin(fAngle);
		
		if (std::abs(fSin) >= eps)
		{
			const float fCoeff = fAngle / fSin;
			
			result.m_xyz[0] = fCoeff*x;
			result.m_xyz[1] = fCoeff*y;
			result.m_xyz[2] = fCoeff*z;
			
			return result;
		}
	}

	result.m_xyz[0] = x;
	result.m_xyz[1] = y;
	result.m_xyz[2] = z;
	
	return result;
}

Quat Quat::slerp(const Quat & quat, float t) const
{
	// slerp.
	
	// select path. this code to select shortest, longest, cw & ccw path are 'inspired' by Allegro's
	// quaternion math functions
	
	float dot = calcDot(quat);
	
	enum METHOD
	{
		METHOD_SHORTEST,
		METHOD_LONGEST,
		METHOD_CW,
		METHOD_CCW
	};
	
	const METHOD method = METHOD_SHORTEST;
	//const METHOD method = METHOD_LONGEST;
	//const METHOD method = METHOD_CW;
	//const METHOD method = METHOD_CCW;
	
	bool invert = false;
	
	if (method == METHOD_SHORTEST && dot < 0.0f)
		invert = true;
	if (method == METHOD_LONGEST && dot > 0.0f)
		invert = true;
	if (method == METHOD_CW && m_w > quat[3])
		invert = true;
	if (method == METHOD_CCW && m_w < quat[3])
		invert = true;
	
	Quat quat2;
	
	if (invert)
	{
		dot = -dot;
		quat2 = -quat;
	}
	else
	{
		quat2 = quat;
	}
	
	// perform slerp
	
	// calculate the quaternion weights (0..1).

	float s1;
	float s2;
	
	// select interpolation method. prefer linear interpolation when the dot product
	// nears 1.0 - eps. eg, when the quaternions represent almost the same rotation

	if (1.0f - std::abs(dot) > 0.000001f)
	{
		float angle = std::acos(dot);
		float angleSin = std::sin(angle);
		s1 = std::sin((1.0f - t) * angle) / angleSin;
		s2 = std::sin((0.0f + t) * angle) / angleSin;
	}
	else
	{
		s1 = 1.0f - t;
		s2 = 0.0f + t;
	}

	Quat result = (*this) * s1 + quat2 * s2;

	return result;
}

Quat Quat::nlerp(const Quat & quat, float t) const
{
	// nlerp
	
	Quat delta = quat - (*this);
	
	 // lerp. this leaves the result non-normalized (always shorter: straight path vs arc)
	 // we need to normalize the result to compensate
	 
	Quat result = (*this) + delta * t;
	
	result.normalize();

	return result;
}

Quat Quat::operator-() const
{
	Quat result;
	
	result[0] = -m_xyz[0];
	result[1] = -m_xyz[1];
	result[2] = -m_xyz[2];
	result[3] = -m_w;
	
	return result;
}

Quat& Quat::operator=(const Quat & quat)
{
	m_xyz[0] = quat.m_xyz[0];
	m_xyz[1] = quat.m_xyz[1];
	m_xyz[2] = quat.m_xyz[2];
	m_w = quat.m_w;
	
	return *this;
}

Quat Quat::operator*(const Quat & quat) const
{
	Quat result;
	
	// perform multiplication using extra-special quaternion maths!
	
	result.m_xyz[0] = m_w * quat.m_xyz[0] + m_xyz[0] * quat.m_w + m_xyz[1] * quat.m_xyz[2] - m_xyz[2] * quat.m_xyz[1];
	result.m_xyz[1] = m_w * quat.m_xyz[1] + m_xyz[1] * quat.m_w + m_xyz[2] * quat.m_xyz[0] - m_xyz[0] * quat.m_xyz[2];
	result.m_xyz[2] = m_w * quat.m_xyz[2] + m_xyz[2] * quat.m_w + m_xyz[0] * quat.m_xyz[1] - m_xyz[1] * quat.m_xyz[0];
	result.m_w = m_w * quat.m_w - m_xyz * quat.m_xyz;
	
	return result;
}

Quat Quat::operator-(const Quat & quat) const
{
	Quat result;
	
	result[0] = (*this)[0] - quat[0];
	result[1] = (*this)[1] - quat[1];
	result[2] = (*this)[2] - quat[2];
	result[3] = (*this)[3] - quat[3];
	
	return result;
}

Quat Quat::operator+(const Quat & quat) const
{
	Quat result;
	
	result[0] = (*this)[0] + quat[0];
	result[1] = (*this)[1] + quat[1];
	result[2] = (*this)[2] + quat[2];
	result[3] = (*this)[3] + quat[3];
	
	return result;
}

Quat Quat::operator/(float v) const
{
	Assert(v != 0.0f);

	Quat result;
	
	result[0] = (*this)[0] / v;
	result[1] = (*this)[1] / v;
	result[2] = (*this)[2] / v;
	result[3] = (*this)[3] / v;
	
	return result;
}

Quat Quat::operator*(float v) const
{
	Quat result;
	
	result[0] = (*this)[0] * v;
	result[1] = (*this)[1] * v;
	result[2] = (*this)[2] * v;
	result[3] = (*this)[3] * v;
	
	return result;
}

Quat & Quat::operator*=(const Quat & quat)
{
	Quat result;
	
	result = (*this) * quat;
	
	(*this) = result;
	
	return (*this);
}

Quat & Quat::operator-=(const Quat & quat)
{
	(*this)[0] -= quat[0];
	(*this)[1] -= quat[1];
	(*this)[2] -= quat[2];
	(*this)[3] -= quat[3];
	
	return (*this);
}

Quat & Quat::operator+=(const Quat & quat)
{
	(*this)[0] += quat[0];
	(*this)[1] += quat[1];
	(*this)[2] += quat[2];
	(*this)[3] += quat[3];
	
	return (*this);
}

Quat & Quat::operator/=(float v)
{
	Assert(v != 0.0f);

	(*this)[0] /= v;
	(*this)[1] /= v;
	(*this)[2] /= v;
	(*this)[3] /= v;
	
	return (*this);
}

Quat & Quat::operator*=(float v)
{
	(*this)[0] *= v;
	(*this)[1] *= v;
	(*this)[2] *= v;
	(*this)[3] *= v;
	
	return (*this);
}

Quat::operator Mat4x4() const
{
	return toMatrix();
}

float & Quat::operator[](int index)
{
	if (index == 3)
		return m_w;
	else
		return m_xyz[index];
}

const float Quat::operator[](int index) const
{
	if (index == 3)
		return m_w;
	else
		return m_xyz[index];
}
