#pragma once

Matrix::Matrix()
{
	SetSize(4);
}

Matrix::Matrix(const Matrix& matrix)
{
	*this = matrix;
}

Matrix::Matrix(
	float __11, float __12, float __13, float __14,
	float __21, float __22, float __23, float __24,
	float __31, float __32, float __33, float __34,
	float __41, float __42, float __43, float __44)
{
	SetSize(4);

	this->_11 = __11; this->_12 = __12; this->_13 = __13; this->_14 = __14;
	this->_21 = __21; this->_22 = __22; this->_23 = __23; this->_24 = __24;
	this->_31 = __31; this->_32 = __32; this->_33 = __33; this->_34 = __34;
	this->_41 = __41; this->_42 = __42; this->_43 = __43; this->_44 = __44;
}

Matrix::~Matrix()
{
}

inline size_t Matrix::Index(size_t x, size_t y) const
{
	return x * m_size + y;
}

inline const float& Matrix::operator()(size_t x, size_t y) const
{
	return m_values[Index(x, y)];	
}

inline float& Matrix::operator()(size_t x, size_t y)
{
	return m_values[Index(x, y)];	
}

inline Matrix& Matrix::operator=(const Matrix& matrix)
{
	SetSize(matrix.GetSize());
	
	const size_t sizeSquared = m_size * m_size;
	
	for (size_t i = 0; i < sizeSquared; ++i)
		m_values[i] = matrix.m_values[i];
	
	return *this;
}

inline Matrix Matrix::operator*(const Matrix& matrix) const
{
	// For an explanation of matrix-matrix multiplication, refer to
	// http://en.wikipedia.org/wiki/Matrix_multiplication
	
	if (matrix.GetSize() != m_size)
		return matrix;
	
	Matrix temp;
	
	temp.SetSize(matrix.GetSize());
 	
	for (size_t x = 0; x < m_size; ++x)
	{
		for (size_t y = 0; y < m_size; ++y)
		{
			float _v = 0.0f;
			
			for (size_t i = 0; i < m_size; ++i)
				_v += this->m_values[Index(i, y)] * matrix(x, i);
				
			temp(x, y) = _v;
		}
	}
	
	return temp;
}

inline Vector Matrix::operator*(const Vector& vector) const
{
	// For an explanation of matrix-vector multiplication, refer to
	// http://en.wikipedia.org/wiki/Matrix_multiplication

	Vector temp;
	
	Apply(&vector, &temp);
	
	return temp;
}

inline Matrix Matrix::operator*(float v) const
{
	Matrix temp;
	
	temp.SetSize(GetSize());

	for (size_t i = 0; i < m_size * m_size; ++i)
		temp.m_values[i] = this->m_values[i] * v;
		
	return temp;
}

inline Matrix Matrix::operator/(float v) const
{
	Matrix temp;
	
	temp.SetSize(GetSize());

	for (size_t i = 0; i < m_size * m_size; ++i)
		temp.m_values[i] = this->m_values[i] / v;
		
	return temp;
}

inline Matrix Matrix::operator+(const Matrix& matrix) const
{
	Matrix temp;
	
	temp.SetSize(GetSize());

	for (size_t i = 0; i < m_size * m_size; ++i)
		temp.m_values[i] = m_values[i] + matrix.m_values[i];
		
	return temp;
}

inline Matrix Matrix::operator-(const Matrix& matrix) const
{
	Matrix temp;
	
	temp.SetSize(GetSize());

	for (size_t i = 0; i < m_size * m_size; ++i)
		temp.m_values[i] = m_values[i] - matrix.m_values[i];
		
	return temp;
}

inline void Matrix::operator*=(const Matrix& matrix)
{
	*this = (*this) * matrix;
}

inline bool Matrix::operator==(const Matrix& matrix)
{
	if (m_size != matrix.m_size)
		return false;

	for (size_t i = 0; i < m_size * m_size; ++i)
		if (m_values[i] != matrix.m_values[i])
			return false;

	return true;
}

inline Matrix Matrix::Multiply(const Matrix& matrix)
{
	return (*this) * matrix;
}

inline Matrix::operator float*()
{
	return m_values;
}

inline void Matrix::Apply3x3_3(const float* v, float* vout) const
{
	for (int i = 0; i < 3; ++i)
		vout[i] =
			v[0] * m_values[Index(0, i)] +
			v[1] * m_values[Index(1, i)] +
			v[2] * m_values[Index(2, i)];
}

inline void Matrix::Apply4x3_3(const float* v, float* vout) const
{
	for (int i = 0; i < 3; ++i)
		vout[i] =
			v[0] * m_values[Index(0, i)] +
			v[1] * m_values[Index(1, i)] +
			v[2] * m_values[Index(2, i)] +
			       m_values[Index(3, i)];
}

inline void Matrix::Apply(const float* v, float* vout) const
{
	for (int i = 0; i < 4; ++i)
		vout[i] =
			v[0] * m_values[Index(0, i)] +
			v[1] * m_values[Index(1, i)] +
			v[2] * m_values[Index(2, i)] +
			v[3] * m_values[Index(3, i)];
}

inline void Matrix::Apply3x3(const Vector* v, Vector* vout) const
{
	Apply3x3_3(v->v, vout->v);
	
	vout->v[3] = 1.0f;
}

inline void Matrix::Apply4x3(const Vector* v, Vector* vout) const
{
	Apply4x3_3(v->v, vout->v);
	
	vout->v[3] = 1.0f;
}

inline void Matrix::Apply(const Vector* v, Vector* vout) const
{
	Apply(v->v, vout->v);
}

inline void Matrix::Apply(const Vector& v, Vector& vout) const
{
	Apply(v.v, vout.v);
}
