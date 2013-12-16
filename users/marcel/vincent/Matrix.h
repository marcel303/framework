#pragma once

#include "Vector.h"

/**
 * 1x1 to 4x4 matrix.
 *
 * The matrix class represents a 4x4, 3x3, 2x2 or 1x1 translation/rotation matrix.
 * The matrix class has a number of methods to create 3D translation, rotation, projection, etc matrix.
 *
 * The matrix uses the D3D memory layout for 4x4 matrices (the default).
 *
 * The matrix size can be specified by calling SetSize. The default (and maximum) size is 4x4 and
 * can be scaled down all the way to 1x1. Note however that the Make methods (to create translation
 * rotation, etc matrices) require 4x4 or 3x3 matrices.
 *
 * Matrices can be multiplied by other matrices and vectors.
 *
 * The matrix class had methods to calculate its inverse, determinant, adjugate, etc.
 *
 * For an introduction to matrices and linear algebra, refer to
 * http://en.wikipedia.org/wiki/Category:Linear_algebra
 *
 * Examples:
 * @code
 *
 * Matrix construction:
 *
 * Matrix matrix;
 *
 * matrix.MakeIdentity(); // The matrix now contains the identity matrix.
 * matrix.MakeTranslation(Vector(0.0f, 1.0f, 2.0f)); // The matrix now is a translation matrix.
 * matrix.MakeRotationYawPitchRoll(0.0f, M_PI / 2.0f, 0.0f); // The matrix now equals a yaw/pitch/roll rotation matrix.
 * matrix.MakePerspectiveFovLH(M_PI / 2.0f, 1.0, 0.01f, 100.0f); // The matrix now is a projection matrix
 *     with a viewing angle of 45 degrees and a near and far clipping plane of 0.01 and 100.0 respectively.
 *
 * Matrix & vector multiplication:
 *
 * Matrix transformA;
 * Matrix transformB;
 *
 * // Combine matrix transformations.
 * Matrix totalTransform = transformA * transformB;
 * totalTransform = transformA.Multiply(transformB); // Equal the previous expression.
 *
 * Vector transformedVector = totalTransform * Vector(1.0f, 0.0f, 2.0f);
 *
 * Matrix projection;
 *
 * // Create der uber matrix.
 * totalTransform *= projection;
 *
 * Acessing matrix using (x, y) indices and D3D names:
 *
 * Matrix matrix;
 *
 * matrix(0, 0) = 10.0f;
 * matrix(1, 1) = 5.0f;
 * matrix(2, 2) = 2.5f;
 *
 * float v11 = matrix._11;
 * float v22 = matrix._2;
 * float v33 = matrix._33;
 *
 * Calculating a matrix' inverse:
 *
 * Matrix transform;
 *
 * Matrix inverseTransform = transform.CalculateInverse();
 *
 * @endcode
 */
class Matrix
{
public:
	inline Matrix(); ///< Default constructor.
	inline Matrix(const Matrix& matrix); ///< Copy constructor.
	inline Matrix(
		float _11, float _12, float _13, float _14,
		float _21, float _22, float _23, float _24,
		float _31, float _32, float _33, float _34,
		float _41, float _42, float _43, float _44); ///< Constructor. Initializes all matrix elements explicitly.
	inline ~Matrix(); ///< Destructor.
	
	// =================================
	// Data. The data may be accessed
	// using either m_values, m_elements
	// or _11 to _44.
	// =================================

	union
	{
		struct
		{
			float m_values[16];
		};
		struct
		{
			float m_elements[4][4];
		};
		struct
		{
			float _11, _12, _13, _14;
			float _21, _22, _23, _24;
			float _31, _32, _33, _34;
			float _41, _42, _43, _44;
		};
		struct
		{
			float row[3][4];
			float _tx, _ty, _tz;
			float _w;
		};
	};

	// =================================
	// Size.
	// =================================

	size_t GetSize() const;    ///< Return the size of the matrix. The matrix is always square.
	void SetSize(size_t size); ///< Set size of the matrix. The matrix is always square.

	// =================================
	// Calculations.
	// =================================

	Matrix CalculateSubMatrix(size_t x, size_t y) const;      ///< Return submatrix of matrix, with column x and row y erased.
	Matrix CalculateAdjugate() const;                         ///< Return the adjugate matrix.
	Matrix CalculateTranspose() const;                        ///< Return the transpose matrix.
	Matrix CalculateInverse() const;                          ///< Return the inverse matrix.
	inline float CalculateCofactor(size_t x, size_t y) const; ///< Return the cofactor for element (x, y).
	float CalculateDeterminant() const;                       ///< Return the determinant.
	
	// =================================
	// Operators.
	// =================================

	// Indexing.
	inline const float& operator()(size_t x, size_t y) const; ///< Return reference to matrix element at location (x, y).
	inline float& operator()(size_t x, size_t y);             ///< Return reference to matrix element at location (x, y).

	// Assignment.
	inline Matrix& operator=(const Matrix& matrix); ///< Assign matrix.

	// Multiplication by matrix and vector.
	inline Matrix operator*(const Matrix& matrix) const; ///< Multiply matrices. Return result.
	inline Vector operator*(const Vector& vector) const; ///< Multiply matrix by vector. Return result.

	// Multiplication/division by scalar.
	inline Matrix operator*(float v) const; ///< Multiply matrix by scalar v. Return result.
	inline Matrix operator/(float v) const; ///< Divide matrix by scalar v. Return result.

	// Addition/subtraction of matrix.
	inline Matrix operator+(const Matrix& matrix) const; ///< Add matrices. Return result.
	inline Matrix operator-(const Matrix& matrix) const; ///< Subtract matrices. Return result.

	// Float conversion.
	inline operator float*(); ///< Return pointer to first matrix element.

	// Matrix multiplication + assign.
	inline void operator*=(const Matrix& matrix); ///< Multiply with matrix.

	// Equality.
	inline bool operator==(const Matrix& matrix); ///< Test matrices for equality. Return true if equal, false otherwise.
	
	// =================================
	// Multiply by matrix.
	// =================================

	inline Matrix Multiply(const Matrix& matrix); ///< Multiply matrices and return result.

	// =================================
	// Multiply by vector.
	// =================================

	// 3-component multiply: apply rotation only.
	inline void Apply3x3_3(const float* v, float* vout) const; ///< Multiply 3D vector v with 3x3 portion of matrix (rotation) and store in 3D result.
	inline void Apply4x3_3(const float* v, float* vout) const; ///< Multiply 3D vector v with 4x3 portion of matrix (rotation + translation) and store in 3D result.
	inline void Apply(const float* v, float* vout) const;      ///< Multiply 4D vector v with 4x4 portion of matrix and store in 4D result.
	// 4-component multiply: apply rotation & translation.
	inline void Apply3x3(const Vector* v, Vector* vout) const; ///< Multiply vector with 3x3 rotation part of matrix.
	inline void Apply4x3(const Vector* v, Vector* vout) const; ///< Multiply vector with 4x3 rotation + translation part of matrix.
	inline void Apply(const Vector* v, Vector* vout) const;    ///< Multiply vector with matrix.
	inline void Apply(const Vector& v, Vector& vout) const;    ///< Multiply vector with matrix.

	// =================================
	// Matrix construction.
	// =================================

	void MakeIdentity();                             ///< Fill matrix with identity values.
	void MakeTranslation(const Vector& translation); ///< Make translation matrix.
	void MakeRotationX(float angle); ///< Make rotation about the X axis.
	void MakeRotationY(float angle); ///< Make rotation about the Y axis.
	void MakeRotationZ(float angle); ///< Make rotation about the Z axis.
	void MakeRotationEuler(const Vector& rotation);                                 ///< Make consecutive (X, Y, Z) rotation matrix.
	void MakeRotationYawPitchRoll(float yaw, float pitch, float roll);              ///< Build a matrix with a specified yaw (=y), pitch (=x), and roll(=z).
	void MakeScaling(const Vector& scale);                                          ///< Make scaling matrix.
	void MakePerspectiveFovLH(float fovY, float aspect, float nearCP, float farCP); ///< Set up a LEFT handed projection matrix.
	void MakePerspectiveFovRH(float fovY, float aspect, float nearCP, float farCP); ///< Set up a RIGHT handed projection matrix.
	void MakeOrthoOffCenterLH(float left, float right, float top, float bottom, float near, float far); ///< Create orthogonal matrix.
	void MakeLookat(const Vector& position, const Vector& target, const Vector& up);                    ///< Make lookat matrix.

private:
	inline float GetSign(size_t x, size_t y) const; ///< Return the sign of cofactor (x, y).
	inline size_t Index(size_t x, size_t y) const;  ///< Return element index of element at location (x, y).

	size_t m_size; ///< Matrix size. The size may vary from 1x1 to 4x4. The size is always squared.

};

#include "Matrix.inl"
