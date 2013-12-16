#include <math.h>
#include "Matrix.h"

#ifdef DEBUG_MATRIX
	#define MATRIX_ASSERT(x) assert(x) ///< Macro used to assert expressions when DEBUG_MATRIX is set.
#else
#define MATRIX_ASSERT(x) { }
#endif

size_t Matrix::GetSize() const
{
	return m_size;
}

void Matrix::SetSize(size_t size)
{
	m_size = size;
}

Matrix Matrix::CalculateSubMatrix(size_t x, size_t y) const
{
	Matrix matrix;
	
	if (m_size == 1)
	{
		// Cannot create sub matrix from 1x1 sized matrix. Return a matrix with undefined values.
		return matrix;
	}
	
	matrix.SetSize(m_size - 1);
	
	for (size_t x1 = 0, x2 = 0; x1 < m_size; ++x1)
	{
		// Skip column x.
		if (x1 != x)
		{
			for (size_t y1 = 0, y2 = 0; y1 < m_size; ++y1)
			{
				// Skip row y.
				if (y1 != y)
				{
					matrix(x2, y2) = m_values[Index(x1, y1)];
					++y2;
				}
			}
			++x2;
		}
	}
	
	return matrix;
}

Matrix Matrix::CalculateAdjugate() const
{
	Matrix matrix;
	
	matrix.SetSize(m_size);
	
	for (size_t x = 0; x < m_size; ++x)
		for (size_t y = 0; y < m_size; ++y)
			matrix(x, y) = CalculateCofactor(y, x);
			
	return matrix;
}

Matrix Matrix::CalculateTranspose() const
{
	Matrix matrix;
	
	matrix.SetSize(m_size);
	
	// Swap x/y locations of values.

	for (size_t x = 0; x < m_size; ++x)
		for (size_t y = 0; y < m_size; ++y)
			matrix(y, x) = m_values[Index(x, y)];
	
	return matrix;
}

Matrix Matrix::CalculateInverse() const
{
	// Calculate adjugate matrix.

	Matrix matrix = CalculateAdjugate();

	// Calculate determinant by developing row 1.
	// Determinant * Sign of row 1 is stored in column 1 of the adjugate transpose.
	
	float determinant = 0.0f;
	
	for (size_t i = 0; i < m_size; ++i)
		determinant += matrix(0, i) * m_values[Index(i, 0)];
	
	if (determinant == 0.0f)
	{
		MATRIX_ASSERT(0);
		return matrix; // The matrix has no inverse.
	}
	
	const float determinantInverse = 1.0f / determinant;
	const size_t sizeSquared = m_size * m_size;
	
	for (size_t i = 0; i < sizeSquared; ++i)
		matrix.m_values[i] *= determinantInverse;
	
	return matrix;
}

float Matrix::CalculateCofactor(size_t x, size_t y) const
{
	return CalculateSubMatrix(x, y).CalculateDeterminant() * GetSign(x, y);	
}

float Matrix::CalculateDeterminant() const
{
	#if 1
	if (m_size == 4)
	{
		const float d1 =
  			m_values[Index(1, 1)] * m_values[Index(2, 2)] * m_values[Index(3, 3)] +
  			m_values[Index(2, 1)] * m_values[Index(3, 2)] * m_values[Index(1, 3)] +
  			m_values[Index(3, 1)] * m_values[Index(1, 2)] * m_values[Index(2, 3)] -
  			m_values[Index(3, 1)] * m_values[Index(2, 2)] * m_values[Index(1, 3)] -
  			m_values[Index(3, 2)] * m_values[Index(2, 3)] * m_values[Index(1, 1)] -
  			m_values[Index(3, 3)] * m_values[Index(2, 1)] * m_values[Index(1, 2)];

		const float d2 =
  			m_values[Index(0, 1)] * m_values[Index(2, 2)] * m_values[Index(3, 3)] +
  			m_values[Index(2, 1)] * m_values[Index(3, 2)] * m_values[Index(0, 3)] +
  			m_values[Index(3, 1)] * m_values[Index(0, 2)] * m_values[Index(2, 3)] -
  			m_values[Index(3, 1)] * m_values[Index(2, 2)] * m_values[Index(0, 3)] -
  			m_values[Index(3, 2)] * m_values[Index(2, 3)] * m_values[Index(0, 1)] -
  			m_values[Index(3, 3)] * m_values[Index(2, 1)] * m_values[Index(0, 2)];

		const float d3 =
  			m_values[Index(0, 1)] * m_values[Index(1, 2)] * m_values[Index(3, 3)] +
  			m_values[Index(1, 1)] * m_values[Index(3, 2)] * m_values[Index(0, 3)] +
  			m_values[Index(3, 1)] * m_values[Index(0, 2)] * m_values[Index(1, 3)] -
  			m_values[Index(3, 1)] * m_values[Index(1, 2)] * m_values[Index(0, 3)] -
  			m_values[Index(3, 2)] * m_values[Index(1, 3)] * m_values[Index(0, 1)] -
  			m_values[Index(3, 3)] * m_values[Index(1, 1)] * m_values[Index(0, 2)];

		const float d4 =
  			m_values[Index(0, 1)] * m_values[Index(1, 2)] * m_values[Index(2, 3)] +
  			m_values[Index(1, 1)] * m_values[Index(2, 2)] * m_values[Index(0, 3)] +
  			m_values[Index(2, 1)] * m_values[Index(0, 2)] * m_values[Index(1, 3)] -
  			m_values[Index(2, 1)] * m_values[Index(1, 2)] * m_values[Index(0, 3)] -
  			m_values[Index(2, 2)] * m_values[Index(1, 3)] * m_values[Index(0, 1)] -
  			m_values[Index(2, 3)] * m_values[Index(1, 1)] * m_values[Index(0, 2)];

    	const float a1 = m_values[Index(0, 0)];
		const float a2 = m_values[Index(1, 0)];
		const float a3 = m_values[Index(2, 0)];
		const float a4 = m_values[Index(3, 0)];

    	const float s1 = GetSign(0, 0);
		const float s2 = GetSign(1, 0);
		const float s3 = GetSign(2, 0);
		const float s4 = GetSign(3, 0);

		return
			d1 * a1 * s1 +
			d2 * a2 * s2 +
			d3 * a3 * s3 +
			d4 * a4 * s4;
	}
	else if (m_size == 3)
	#else
	if (m_size == 3)
	#endif
	{
		return
  			m_values[Index(0, 0)] * m_values[Index(1, 1)] * m_values[Index(2, 2)] +
  			m_values[Index(1, 0)] * m_values[Index(2, 1)] * m_values[Index(0, 2)] +
  			m_values[Index(2, 0)] * m_values[Index(0, 1)] * m_values[Index(1, 2)] -
  			m_values[Index(2, 0)] * m_values[Index(1, 1)] * m_values[Index(0, 2)] -
  			m_values[Index(2, 1)] * m_values[Index(1, 2)] * m_values[Index(0, 0)] -
  			m_values[Index(2, 2)] * m_values[Index(1, 0)] * m_values[Index(0, 1)];
	}
	else if (m_size == 2)
	{
		return
  			m_values[Index(0, 0)] * m_values[Index(1, 1)] -
  			m_values[Index(1, 0)] * m_values[Index(0, 1)];
	}
	else if (m_size == 1)
	{
		return m_values[Index(0, 0)];
	}
	else
	{
		float d = 0.0f;
		
		for (size_t x = 0; x < m_size; ++x) // Develop row 1.
		{
			const float dSub = CalculateSubMatrix(x, 0).CalculateDeterminant();
	    	const float a = m_values[Index(x, 0)];
	    	const float s = GetSign(x, 0);

			d += dSub * s * a * m_size;
		}
		
		return d;
	}
}

void Matrix::MakeIdentity()
{
	for (size_t x = 0; x < m_size; ++x)
		for (size_t y = 0; y < m_size; ++y)
			m_values[Index(x, y)] = x == y ? 1.0f : 0.0f;
}

void Matrix::MakeTranslation(const Vector& translation)
{
	MATRIX_ASSERT(m_size == 4);

	MakeIdentity();
	
	m_values[Index(3, 0)] = translation[0];
	m_values[Index(3, 1)] = translation[1];
	m_values[Index(3, 2)] = translation[2];
}

void Matrix::MakeRotationX(float angle)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);

	MakeIdentity();
	
	m_values[Index(1, 1)] = +cosf(angle);
	m_values[Index(2, 1)] = +sinf(angle);
	m_values[Index(1, 2)] = -sinf(angle);
	m_values[Index(2, 2)] = +cosf(angle);
}

void Matrix::MakeRotationY(float angle)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);

	MakeIdentity();
	
	m_values[Index(0, 0)] = +cosf(angle);
	m_values[Index(2, 0)] = -sinf(angle);
	m_values[Index(0, 2)] = +sinf(angle);
	m_values[Index(2, 2)] = +cosf(angle);
}

void Matrix::MakeRotationZ(float angle)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);
	MakeIdentity();
	
	m_values[Index(0, 0)] = +cosf(angle);
	m_values[Index(1, 0)] = +sinf(angle);
	m_values[Index(0, 1)] = -sinf(angle);
	m_values[Index(1, 1)] = +cosf(angle);
}

void Matrix::MakeRotationEuler(const Vector& rotation)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);

	// Make X, Y and Z rotation matrices.
	
	Matrix rotationX; rotationX.MakeRotationX(rotation[0]);
	Matrix rotationY; rotationY.MakeRotationY(rotation[1]);
	Matrix rotationZ; rotationZ.MakeRotationZ(rotation[2]);
	
	// Assign concatenation of X, Y and Z rotation to self.
	
	*this = rotationZ * rotationY * rotationX;
}

void Matrix::MakeRotationYawPitchRoll(float yaw, float pitch, float roll)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);

	// Make Y, X and Z rotation matrices.
	
	Matrix rotationY; rotationY.MakeRotationY(yaw);
	Matrix rotationX; rotationX.MakeRotationX(pitch);
	Matrix rotationZ; rotationZ.MakeRotationZ(roll);
	
	// Assign concatenation of Y, X and Z rotation to self.
	
	*this = rotationZ * rotationX * rotationY;
}

void Matrix::MakeScaling(const Vector& scale)
{
	MATRIX_ASSERT(m_size == 3 || m_size == 4);
	
	MakeIdentity();
	
	m_values[Index(0, 0)] = scale[0];
	m_values[Index(1, 1)] = scale[1];
	m_values[Index(2, 2)] = scale[2];
}

void Matrix::MakePerspectiveFovLH(float fovY, float aspect, float nearCP, float farCP)
{
	MATRIX_ASSERT(m_size == 4);

	float lyScale = 1.0f / tanf(fovY / 2.0f);
	float lxScale = aspect * lyScale;
	float l_33 = farCP / ( farCP - nearCP );
	float l_43 = -nearCP * farCP / ( farCP - nearCP );

	_11 = lxScale; _12 = 0.0f;    _13 = 0.0f; _14 = 0.0f;
	_21 = 0.0f;    _22 = lyScale; _23 = 0.0f; _24 = 0.0f;
	_31 = 0.0f;    _32 = 0.0f;    _33 = l_33; _34 = 1.0f;
	_41 = 0.0f;    _42 = 0.0f;    _43 = l_43; _44 = 0.0f;
}

void Matrix::MakePerspectiveFovRH(float fovY, float aspect, float nearCP, float farCP)
{
	MATRIX_ASSERT(m_size == 4);

	float lyScale = 1.0f / tanf(fovY / 2.0f);
	float lxScale = aspect * lyScale;
	float l_33 = farCP / ( farCP - nearCP );
	float l_43 = +nearCP * farCP / ( farCP - nearCP );

	_11 = lxScale; _12 = 0.0f;    _13 = 0.0f; _14 = 0.0f;
	_21 = 0.0f;    _22 = lyScale; _23 = 0.0f; _24 = 0.0f;
	_31 = 0.0f;    _32 = 0.0f;    _33 = l_33; _34 =-1.0f;
	_41 = 0.0f;    _42 = 0.0f;    _43 = l_43; _44 = 0.0f;
}

void Matrix::MakeOrthoOffCenterLH(float left, float right, float top, float bottom, float nearCP, float farCP)
{
	MATRIX_ASSERT(m_size == 4);

	float rl = 2.0f / (right - left);
	float tb = 2.0f / (top - bottom);
	float fn = 1.0f / (farCP - nearCP);
	
 	float tx = (right + left) / (left - right);
 	float ty = (top + bottom) / (bottom - top);
 	float tz = nearCP / (nearCP - farCP);
 	
	_11 = rl;      _12 = 0.0f;    _13 = 0.0f;       _14 = 0.0f;
	_21 = 0.0f;    _22 = tb;      _23 = 0.0f;       _24 = 0.0f;
	_31 = 0.0f;    _32 = 0.0f;    _33 = fn;         _34 = 0.0f;
	_41 = tx;      _42 = ty;       _43 = tz;         _44 = 1.0f;
}

void Matrix::MakeLookat(const Vector& position, const Vector& target, const Vector& up)
{
	MATRIX_ASSERT(m_size == 4);
	
	Matrix temp;
	
	// Calculate Z, X and Y axis of local coordinate system of view.
	
	Vector axisZ = target - position;

	if (axisZ.GetSquaredSize() == 0.0f)
		axisZ = Vector(up[1], up[2], up[0]);

	Vector axisX = up % axisZ;
	Vector axisY = axisZ % axisX;
	
	// Normalize.
	
	axisX.Normalize();
	axisY.Normalize();
	axisZ.Normalize();
	
	// Fill in matrix with local coordinate system.
	
	for (int x = 0; x < 3; ++x)
	{
		temp(x, 0) = axisX[x];
		temp(x, 1) = axisY[x];
		temp(x, 2) = axisZ[x];
	}
	
	// Fill in gaps.
	
	temp(3, 0) = 0.0f;
	temp(3, 1) = 0.0f;
	temp(3, 2) = 0.0f;
	
	temp(0, 3) = 0.0f;
	temp(1, 3) = 0.0f;
	temp(2, 3) = 0.0f;
	temp(3, 3) = 1.0f;

	// Make translation matrix and assign translation * rotation to self.
	
	MakeTranslation(-position);
	
	*this = temp * (*this);
}

float Matrix::GetSign(size_t x, size_t y) const
{ 
	return (x + y) & 1 ? -1.0f : +1.0f;	
}

#ifdef DEBUG_MATRIX
	#undef MATRIX_ASSERT
#endif
