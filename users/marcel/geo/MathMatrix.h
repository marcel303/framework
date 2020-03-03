// FIXME: Move inline methods to MathMatrix.inl.

#ifndef __MathMatrix_h__
#define __MathMatrix_h__

#include "MathVector.h"

#define MATRIX_FIXED_SIZE

// Default size = 4 x 4.

class Matrix
{

	public:
	
	Matrix();
	Matrix(const Matrix& matrix); ///< Copy constructor.
	~Matrix();
	
	protected:
	
	int s; ///< Size of square matrix. Matrix has s x s elements.
	#if defined(MATRIX_FIXED_SIZE)
	float v[16];
	#else
	float* v; ///< s x s float values.
	#endif
	
	public:
	
	void SetSize(int s); ///< Set size of matrix to s x s elements.
	int GetSize() const; ///< Return the size of the matrix.
	
	public:
	
	void Identity(); ///< Fill matrix with identity values.
	Matrix SubMatrix(int x, int y) const; ///< Return submatrix of matrix, with column x and row y erased.
	Matrix Adjugate() const; ///< Return the adjugate matrix.
	Matrix Transpose() const; ///< Return the transpose matrix.
	Matrix Inverse() const; ///< Return the inverse matrix.
	
	public:
	
	inline float Cofactor(int x, int y) const; ///< Return the cofactor for element (x, y).
	
	protected:
	
	inline float Sign(int x, int y) const; ///< Return the sign of cofactor (x, y).

	public:
	
	float Determinant() const; ///< Return the determinant.
	
	protected:
	
	inline int Index(int x, int y) const; ///< Return element index of element at location (x, y).
	
	public:
	
	inline const float& operator()(int x, int y) const; ///< Return reference to matrix element at location (x, y).
	inline float& operator()(int x, int y); ///< Return reference to matrix element at location (x, y).
 	
	public:

	void operator=(const Matrix& matrix); ///< Assign matrix.
	Matrix operator*(const Matrix& matrix) const; ///< Multiply matrices. Return result.
	Vector operator*(const Vector& vector) const; ///< Multiply matrix by vector. Return result.
	Matrix operator*(float v) const; ///< Multiply matrix by scalar v. Return result.
	Matrix operator/(float v) const; ///< Divide matrix by scalar v. Return result.
	Matrix operator+(const Matrix& matrix) const; ///< Add matrices. Return result.
	Matrix operator-(const Matrix& matrix) const; ///< Subtract matrices. Return result.
	inline operator float*(); ///< Return pointer to first matrix element.
	
	public:
	
	void Apply3x3_3(const float* v, float* vout) const; ///< Multiply 3D vector v with 3x3 portion of matrix (rotation) and store in 3D result.
	void Apply4x3_3(const float* v, float* vout) const; ///< Multiply 3D vector v with 4x3 portion of matrix (rotation + translation) and store in 3D result.
	void Apply(const float* v, float* vout) const; ///< Multiply 4D vector v with 4x4 portion of matrix and store in 4D result.
	
	public:

	void Apply3x3(const Vector* v, Vector* vout) const; ///< Multiply vector with 3x3 rotation part of matrix.
	void Apply4x3(const Vector* v, Vector* vout) const; ///< Multiply vector with 4x3 rotation + translation part of matrix.
	void Apply(const Vector* v, Vector* vout) const; ///< Multiply vector with matrix.
	void Apply(const Vector& v, Vector& vout) const; ///< Multiply vector with matrix.
	
	public:
	
	void MakeIdentity(); ///< Make identity matrix.
	void MakeLookat(Vector position, Vector target, Vector up); ///< Make lookat matrix.
	void MakeTranslation(Vector translation); ///< Make translation matrix.
	void MakeRotationX(float angle); ///< Make rotation about the X axis.
	void MakeRotationY(float angle); ///< Make rotation about the Y axis.
	void MakeRotationZ(float angle); ///< Make rotation about the Z axis.
	void MakeRotationEuler(Vector rotation); ///< Make consecutive (X, Y, Z) rotation matrix.
	void MakeScaling(Vector scale); ///< Make scaling matrix.
	
};

#include "MathMatrix.inl"

#endif // !__MathMatrix_h__