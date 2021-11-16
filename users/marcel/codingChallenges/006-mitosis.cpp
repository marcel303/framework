// Daniel Shiffman
// http://codingtra.in
// http://patreon.com/codingtrain
// Code for: https://youtu.be/jxGS3fKPKJA

// instance mode by Naoto Hieda
// framework/c++ port by Marcel Smit

#include "framework.h"

Vec2 random2D()
{
	const float angle = random<float>(0.f, 2.f * float(M_PI));
	return Vec2(cosf(angle), sinf(angle));
}

struct Cell
{
	Vec2 pos = Vec2(random<float>(100, 700), random<float>(100, 700));
	Vec2 vel = random2D() * .1f;

	float r = 120.f;
	Color c = Color(255, 255, 255, 100);

	Cell() { }
	Cell(Vec2 pos, Vec2 vel, float r, Color c);

	std::vector<Cell> mitosis();
	void move();
	void show() const;
	
	static void beginShow();
	static void endShow();
};

Cell::Cell(Vec2 pos, Vec2 vel, float r, Color c)
{
	this->pos = pos;
	this->vel = vel;

	this->r = r;
	this->c = c;
}

std::vector<Cell> Cell::mitosis()
{
	if (r < 10)
		return { };
	
	//pos[0] += random<float>(-r, r);
	
	auto v = random2D();
	v *= r * .1f;
	
	auto vi = v;
	v += vel;
	auto cell0 = Cell(pos, v, r * .8f, c);
	
	vi = -vi;
	vi += vel;
	auto cell1 = Cell(pos, vi, r * .8f, c);
	
	return { cell0, cell1 };
}

void Cell::move()
{
	auto v = random2D();
	
	v *= .1f * r / 60.f;
	
	vel += v;
	
	// var vc = sketch.createVector(-this.pos.x + sketch.width / 2, -this.pos.y + sketch.height / 2);
	// vc.mult(.0001f);
	// this.vel.add(vc);
	
	pos += vel;
	vel *= .9f;
}

void Cell::show() const
{
	setColor(c);
	hqFillCircle(pos[0], pos[1], r);
}

void Cell::beginShow()
{
	hqBegin(HQ_FILLED_CIRCLES);
}

void Cell::endShow()
{
	hqEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 800))
		return -1;

	std::vector<Cell> cells;

	cells.push_back(Cell());
	cells.push_back(Cell());

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT) || cells.empty())
		{
			cells.clear();
			
			cells.push_back(Cell());
			cells.push_back(Cell());
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			for (int i = cells.size() - 1; i >= 0; i--)
			{
				if (random<float>(0.f, 1.f) > .95f)
				{
					auto c = cells[i].mitosis();
					
					for (auto & j : c)
						cells.push_back(j);

					cells.erase(cells.begin() + i);
				}
			}

			for (auto & cell : cells)
				cell.move();
		
			hqSetGradient(GRADIENT_RADIAL, Mat4x4(true)
				.Scale(1/600.f)
				.Translate(-800/2, -800/2, 0),
				colorWhite,
				Color(100, 50, 200),
				COLOR_MUL);
			
			Cell::beginShow();
			{
				for (auto & cell : cells)
					cell.show();
			}
			Cell::endShow();
			
			hqClearGradient();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
