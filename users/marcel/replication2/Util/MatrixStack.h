#ifndef MATRIXSTACK_H
#define MATRIXSTACK_H
#pragma once

#include "Mat4x4.h"
#include "Types.h"

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
	uint32_t GetDepth() const;

	void SetHandler(MatrixHandler* handler);

	void Push(const Mat4x4& mat);
	void PushI();
	void PushI(const Mat4x4& mat);
	void PushTranslation(float x, float y, float z);
	void PushScaling(float x, float y, float z);
	void Pop();

private:
	const static uint32_t MAX_DEPTH = 64;

	uint32_t m_depth;
	Mat4x4 m_stack[MAX_DEPTH];

	MatrixHandler* m_handler;
};

#endif
