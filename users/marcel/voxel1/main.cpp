#include "framework.h"
#include "Mat4x4.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>

#if 0
	#define MAP_SX 64
	#define MAP_SY 64
	#define MAP_SZ 32
#elif 1
	#define MAP_SX 64
	#define MAP_SY 64
	#define MAP_SZ 64
#else
	#define MAP_SX 16
	#define MAP_SY 16
	//#define MAP_SZ 16
	#define MAP_SZ 16
#endif

#ifdef VX_DEBUG
#define LOG(x, ...) { printf(x, __VA_ARGS__); printf("\n"); }
#else
#define LOG(x, ...)
#endif

static float g_FpsTimer = 0.f;
static int g_FpsFrame = 0;
static int g_Fps = 0;

struct Voxel
{
	char IsSolid;
};

struct Map
{
	Voxel m_Voxels[MAP_SX][MAP_SY][MAP_SX];
};

static void FillMap(Map* map)
{
	for (int x = 0; x < MAP_SX; ++x)
	{
		for (int y = 0; y < MAP_SY; ++y)
		{
			for (int z = 0; z < MAP_SZ; ++z)
			{
				int isSolid;

				//

				float sx = (x - (MAP_SX - 1.0f) / 2);
				float sy = (y - (MAP_SY - 1.0f) / 2);
				float sz = (z - (MAP_SZ - 1.0f) / 2);

			#if 0
				sx *= 2.0f * M_PI;
				sy *= 2.0f * M_PI;
				sz *= 2.0f * M_PI;

				sx /= 10.0f;
				sy /= 10.0f;
				sz /= 10.0f;

				float s = cos(sx) + cos(sy) + cos(sz);
			#else
				float d = sqrtf(sx * sx + sy * sy + sz * sz);
				
				//d *= sin(d);

				float s = 5.0f - d;
			#endif

				isSolid = s <= 0.0f;

				//

			#if 1
				//isSolid = FALSE;
				isSolid |= (rand() % 10) == 0;
				//isSolid |= (rand() % 20) == 0;
				isSolid &= (rand() % 5) == 0;
			#endif

				//

				map->m_Voxels[x][y][z].IsSolid = isSolid;
			}
		}
	}
}

#include "Types.h"
#include "Drawing.h"
#include "BaseTests.h"
#include "KdTree.h"
#include "Helpers.h"

class View
{
public:
	View()
	{
		Initialize();
	}

	void Initialize()
	{
		m_Sx = 0;
		m_Sy = 0;
		m_Dir = 0;
	}

	void SetSize(int sx, int sy)
	{
		delete[] m_Dir;
		m_Dir = 0;

		m_Sx = sx;
		m_Sy = sy;

		if (sx * sy == 0)
			return;

		m_Dir = new Vec3[sx * sy];

		Calculate();
	}

	void Calculate()
	{
		for (int x = 0; x < m_Sx; ++x)
		{
			for (int y = 0; y < m_Sy; ++y)
			{
				Vec3 dir;

				dir[0] = (x / (m_Sx - 1.0f) - 0.5f) * 2.0f;
				dir[1] = (y / (m_Sy - 1.0f) - 0.5f) * 2.0f;
				dir[2] = 1.0f;

				dir.Normalize();

				int index = CalcIndex(x, y);

				m_Dir[index] = dir;
			}
		}
	}

	inline const Vec3& GetDir(int x, int y) const
	{
		int index = CalcIndex(x, y);

		return m_Dir[index];
	}

	inline int CalcIndex(int x, int y) const
	{
		return x + y * m_Sx;
	}

	int m_Sx;
	int m_Sy;
	Vec3* m_Dir;
	Mat4x4 m_Matrix;
};

static void IntersectHandler(const KdNode* node)
{
	DrawNode(node);

	//rest(2000);
	//rest(20);
}

static void TestAACC(KdTree* tree)
{
	float step = 1.0f;

	for (float x = step / 2.0f; x < MAP_SX; x += step)
	{
		for (float y = step / 2.0f; y < MAP_SY; y += step)
		{
			for (float z = step / 2.0f; z < MAP_SZ; z += step)
			{
				SimdVec pos(x, y, z);

				KdNode* node = Locate(tree, pos);

			#if 1
				Assert(node);

				Assert(x >= node->m_Extents.m_Min[0]);
				Assert(y >= node->m_Extents.m_Min[1]);
				Assert(z >= node->m_Extents.m_Min[2]);

				Assert(x <= node->m_Extents.m_Max[0]);
				Assert(y <= node->m_Extents.m_Max[1]);
				Assert(z <= node->m_Extents.m_Max[2]);
			#endif
			}
		}
	}
}

static void DrawAACC(KdTree* tree)
{
	Draw_BeginScene();

//	for (int i = 1; i < 10; ++i)
	{
		DrawNodes(tree, 0);
	}

	Draw_EndScene();
}

static void DrawHeli(KdTree* tree)
{
	Vec3 pos(
		MAP_SX / 2 + 0.5f,
		MAP_SY / 2 + 0.5f,
		MAP_SZ / 2);
	Vec3 dir;

	Vec3 mousePos = Vec3(mouse.x, mouse.y, pos[2] * DRAW_SCALE);

	dir = mousePos - pos * DRAW_SCALE;

	dir.Normalize();

#if 0
	LOG("Intersect: Ray: %f %f %f, %f %f %f",
		pos.m_V[0],
		pos.m_V[1],
		pos.m_V[2],
		dir.m_V[0],
		dir.m_V[1],
		dir.m_V[2]);
#endif

	IntersectResult result;

	Draw_BeginScene();

	//DrawNodes(tree, 10.0f);

	Draw_Circle(pos[0], pos[1], 1.0f);

	Draw_Line(pos[0], pos[1], pos[0] + dir[0] * 4.0f, pos[1] + dir[1] * 4.0f);

	if (Intersect(tree, pos, dir, IntersectHandler, &result))
	{
		Assert(result.m_Node->IsLeaf_get());

		DrawNode(result.m_Node);
	}

	Draw_EndScene();
}

static View g_VoxiView;

static void DrawVoxi(KdTree* tree, Vec3 pos, Vec3 rot)
{
	g_DrawStretch = TRUE;
	//g_DrawStretch = FALSE;

	Draw_BeginScene();

	uint32_t* buffer = Draw_GetBuffer();

	const uint32_t backColor = makecol(0, 0, 255);

	for (int y = 0; y < g_BufferSy; ++y)
	{
		uint32_t* line = buffer + y * g_BufferSx;

		for (int x = 0; x < g_BufferSx; ++x)
		{
			Vec3 dir = g_VoxiView.GetDir(x, y);

			dir = g_VoxiView.m_Matrix.Mul3(dir);

		#if 0
			LOG("Intersect: Ray: %f %f %f, %f %f %f",
				pos.m_V[0],
				pos.m_V[1],
				pos.m_V[2],
				dir.m_V[0],
				dir.m_V[1],
				dir.m_V[2]);
		#endif

			IntersectResult result;

			if (Intersect(tree, pos, dir, 0, &result))
			{
				Assert(result.m_Node->IsLeaf_get());

			#if 0
				LOG("Intersect: %d %d %d, %d %d %d",
					iResult.m_Node->m_Extents.m_Min.m_V[0],
					iResult.m_Node->m_Extents.m_Min.m_V[1],
					iResult.m_Node->m_Extents.m_Min.m_V[2],
					iResult.m_Node->m_Extents.m_Size.m_V[0],
					iResult.m_Node->m_Extents.m_Size.m_V[1],
					iResult.m_Node->m_Extents.m_Size.m_V[2]);
			#endif

			#if 0
				LOG("Intersect=X=%d, Y=%d, Z=%d", mid.m_X, mid.m_Y, mid.m_Z);
			#endif

			#if 0
				LOG("Intersect=Y=%d", mid.m_Y);
			#endif

			#if 0
				int rgb[3] =
				{
					int((result.m_Normal.m_X + 1.0f) / 2.0f * 255.0f),
					int((result.m_Normal.m_Y + 1.0f) / 2.0f * 255.0f),
					int((result.m_Normal.m_Z + 1.0f) / 2.0f * 255.0f)
				};

				*line = makecol(rgb[0], rgb[1], rgb[2]);
			#else
				int v = int(result.m_Distance * 10.0f);

				if (v < 0)
					v = 0;
				if (v > 255)
					v = 255;

				*line = makecol(0, v, 0);
			#endif
			}
			else
			{
				*line = backColor;
			}

			line++;
		}
	}

	Draw_EndScene();

	g_DrawStretch = FALSE;
}

static void HandleFps()
{
	g_Fps = g_FpsFrame;
	g_FpsFrame = 0;
}

int main(int arg, char* argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(640, 480))
		return -1;

	//

	Draw_Init(640, 480);

	//

	LOG("Instantiating map");

	Map* map = new Map();

	LOG("Filling map");

	FillMap(map);

	LOG("Creating tree");

	KdTree* tree = CreateTree(map);

	PrintStats(tree);

	LOG("Optimizing tree");

	Optimize(tree, map);

	PrintStats(tree);

	// ----------

	//PrintNodes(tree);

	for (int i = 0; i < 0; ++i)
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			DrawAACC(tree);
		}
		framework.endDraw();
	}

	//TestAACC(tree);

	// ----------

#if defined(DEBUG)
	Draw_Init(160, 120);
#else
	Draw_Init(320, 240);
#endif

	//g_DrawStretch = TRUE;

	g_VoxiView.SetSize(g_BufferSx, g_BufferSy);

	Vec3 pos(
		MAP_SX / 2.0f,
		MAP_SY / 2.0f,
		MAP_SZ / 2.0f);
	Vec3 posV(0.0f, 0.0f, 0.0f);

	Vec3 rot(0.0f, 0.0f, 0.0f);
	Vec3 rotV(0.0f, 0.0f, 0.0f);

	enum VisMode
	{
		VisMode_Heli,
		VisMode_Voxi
	};

	VisMode visMode = VisMode_Voxi;

	while (!keyboard.isDown(SDLK_ESCAPE))
	{
		framework.process();
		
		// input

		if (keyboard.isDown(SDLK_1))
		{
			Draw_Init(MAP_SX * DRAW_SCALE, MAP_SY * DRAW_SCALE);

			visMode = VisMode_Heli;
		}
		if (keyboard.isDown(SDLK_2))
		{
			//Draw_Init(150, 150);

			visMode = VisMode_Voxi;
		}

		float accel = 0.05f;

		Vec3 posA(0.0f, 0.0f, 0.0f);

		if (keyboard.isDown(SDLK_LEFT))
			posA[0] -= accel;
		if (keyboard.isDown(SDLK_RIGHT))
			posA[0] += accel;
		if (keyboard.isDown(SDLK_a))
			posA[1] -= accel;
		if (keyboard.isDown(SDLK_z))
			posA[1] += accel;
		if (keyboard.isDown(SDLK_DOWN) || (mouse.isDown(BUTTON_RIGHT)))
			posA[2] -= accel;
		if (keyboard.isDown(SDLK_UP) || (mouse.isDown(BUTTON_LEFT)))
			posA[2] += accel;

		int dx = mouse.dx;
		int dy = mouse.dy;

	#if 0
		rot[0] += 0.1f;
		rot[1] += 0.1f;
		rot[2] += 0.1f;
	#endif

		rotV[0] += -dy / 20.0f;
		rotV[1] += +dx / 20.0f;

		// logic

		Mat4x4 mat;
		Mat4x4 matRX;
		Mat4x4 matRY;
		matRX.MakeRotationX(-rot[0]);
		matRY.MakeRotationY(-rot[1]);
		mat = matRY * matRX;

		Mat4x4 matInv;
		Mat4x4 matRXInv;
		Mat4x4 matRYInv;
		matRXInv.MakeRotationX(+rot[0]);
		matRYInv.MakeRotationY(+rot[1]);
		matInv = matRXInv * matRYInv;

		posA = g_VoxiView.m_Matrix.Mul4(posA);

		if (false)
		{
			// gravity
			KdNode* node = Locate(tree, ToSimdVec(pos));
			if (node == 0 || node->m_BB.Inside(ToSimdVec(pos)) == false)
				posA[1] += accel * 0.2f;
		}

		posV = posV + posA;
		//pos = pos + posV;
		rot = rot + rotV * framework.timeStep;

		posV = posV * powf(0.1f, framework.timeStep);
		rotV = rotV * powf(0.2f, framework.timeStep);

		//Vec3 dir(0.0f, 0.0f, 1.0f);
		Vec3 dir = posV;

		//pos = pos + dir * speed;

		for (int i = 0; i < 3; ++i)
		{
			Vec3 temp = pos;

			//temp[i] += posS[i];
			temp[i] += dir[i];

			KdNode* node = Locate(tree, ToSimdVec(temp));

			if (!node || !node->m_BB.Inside(ToSimdVec(temp)) || node->m_IsEmpty)
				pos = temp;
			else
				posV[i] = 0.0f;
		}

		g_VoxiView.m_Matrix = mat;

		framework.beginDraw(0, 0, 0, 0);
		{
			switch (visMode)
			{
			case VisMode_Heli:
				DrawHeli(tree);
				break;

			case VisMode_Voxi:
				DrawVoxi(tree, pos, rot);
				break;
			}

			setColor(colorWhite);
			drawText(4, 4, 12, +1, +1, "FPS: %d", g_Fps);
		}
		framework.endDraw();

		g_FpsFrame++;
		
		g_FpsTimer += framework.timeStep;
		
		if (g_FpsTimer >= 1.f)
		{
			g_FpsTimer = 0.f;
			
			HandleFps();
		}
	}

	delete tree;

	delete map;

	framework.shutdown();
	
	return 0;
}
