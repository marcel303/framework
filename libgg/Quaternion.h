#pragma once

#include "Matrix.h"
#include "Types2.h"
#include "Vector.h"

/**
 * 4D spherical rotation.
 *
 * The quaternion class represents a rotation using 4D quaternion math.
 * A quaternion rotation, unlike a rotation matrix, does not suffer from gimbal lock, and
 * interpolates nicely using the shortest arc.
 * Quaternions can be interpolated using Slerp and Nlerp. Slerp offers the highest quality,
 * while Nlerp offers more speed, at the loss of some quality.
 *
 * Interpolation using Slerp or Nlerp requires 2 quaternions. The first is the start
 * rotation and the second the end rotation. A third parameter, t, is used to interpolate
 * between the two and ranges from 0.0 to 1.0.
 *
 * Rotations are added using the quaternion multiplication operator. As with rotation matrices, this operation
 * yields a 'combined' rotation.
 *
 * Examples:
 * @code
 *
 * Adding matrix rotations:
 *
 * Matrix rotation1;
 * Matrix rotation2;
 *
 * rotation1.MakeRotationX(M_PI / 4.0f);
 * rotation2.MakeRotationY(M_PI / 2.0f);
 *
 * Quaternion quaternion1;
 * Quaternion quaternion2;
 *
 * quaternion1.FromMatrix(rotation1);
 * quaternion2.FromMatrix(rotation2);
 *
 * Quaternion combinedRotationQ = quaternion1 * quaternion2;
 *
 * Matrix combinedRotation = combinedRotationQ.ToMatrix();
 *
 * Creating a rotation from an axis/angle pair:
 *
 * Quaternion quaternion;
 *
 * quaternion.FromAxisAngle(Vector(1.0f, 1.0f, 0.0f), M_PI / 2.0f);
 *
 * Converting a rotation matrix to an axis/angle pair:
 *
 * Matrix rotation;
 * Quaternion quaternion;
 * Vector axis;
 * float angle;
 *
 * rotation = ...;
 * quaternion.FromMatrix(rotation);
 * quaternion.ToAxisAngle(axis, angle);
 *
 * std::cout << "Axis: " << axis[0] << " " << axis[1] << " " << axis[2] << std::endl;
 * std::cout << "Angle: " << angle << std::endl;
 *
 * Interpolating between rotations:
 *
 * Quaternion rotation1;
 * Quaternion rotation2;
 *
 * rotation1.FromAxisAngle(Vector(1.0f, 0.0f, 0.0f), M_PI / 3.0f);
 * rotation2.FromAxisAngle(Vector(0.0f, 0.0f, 1.0f), M_PI / 2.0f);
 *
 * // Calculate point midway rotation1 and rotation2.
 *
 * Quaternion interpolatedRotation = rotation1.Slerp(rotation2, 0.5f);
 *
 * // Show results.
 *
 * Vector axis;
 * float angle;
 * interpolatedRotation.ToAxisAngle(axis, angle);
 * std::cout << "Axis: " << axis[0] << " " << axis[1] << " " << axis[2] << std::endl;
 * std::cout << "Angle: " << angle << std::endl;
 *
 * @endcode
 */
class Quaternion
{
public:
	Quaternion();                             ///< Default constructor.
	Quaternion(const Quaternion& quaternion); ///< Copy constructor.
	~Quaternion();                            ///< Destructor.

	float Size() const;                           ///< Return the length of quaternion.
	void Normalize();                             ///< Normalize quaternion.
	Quaternion Conjugate() const;                 ///< Return the adjugate of the quaternion.
	void FromAxisAngle(Vector axis, float angle); ///< Create quaternion from axis-angle pair.
	void FromMatrix(const Matrix& matrix);        ///< Create quaternion from rotation matrix.
	Matrix ToMatrix() const;                      ///< Convert quaternion to rotation matrix. Return result.
	void ToAxisAngle(Vector& out_axis, float& out_angle) const; ///< Convert quaternion to axis-angle pair. Return result.
	float Dot(const Quaternion& quaternion) const; ///< Return dot product with quaternion.
	void MakeIdentity();                           ///< Set quaternion to identity (no) rotation.
	Quaternion Inverse() const;                    ///< Return the inverse quaternion (rotation).

	Quaternion Slerp(const Quaternion& quaternion, float t) const; ///< Performs a SLERP between the quaternion and another quaternion.
	Quaternion Nlerp(const Quaternion& quaternion, float t) const; ///< Performs a normalized LERP between the quaternion and another quaternion.

	Quaternion operator-() const;                        ///< Return negated quaternion.
	Quaternion& operator=(const Quaternion& quaternion); ///< Copy from quaternion.

	Quaternion operator*(const Quaternion& quaternion) const; ///< Multiply quaternions (add rotations). Return result.
	Quaternion operator-(const Quaternion& quaternion) const; ///< Subtract quaternions. Return result.
	Quaternion operator+(const Quaternion& quaternion) const; ///< Add quaternions. Return result.
	Quaternion operator/(float v) const;                      ///< Divide quaternion by scalar. Return result.
	Quaternion operator*(float v) const;                      ///< Multiply quaternion by scalar. Return result.

	Quaternion& operator*=(const Quaternion& quaternion); ///< Multiply quaternions (add rotations).
	Quaternion& operator-=(const Quaternion& quaternion); ///< Subtract quaternions.
	Quaternion& operator+=(const Quaternion& quaternion); ///< Add quaternions.
	Quaternion& operator/=(float v);                      ///< Divide quaternion by scalar.
	Quaternion& operator*=(float v);                      ///< Multiply quaternion by scalar.

	operator Matrix() const;                 ///< Converst to matrix.
	float&      operator[](int index);       ///< Index into (x, y, z, w).
	const float operator[](int index) const; ///< Index into (x, y, z, w).

	Vector m_xyz; ///< 'Axis'.
	float m_w;    ///< 'Angle'.
};
