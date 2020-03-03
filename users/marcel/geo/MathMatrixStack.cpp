#include "MathMatrixStack.h"

MatrixStack::MatrixStack()
{

	Matrix matrix;
	
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
	
Matrix& MatrixStack::GetMatrix()
{

	return vMatrix.back();
	
}

const Matrix& MatrixStack::GetMatrix() const
{

	return vMatrix.back();
	
}

void MatrixStack::ApplyTranslation(Vector translation)
{

	Matrix matrix;
	matrix.MakeTranslation(translation);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationEuler(Vector rotation)
{

	Matrix matrix;
	matrix.MakeRotationEuler(rotation);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationX(float angle)
{

	Matrix matrix;
	matrix.MakeRotationX(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationY(float angle)
{

	Matrix matrix;
	matrix.MakeRotationY(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyRotationZ(float angle)
{

	Matrix matrix;
	matrix.MakeRotationZ(angle);
	GetMatrix() = GetMatrix() * matrix;
	
}

void MatrixStack::ApplyScaling(Vector scale)
{

	Matrix matrix;
	matrix.MakeScaling(scale);
	GetMatrix() = GetMatrix() * matrix;
	
}