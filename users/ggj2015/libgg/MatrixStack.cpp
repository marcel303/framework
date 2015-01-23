#include "Debugging.h"
#include "MatrixStack.h"

MatrixStack::MatrixStack()
{
	m_depth = 1;
	m_stack[0].MakeIdentity();

	m_handler = 0;
}

const Mat4x4& MatrixStack::Top() const
{
	return m_stack[m_depth - 1];
}

int MatrixStack::GetDepth() const
{
	return m_depth - 1;
}

void MatrixStack::SetHandler(MatrixHandler* handler)
{
	m_handler = handler;
}

void MatrixStack::Push(const Mat4x4& mat)
{
	m_stack[m_depth] = m_stack[m_depth - 1] * mat;

	m_depth++;

	if (m_handler)
		m_handler->OnMatrixUpdate(this, Top());
}

void MatrixStack::PushI(const Mat4x4& mat)
{
	m_stack[m_depth] = mat;

	m_depth++;

	if (m_handler)
		m_handler->OnMatrixUpdate(this, Top());
}

void MatrixStack::PushI()
{
	Mat4x4 identity;

	identity.MakeIdentity();

	m_stack[m_depth] = identity;

	m_depth++;

	if (m_handler)
		m_handler->OnMatrixUpdate(this, Top());
}

void MatrixStack::PushTranslation(float x, float y, float z)
{
	Mat4x4 mat;

	mat.MakeTranslation(x, y, z);

	Push(mat);
}

/*
void MatrixStack::PushRotation(float x, float y, float z)
{
	Mat4x4 mat;

	mat.MakeRotation(x, y, z);

	Push(mat);
}*/

void MatrixStack::PushScaling(float x, float y, float z)
{
	Mat4x4 mat;

	mat.MakeScaling(x, y, z);

	Push(mat);
}

void MatrixStack::Pop()
{
	Assert(m_depth > 0);

	m_depth--;

	if (m_handler)
		m_handler->OnMatrixUpdate(this, Top());
}

