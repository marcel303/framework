#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "Types2.h"

/**
 * 3/4 component vector.
 *
 * The vector class is a general purpose vector type that has 4 components.
 * The vector can be used as a 3D vector by leaving the 4th (w) component set to 1.
 * The vector supports the following operations:
 * Calculating size, squared size, dot & cross products, addition, subtraction, scaling, etc.
 * The vector components are accessed using vector[0] to vector[3], or vector.x, .y, .z, .w alternatively.
 * The % operator is used to calculate the cross product with another vector.
 * The ^ operator is used to calculate the component-wise multiplcation of two vectors.
 * To convert the vector to an array of floats, use (float*)vector.
 *
 * Examples:
 * @code
 *
 * Calculate the difference between two vectors and the difference vector's length:
 *
 * Vector vectorA;
 * Vector vectorB;
 *
 * Vector difference = vectorB - vectorA;
 * float length = difference.GetSize();
 *
 * Converting a vector to a unit vector:
 *
 * Vector vector(10.0f, 5.0f, 1.0f);
 * vector.Normalize();
 * float length = vector.GetSize(); // length now equals 1.
 *
 * Calculating the cross product between 2 vectors:
 *
 * Vector vectorA(1.0f, 0.0f, 0.0f);
 * Vector vectorB(0.0f, 1.0f, 0.0f);
 *
 * Vector cross = vectorA % vectorB;
 *
 * @endcode
 */
class Vector
{
public:
	inline Vector();                                          ///< Default vector constructor. Initializes XYZW components to (0, 0, 0, 1).
	inline Vector(float x, float y, float z, float w = 1.0f); ///< Initialize vector using XYZ(W) components. W defaults to 1.0.
	inline Vector(const Vector& vector);                      ///< Copy constructor.
	inline ~Vector();                                         ///< Destructor.

	// =================================
	// Data. The data may be access by 
	// using either values, v or x, y, z & w.
	// =================================

	union
	{
		float values[4]; ///< XYZW components.
		float v[4];      ///< XYZW components.
		struct
		{
			float x; ///< X component.
			float y; ///< Y component.
			float z; ///< Z component.
			float w; ///< W component.
		};
	};

public:
	inline void Set(float x, float y, float z, float w = 1.0f); ///< Set XYZ(W) components of matrix. W defaults to 1.0.
	inline void Copy(const Vector& vector);                     ///< Copy from vector.

	// =================================
	// Calculations.
	// =================================

	inline float GetSize() const;        ///< Return the length of the vector.
	inline float GetSquaredSize() const; ///< Return the squared length of the vector.
	inline void Normalize();             ///< Normalize vector.

	// =================================
	// Operators.
	// =================================

	// Negate.
	inline Vector operator-() const; ///< Negate vector. Return result.
	
	// Operation between two vectors.
	inline Vector operator+(const Vector& vector) const; ///< Add vectors. Return result.
	inline Vector operator-(const Vector& vector) const; ///< Add vectors. Return result.
	inline Vector operator%(const Vector& vector) const; ///< Return cross product of vectors.
	inline Vector operator^(const Vector& vector) const; ///< Component multiply. Return result.
	inline Vector operator/(const Vector& vector) const; ///< Component divide. Return result.

	// Operation between two vectors.
	inline void operator+=(const Vector& vector); ///< Add vector.
	inline void operator-=(const Vector& vector); ///< Subtract vector.
	inline void operator%=(const Vector& vector); ///< Cross product with vector.
	inline void operator^=(const Vector& vector); ///< Component multiply with vector.
	inline void operator/=(const Vector& vector); ///< Component devide with vector.

	// Dot product.
	inline float operator*(const Vector& vector) const; ///< Return inner product.

	// Scalar multiplication & division.
	inline Vector operator*(float v) const; ///< Multiply components with v. Return result.
	inline Vector operator/(float v) const; ///< Divide components with v. Return result.

	// Scalar multiplication & division.
	inline void operator*=(float v); ///< Multiply components with v.
	inline void operator/=(float v); ///< Divide components with v.

	// Assignment & comparison.
	const inline Vector& operator=(const Vector& vector); ///< Assign vector.
	inline bool operator==(const Vector& vector) const;   ///< Check for equality. If the vectors are equal, return true, false otherwise.
	inline bool operator<(const Vector& vector) const;    ///< Check if the vector is 'smaller' than the argument, sorted by X, Y, Z and then W. Required to use Vector in STL maps.

	// Indexing.
	inline float& operator[](size_t index);             ///< Index into (x, y, z, w).
	inline float& operator[](int index);                ///< Index into (x, y, z, w).
	const inline float& operator[](size_t index) const; ///< Index into (x, y, z, w).
	const inline float& operator[](int index) const;    ///< Index into (x, y, z, w).
	
	// =================================
	// Direct access.
	// =================================

	inline operator float*(); ///< Return pointer to first component (XYZW).
};

#include "Vector.inl"

#endif
