#pragma once

#include <allegro.h>
#include "types.h"

#define RENDERER_STACK_DEPTH 10

class Renderer
{
public:
	Renderer()
	{
		MatrixStack[0] = identity_matrix_f;
		MatrixStackDepth = 0;
	}

	void PushT(const Vec2& pos)
	{
		MATRIX_f mat;

		get_translation_matrix_f(&mat, pos[0], pos[1], 0.0f);

		PushMul(mat);
	}

	void PushR(float angle)
	{
		MATRIX_f mat;

		get_z_rotate_matrix_f(&mat, RAD2ALLEG(angle));

		PushMul(mat);
	}

	void PushS(float x, float y)
	{
		MATRIX_f mat;

		get_scaling_matrix_f(&mat, x, y, 1.0f);

		PushMul(mat);
	}

	void PushMul(MATRIX_f& mat)
	{
		const MATRIX_f& mold = Get();

		MatrixStackDepth++;

		matrix_mul_f(&mat, &mold, &MatrixStack[MatrixStackDepth]);
	}

	void Pop()
	{
		MatrixStackDepth--;
	}

	const MATRIX_f& Get() const
	{
		return MatrixStack[MatrixStackDepth];
	}

	Vec2 Project(const Vec2& p) const
	{
		const MATRIX_f& mat = Get();

		float xyz[3];

		apply_matrix_f(&mat,
			p[0],
			p[1],
			0.0f,
			&xyz[0],
			&xyz[1],
			&xyz[2]);

		return Vec2(xyz[0], xyz[1]);
	}

	void Poly(BITMAP* buffer, const std::vector<Vec2>& points, int c) const
	{
		for (size_t i = 0; i < points.size(); ++i)
		{
			int index1 = (i + 0) % points.size();
			int index2 = (i + 1) % points.size();

			Line(
				buffer,
				points[index1],
				points[index2],
				c);
		}
	}

	void PolyFill(BITMAP* buffer, const std::vector<Vec2>& points, int c) const
	{
		int iPoints[32];

		// todo
		if (points.size() > 16)
			return;

		for (int i = 0; i < points.size(); ++i)
		{
			//Vec2 p = Project(points[i]);

			iPoints[i * 2 + 0] = points[i][0];
			iPoints[i * 2 + 1] = points[i][1];
		}

		polygon(buffer, points.size(), iPoints, c);
	}

	void Line(BITMAP* buffer, const Vec2& p1, const Vec2& p2, int c) const
	{
		line(
			buffer,
			p1[0],
			p1[1],
			p2[0],
			p2[1],
			c);
	}

	void Circle(BITMAP* buffer, const Vec2& p, float r, int c) const
	{
		//p = Project(p);

		circle(
			buffer,
			p[0],
			p[1],
			r,
			c);
	}

	MATRIX_f MatrixStack[RENDERER_STACK_DEPTH];
	int MatrixStackDepth;
};

extern Renderer g_Renderer;
