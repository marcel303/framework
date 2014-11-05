#if defined(FDEBUG)
#define GFX_W 800
#define GFX_H 600
#define GFX_FS false
#else
#define GFX_W 640
#define GFX_H 480
#define GFX_FS false
#endif

#define GFX_DEVICE GraphicsDeviceGL
//#define GFX_DEVICE GraphicsDeviceD3D9

#include <iostream>
#include <stdio.h>
#include <SDL/SDL.h>
#include "Archive.h"
#include "Calc.h"
#include "Engine.h"
#include "EntityBrick.h"
#include "EventManager.h"
#include "GameBrickBuster.h"
#include "GraphicsDevice.h"
//#include "GraphicsDeviceD3D9.h"
#include "GraphicsDeviceGL.h"
#include "Mesh.h"
#include "Frustum.h"
#include "Player.h"
#include "ProcTexcoordMatrix2DAutoAlign.h"
#include "Renderer.h"
#include "ResLoaderPS.h"
#include "ResLoaderSnd.h"
#include "ResLoaderTex.h"
#include "ResLoaderVS.h"
#include "ResMgr.h"
#include "ResShader.h"
#include "Scene.h"
#include "ShapeBuilder.h"
#include "SoundDevice.h"
#include "Stats.h"
#include "SystemDefault.h"

static void ThreadSome(int delay);

static void TestReplication();

template <class T>
void RenderStatValue(StatValue<T>& value, const std::string& name, int x, int y, int w, int h)
{
	static ShShader shader(new ResShader());
	shader->InitDepthTest(false, CMP_EQ);
	shader->Apply(Renderer::I().GetGraphicsDevice());

	Mat4x4 ortho;
	int gfxW = 800;
	int gfxH = 600;
	Mat4x4 ortho0;
	Mat4x4 ortho1;
	Mat4x4 ortho2;
	ortho0.MakeScaling(1.0f, -1.0f, 1.0f);
	ortho1.MakeTranslation(Vec3(-1.0f, -1.0f, 0.0f));
	ortho2.MakeScaling(Vec3(1.0f / gfxW, 1.0f / gfxH, 1.0f));
	ortho = ortho0 * ortho1 * ortho2;

	Renderer::I().MatW().PushI();
	Renderer::I().MatV().PushI();
	Renderer::I().MatP().PushI(ortho);

	const T min = value.CalcMin();
	const T max = value.CalcMax();

	T diff = max - min;

	if (diff == 0) diff = 1;

	//const T last = value.m_values[value.Loop(value.m_position - 1)].m_value;

	glBegin(GL_LINES);
	{
		for (int i = 0; i < value.m_size - 1; ++i)
		{
			const int position1 = value.Loop(value.m_position - value.m_size + i + 0);
			const int position2 = value.Loop(value.m_position - value.m_size + i + 1);

			const int x1 = (i + 0) * w / (value.m_size - 1);
			const int x2 = (i + 1) * w / (value.m_size - 1);

			const T value1 = (diff - (value.m_values[position1].m_value - min)) * h / diff;
			const T value2 = (diff - (value.m_values[position2].m_value - min)) * h / diff;

			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(static_cast<float>(x + x1), static_cast<float>(y + value1), 1.0f);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(static_cast<float>(x + x2), static_cast<float>(y + value2), 1.0f);
		}
	}
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	Renderer::I().MatW().Pop();
	Renderer::I().MatV().Pop();
	Renderer::I().MatP().Pop();
}

int main(int argc, char* argv[])
{
	Calc::Initialize();

	TestReplication();

	char c[2];
	gets(c);

	return 0;
}

void ThreadSome(int delay)
{
	Sleep(delay);
}

class BrickInfo
{
public:
	float px, py, pz;
	float sx, sy, sz;
};

class ButtonState
{
public:
	int m_state1;
	int m_state2;
};

class Edit
{
public:
	Edit()
	{
		m_stop = false;
	}

	void Draw()
	{
		for (size_t i = 0; i < m_bricks.size(); ++i)
		{
			int x1, y1, x2, y2;

			x1 = (int)(m_bricks[i].px - m_bricks[i].sx / 2.0f);
			y1 = (int)(m_bricks[i].pz - m_bricks[i].sz / 2.0f);
			x2 = (int)(m_bricks[i].px + m_bricks[i].sx / 2.0f);
			y2 = (int)(m_bricks[i].pz + m_bricks[i].sz / 2.0f);

			//fixme rect(screen, x1, y1, x2, y2, makecol(255, 0, 0));
		}
	}

	bool m_stop;

	std::vector<BrickInfo> m_bricks;
};

class EditorEventHandler : public EventHandler
{
public:
	EditorEventHandler(Edit& editor) : m_editor(editor)
	{
	}

	virtual void OnEvent(Event& event)
	{
		if (event.type == EVT_QUIT)
		{
			m_editor.m_stop = true;
		}

		if (event.type == EVT_KEY)
		{
			if (event.key.key == IK_q && event.key.state == BUTTON_DOWN)
				EventManager::I().AddEvent(Event(EVT_QUIT));
		}

		if (event.type == EVT_MOUSEBUTTON)
		{
			if (event.mouse_button.button == INPUT_BUTTON1 && event.mouse_button.state == BUTTON_DOWN)
			{
				float px = static_cast<float>(event.mouse_button.x);
				float py = 0.0f;
				float pz = static_cast<float>(event.mouse_button.y);

				float s = 10.0f;

				float sx = s;
				float sy = s;
				float sz = s;

				BrickInfo brick;

				brick.px = px;
				brick.py = py;
				brick.pz = pz;
				brick.sx = sx;
				brick.sy = sy;
				brick.sz = sz;

				m_editor.m_bricks.push_back(brick);
			}
		}
	}

private:
	Edit& m_editor;
};

#if 0
static std::vector<BrickInfo> draw_map()
{
	SysInitialize();

	Edit editor;

	EditorEventHandler eventHandler(editor);

	EventManager::I().AddEventHandler(&eventHandler);

	ButtonState buttons[26 + 3] = { 0 };

	bool stop = false;

	while (!editor.m_stop)
	{
		// TODO: Move code below to DisplayAL.
		for (int i = KEY_A; i <= KEY_Z; ++i)
		{
			int index = i - KEY_A;

			if (key[i])
				buttons[index].m_state2 = 1;
			else
				buttons[index].m_state2 = 0;

			if (buttons[index].m_state1 != buttons[index].m_state2)
			{
				EventManager::I().AddEvent(Event(EVT_KEY, IK_a + index, buttons[index].m_state2));

				buttons[index].m_state1 = buttons[index].m_state2;
			}
		}

		int mouseX = mouse_x;
		int mouseY = mouse_y;

		for (int i = 0; i < 3; ++i)
		{
			int index = i + 26;

			if (mouse_b & (1 << i))
				buttons[index].m_state2 = 1;
			else
				buttons[index].m_state2 = 0;

			if (buttons[index].m_state1 != buttons[index].m_state2)
			{
				EventManager::I().AddEvent(Event(EVT_MOUSEBUTTON, i, buttons[index].m_state2, mouseX, mouseY));

				buttons[index].m_state1 = buttons[index].m_state2;
			}
		}

		EventManager::I().Purge();

		acquire_bitmap(screen);

		clear(screen);

		editor.Draw();

		release_bitmap(screen);
	}

	SysShutdown();

	return editor.m_bricks;
}
#endif

class TestEventHandler : public EventHandler
{
public:
	TestEventHandler() : EventHandler()
	{
		m_stop = false;
	}

	virtual void OnEvent(Event& event)
	{
		if (event.type == EVT_QUIT)
			m_stop = true;
	}

	bool m_stop;
};

void TestReplication()
{
	bool cfg_fullscreen;
	bool cfg_server;
	bool cfg_client;

	printf("S: server\n");
	printf("C: client\n");
	printf("B: client + server\n");

	char c = getc(stdin);

	cfg_fullscreen = false;
	cfg_server = c == 's' || c == 'b';
	cfg_client = c == 'c' || c == 'b';

	Engine::ROLE role;
	bool localConnect = false;

	if (!cfg_server && !cfg_client)
		Assert(0);
	if (cfg_server && !cfg_client)
		role = Engine::ROLE_SERVER;
	if (!cfg_server && cfg_client)
		role = Engine::ROLE_CLIENT;
	if (cfg_server && cfg_client)
	{
		role = Engine::ROLE_SERVER_AND_CLIENT;
		localConnect = true;
	}

	{
		Game* game = new GameBrickBuster();

		System* system = new SystemDefault();

		system->Initialize(game, GFX_W, GFX_H, cfg_fullscreen);

		GraphicsDevice& gfx = *system->GetGraphicsDevice();
		SoundDevice& sfx = *system->GetSoundDevice();

		{
			Engine* engine = system->GetEngine();

			engine->Initialize(role, localConnect);

			TestEventHandler eventHandler;

			EventManager::I().AddEventHandler(&eventHandler);

			bool captured = false;
			bool renderStats = true;

			bool stop = false;

			Timer t;

			while (!eventHandler.m_stop)
			{
				Display* display = gfx.GetDisplay();

				display->Update();

				engine->Update();

				sfx.Update();

				// FIXME: GraphicsDevice/Display relationship is.. awkward..

				Renderer::I().SetGraphicsDevice(&gfx);

				gfx.SceneBegin();
				{
					Player* player = 0;

					if (engine->m_clientClient)
						player = (Player*)engine->m_clientClient->m_clientScene->m_activeEntity.lock().get();

					Mat4x4 matW;
					Mat4x4 matV;
					Mat4x4 matP;

					if (player)
					{
						//Mat4x4 temp;
						//temp.MakeTranslation(player->GetEyePosition());

						matW.MakeIdentity();
						//matV = (temp * player->GetOrientationMat()).CalcInv();
						matV = player->GetCamera().CalcInv();
						//matP.MakePerspectiveLH(player->GetFOV(), player->m_scene->GetRenderer()->GetAspect(), player->m_scene->GetRenderer()->m_camNear, player->m_scene->GetRenderer()->m_camFar);
						matP = player->GetPerspective();

						sfx.SetHeadPosition(player->GetCameraPosition());
						sfx.SetHeadVelocity(player->m_phyObject.m_velocity);
						sfx.SetHeadOrientation(-player->GetCameraOrientation(), -Vec3(0.0f, 1.0f, 0.0f)); // fixme, y mirrored?
					}
					else
					{
						matW.MakeIdentity();
						matV.MakeIdentity();
						matP.MakePerspectiveLH(Calc::mPI / 2.0f, gfx.GetRTH() / float(gfx.GetRTW()), 1.0f, 1000.0f);
					}

					Renderer::I().MatW().Push(matW);
					Renderer::I().MatV().Push(matV);
					Renderer::I().MatP().Push(matP);

					gfx.Clear(BUFFER_ALL, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

					//////

					engine->Render();

					if (renderStats)
					{
						RenderStatValue<int>(Stats::I().m_net.m_packetsSent,      "NET: Packets sent",      0,   0,   295, 45);
						RenderStatValue<int>(Stats::I().m_net.m_packetsReceived,  "NET: Packets received",  300, 0,   295, 45);
						RenderStatValue<int>(Stats::I().m_net.m_bytesSent,        "NET: Bytes sent",        0,   50,  295, 45);
						RenderStatValue<int>(Stats::I().m_net.m_bytesReceived,    "NET: Bytes received",    300, 50,  295, 45);
						RenderStatValue<int>(Stats::I().m_rep.m_bytesReceived,    "REP: Bytes received",    0,   100, 295, 45);
						RenderStatValue<int>(Stats::I().m_rep.m_objectsCreated,   "REP: Objects created",   300, 100, 295, 45);
						RenderStatValue<int>(Stats::I().m_rep.m_objectsDestroyed, "REP: Objects destroyed", 0,   150, 295, 45);
						RenderStatValue<int>(Stats::I().m_rep.m_objectsUpdated,   "REP: Objects updated",   300, 150, 295, 45);
						RenderStatValue<int>(Stats::I().m_rep.m_objectsVersioned, "REP: Objects versioned", 0,   200, 295, 45);
						RenderStatValue<int>(Stats::I().m_gfx.m_fps,              "GFX: FPS",               0,   250, 295, 45);
					}

					////////////
					Renderer::I().MatW().Pop();
					Renderer::I().MatV().Pop();
					Renderer::I().MatP().Pop();
				}
				gfx.SceneEnd();

				gfx.Present();
				////////////
			}

			EventManager::I().RemoveEventHandler(&eventHandler);

			engine->Shutdown();
		}

		system->Shutdown();

		delete system;

		delete game;
	}
}


static float eps = 1e-3f;

class ClipPoly
{
public:
	const static int MAX_VERT = 64;

	Vec3 m_vertices[MAX_VERT];
	int m_vertexCount;

	ClipPoly()
		: m_vertexCount(0)
	{
	}

	void Add(const Vec3&  v)
	{
		m_vertices[m_vertexCount++] = v;
	}

	void SetupFromPlane(const Mx::Plane& plane)
	{
		const Vec3& n = plane.m_normal;

		Vec3 tan1(n[1], n[2], n[0]);
		tan1 -= n * (tan1 * n);
		tan1.Normalize();
		Vec3 tan2 = tan1 % n;

		const float dot1 = n * tan1;
		const float dot2 = n * tan2;
		Assert(fabsf(dot1) < 0.01f);
		Assert(fabsf(dot2) < 0.01f);
		Assert(tan1.CalcSizeSq() != 0.0f);
		Assert(tan2.CalcSizeSq() != 0.0f);

		const Vec3 p = plane.m_normal * plane.m_distance;
		
		const float s = 1000.0f;

		Add(p - tan1 * s - tan2 * s);
		Add(p + tan1 * s - tan2 * s);
		Add(p + tan1 * s + tan2 * s);
		Add(p - tan1 * s + tan2 * s);
	}

	void ClipByPlane_KeepFront(const Mx::Plane& plane)
	{
		bool keepAll = true;

		for (int i = 0; i < m_vertexCount; ++i)
		{
			if (plane * m_vertices[i] < -eps)
			{
				keepAll = false;
				break;
			}
		}

		if (keepAll)
		{
			return;
		}

		ClipPoly temp;

		for (int i = 0; i < m_vertexCount; ++i)
		{
			const int index1 = (i + 0) % m_vertexCount;
			const int index2 = (i + 1) % m_vertexCount;

			const Vec3& v1 = m_vertices[index1];
			const Vec3& v2 = m_vertices[index2];

			const float d1 = plane * v1;
			const float d2 = plane * v2;

			if (d1 >= 0.0f)
			{
				temp.Add(v1);

				if (d2 < 0.0f)
				{
					const float t = - d1 / (d2 - d1);
					temp.Add(v1 + (v2 - v1) * t);
				}
			}
			else
			{
				if (d2 > 0.0f)
				{
					const float t = - d1 / (d2 - d1);
					temp.Add(v1 + (v2 - v1) * t);
				}
			}
		}

		*this = temp;
	}
};

class DebugRender
{
public:
	void DrawTriangle(const Vec3& v1, const Vec3& v2, const Vec3& v3, float r, float g, float b, float a)
	{
		ResVB vb;
		vb.Initialize(&g_alloc, 3, FVF_XYZ | FVF_COLOR);
		vb.SetPosition(0, v1[0], v1[1], v1[2]); vb.SetColor(0, r, g, b, a);
		vb.SetPosition(1, v2[0], v2[1], v2[2]); vb.SetColor(1, r, g, b, a);
		vb.SetPosition(2, v3[0], v3[1], v3[2]); vb.SetColor(2, r, g, b, a);
		GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();
		gfx->SetVB(&vb);
		gfx->SetIB(0);
		gfx->Draw(PT_TRIANGLE_LIST);
	}

	void DrawPoly(const Vec3* v, int vertexCount, float r, float g, float b, float a)
	{
		ResVB vb;
		vb.Initialize(&g_alloc, vertexCount, FVF_XYZ | FVF_COLOR);
		for (int i = 0; i < vertexCount; ++i)
		{
			vb.SetPosition(i, v[i][0], v[i][1], v[i][2]);
			vb.SetColor(i, r, g, b, a);
		}
		GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();
		gfx->SetVB(&vb);
		gfx->SetIB(0);
		gfx->Draw(PT_TRIANGLE_FAN);
	}

	void DrawPolyLine(const Vec3* v, int vertexCount, float r, float g, float b, float a)
	{
		if (vertexCount >= 2)
		{
			ResVB vb;
			vb.Initialize(&g_alloc, vertexCount + 1, FVF_XYZ | FVF_COLOR);
			for (int i = 0; i < vertexCount; ++i)
			{
				vb.SetPosition(i, v[i][0], v[i][1], v[i][2]);
				vb.SetColor(i, r, g, b, a);
			}
			vb.SetPosition(vertexCount, v[0][0], v[0][1], v[0][2]);
			vb.SetColor(vertexCount, r, g, b, a);
			GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();
			gfx->SetVB(&vb);
			gfx->SetIB(0);
			gfx->Draw(PT_LINE_STRIP);
		}
		else
		{
			printf("??\n");
		}
	}
};

DebugRender sDebugRender;
