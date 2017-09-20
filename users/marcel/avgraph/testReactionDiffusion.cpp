/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "testBase.h"

extern const int GFX_SX;
extern const int GFX_SY;

struct GridCell
{
	float a;
	float b;

	GridCell()
		: a(1.f)
		, b(0.f)
	{
	}
};

struct Grid
{
	const static int kGridSx = 256;
	const static int kGridSy = 256;

	GridCell cell[kGridSx][kGridSy];

	void randomize()
	{
		for (int i = 0; i < 10; ++i)
		{
			const int size = 3;
			const int x = 1 + size + (rand() % (kGridSx - size * 2 - 2));
			const int y = 1 + size + (rand() % (kGridSy - size * 2 - 2));
			
			for (int cx = x - size; cx <= x + size; ++cx)
			{
				for (int cy = y - size; cy <= y + size; ++cy)
				{
					cell[cx][cy].a = 1.f;
					cell[cx][cy].b = 1.f;
				}
			}
		}
	}

	void laplacian(const int x, const int y, const float kernel[3][3], float & _a, float & _b) const
	{
		const int x1 = x - 1;
		const int y1 = y - 1;

		float a = 0.f;
		float b = 0.f;

		for (int kernelX = 0; kernelX < 3; ++kernelX)
		{
			const int cellX = kernelX + x1;
			
			const float * __restrict kernelCol = kernel[kernelX];
			const GridCell * __restrict cellCol = cell[cellX];
			
			for (int kernelY = 0; kernelY < 3; ++kernelY)
			{
				const int cellY = kernelY + y1;
				
				a += kernelCol[kernelY] * cellCol[cellY].a;
				b += kernelCol[kernelY] * cellCol[cellY].b;
			}
		}

		_a = a;
		_b = b;
	}
};

struct ReactionDiffusion
{
	Grid grid[2];

	int curGridIndex;

	ReactionDiffusion()
		: curGridIndex(0)
	{
	}

	void randomize()
	{
		grid[curGridIndex].randomize();
	}

	void tick(const float dt)
	{
		// @see http://karlsims.com/rd.html
		// aNew = a + (aDiffusion * aLaplacian(a) - a * b * b + feed * (1 - a)) * dt
		// bNew = b + (bDiffusion * bLaplacian(b) + a * b * b - (kill + feed) * b) * dt

		const float aDiffusion = 1.f;
		const float bDiffusion = .3f;
		
		//const float feed = .0550f; const float kill = .0620f;
		const float feed = .0545f; const float kill = .0620f;
		//const float feed = .0367f; const float kill = .0649f;
		//const float feed = .1100f; const float kill = .0523f;
		//const float feed = mouse.x / float(GFX_SX) / 10.f; const float kill = mouse.y / float(GFX_SY) / 10.f;
		
		const float kernel[3][3] =
		{
			{ .05f, .20f, .05f },
			{ .20f, -1.f, .20f },
			{ .05f, .20f, .05f }
		};

		const Grid & curGrid = grid[curGridIndex];
		      Grid & newGrid = grid[1 - curGridIndex];

		for (int x = 1; x < Grid::kGridSx - 1; ++x)
		{
			for (int y = 1; y < Grid::kGridSy - 1; ++y)
			{
				const float a = curGrid.cell[x][y].a;
				const float b = curGrid.cell[x][y].b;
				
				float laplacianA;
				float laplacianB;
				
				curGrid.laplacian(x, y, kernel, laplacianA, laplacianB);

				const float aNew = a + (aDiffusion * laplacianA - a * b * b + feed * (1 - a)) * dt;
				const float bNew = b + (bDiffusion * laplacianB + a * b * b - (kill + feed) * b) * dt;

				newGrid.cell[x][y].a = aNew;
				newGrid.cell[x][y].b = bNew;
			}
		}

		curGridIndex = 1 - curGridIndex;
	}
};

void testReactionDiffusion()
{
	ReactionDiffusion rd;

	//rd.randomize();
	//rd.randomize();
	//rd.randomize();

	do
	{
		framework.process();

		//
		
		if (mouse.isDown(BUTTON_LEFT) && mouse.x >= 0 && mouse.x < Grid::kGridSx && mouse.y >= 0 && mouse.y < Grid::kGridSy)
		{
			rd.grid[rd.curGridIndex].cell[mouse.x][mouse.y].a = 1.f;
			rd.grid[rd.curGridIndex].cell[mouse.x][mouse.y].b = 1.f;
		}

		const float dt = std::min(1.f / 30.f, framework.timeStep) * 10.f;
		
		for (int i = 0; i < 10; ++i)
		{
			rd.tick(dt);
		}
		
		if (keyboard.wentDown(SDLK_r))
			rd.randomize();

		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const Grid & grid = rd.grid[rd.curGridIndex];

			gxBegin(GL_POINTS);
			{
				for (int x = 0; x < Grid::kGridSx; ++x)
				{
					for (int y = 0; y < Grid::kGridSy; ++y)
					{
						const float a = grid.cell[x][y].a;
						const float b = grid.cell[x][y].b;

						gxColor4f(b, 0.f, a / 4.f, 1.f);
						gxVertex2f(x, y);
					}
				}
			}
			gxEnd();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
