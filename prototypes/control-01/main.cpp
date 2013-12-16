#define ALLEGRO_USE_CONSOLE
#include <allegro.h>
#include <math.h>
#include "boss.h"
#include "SelectionBuffer.h"
#include "types.h"
#include "XiCore.h"
#include "XiPart.h"

#define DEG2RAD(x) ((x) / 360.0f * 2.0f * M_PI)

#define R1 DEG2RAD(5.0f)
#define R2 DEG2RAD(40.0f)
#define DIS_MAX 100.0f
#define DIS_MIN 20.0f
#define PC 1000
#define DEADZONE_SIZE 10.0f
#define MAP_BORDER 10.0f

int sx = 480;
int sy = 320;
int scale = 1;
//int scale = 2;

//

static int g_FpsFrame = 0;
static int g_Fps = 0;

static BOOL c_DrawFPS = TRUE;
static BOOL c_DrawMany = TRUE;
static BOOL c_Vsync = FALSE;
static BOOL c_DrawSelectionBuffer = FALSE;
static BOOL c_StretchBlit = TRUE;
static BOOL c_Blit = TRUE;

//

Vec2 g_Pos;
Vec2 g_PosDelta;
BoundingBox g_MapBoundingBox;

static void TestTriangulation(BITMAP* buffer, SelectionBuffer* sb);

class Particle
{
public:
	Particle()
	{
		IsAlive = FALSE;
		P = Vec2(0.0f, 0.0f);
		V = Vec2(0.0f, 0.0f);
	}

	void Create(Vec2 p, Vec2 v)
	{
		IsAlive = TRUE;
		P = p;
		V = v;
	}

	BOOL IsAlive;
	Vec2 P;
	Vec2 V;
};

class ParticleList
{
public:
	ParticleList()
	{
		Index = 0;
	}

	Particle& Allocate()
	{
		Particle& result =  Particles[Index];

		Index = (Index + 1) % PC;

		return result;
	}

	void Update()
	{
		for (int i = 0; i < PC; ++i)
		{
			if (!Particles[i].IsAlive)
				continue;

			if (!g_MapBoundingBox.Inside(Particles[i].P))
			{
				Particles[i].IsAlive = FALSE;
				continue;
			}

			Particles[i].P += Particles[i].V;
		}
	}

	void Render(BITMAP* buffer)
	{
		for (int i = 0; i < PC; ++i)
		{
			if (!Particles[i].IsAlive)
				continue;

			circle(buffer,
				Particles[i].P[0],
				Particles[i].P[1],
				2,
				makecol(255, 255, 255));
		}
	}

	Particle Particles[PC];
	int Index;
};

class Controller
{
public:
	Controller()
	{
		FixedAngles = true;
		IsDown = false;
		Pos1 = Vec2(0.0f, 0.0f);
		Pos2 = Vec2(0.0f, 0.0f);
	}

	void Update()
	{
		FixedAngles = mouse_b & 2;

		//

		Pos2 += g_PosDelta;

		for (int i = 0; i < 2; ++i)
		{
			if (Pos2[i] < g_MapBoundingBox.Min[i])
			{
				float delta = g_MapBoundingBox.Min[i] - Pos2[i];
				Pos2[i] += delta;
				Pos1[i] += delta;
			}
			if (Pos2[i] > g_MapBoundingBox.Max[i])
			{
				float delta = g_MapBoundingBox.Max[i] - Pos2[i];
				Pos2[i] += delta;
				Pos1[i] += delta;
			}
		}

		// todo: constrained movement along map border focuses beam (pushes pivot away in opposite direction).
		// todo: pivot follow mode to fixate beam direction/concentration.

		if (FixedAngles)
		{
			Pos1 += g_PosDelta;
		}

		{
			Vec2 delta = Pos2 - Pos1;

			float length = delta.Length_get();

			delta.Normalize();

			float offset1 = length - DIS_MAX;
			float offset2 = length - DIS_MIN;

			// Pivot follows ship if ship moves away too far.

			if (offset1 > 0.0f)
			{
				Pos1 += delta * offset1;
			}

#if 0
			// Push pivot away if ship gets too near.

			if (offset2 < 0.0f)
			{
				Pos1 += delta * offset2;
			}
#endif
		}

		GetSprayAngles(&m_SprayAngle1, &m_SprayAngle2);
	}

	float m_SprayAngle1;
	float m_SprayAngle2;

	void GetSprayAngles(float* o_R1, float* o_R2)
	{
		Vec2 direction = Direction_get();

		float r0 = atan2(direction[1], direction[0]);

		float length = direction.Length_get();

		float f = 1.0f - (length - DIS_MIN) / (DIS_MAX - DIS_MIN);

		float r = R1 + (R2 - R1) * f;

		*o_R1 = r0 - r;
		*o_R2 = r0 + r;
	}

	Vec2 GetRandomSprayDir()
	{
		float r1 = m_SprayAngle1;
		float r2 = m_SprayAngle2;

		float t = (rand() & 1023) / 1023.0f;

		float r = r1 + (r2 - r1) * t;

		return Vec2(
			cos(r),
			sin(r));
	}

	void Render(BITMAP* buffer)
	{
		circle(buffer, Pos1[0], Pos1[1], 5, makecol(0, 255, 0));
		circle(buffer, Pos2[0], Pos2[1], 5, makecol(255, 0, 0));

		Vec2 delta = Pos1 - Pos2;

		float length = delta.Length_get();

		delta.Normalize();

		float r1;
		float r2;

		GetSprayAngles(&r1, &r2);

		//r *= 360.0f;

		r1 *= 256.0f / (2.0f * M_PI);
		r2 *= 256.0f / (2.0f * M_PI);

		MATRIX_f matrix;
		float dx1, dy1, dz1;
		float dx2, dy2, dz2;

		get_z_rotate_matrix_f(&matrix, r1);
		apply_matrix_f(&matrix, 1.0f, 0.0f, 0.0f, &dx1, &dy1, &dz1);

		get_z_rotate_matrix_f(&matrix, r2);
		apply_matrix_f(&matrix, 1.0f, 0.0f, 0.0f, &dx2, &dy2, &dz2);

		float sweepSize = 1000.0f;

		int points[6] =
		{
			Pos2[0],
			Pos2[1],
			Pos2[0] + dx1 * sweepSize,
			Pos2[1] + dy1 * sweepSize,
			Pos2[0] + dx2 * sweepSize,
			Pos2[1] + dy2 * sweepSize
		};

		polygon(buffer, 3, points, makecol(7, 31, 7));

		line(buffer, 
			Pos2[0], 
			Pos2[1],
			Pos2[0] + dx1 * sweepSize,
			Pos2[1] + dy1 * sweepSize, makecol(31, 255, 31));

		line(buffer, 
			Pos2[0], 
			Pos2[1],
			Pos2[0] + dx2 * sweepSize,
			Pos2[1] + dy2 * sweepSize, makecol(31, 255, 31));
	}

	Vec2 Direction_get() const
	{
		Vec2 delta = Pos1 - Pos2;

		return delta;
	}

	bool FixedAngles;
	bool IsDown;
	Vec2 Pos1;
	Vec2 Pos2;
};

class Input
{
public:
	Input()
	{
		Digital = true;
		//Digital = false;
		IsDown = false;
	}

	void Initialize(BoundingBox bb)
	{
		BB = bb;

		Mid = (bb.Min + bb.Max) / 2.0f;
	}

	void Update()
	{
		Vec2 pos = Vec2(mouse_x / scale, mouse_y / scale);

		if (mouse_b & 1)
		{
			if (!IsDown)
			{
				if (BB.Inside(pos))
				{
					PosDown = pos;
					IsDown = true;
				}
			}
		}
		else
		{
			if (IsDown)
			{
				IsDown = false;
			}
		}

		PosCurr = pos;
	}

	void Render(BITMAP* buffer)
	{
		int color;

		if (IsDown)
			color = makecol(255, 255, 0);
		else
			color = makecol(0, 0, 255);

		rect(buffer,
			BB.Min[0],
			BB.Min[1],
			BB.Max[0],
			BB.Max[1],
			color);

		if (IsDown)
		{
			line(
				buffer,
				PosDown[0],
				PosDown[1],
				PosCurr[0],
				PosCurr[1],
				makecol(127, 127, 127));

			Vec2 dir = GetMovement();

			line(
				buffer,
				PosDown[0],
				PosDown[1],
				PosDown[0] + dir[0] * 100.0f,
				PosDown[1] + dir[1] * 100.0f,
				makecol(127, 127, 127));
		}
	}

	Vec2 GetMovement()
	{
		if (!IsDown)
			return Vec2(0.0f, 0.0f);

		Vec2 delta = PosCurr - PosDown;

		float length = delta.Length_get();

		length -= DEADZONE_SIZE / 2.0f;

		if (length < 0.0f)
			length = 0.0f;

		delta.Normalize();

		delta *= length;

		delta /= 20.0f; // fixme

		if (Digital)
		{
			for (int i = 0; i < 2; ++i)
			{
				if (delta[i] < 0.0f)
				{
					if (delta[i] < -1.0f)
						delta[i] = -1.0f;
					else
						delta[i] = 0.0f;
				}
				if (delta[i] > 0.0f)
				{
					if (delta[i] > +1.0f)
						delta[i] = +1.0f;
					else
						delta[i] = 0.0f;
				}
			}
		}

		return delta;
	}

	bool Digital;
	bool IsDown;
	Vec2 Mid;
	Vec2 PosDown;
	Vec2 PosCurr;
	BoundingBox BB;
};

#define KEY_ARRAY_SIZE 256 // must be a multiple of 4

class KeyMgr
{
public:
	KeyMgr()
	{
		memset(KeyState, 0x00, KEY_ARRAY_SIZE);
		memset(KeyDown, 0x00, KEY_ARRAY_SIZE);
		memset(KeyUp, 0x00, KEY_ARRAY_SIZE);
	}

	void Update()
	{
		int n = KEY_ARRAY_SIZE >> 2;

		int* src = (int*)key;
		int* dstS = (int*)KeyState;
		int* dstD = (int*)KeyDown;
		int* dstU = (int*)KeyUp;

		for (int i = 0; i < n; ++i)
		{
			int state = src[i];
			int change = state ^ dstS[i];
			int down = state & change;
			int up = ~state & change;

			dstS[i] = state;
			dstD[i] = down;
			dstU[i] = up;
		}
	}

	char KeyState[KEY_ARRAY_SIZE];
	char KeyDown[KEY_ARRAY_SIZE];
	char KeyUp[KEY_ARRAY_SIZE];
};

static void HandleFps()
{
	g_Fps = g_FpsFrame;
	g_FpsFrame = 0;
}

int main(int argc, char* argv[])
{
	allegro_init();
	set_color_depth(32);
	//set_gfx_mode(GFX_AUTODETECT_WINDOWED, 960, 640, 0, 0);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, sx * scale, sy * scale, 0, 0);
	//set_gfx_mode(GFX_AUTODETECT_FULLSCREEN, 800, 600, 0, 0);
	install_keyboard();
	install_mouse();
	install_timer();
	install_int(HandleFps, 1000);
	show_mouse(screen);

	BITMAP* buffer = create_bitmap(sx, sy);
	KeyMgr keyMgr;

	g_MapBoundingBox.Min = Vec2(MAP_BORDER, MAP_BORDER);
	g_MapBoundingBox.Max = Vec2(sx - MAP_BORDER, sy - MAP_BORDER);

	Input input;
	Controller controller;
	ParticleList particles;
	
	input.Initialize(
		BoundingBox(
			Vec2(sx / 2.0f - 50.0f, sy / 2.0f - 50.0f),
			Vec2(sx / 2.0f + 50.0f, sy / 2.0f + 50.0f)));

	g_Pos = Vec2(sx / 2.0f, sy / 2.0f);

	Xi::Core boss;

	// Create shapes.
	Shape_Polygon shp_Core;
	shp_Core.Make_Rect(Vec2(-10.0f, -10.0f), Vec2(+10.0f, +10.0f));
	shp_Core.Finalize();

	Shape_Polygon shp_Arm;
	shp_Arm.Outline.push_back(Vec2(0.0f, -10.0f));
	shp_Arm.Outline.push_back(Vec2(20.0f, -10.0f));
	shp_Arm.Outline.push_back(Vec2(25.0f, -2.0f));
	shp_Arm.Outline.push_back(Vec2(100.0f, -2.0f));
	shp_Arm.Outline.push_back(Vec2(80.0f, 0.0f));
	shp_Arm.Outline.push_back(Vec2(70.0f, +2.0f));
	shp_Arm.Outline.push_back(Vec2(0.0f, +2.0f));
	shp_Arm.Finalize();

	Shape_Polygon shp_Vulcan;
	shp_Vulcan.Make_Rect(Vec2(-1.0f, 0.0f), Vec2(+1.0f, 6.0f));
	shp_Vulcan.Finalize();

	Shape_Polygon shp_Shield;
	shp_Shield.Outline.push_back(Vec2(-15.0f, -15.0f));
	shp_Shield.Outline.push_back(Vec2(0.0f, -8.0f));
	shp_Shield.Outline.push_back(Vec2(2.0f, 0.0f));
	shp_Shield.Outline.push_back(Vec2(0.0f, +8.0f));
	shp_Shield.Outline.push_back(Vec2(-15.0f, +15.0f));
	shp_Shield.Outline.push_back(Vec2(0.0f, 0.0f));
	shp_Shield.Finalize();

	// todo: use boss template format, using weak linked objects. resolve at compile time.

	// Create part templates.
	Xi::Part tpl_Core;
	tpl_Core.Shape = shp_Core;
	Xi::Part tpl_Arm;
	tpl_Arm.Shape = shp_Arm;
	Xi::Part tpl_Vulcan;
	tpl_Vulcan.Shape = shp_Vulcan;
	tpl_Vulcan.PartType = Xi::PartType_Weapon;
	Xi::Part tpl_Shield;
	tpl_Shield.Shape = shp_Shield;

	// Instantiate parts.
	Xi::Part* seg_Core = tpl_Core.Copy();
	Xi::Part* seg_Arm1 = tpl_Arm.Copy();
	Xi::Part* seg_Shield1 = tpl_Shield.Copy();
	Xi::Part* seg_Arm2 = tpl_Arm.Copy();
	Xi::Part* seg_Shield2 = tpl_Shield.Copy();
	Xi::Part* seg_Vulcan1[5];
	Xi::Part* seg_Vulcan2[5];

	for (int i = 0; i < 5; ++i)
	{
		seg_Vulcan1[i] = tpl_Vulcan.Copy();
		seg_Vulcan2[i] = tpl_Vulcan.Copy();
	}

	// Instantiate links.
	if (0)
	for (int i = 0; i < 5; ++i)
	{
		Xi::Link* lnk_Arm1 = new Xi::Link();
		lnk_Arm1->Position = Vec2(30.0f + i * 10.0f, -2.0f);
		lnk_Arm1->RotationMin = M_PI;
		lnk_Arm1->RotationMax = M_PI;
		lnk_Arm1->Part = seg_Vulcan1[i];
		seg_Arm1->Links.push_back(lnk_Arm1);

		Xi::Link* lnk_Arm2 = new Xi::Link();
		lnk_Arm2->Position = Vec2(30.0f + i * 10.0f, -2.0f);
		lnk_Arm2->RotationMin = M_PI;
		lnk_Arm2->RotationMax = M_PI;
		lnk_Arm2->Part = seg_Vulcan2[i];
		seg_Arm2->Links.push_back(lnk_Arm2);
	}

	// Link parts.
	Xi::Link* lnk_Arm1 = new Xi::Link();
	lnk_Arm1->Position = Vec2(100.0f, 0.0f);
	lnk_Arm1->RotationMin = 0.0f;
	lnk_Arm1->RotationMax = M_PI / 4.0f;
	seg_Arm1->Links.push_back(lnk_Arm1);

	Xi::Link* lnk_Arm2 = new Xi::Link();
	lnk_Arm2->Position = Vec2(100.0f, 0.0f);
	lnk_Arm2->RotationMin = 0.0f;
	lnk_Arm2->RotationMax = M_PI / 4.0f;
	seg_Arm1->Links.push_back(lnk_Arm2);

	Xi::Link* lnk_Core1 = new Xi::Link();
	lnk_Core1->Position = Vec2(-10.0f, 0.0f);
	lnk_Core1->Mirrored = true;
	lnk_Core1->RotationMin = -1.0f;
	lnk_Core1->RotationMax = +1.0f;
	lnk_Core1->Part = seg_Arm1;
	Xi::Link* lnk_Core2 = new Xi::Link();
	lnk_Core2->Position = Vec2(+10.0f, 0.0f);
	lnk_Core2->Mirrored = false;
	lnk_Core2->RotationMin = -1.0f;
	lnk_Core2->RotationMax = +1.0f;
	lnk_Core2->Part = seg_Arm2;

	seg_Core->Links.push_back(lnk_Core1);
	seg_Core->Links.push_back(lnk_Core2);

	boss.Root->RotationMin = -M_PI * 2.0f * 1000.0f;
	boss.Root->RotationMax = +M_PI * 2.0f * 1000.0f;
	boss.Root->Part = seg_Core;

#if 1
	float s = 1.0f / 15.0f;
	seg_Arm1->RotationSpeed = 0.01f * s;
	seg_Arm2->RotationSpeed = 0.01f * s;
	seg_Shield1->RotationSpeed = 0.05f * s;
	seg_Shield2->RotationSpeed = 0.05f * s;
	boss.Root->Part->RotationSpeed = 0.001f * s;
#endif

	SelectionBuffer selectionBuffer;

	while (!key[KEY_ESC])
	{
		keyMgr.Update();

#define INVERT(x) { x = !x; printf("Changed " # x ## " to %s\n", x ? "true" : "false"); }

		if (keyMgr.KeyDown[KEY_F1])
			INVERT(c_DrawFPS);
		if (keyMgr.KeyDown[KEY_F2])
			INVERT(c_DrawMany);
		if (keyMgr.KeyDown[KEY_F3])
			INVERT(c_Vsync);
		if (keyMgr.KeyDown[KEY_F4])
			INVERT(c_DrawSelectionBuffer);
		if (keyMgr.KeyDown[KEY_F5])
			INVERT(c_StretchBlit);
		if (keyMgr.KeyDown[KEY_F6])
			INVERT(input.Digital);
		if (keyMgr.KeyDown[KEY_F7])
			INVERT(c_Blit);

		input.Update();

		static int frame = 0;

		//if ((rand() % 50) == 0)
		if ((rand() % 150) == 0)
		//if ((frame % 50) == 0)
		{
			seg_Arm1->RotationSpeed *= -1.0f;
			seg_Arm2->RotationSpeed *= -1.0f;
		}

		if ((rand() % 50) == 0)
		{
			seg_Shield1->RotationSpeed *= -1.0f;
			seg_Shield2->RotationSpeed *= -1.0f;
		}

		frame++;

		g_PosDelta = input.GetMovement() * 2.0f;

#if 1
		float ks = 3.0f;

		if (key[KEY_LEFT])
			g_PosDelta[0] -= ks;
		if (key[KEY_RIGHT])
			g_PosDelta[0] += ks;
		if (key[KEY_UP])
			g_PosDelta[1] -= ks;
		if (key[KEY_DOWN])
			g_PosDelta[1] += ks;
#endif

		g_Pos += g_PosDelta;

		controller.Update();

		// Render selection buffer.

		selectionBuffer.Clear();

		boss.RenderSB(&selectionBuffer);

		// Update boss.

		boss.Update();

		// Test line based selection query.

		std::vector<void*> hits = g_SelectionMap.Query_Line(&selectionBuffer, Vec2(0.0f, sy / 2.0f), Vec2(sx, sy / 2.0f));

		for (int i = 0; i < hits.size(); ++i)
		{
			Xi::Part* part = (Xi::Part*)hits[i];

			part->Hit();
		}

		// Test point based selection query.

		for (int i = 0; i < PC; ++i)
		{
			if (!particles.Particles[i].IsAlive)
				continue;

			CD_TYPE c = selectionBuffer.Get(
				particles.Particles[i].P[0],
				particles.Particles[i].P[1]);

			if (c != 0)
			{
				void* p = g_SelectionMap.Get(c);

				Xi::Part* part = (Xi::Part*)p;

				part->Hit();

				particles.Particles[i].IsAlive = FALSE;
			}
		}

		// Update particle system.

		//if (frame % 2 == 0)
		{
			Particle& p = particles.Allocate();
			Vec2 v = controller.GetRandomSprayDir() * 20.0f;
			//Vec2 v = controller.GetRandomSprayDir() * 5.0f;
			//Vec2 v = controller.GetRandomSprayDir() * 2.0f;
			//Vec2 v = controller.GetRandomSprayDir() * 1.0f;
			p.Create(controller.Pos2, v);
		}

		particles.Update();

		// Render.

		clear(buffer);

		controller.Render(buffer);

		int nx = 0;
		int ny = 0;

		c_DrawMany = false;

		if (c_DrawMany)
		{
			nx = 4;
			ny = 2;
		}
		else
		{
			nx = 0;
			ny = 0;
		}
		
		for (int x = 0; x <= nx; ++x)
		{
			for (int y = 0; y <= ny; ++y)
			{
				boss.Position.v = Vec2(x * 90.0f + 60.0f, y * 80.0f + 30.0f);

				boss.Render(buffer);
			}
		}

		input.Render(buffer);

		particles.Render(buffer);

		//

		if (c_DrawSelectionBuffer)
		{
			for (int y = 0; y < CD_SY; ++y)
			{
				CD_TYPE* line = selectionBuffer.buffer + y * CD_SX;

				for (int x = 0; x < CD_SX; ++x)
				{
					CD_TYPE c = line[x];

					if (c)
					{
						_putpixel32(buffer, x, y, makecol32(c, c, c));
					}
				}
			}
		}

		if (c_DrawFPS)
		{
			textprintf(buffer, font, 0, 0, makecol(255, 255, 255), "FPS: %d", g_Fps);
		}

		g_FpsFrame++;

		if (c_Vsync)
		{
			vsync();
		}

		//scare_mouse();

		if (c_Blit)
		{
			if (c_StretchBlit)
			{
				stretch_blit(buffer, screen, 0, 0, buffer->w, buffer->h, 0, 0, SCREEN_W, SCREEN_H);
			}
			else
			{
				blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
			}
		}

		//unscare_mouse();
	}

	destroy_bitmap(buffer);

	return 0;
}
END_OF_MAIN();

static void TestTriangulation(BITMAP* buffer, SelectionBuffer* sb)
{
	Vector2dVector a;

	a.push_back( Vec2(0,6));
	a.push_back( Vec2(0,0));
	a.push_back( Vec2(3,0));
	a.push_back( Vec2(4,1));
	a.push_back( Vec2(6,1));
	a.push_back( Vec2(8,0));
	a.push_back( Vec2(12,0));
	a.push_back( Vec2(13,2));
	a.push_back( Vec2(8,2));
	a.push_back( Vec2(8,4));
	a.push_back( Vec2(11,4));
	a.push_back( Vec2(11,6));
	a.push_back( Vec2(6,6));
	a.push_back( Vec2(4,3));
	a.push_back( Vec2(2,6));

	for (size_t i = 0; i < a.size(); ++i)
		a[i] *= 20.0f;

	// allocate an STL vector to hold the answer.

	Vector2dVector result;

	//  Invoke the triangulator to triangulate this polygon.
	Triangulate::Process(a,result);

	// print out the results.
	int tcount = result.size()/3;

	for (int i=0; i<tcount; i++)
	{
		const Vec2 &p1 = result[i*3+0];
		const Vec2 &p2 = result[i*3+1];
		const Vec2 &p3 = result[i*3+2];
		//printf("Triangle %d => (%0.0f,%0.0f) (%0.0f,%0.0f) (%0.0f,%0.0f)\n",i+1,p1[0],p1[1],p2[0],p2[1],p3[0],p3[1]);

		Vec2 p[3] = { p1, p2, p3 };

		if (sb)
		{
			sb->Scan_Triangle(p, 255);
		}

		if (buffer && 0)
		for (int j = 0; j < 3; ++j)
		{
			int i1 = (j + 0) % 3;
			int i2 = (j + 1) % 3;

			line(
				buffer,
				p[i1][0],
				p[i1][1],
				p[i2][0],
				p[i2][1],
				makecol(255, 0, 255));
		}
	}
}
