#pragma once

#include "Mat4x4.h"

class MatrixStack;

class MatrixHandler
{
public:
	virtual void OnMatrixUpdate(MatrixStack* stack, const Mat4x4& mat) = 0;
};

class MatrixStack
{
public:
	MatrixStack();

	const Mat4x4& Top() const;
	int GetDepth() const;

	void SetHandler(MatrixHandler* handler);

	void Push(const Mat4x4& mat);
	void PushI();
	void PushI(const Mat4x4& mat);
	void PushTranslation(float x, float y, float z);
	//void PushRotation(float x, float y, float z);
	void PushScaling(float x, float y, float z);
	void Pop();

private:
	const static int MAX_DEPTH = 64;

	int m_depth;
	Mat4x4 m_stack[MAX_DEPTH];

	MatrixHandler* m_handler;
};
