#include "Mat4x4.h"
#include "Quat.h"

Mat4x4 Mat4x4::Rotate(const Quat & q) const
{
	Mat4x4 t = q.toMatrix();
	return (*this) * t;
}
