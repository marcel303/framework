#ifndef __MathVector_h__
#define __MathVector_h__

class Vector
{

	public:

	Vector();
	Vector(float v1, float v2, float v3, float v4 = 1.0f); ///< Initialize vector using XYZ(W) components. W defaults to 1.0.
	Vector(const Vector& vector); ///< Copy constructor.
	~Vector();

	public:

	float v[4]; ///< XYZW components.

	public:

	void Set(float v1, float v2, float v3, float v4 = 1.0f); ///< Set XYZ(W) components of matrix. W defaults to 1.0.

	public:

	float GetSize() const; ///< Return the length of the vector.
	float GetSize2() const; ///< Return the squared length of the vector.
	void Normalize(); ///< Normalize vector.
	void Copy(Vector vector); ///< Copy from vector.

	public:
	
	Vector operator-() const; ///< Negate vector. Return result.
	
	public:

	Vector operator+(Vector vector) const; ///< Add vectors. Return result.
	Vector operator-(Vector vector) const; ///< Add vectors. Return result.
	Vector operator%(Vector vector) const; ///< Return cross product of vectors.
	Vector operator^(Vector vector) const; ///< Component multiply. Return result.
	Vector operator/(Vector vector) const; ///< Component divide. Return result.

	public:

	void operator+=(Vector vector); ///< Add vector.
	void operator-=(Vector vector); ///< Subtract vector.
	void operator%=(Vector vector); ///< Cross product with vector.
	void operator^=(Vector vector); ///< Component multiply with vector.
	void operator/=(Vector vector); ///< Component device with vector.

	public:

	float operator*(Vector vector) const; ///< Return inner product.

	public:

	Vector operator*(float v) const; ///< Multiply components with v. Return result.
	Vector operator/(float v) const; ///< Divide components with v. Return result.

	public:

	void operator*=(float v); ///< Multiply components with v.
	void operator/=(float v); ///< Divide components with v.

	public:

	void operator=(const Vector& vector); ///< Assign vector.

	public:

	float& operator[](int index); ///< Index into (x, y, z, w).
	float operator[](int index) const; ///< Index into (x, y, z, w).

	public:

	operator float*(); ///< Return pointer to first component (XYZW).

};

#endif // !__MathVector_h__