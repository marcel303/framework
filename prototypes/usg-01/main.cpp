#define ALLEGRO_USE_CONSOLE
#include <allegro.h>
#include <deque>
#include "Awesomeness.h"
#include "CallBack.h"
#include "Input.h"
#include "Map.h"
#include "MapCam.h"
#include "MiniMap.h"
#include "Objects.h"
#include "Projectile.h"
#include "Renderer.h"
#include "SelectionBuffer.h"
#include "SelectionMap.h"
#include "Types.h"

static int g_AllocationCount = 0;
static int g_DeallocationCount = 0;
static int g_CurrentAllocationCount = 0;

void* Allocate(size_t size)
{
	g_AllocationCount++;
	g_CurrentAllocationCount++;

	return malloc(size);
}

void Deallocate(void* p)
{
	free(p);

	g_DeallocationCount++;
	g_CurrentAllocationCount--;
}

void* operator new(size_t size)
{
	return Allocate(size);
}

void* operator new[](size_t size)
{
	return Allocate(size);
}

void operator delete(void* p)
{
	Deallocate(p);
}

void operator delete[](void* p)
{
	Deallocate(p);
}

enum ScrollerSide
{
	ScrollerSide_Left = 0x1,
	ScrollerSide_Right = 0x2,
	ScrollerSide_Top = 0x4,
	ScrollerSide_Bottom = 0x8
};

class Scroller
{
public:
	Scroller()
	{
	}

	void Setup(Vec2I position, Vec2I size, int borderSize)
	{
		m_Position = position;
		m_Size = size;
		m_BorderSize = borderSize;

		//

		m_Area.Setup(position, size - Vec2I(1, 1));
		m_InnerArea.Setup(
			position + Vec2I(borderSize, borderSize),
			size - Vec2I(borderSize, borderSize) * 2);
	}

	BOOL IsInside(Vec2I pos) const
	{
		if (!m_Area.IsInside(pos))
			return FALSE;
		if (m_InnerArea.IsInside(pos))
			return FALSE;

		return TRUE;
	}

	int Classify(Vec2I pos) const
	{
		if (!IsInside(pos))
			return 0;

		int result = 0;

		if (pos[0] < m_InnerArea.Min_get()[0])
			result |= ScrollerSide_Left;
		if (pos[1] < m_InnerArea.Min_get()[1])
			result |= ScrollerSide_Top;

		if (pos[0] > m_InnerArea.Max_get()[0])
			result |= ScrollerSide_Right;
		if (pos[1] > m_InnerArea.Max_get()[1])
			result |= ScrollerSide_Bottom;

		return result;
	}

	BOOL HandleInput(const InputEvent& e)
	{
		int test = Classify(Vec2I(e.x, e.y));

		if (!test)
			return FALSE;

		Vec2I delta(0, 0);

		if (test & ScrollerSide_Left)
			delta[0] -= 1;
		if (test & ScrollerSide_Top)
			delta[1] -= 1;
		if (test & ScrollerSide_Right)
			delta[0] += 1;
		if (test & ScrollerSide_Bottom)
			delta[1] += 1;

		if (OnScroll.IsSet())
			OnScroll.Invoke(&delta);

		return TRUE;
	}

	CallBack OnScroll;

	Vec2I m_Position;
	Vec2I m_Size;
	int m_BorderSize;

	RectI m_Area;
	RectI m_InnerArea;
};

enum ScreenArea
{
	ScreenArea_None,
	ScreenArea_Map,
	ScreenArea_MiniMap,
	ScreenArea_Scroller
};

static void HandleScroll(void* self, void* arg)
{
	MapCam* cam = (MapCam*)self;
	Vec2I* delta = (Vec2I*)arg;

	cam->SetDesiredCenterLocation(cam->m_Position + *delta);
}

static void Blit(SelectionBuffer* src, BITMAP* dst)
{
	for (int y = 0; y < src->m_Sy; ++y)
	{
		CD_TYPE* sline = src->buffer + y * src->m_Sx;
		int* dline = (int*)dst->line[y];

		for (int x = 0; x < src->m_Sx; ++x)
		{
			CD_TYPE c = sline[x];

			if (c)
			{
				dline[x] = makecol32(c, c, c);
			}
		}
	}
}

// todo: use ObjectList ?

//static std::vector<RectI> DM_Rects;
#define DM_MAX_RECTS 10000

static RectI DM_Rects[DM_MAX_RECTS];
static int DM_RectCount = 0;

static void DM_HandleDirtyRect(void* obj, void* arg)
{
	RectI* rect = (RectI*)arg;

	//DM_Rects.push_back(*rect);
	DM_Rects[DM_RectCount++] = *rect;
}

static void DM_ClearSB(SelectionBuffer* sb)
{
	//for (int i = 0; i < DM_Rects.size(); ++i)
	for (int i = 0; i < DM_RectCount; ++i)
	{
		sb->Clear(DM_Rects[i]);
	}

	//DM_Rects.clear();
	DM_RectCount = 0;
}

int main(int argc, char* argv[])
{
	allegro_init();
	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 480, 320, 0, 0);
	install_keyboard();
	install_mouse();
	set_display_switch_mode(SWITCH_BACKGROUND);

	show_mouse(screen);

	BITMAP* buffer = create_bitmap(SCREEN_W, SCREEN_H);

	Vec2I screenSize(buffer->w, buffer->h);
	Vec2I mapSize(buffer->w * 3, buffer->h * 4);
	//Vec2I mapSize(buffer->w * 2, buffer->h * 2);
	//Vec2I mapSize(buffer->w * 1, buffer->h * 1);

	Map map(Vec2F(mapSize[0], mapSize[1]));

	map.Setup();

	map.m_SelectionBuffer->OnDirtyRect = CallBack(0, DM_HandleDirtyRect);

	//SelectionBuffer sb;
	//sb.SetSize(mapSize[0], mapSize[1]);
	BITMAP* sbBitmap = create_bitmap_ex(32, map.m_SelectionBuffer->m_Sx, map.m_SelectionBuffer->m_Sy);

	MiniMap miniMap(&map, buffer->w - 210, buffer->h - 110);

	MapCam cam;

	cam.Setup(screenSize, mapSize);

	Scroller scroller;

	scroller.Setup(Vec2I(0, 0), screenSize, 4);
	scroller.OnScroll = CallBack(&cam, HandleScroll);

	ScreenArea activeArea = ScreenArea_Map;

	InputMgr inputMgr;

	int frame = 0;

	while (!key[KEY_ESC])
	{
		if (key[KEY_D])
		{
			for (ListNode<IObject*>* node = map.m_UpdateList.m_Head; node; node = node->m_Next)
				if (node->m_Object->m_ObjectType == ObjectType_Ship)
					node->m_Object->SetFlag(ObjectFlag_Dead);
		}

		if ((frame % 100) == 0)
		//if ((frame % 10) == 0)
		{
			Ship* ship = new Ship(&map);

			//ship->Spawn(Vec2F(0.0f, RANDOM(0.0f, mapSize[1])));
			ship->Spawn(Vec2F(RANDOM(0.0f, mapSize[0]), RANDOM(0.0f, mapSize[1])));

			map.m_UpdateList.AddTail(ship);
		}

		inputMgr.Update();

		InputEvent e;

		while (inputMgr.Read(e))
		{
			// todo: create HitTest method.

			// Update focus object.

			switch (e.type)
			{
			case InputType_TouchDown:
				if (scroller.IsInside(Vec2I(e.x, e.y)))
				{
					activeArea = ScreenArea_Scroller;
				}
				else if (miniMap.m_Area.IsInside(Vec2I(e.x, e.y)))
				{
					activeArea = ScreenArea_MiniMap;
				}
				else
				{
					activeArea = ScreenArea_Map;
				}
				break;
			}

			switch (activeArea)
			{
			case ScreenArea_MiniMap:
				{
					e = e.TranslateXY(
						miniMap.m_Area.m_Position[0],
						miniMap.m_Area.m_Position[1]);

					miniMap.HandleInput(e);

					// fixme: let minimap generate positioning events.

					Vec2I camDesired = Vec2I(miniMap.m_Pos[0], miniMap.m_Pos[1]);

					cam.SetDesiredCenterLocation(camDesired);

					//printf("Cam-D: %d, %d\n", camDesired[0], camDesired[1]);
					//printf("Cam-G: %d, %d\n", cam.m_Position[0], cam.m_Position[1]);
				}
				break;

			case ScreenArea_Map:
				{
					Vec2I camMin = cam.MapRect_get().Min_get();

					e = e.TranslateXY(
						-camMin[0],
						-camMin[1]);

					map.HandleInput(e);
				}
				break;

			case ScreenArea_Scroller:
				{
					scroller.HandleInput(e);
				}
				break;
			}
		}

		//

		// todo: use dirty rectangles to clear selection buffer.

		//map.m_SelectionBuffer->Clear();

		map.RenderSB(map.m_SelectionBuffer);

		//

		g_ProjectileMgr.Update(&map);

		map.Update();

		miniMap.Update();

		//

		//bool render = (frame % 100) == 0;
		bool render = true;

		if (render)
		{
			clear(buffer);

	#if 0
			Vec2F camT = -miniMap.m_Pos + Vec2F(buffer->w / 2.0f, buffer->h / 2.0f);

			if (camT[0] > 0.0f)
				camT[0] = 0.0f;
			if (camT[1] > 0.0f)
				camT[1] = 0.0f;
	#else
			Vec2I camMin = cam.m_Position - screenSize / 2;

			Vec2F camT = Vec2F(-camMin[0], -camMin[1]);
	#endif

			g_Renderer.PushT(camT);

			map.Render(buffer);

			g_ProjectileMgr.Render(buffer);

			g_Renderer.Pop();

			miniMap.Render(buffer);

	#if 0
			static int y1 = 0;
			static int y2 = 0;

			Awesome::Draw_FractalLine(buffer, Vec2F(0.0f, y1), Vec2F(SCREEN_W, y2), 100.0f, makecol(255, 0, 255));

			y1 = (y1 + 4) % SCREEN_H;
			y2 = (y2 + 5) % SCREEN_H;
	#endif

	#if 0
			clear(sbBitmap);
			Blit(map.m_SelectionBuffer, sbBitmap);
			set_add_blender(255, 255, 255, 255);
			draw_trans_sprite(buffer, sbBitmap, -camMin[0], -camMin[1]);
			solid_mode();
	#endif
		}

		DM_ClearSB(map.m_SelectionBuffer);

		//

		if (render)
		{
			textprintf(buffer, font, 0, 0, makecol(255, 255, 255), "frame: %d, PC: %d, GOC: %d", frame, g_ProjectileMgr.m_Projectiles.m_Count, map.m_UpdateList.m_Count);
			textprintf(buffer, font, 0, 10, makecol(255, 255, 255), "GA: %d, GD: %d, GC: %d", g_AllocationCount, g_DeallocationCount, g_CurrentAllocationCount);

			vsync();

			scare_mouse();
			blit(buffer, screen, 0, 0, 0, 0, buffer->w, buffer->h);
			unscare_mouse();
		}

		frame++;
	}

	return 0;
}
END_OF_MAIN();
