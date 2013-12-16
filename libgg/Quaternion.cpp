#include <math.h>
#include "Debugging.h"
#include "Quaternion.h"

Quaternion::Quaternion()
{
	m_w = 1.0f;
}

Quaternion::Quaternion(const Quaternion& quaternion)
{
	m_xyz = quaternion.m_xyz;
	m_w = quaternion.m_w;
}

Quaternion::~Quaternion()
{
}

float Quaternion::Size() const
{
	return sqrt(m_xyz * m_xyz + m_w * m_w);
}

void Quaternion::Normalize()
{
	// Calculate length of quaternion and divde each component by this length to create a unit quaternion.

	const float size = Size();
	
	assert(size != 0.0f);

	m_w /= size;
	m_xyz /= size;
}

Quaternion Quaternion::Conjugate() const
{
	Quaternion temp;
	
	// Mirror along the real axis.

	temp.m_xyz = -m_xyz;
	temp.m_w = m_w;
	
	return temp;
}

void Quaternion::FromAxisAngle(Vector axis, float angle)
{
	axis.Normalize();
	
	// Setup 'axis'.

	m_xyz[0] = axis[0] * sin(angle / 2.0f);
	m_xyz[1] = axis[1] * sin(angle / 2.0f);
	m_xyz[2] = axis[2] * sin(angle / 2.0f);
	
	// 'Setup 'angle'.

	m_w = cos(angle / 2.0f);
}

void Quaternion::FromMatrix(const Matrix& matrix)
{
	const float eps = 0.000001f;

	// We need four cases, 1 special (and preferred) and 3 backups, to extract
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
	else if (matrix(0, 0) > matrix(1, 1) && matrix(0, 0) > matrix(2, 2))
	{ 
		const float s = 2.0f * sqrt(1.0f + matrix(0, 0) - matrix(1, 1) - matrix(2, 2));

		m_xyz[0] = 0.25f * s;		
		m_xyz[1] = (matrix(1, 0) + matrix(0, 1)) / s;
		m_xyz[2] = (matrix(2, 0) + matrix(0, 2)) / s;
		m_w = (matrix(2, 1) - matrix(1, 2)) / s;
	}
	else if (matrix(1, 1) > matrix(2, 2))
	{
		const float s = 2.0f * sqrt(1.0f + matrix(1, 1) - matrix(0, 0) - matrix(2, 2)); 
		
		m_xyz[0] = (matrix(1, 0) + matrix(0, 1) ) / s;
		m_xyz[1] = 0.25f * s;
		m_xyz[2] = (matrix(2, 1) + matrix(1, 2) ) / s;
		m_w = (matrix(2, 0) - matrix(0, 2) ) / s;
	} 
	else
	{
		const float s = 2.0f * sqrt(1.0f + matrix(2, 2) - matrix(0, 0) - matrix(1, 1)); 
		
		m_xyz[0] = (matrix(2, 0) + matrix(0, 2) ) / s;
		m_xyz[1] = (matrix(2, 1) + matrix(1, 2) ) / s;
		m_xyz[2] = 0.25f * s;
		m_w = (matrix(1, 0) - matrix(0, 1) ) / s;
	} 
}

Matrix Quaternion::ToMatrix() const
{
	Matrix matrix;
	
	// Create identity, so we don't have to bother to fill in the rest of the values besides the 3x3 rotation part.

	matrix.MakeIdentity();
	
	// Calculate 3x3 rotation part.

	matrix(0, 0) = 1.0f - 2.0f * m_xyz[1] * m_xyz[1] - 2.0f * m_xyz[2] * m_xyz[2];
	matrix(1, 0) = 2.0f * m_xyz[0] * m_xyz[1] - 2.0f * m_w * m_xyz[2];
	matrix(2, 0) = 2.0f * m_xyz[0] * m_xyz[2] + 2.0f * m_w * m_xyz[1];
	
	matrix(0, 1) = 2.0f * m_xyz[0] * m_xyz[1] + 2.0f * m_w * m_xyz[2];
	matrix(1, 1) = 1.0f - 2.0f * m_xyz[0] * m_xyz[0] - 2.0f * m_xyz[2] * m_xyz[2];
	matrix(2, 1) = 2.0f * m_xyz[1] * m_xyz[2] - 2.0f * m_w * m_xyz[0];
	
	matrix(0, 2) = 2.0f * m_xyz[0] * m_xyz[2] - 2.0f * m_w * m_xyz[1];
	matrix(1, 2) = 2.0f * m_xyz[1] * m_xyz[2] + 2.0f * m_w * m_xyz[0];
	matrix(2, 2) = 1.0f - 2.0f * m_xyz[0] * m_xyz[0] - 2.0f * m_xyz[1] * m_xyz[1];
	
	return matrix;
}

void Quaternion::ToAxisAngle(Vector& out_axis, float& out_angle) const
{
	// Calculation are easier when dealing with a unit quaternion,
	// so create a temp quaternion and normalize it.

	Quaternion temp = (*this);
	
	temp.Normalize();

	// Calculate axis & angle.

	out_angle = acos(temp.m_w) * 2.0f;
	
	const float sinAngle = sin(out_angle / 2.0f);
	
	if (sinAngle != 0.0f)
	{
		// We have an axis! Calculate it.

		out_axis[0] = temp.m_xyz[0] / sinAngle;
		out_axis[1] = temp.m_xyz[1] / sinAngle;
		out_axis[2] = temp.m_xyz[2] / sinAngle;
	}
	else
	{
		// Select arbitrary axis.

		out_axis[0] = 1.0f;
		out_axis[1] = 0.0f;
		out_axis[2] = 0.0f;
	}
}

float Quaternion::Dot(const Quaternion& quaternion) const
{
	return m_w * quaternion.m_w + m_xyz * quaternion.m_xyz;
}

void Quaternion::MakeIdentity()
{
	m_xyz = Vector(0.0f, 0.0f, 0.0f);
	m_w = 1.0f;
}

Quaternion Quaternion::Inverse() const
{
	// Calculate the inverse, which is basically the opposite rotation combined and the inverse length.

	Quaternion temp = Conjugate();
	
	const float size = Size();
	
	if (size != 0.0f)
		temp /= size;
		
	return temp;
}

Quaternion Quaternion::Slerp(const Quaternion& quaternion, float t) const
{
	// Slerp.
	
	// Select path. This code to select shortest, longest, cw & ccw path are 'inspired' by Allegro's
	// quaternion math functions.
	
	float dot = Dot(quaternion);
	
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
	if (method == METHOD_CW && m_w > quaternion[3])
		invert = true;
	if (method == METHOD_CCW && m_w < quaternion[3])
		invert = true;
	
	Quaternion quaternion2;
	
	if (invert)
	{
		dot = -dot;
		quaternion2 = -quaternion;
	}
	else
	{
		quaternion2 = quaternion;
	}
	
	// Perform slerp.
	
	// Calculate quaternion weights (0..1).

	float s1;
	float s2;
	
	// Select interpolation method. Prefer linear interpolation when the dot product
	// nears 1.0 - eps. Eg, when the quaternions represent almost the same rotation.

	if (1.0f - fabs(dot) > 0.000001f)
	{
		float angle = acos(dot);
		float angleSin = sin(angle);
		s1 = sin((1.0f - t) * angle) / angleSin;
		s2 = sin((0.0f + t) * angle) / angleSin;
	}
	else
	{
		s1 = 1.0f - t;
		s2 = 0.0f + t;
	}

	Quaternion temp = (*this) * s1 + quaternion2 * s2;

	return temp;
}

Quaternion Quaternion::Nlerp(const Quaternion& quaternion, float t) const
{
	// Nlerp.
	
	Quaternion delta = quaternion - (*this);
	
	 // Lerp. This leaves the result non-normalized (always shorter: straight path vs arc).
	 // We need to normalize to compensate.
	 
	Quaternion temp = (*this) + delta * t;
	
	temp.Normalize();

	return temp;
}

Quaternion Quaternion::operator-() const
{
	Quaternion temp;
	
	temp[0] = -m_xyz[0];
	temp[1] = -m_xyz[1];
	temp[2] = -m_xyz[2];
	temp[3] = -m_w;
	
	return temp;
}

Quaternion& Quaternion::operator=(const Quaternion& quaternion)
{
	m_xyz[0] = quaternion.m_xyz[0];
	m_xyz[1] = quaternion.m_xyz[1];
	m_xyz[2] = quaternion.m_xyz[2];
	m_w = quaternion.m_w;
	
	return *this;
}

Quaternion Quaternion::operator*(const Quaternion& quaternion) const
{
	Quaternion temp;
	
	// Perform multiplication using extra-special quaternion maths!

	temp.m_xyz[0] = m_w * quaternion.m_xyz[0] + m_xyz[0] * quaternion.m_w + m_xyz[1] * quaternion.m_xyz[2] - m_xyz[2] * quaternion.m_xyz[1];
	temp.m_xyz[1] = m_w * quaternion.m_xyz[1] + m_xyz[1] * quaternion.m_w + m_xyz[2] * quaternion.m_xyz[0] - m_xyz[0] * quaternion.m_xyz[2];
	temp.m_xyz[2] = m_w * quaternion.m_xyz[2] + m_xyz[2] * quaternion.m_w + m_xyz[0] * quaternion.m_xyz[1] - m_xyz[1] * quaternion.m_xyz[0];
	temp.m_w = m_w * quaternion.m_w - m_xyz * quaternion.m_xyz;

	return temp;
}

Quaternion Quaternion::operator-(const Quaternion& quaternion) const
{
	Quaternion temp;
	
	temp[0] = (*this)[0] - quaternion[0];
	temp[1] = (*this)[1] - quaternion[1];
	temp[2] = (*this)[2] - quaternion[2];
	temp[3] = (*this)[3] - quaternion[3];
	
	return temp;
}

Quaternion Quaternion::operator+(const Quaternion& quaternion) const
{
	Quaternion temp;
	
	temp[0] = (*this)[0] + quaternion[0];
	temp[1] = (*this)[1] + quaternion[1];
	temp[2] = (*this)[2] + quaternion[2];
	temp[3] = (*this)[3] + quaternion[3];
	
	return temp;
}

Quaternion Quaternion::operator/(float v) const
{
	assert(v != 0.0f);

	Quaternion temp;
	
	temp[0] = (*this)[0] / v;
	temp[1] = (*this)[1] / v;
	temp[2] = (*this)[2] / v;
	temp[3] = (*this)[3] / v;
	
	return temp;
}

Quaternion Quaternion::operator*(float v) const
{
	Quaternion temp;
	
	temp[0] = (*this)[0] * v;
	temp[1] = (*this)[1] * v;
	temp[2] = (*this)[2] * v;
	temp[3] = (*this)[3] * v;
	
	return temp;
}

Quaternion& Quaternion::operator*=(const Quaternion& quaternion)
{
	Quaternion temp;
	
	temp = (*this) * quaternion;
	
	(*this) = temp;
	
	return (*this);
}

Quaternion& Quaternion::operator-=(const Quaternion& quaternion)
{
	(*this)[0] -= quaternion[0];
	(*this)[1] -= quaternion[1];
	(*this)[2] -= quaternion[2];
	(*this)[3] -= quaternion[3];
	
	return (*this);
}

Quaternion& Quaternion::operator+=(const Quaternion& quaternion)
{
	(*this)[0] += quaternion[0];
	(*this)[1] += quaternion[1];
	(*this)[2] += quaternion[2];
	(*this)[3] += quaternion[3];
	
	return (*this);
}

Quaternion& Quaternion::operator/=(float v)
{
	assert(v != 0.0f);

	(*this)[0] /= v;
	(*this)[1] /= v;
	(*this)[2] /= v;
	(*this)[3] /= v;
	
	return (*this);
}

Quaternion& Quaternion::operator*=(float v)
{
	(*this)[0] *= v;
	(*this)[1] *= v;
	(*this)[2] *= v;
	(*this)[3] *= v;
	
	return (*this);
}

Quaternion::operator Matrix() const
{
	return ToMatrix();
}

float& Quaternion::operator[](int index)
{
	if (index == 3)
		return m_w;
	else
		return m_xyz[index];
}

const float Quaternion::operator[](int index) const
{
	if (index == 3)
		return m_w;
	else
		return m_xyz[index];
}
