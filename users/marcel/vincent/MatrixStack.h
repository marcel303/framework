#ifndef __MATRIXSTACK_H__
#define __MATRIXSTACK_H__

#include <stack>
#include "Matrix.h"

class MatrixStack
{
public:
	MatrixStack();
	~MatrixStack();

	const Matrix& Top();
	void Push(const Matrix& matrix);
	void PushTranslation(const Vector& translation);
	void PushRotation(const Vector& rotation);
	void PushScaling(const Vector& scale);
	void Pop();

private:
	std::stack<Matrix> m_stack;
};

#endif
