#include <math.h>
#include "Debugging.h"
#include "MathMatrix.h"

#if defined(MATRIX_FIXED_SIZE)
	#define MATRIX_ASSERT(x)
#else
	#define MATRIX_ASSERT(x) Assert(x)
#endif

Matrix::Matrix()
{

	s = 0;
	#if !defined(MATRIX_FIXED_SIZE)
	v = 0;
	#endif
	
	SetSize(4);

}

Matrix::Matrix(const Matrix& matrix)
{

	s = 0;
	#if !defined(MATRIX_FIXED_SIZE)
	v = 0;
	#endif

	*this = matrix;

}

Matrix::~Matrix()
{

	SetSize(0);

}

void Matrix::SetSize(int s)
{

	#if !defined(MATRIX_FIXED_SIZE)
	if (this->v)
	{
		delete[] this->v;
	}
	#endif
		
	this->s = 0;
	#if !defined(MATRIX_FIXED_SIZE)
	this->v = 0;
	#endif
	
	if (s <= 0)
	{
		return;
	}
		
	this->s = s;
	#if !defined(MATRIX_FIXED_SIZE)
	this->v = new float[s * s];
	#endif
	
}

int Matrix::GetSize() const
{

	return s;
	
}

void Matrix::Identity()
{

	for (int x = 0; x < s; ++x)
	{
		for (int y = 0; y < s; ++y)
		{
			v[Index(x, y)] = x == y ? 1.0f : 0.0f;
		}
	}

}

Matrix Matrix::SubMatrix(int x, int y) const
{

	Matrix matrix;
	
	if (s == 1)
	{
		return matrix;
	}
	
	matrix.SetSize(s - 1);
	
	for (int x1 = 0, x2 = 0; x1 < s; ++x1)
	{
		if (x1 != x)
		{
			for (int y1 = 0, y2 = 0; y1 < s; ++y1)
			{
				if (y1 != y)
				{
					matrix(x2, y2) = v[Index(x1, y1)];
					++y2;
				}
			}
			++x2;
		}
	}
	
	return matrix;

}

Matrix Matrix::Adjugate() const
{

	Matrix matrix;
	
	matrix.SetSize(s);
	
	for (int x = 0; x < s; ++x)
	{
		for (int y = 0; y < s; ++y)
		{
			matrix(x, y) = Cofactor(y, x);
		}
	}
			
	return matrix;
			
}

Matrix Matrix::Transpose() const
{

	Matrix matrix;
	
	matrix.SetSize(s);
	
	for (int x = 0; x < s; ++x)
	{
		for (int y = 0; y < s; ++y)
		{
			matrix(y, x) = v[Index(x, y)];
		}
	}
	
	return matrix;
	
}

Matrix Matrix::Inverse() const
{

	Matrix matrix = Adjugate();

	// Develop row 1.
	// Determinant * Sign of row 1 is stored in column 1 of the adjugate transpose.
	
	float d = 0.0;
	
	for (int i = 0; i < s; ++i)
	{
		d += matrix(0, i) * v[Index(i, 0)];
	}
	
	if (d == 0.0)
	{
		return matrix; // The matrix has no inverse.
	}
	
	const float d_1 = 1.0f / d;
	const int s_s = s * s;
	
	for (int i = 0; i < s_s; ++i)
	{
		matrix.v[i] *= d_1;
	}
	
	return matrix;
	
}

float Matrix::Cofactor(int x, int y) const
{

	return SubMatrix(x, y).Determinant() * Sign(x, y);	

}

float Matrix::Sign(int x, int y) const
{

	return (x + y) & 1 ? -1.0f : +1.0f;	

}
	
float Matrix::Determinant() const
{

	if (s == 4)
	{

		const float d1 =
  			v[Index(1, 1)] * v[Index(2, 2)] * v[Index(3, 3)] +
  			v[Index(2, 1)] * v[Index(3, 2)] * v[Index(1, 3)] +
  			v[Index(3, 1)] * v[Index(1, 2)] * v[Index(2, 3)] -
  			v[Index(3, 1)] * v[Index(2, 2)] * v[Index(1, 3)] -
  			v[Index(3, 2)] * v[Index(2, 3)] * v[Index(1, 1)] -
  			v[Index(3, 3)] * v[Index(2, 1)] * v[Index(1, 2)];

		const float d2 =
  			v[Index(0, 1)] * v[Index(2, 2)] * v[Index(3, 3)] +
  			v[Index(2, 1)] * v[Index(3, 2)] * v[Index(0, 3)] +
  			v[Index(3, 1)] * v[Index(0, 2)] * v[Index(2, 3)] -
  			v[Index(3, 1)] * v[Index(2, 2)] * v[Index(0, 3)] -
  			v[Index(3, 2)] * v[Index(2, 3)] * v[Index(0, 1)] -
  			v[Index(3, 3)] * v[Index(2, 1)] * v[Index(0, 2)];

		const float d3 =
  			v[Index(0, 1)] * v[Index(1, 2)] * v[Index(3, 3)] +
  			v[Index(1, 1)] * v[Index(3, 2)] * v[Index(0, 3)] +
  			v[Index(3, 1)] * v[Index(0, 2)] * v[Index(1, 3)] -
  			v[Index(3, 1)] * v[Index(1, 2)] * v[Index(0, 3)] -
  			v[Index(3, 2)] * v[Index(1, 3)] * v[Index(0, 1)] -
  			v[Index(3, 3)] * v[Index(1, 1)] * v[Index(0, 2)];

		const float d4 =
  			v[Index(0, 1)] * v[Index(1, 2)] * v[Index(2, 3)] +
  			v[Index(1, 1)] * v[Index(2, 2)] * v[Index(0, 3)] +
  			v[Index(2, 1)] * v[Index(0, 2)] * v[Index(1, 3)] -
  			v[Index(2, 1)] * v[Index(1, 2)] * v[Index(0, 3)] -
  			v[Index(2, 2)] * v[Index(1, 3)] * v[Index(0, 1)] -
  			v[Index(2, 3)] * v[Index(1, 1)] * v[Index(0, 2)];

    	const float a1 = v[Index(0, 0)];
		const float a2 = v[Index(1, 0)];
		const float a3 = v[Index(2, 0)];
		const float a4 = v[Index(3, 0)];

    	const float s1 = Sign(0, 0);
		const float s2 = Sign(1, 0);
		const float s3 = Sign(2, 0);
		const float s4 = Sign(3, 0);

		return
			d1 * a1 * s1 +
			d2 * a2 * s2 +
			d3 * a3 * s3 +
			d4 * a4 * s4;

	}
	else if (s == 3)
	{
		return
  			v[Index(0, 0)] * v[Index(1, 1)] * v[Index(2, 2)] +
  			v[Index(1, 0)] * v[Index(2, 1)] * v[Index(0, 2)] +
  			v[Index(2, 0)] * v[Index(0, 1)] * v[Index(1, 2)] -
  			v[Index(2, 0)] * v[Index(1, 1)] * v[Index(0, 2)] -
  			v[Index(2, 1)] * v[Index(1, 2)] * v[Index(0, 0)] -
  			v[Index(2, 2)] * v[Index(1, 0)] * v[Index(0, 1)];
	}
	else if (s == 2)
	{
		return
  			v[Index(0, 0)] * v[Index(1, 1)] -
  			v[Index(1, 0)] * v[Index(0, 1)];
	}
	else if (s == 1)
	{
		return v[Index(0, 0)];
	}
	else
	{

		// TODO: Test if this works correctly.

		float d = 0.0;
		
		for (int x = 0; x < s; ++x) // Develop row 1.
		{

			const float dSub = SubMatrix(x, 0).Determinant();
	    	const float a = v[Index(x, 0)];
	    	const float s = Sign(x, 0);

			d += dSub * a * s;

		}
		
		return d;

	}

}

void Matrix::operator=(const Matrix& matrix)
{

	SetSize(matrix.GetSize());
	
	const int s_s = s * s;
	
	for (int i = 0; i < s_s; ++i)
	{
		v[i] = matrix.v[i];
	}
	
}

Matrix Matrix::operator*(const Matrix& matrix) const
{

	if (matrix.GetSize() != s)
	{
		return matrix;
	}
	
	Matrix temp;

	temp.SetSize(matrix.GetSize());
 	
	for (int x = 0; x < s; ++x)
	{
		for (int y = 0; y < s; ++y)
		{
		
			float v = 0.0;
			
			for (int i = 0; i < s; ++i)
			{
				v += this->v[Index(i, y)] * matrix(x, i);
			}
				
			temp(x, y) = v;
			
		}
	}
	
	return temp;
	
}

Vector Matrix::operator*(const Vector& vector) const
{

	Vector temp;
	
	Apply(&vector, &temp);
	
	return temp;
	
}

Matrix Matrix::operator*(float v) const
{

	Matrix temp;
	
	for (int i = 0; i < 16; ++i)
		temp.v[i] = this->v[i] * v;
		
	return temp;
	
}

Matrix Matrix::operator/(float v) const
{

	Matrix temp;
	
	for (int i = 0; i < 16; ++i)
		temp.v[i] = this->v[i] / v;
		
	return temp;
	
}

Matrix Matrix::operator+(const Matrix& matrix) const
{

	Matrix temp;
	
	for (int i = 0; i < 16; ++i)
		temp.v[i] = v[i] + matrix.v[i];
		
	return temp;
	
}

Matrix Matrix::operator-(const Matrix& matrix) const
{

	Matrix temp;
	
	for (int i = 0; i < 16; ++i)
		temp.v[i] = v[i] - matrix.v[i];
		
	return temp;
	
}

void Matrix::Apply3x3_3(const float* v, float* vout) const
{

	for (int i = 0; i < 3; ++i)
	{
	
		vout[i] =
			v[0] * this->v[Index(0, i)] +
			v[1] * this->v[Index(1, i)] +
			v[2] * this->v[Index(2, i)];
     		
	}
	
}

void Matrix::Apply4x3_3(const float* v, float* vout) const
{

	for (int i = 0; i < 3; ++i)
	{
	
		vout[i] =
			v[0] * this->v[Index(0, i)] +
			v[1] * this->v[Index(1, i)] +
			v[2] * this->v[Index(2, i)] +
			       this->v[Index(3, i)];
     	
	}
	
}

void Matrix::Apply(const float* v, float* vout) const
{

	for (int i = 0; i < 4; ++i)
	{
	
		vout[i] =
			v[0] * this->v[Index(0, i)] +
			v[1] * this->v[Index(1, i)] +
			v[2] * this->v[Index(2, i)] +
			v[3] * this->v[Index(3, i)];
     	
	}
	
}

void Matrix::Apply3x3(const Vector* v, Vector* vout) const
{

	Apply3x3_3(v->v, vout->v);
	
	vout->v[3] = 1.0f;
	
}

void Matrix::Apply4x3(const Vector* v, Vector* vout) const
{

	Apply4x3_3(v->v, vout->v);
	
	vout->v[3] = 1.0f;

}

void Matrix::Apply(const Vector* v, Vector* vout) const
{

	Apply(v->v, vout->v);

}

void Matrix::Apply(const Vector& v, Vector& vout) const
{

	Apply(v.v, vout.v);

}

void Matrix::MakeIdentity()
{

	Identity();
	
}

void Matrix::MakeLookat(Vector position, Vector target, Vector up)
{

	MATRIX_ASSERT(s == 4);
	
	Matrix temp;
	
	temp.MakeIdentity();
	
	Vector axisZ = target - position;
	Vector axisX = up % axisZ;
	Vector axisY = axisZ % axisX;
	
	axisX.Normalize();
	axisY.Normalize();
	axisZ.Normalize();
	
	for (int x = 0; x < 3; ++x)
	{
		temp(x, 0) = axisX[x];
		temp(x, 1) = axisY[x];
		temp(x, 2) = axisZ[x];
	}

	MakeTranslation(-position);
	
	*this = temp * (*this);
	
}

void Matrix::MakeTranslation(Vector translation)
{

	MATRIX_ASSERT(s == 4);

	MakeIdentity();
	
	v[Index(3, 0)] = translation[0];
	v[Index(3, 1)] = translation[1];
	v[Index(3, 2)] = translation[2];
	
}

void Matrix::MakeRotationX(float angle)
{

	MATRIX_ASSERT(s == 3 || s == 4);

	MakeIdentity();
	
	v[Index(1, 1)] = +cos(angle);
	v[Index(2, 1)] = +sin(angle);
	v[Index(1, 2)] = -sin(angle);
	v[Index(2, 2)] = +cos(angle);
	
}

void Matrix::MakeRotationY(float angle)
{

	MATRIX_ASSERT(s == 3 || s == 4);

	MakeIdentity();
	
	v[Index(0, 0)] = +cos(angle);
	v[Index(2, 0)] = -sin(angle);
	v[Index(0, 2)] = +sin(angle);
	v[Index(2, 2)] = +cos(angle);
	
}

void Matrix::MakeRotationZ(float angle)
{

	MATRIX_ASSERT(s == 3 || s == 4);

	MakeIdentity();
	
	v[Index(0, 0)] = +cos(angle);
	v[Index(1, 0)] = +sin(angle);
	v[Index(0, 1)] = -sin(angle);
	v[Index(1, 1)] = +cos(angle);
	
}

void Matrix::MakeRotationEuler(Vector rotation)
{

	MATRIX_ASSERT(s == 3 || s == 4);

	MakeIdentity();
	
	Matrix rotationX; rotationX.MakeRotationX(rotation[0]);
	Matrix rotationY; rotationY.MakeRotationY(rotation[1]);
	Matrix rotationZ; rotationZ.MakeRotationZ(rotation[2]);
	
	Matrix temp = rotationZ * rotationY * rotationX;
	
	*this = temp;
	
}

void Matrix::MakeScaling(Vector scale)
{

	MATRIX_ASSERT(s == 3 || s == 4);
	
	MakeIdentity();
	
	v[Index(0, 0)] = scale[0];
	v[Index(1, 1)] = scale[1];
	v[Index(2, 2)] = scale[2];
	
}
