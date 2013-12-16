#include "Precompiled.h"
#define ALLEGRO_USE_CONSOLE
#define ALLEGRO_NO_MAGIC_MAIN
#include <allegro.h>
#include <math.h>
#include <stdarg.h>
#include <vector>
#include "beztest.h"
#include "Buffer.h"
#include "Shape.h"
#include "ShapeIO.h"
#include "Types.h"
#include "VecRend.h"
#include "VecRend_Allegro.h"

//

int main(int argc, char* argv[])
{
	PointF curve[4] =
	{
		PointF( 0.0,   0.0  ),
		PointF( 100.0, 200.0 ),
		PointF( 300.0, 300.0 ),
		PointF( 400.0, 200.0 ),
    };

    //PointF point = { 35.0, 20.0 };
	PointF point(10.0, 30.0);

//	PointF target = Curve_CalcNearestPoint(curve, point);
	PointF target;

	allegro_init();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 480, 320, 0, 0);
	//set_gfx_mode(GFX_AUTODETECT_WINDOWED, 128, 128, 0, 0);
	//set_gfx_mode(GFX_AUTODETECT_WINDOWED, 64, 64, 0, 0);
	install_keyboard();
	set_display_switch_mode(SWITCH_BACKGROUND);

	Shape shape;

	//ShapeIO::Load("test.vg", shape);

#if 1
	for (int i = 0; i < 5; ++i)
	{
		ShapeItem item;
		item.m_Type = ShapeType_Circle;

		item.m_Circle.x = SCREEN_W / 2;
		item.m_Circle.y = SCREEN_H / 2;
		item.m_Circle.r = 100 / (i + 1);
		item.m_Stroke = 3.0f;
		item.m_Hardness = 2.0f;

		shape.m_Items.push_back(item);
	}
#endif

#if 0
	{
		ShapeItem item;
		item.m_Type = ShapeType_Poly;

		for (int i = 0; i < 10; ++i)
		{
			PointI point;

			point.x = rand() % SCREEN_W;
			point.y = rand() % SCREEN_H;

			item.m_Poly.m_Poly.m_Points.push_back(point);
		}

		shape.m_Items.push_back(item);
	}
#endif
	
#if 0
	{
		ShapeItem item;
		item.m_Type = ShapeType_Poly;

		item.m_Poly.m_Poly.m_Points.push_back(PointI(0, 0));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(10, 0));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(5, 10));

		shape.m_Items.push_back(item);
	}
#endif

#if 0
	{
		ShapeItem item;
		item.m_Type = ShapeType_Poly;

		item.m_Poly.m_Poly.m_Points.push_back(PointI(0, 0));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(10, 0));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(10, 10));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(0, 10));

		item.Offset(20, 20);

		shape.m_Items.push_back(item);
	}
#endif

#if 1
	{
		ShapeItem item;
		item.m_Type = ShapeType_Poly;

		item.m_Poly.m_Poly.m_Points.push_back(PointI(0, 0));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(10, 10));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(0, 30));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(15, 60));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(30, 30));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(20, 10));
		item.m_Poly.m_Poly.m_Points.push_back(PointI(30, 0));
		item.m_FillOpacity = 0.8f;
		item.m_Stroke = 3.0f;
		item.m_Hardness = 2.0f;

		for (size_t i = 0; i < item.m_Poly.m_Poly.m_Points.size(); ++i)
		{
			item.m_Poly.m_Poly.m_Points[i].x += SCREEN_W / 2 - 50;
			item.m_Poly.m_Poly.m_Points[i].y += SCREEN_H / 2 - 50;
		}

		item.Offset(40, 40);

		shape.m_Items.push_back(item);
	}
#endif

#if 1
	shape.Scale(2);
#endif

#if 1
	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		ShapeItem& item = shape.m_Items[i];

		item.m_RGB[0] = (rand() & 4095) / 4095.0f * 255.0f;
		item.m_RGB[1] = (rand() & 4095) / 4095.0f * 255.0f;
		item.m_RGB[2] = (rand() & 4095) / 4095.0f * 255.0f;

		item.m_FillColor = 127;
		item.m_LineColor = 255;
	}
#endif

	int frame = 0;

	while (!key[KEY_ESC])
	{
#if 1
	for (size_t i = 0; i < shape.m_Items.size(); ++i)
	{
		ShapeItem& item = shape.m_Items[i];

		item.m_RGB[0] = (rand() & 4095) / 4095.0f * 255.0f;
		item.m_RGB[1] = (rand() & 4095) / 4095.0f * 255.0f;
		item.m_RGB[2] = (rand() & 4095) / 4095.0f * 255.0f;

		item.m_FillColor = 61;
		item.m_LineColor = 255;
	}
#endif

//		ShapeIO::SaveVG(shape, "test.vg");

		Buffer* buffer = VecRend_CreateBuffer(shape, 3, DrawMode_Regular);
		//Buffer* buffer = VecRend_CreateBuffer(shape, 1, DrawMode_Regular);
		BITMAP* tex = VecRend_ToBitmap(buffer);
		delete buffer;
		
		//vsync();

		acquire_screen();

		//clear_to_color(screen, makecol(127, 127, 127));

		blit(tex, screen, 0, 0, 0, 0, tex->w, tex->h);

		destroy_bitmap(tex);

		textprintf(screen, font, 0, SCREEN_H - 20, makecol(127, 0, 0), "iteration: %d", frame);
		
		//

#if 0
		PointF target;
		
		for (int i = 0; i < 1; ++i)
		{
			target = Curve_CalcNearestPoint(curve, point);
		}

		circle(screen, point.x, point.y, 3, makecol(0, 255, 0));
		circle(screen, target.x, target.y, 3, makecol(0, 0, 255));

		for (float t = 0.0f; t <= 1.0f; t += 1.0f / 1000.0f)
		{
			// (1-t)^1*p1 + (1-t)^2*p2 + t^2*p3 + t^1*p4

			float t1 = 1.0f - t;
			float t2 = t;

			float x =
				1.0f * curve[0].x * t1 * t1 * t1 +
				3.0f * curve[1].x * t1 * t1 * t2 +
				3.0f * curve[2].x * t2 * t2 * t1 +
				1.0f * curve[3].x * t2 * t2 * t2;

			float y =
				1.0f * curve[0].y * t1 * t1 * t1 +
				3.0f * curve[1].y * t1 * t1 * t2 +
				3.0f * curve[2].y * t2 * t2 * t1 +
				1.0f * curve[3].y * t2 * t2 * t2;

			putpixel(screen, x, y, makecol(255, 255, 255));
		}
#endif

		release_screen();

		point.x += +1.0f;
		point.y += +0.5f;

		//

		frame++;
	}

	return 0;
}
END_OF_MAIN();
