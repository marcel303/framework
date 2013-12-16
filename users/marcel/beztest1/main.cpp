//#define ALLEGRO_NO_MAGIC_MAIN
#include <allegro.h>
#include <cmath>
#include <vector>
#include "Bezier.h" 
#include "BezierTraveller.h"
#include "Log.h"

class PointSet
{
public:
	void Clear()
	{
		points.clear();
		curves.clear();
	}

	std::vector<Vec2F> points;
	std::vector<BezierCurve> curves;
};

static void HandleTravel(void* obj, BezierTravellerState state, float x, float y)
{
	PointSet* pointSet = (PointSet*)obj;

	pointSet->points.push_back(Vec2F(x, y));
}

static void HandleTangent(void* obj, const BezierCurve& curve)
{
	PointSet* pointSet = (PointSet*)obj;

	pointSet->curves.push_back(curve);
}

static inline void putpixel_blended(BITMAP* bmp, int x, int y, int r2, int g2, int b2, int opacity2)
{
	if (x < 0 || x >= bmp->w || y < 0 || y >= bmp->h)
		return;
	
	int* p = ((int*)bmp->line[y]) + x;
	
	const int c2 = *p;

	const int r1 = getr32(c2);
	const int g1 = getg32(c2);
	const int b1 = getb32(c2);

//	const int opacity2 = opacity;
	const int opacity1 = 255 - opacity2;
	
	const int r = (r1 * opacity1 + r2 * opacity2) >> 8;
	const int g = (g1 * opacity1 + g2 * opacity2) >> 8;
	const int b = (b1 * opacity1 + b2 * opacity2) >> 8;

	*p = makecol32(r, g, b);
}

static void putpixel_aa(BITMAP* bmp, float x, float y, int c)
{
	const float xf = std::floor(x);
	const float yf = std::floor(y);

	const int dx = (x - xf) * 255.0f;
	const int dy = (y - yf) * 255.0f;

	const int x1 = 255 - dx;
	const int y1 = 255 - dy;
	const int x2 = dx;
	const int y2 = dy;

	const int v11 = (x1 * y1) >> 8;
	const int v21 = (x2 * y1) >> 8;
	const int v12 = (x1 * y2) >> 8;
	const int v22 = (x2 * y2) >> 8;

	const int ix = xf;
	const int iy = yf;

	const int r2 = getr32(c);
	const int g2 = getg32(c);
	const int b2 = getb32(c);
	
	putpixel_blended(bmp, ix + 0, iy + 0, r2, g2, b2, v11);
	putpixel_blended(bmp, ix + 1, iy + 0, r2, g2, b2, v21);
	putpixel_blended(bmp, ix + 0, iy + 1, r2, g2, b2, v12);
	putpixel_blended(bmp, ix + 1, iy + 1, r2, g2, b2, v22);
}

static void line_aa(BITMAP* bmp, float x1, float y1, float x2, float y2, int c)
{
	float dx = x2 - x1;
	float dy = y2 - y1;

	const float s = std::sqrt(dx * dx + dy * dy);

	if (s == 0.0f)
		return;

	const int stepCount = std::ceil(s);

	dx /= stepCount;
	dy /= stepCount;

	float x = x1;
	float y = y1;

	for (int i = stepCount; i; --i)
	{
		putpixel_aa(bmp, x, y, c);

		x += dx;
		y += dy;
	}
}

static void Generate(PointSet& pointSet, int startX, int startY)
{
	pointSet.Clear();

	BezierTraveller traveller;
	traveller.Setup(1.0f, HandleTravel, HandleTangent, &pointSet);
	float x = startX;
	float y = startY;
	traveller.Begin(x, y);
	float s1 = 5.0f;
	//float s2 = 100.0f;
	float s2 = 50.0f;
	//float s2 = 10.0f;
	float angle = 0.0f;
	for (int i = 0; i < 40; ++i)
	{
		//float angle = (rand() % 100) / 100.0f * M_PI * 2.0;
		angle += ((rand() % 100) / 100.0f - 0.5f) * M_PI * 2.0 / 2.0f;
		float s = s1 + (s2 - s1) * (rand() % 100) / 100.0f;

		x += sinf(angle) * s;
		y += cosf(angle) * s;

		traveller.Update(x, y);
	}
	traveller.End(traveller.LastLocation_get()[0], traveller.LastLocation_get()[1]);
}

static void Generate(PointSet& pointSet, const std::vector<Vec2F>& coords)
{
	pointSet.Clear();

	if (coords.size() == 0)
		return;

	BezierTraveller traveller;
	traveller.Setup(1.0f, HandleTravel, HandleTangent, &pointSet);
	traveller.Begin(coords[0][0], coords[0][1]);
	for (size_t i = 1; i < coords.size(); ++i)
		traveller.Update(coords[i][0], coords[i][1]);
	traveller.End(traveller.LastLocation_get()[0], traveller.LastLocation_get()[1]);
}

static void Fade(BITMAP* bmp, int amount)
{
	for (int y = 0; y < bmp->h; ++y)
	{
		int* line = (int*)bmp->line[y];
		for (int x = bmp->w; x; --x)
		{
			const int c = *line;
			register int r = getr32(c) - amount;
			register int g = getg32(c) - amount;
			register int b = getb32(c) - amount;
			if (r < 0) r = 0;
			if (g < 0) g = 0;
			if (b < 0) b = 0;
			*line = makecol32(r, g, b);
			line++;
		}
	}
}

static void Render(BITMAP* bmp, PointSet& pointSet)
{
	acquire_bitmap(bmp);

	//Fade(bmp, 10);
	clear(bmp);
	//clear_to_color(bmp, makecol(255, 255, 255));

	for (size_t i = 0; i < pointSet.points.size(); ++i)
		putpixel_aa(bmp, pointSet.points[i][0], pointSet.points[i][1], makecol(255, 255, 255));

#if 1
	for (size_t i = 0; i < pointSet.curves.size(); ++i)
	{
		const BezierCurve& curve = pointSet.curves[i];

		line_aa(bmp, 
			curve.mPoints[0][0], curve.mPoints[0][1],
			curve.mPoints[1][0], curve.mPoints[1][1],
			makecol(0, 0, 255));

		line_aa(bmp, 
			curve.mPoints[2][0], curve.mPoints[2][1],
			curve.mPoints[3][0], curve.mPoints[3][1],
			makecol(255, 0, 0));
	}
#endif

#if 1
	for (size_t i = 0; i < pointSet.curves.size(); ++i)
	{
		const BezierCurve& curve = pointSet.curves[i];

		putpixel_aa(bmp,
			curve.mPoints[0][0],
			curve.mPoints[0][1],
			makecol(255, 0, 255));
	}
#endif
	
	release_bitmap(bmp);
}

int main(int argc, char* argv[])
{
	LOG_DBG("seeding random", 0);
	srand(time(0));

	LOG_DBG("initializing allegro", 0);
	allegro_init();
	
	LOG_DBG("setting gfx mode", 0);
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 1000, 1000, 0, 0);
	
	LOG_DBG("installing keyboard", 0);
	install_keyboard();
	LOG_DBG("installing mouse", 0);
	install_mouse();

	LOG_DBG("creating off-screen buffer", 0);
	//BITMAP* buffer = create_bitmap(250, 250);
	BITMAP* buffer = create_bitmap(SCREEN_W, SCREEN_H);

	clear(buffer);

	std::vector<Vec2F> coords;

	int c = KEY_SPACE;

	bool freemode = false;
	bool freemode2 = false;
	bool stop = false;

	while (!stop)
	{
		if (c == KEY_ESC)
			stop = true;
		if (c == KEY_SPACE)
		{
			PointSet pointSet;

			Generate(pointSet, buffer->w/2, buffer->h/2);

			Render(buffer, pointSet);

			stretch_blit(buffer, screen, 0, 0, buffer->w, buffer->h, 0, 0, SCREEN_W, SCREEN_H);
		}
		if (c == KEY_F)
		{
			show_mouse(screen);
			if (freemode)
				freemode2 = true;
			freemode = true;
			while (key[KEY_F]);
		}

		if (!freemode)
		{
			c = readkey() >> 8;
		}
		else
		{
			if (mouse_b & 1)
			{
				Vec2F coord(mouse_x, mouse_y);
				coords.push_back(coord);
				if (!freemode2)
					while (mouse_b & 1);
			}
			if (mouse_b & 2)
				stop = true;

			PointSet pointSet;

			Generate(pointSet, coords);

			Render(buffer, pointSet);

			textprintf(buffer, font, 0, 0, makecol(255, 255, 255), "pointCount: %lu", coords.size());

			stretch_blit(buffer, screen, 0, 0, buffer->w, buffer->h, 0, 0, SCREEN_W, SCREEN_H);
		}
	}

	destroy_bitmap(buffer);

	allegro_exit();

	return 0;
}
END_OF_MAIN();
