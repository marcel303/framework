#include <assert.h>
#include "MatrixStack.h"

MatrixStack::MatrixStack()
{
	Matrix root;
	root.MakeIdentity();
	m_stack.push(root);
}

MatrixStack::~MatrixStack()
{
	assert(m_stack.size() == 1);
}

const Matrix& MatrixStack::Top()
{
	return m_stack.top();
}

void MatrixStack::Push(const Matrix& matrix)
{
	Matrix combined = m_stack.top() * matrix;
	m_stack.push(combined);
}

void MatrixStack::PushTranslation(const Vector& translation)
{
	Matrix transform;
	transform.MakeTranslation(translation);
	Push(transform);
}

void MatrixStack::PushRotation(const Vector& rotation)
{
	Matrix transform;
	transform.MakeRotationEuler(rotation);
	Push(transform);
}

void MatrixStack::PushScaling(const Vector& scale)
{
	Matrix transform;
	transform.MakeScaling(scale);
	Push(transform);
}

void MatrixStack::Pop()
{
	m_stack.pop();
}
