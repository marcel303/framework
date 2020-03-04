#include "MathMatrixStack.h"
#include "Quat.h"

MatrixStack::MatrixStack()
{

	Mat4x4 matrix;
	
	matrix.MakeIdentity();
	
	vMatrix.push_back(matrix);
	
}

MatrixStack::~MatrixStack()
{

}

void MatrixStack::Push()
{

	vMatrix.push_back(vMatrix.back());

}

void MatrixStack::Pop()
{

	vMatrix.pop_back();

}
	
Mat4x4& MatrixStack::GetMatrix()
{

	return vMatrix.back();
	
}

const Mat4x4& MatrixStack::GetMatrix() const
{

	return vMatrix.back();
	
}

void MatrixStack::ApplyTranslation(Vec3Arg translation)
{

	Mat4x4 matrix;
	matrix.MakeTranslation(translation);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationAngleAxis(float angle, Vec3Arg axis)
{

	Quat q;
	q.fromAxisAngle(axis, angle);
	
	Mat4x4 matrix = q.toMatrix();
	
	GetMatrix() = GetMatrix() * matrix;

}

void MatrixStack::ApplyRotationEuler(Vec3Arg rotation)
{

	Mat4x4 rotationX; rotationX.MakeRotationX(rotation[0]);
	Mat4x4 rotationY; rotationY.MakeRotationY(rotation[1]);
	Mat4x4 rotationZ; rotationZ.MakeRotationZ(rotation[2]);
	
	Mat4x4 matrix = rotationZ * rotationY * rotationX;
	
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationX(float angle)
{

	Mat4x4 matrix;
	matrix.MakeRotationX(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationY(float angle)
{

	Mat4x4 matrix;
	matrix.MakeRotationY(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationZ(float angle)
{

	Mat4x4 matrix;
	matrix.MakeRotationZ(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyScaling(Vec3Arg scale)
{

	Mat4x4 matrix;
	matrix.MakeScaling(scale);
	GetMatrix() = GetMatrix() * matrix;
	
}
