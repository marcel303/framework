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
#include "FileSysMgr.h"
#include "FileSysNative.h"
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
//#include "ScriptState.h"
//#include "ScriptSystem.h"
#include "ShapeBuilder.h"
#include "SoundDevice.h"
#include "Stats.h"
#include "SystemDefault.h"

void ThreadSome(int delay);
void TestGraphics();
void TestArchive();
void TestEngine2();
void TestGraphics2();
void TestGraphics3();
void TestShaders();
void TestShapeBuilder_GetEdges();
void TestShapeBuilder_Patches();
void TestShadowCube();

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
	//TestGraphics();
	TestArchive();
	//for (int i = 0; i < 40; ++i)
		TestEngine2();
	//TestGraphics2();
	//TestGraphics3();
	//TestShaders();
	//TestShapeBuilder_GetEdges();
	//TestShapeBuilder_Patches();
	//TestShadowCube();

	char c[2];
	gets(c);

	return 0;
}

void ThreadSome(int delay)
{
	Sleep(delay);
}

void TestGraphics()
{
	Game game;

	SystemDefault system;

	system.Initialize(&game, GFX_W, GFX_H, GFX_FS);

	GraphicsDevice& gfx = *system.GetGraphicsDevice();
	{
		Mesh mesh;

		ShapeBuilder sb;

		sb.CreateCube(&g_alloc, mesh, FVF_XYZ | FVF_TEX1);

		mesh.GetVB()->tex[0][0][0] = 0.0f;
		mesh.GetVB()->tex[0][0][1] = 0.0f;
		mesh.GetVB()->tex[0][1][0] = 1.0f;
		mesh.GetVB()->tex[0][1][1] = 0.0f;
		mesh.GetVB()->tex[0][2][0] = 0.0f;
		mesh.GetVB()->tex[0][2][1] = 1.0f;

		ShTex tex(new ResTex());
		tex->SetSize(4, 4);
		for (int x = 0; x < 4; ++x)
			for (int y = 0; y < 4; ++y)
				//tex->SetPixel(x, y, Color(x / 3.0f, y / 3.0f, 0.0f, (x == 1 && y == 1) ? 0.0f : 1.0f));
				tex->SetPixel(x, y, Color(255, 255, 255, 255));

		ShShader shader(new ResShader());
		shader->InitDepthTest(true, CMP_LE);
		//shader->InitAlphaTest(true, CMP_GE, 0.5f);
		shader->InitCull(CULL_NONE);

		bool stop = false;

		for (int i = 0; i < 100000 && !stop; ++i)
		{
			SDL_Event e;

			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
					stop = true;
			}

			Renderer::I().SetGraphicsDevice(&gfx);

			gfx.SceneBegin();
			{
				Mat4x4 matW;
				Mat4x4 matV;
				Mat4x4 matP;

				const float x = - 1.0f + cos(i / 100.0f);
				const float y = + 1.0f - cos(i / 1000.0f);

				//matW.MakeRotationY(i / 10000.0f);
				matW.MakeTranslation(x, y, 0.0f);
				//matW.MakeIdentity();
				matV.MakeTranslation(0.0f, 0.0f, 4.0f);
				matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, 0.01f, 100.0f);

				Renderer::I().MatW().Push(matW);
				Renderer::I().MatV().Push(matV);
				Renderer::I().MatP().Push(matP);

				// Convert right handed to left handed.
	#if 0
				matP.MakeScaling(1.0f, 1.0f, -1.0f);
				Renderer::I().MatP().Push(matP);
	#endif

				gfx.Clear(BUFFER_ALL, 0.0f, 0.25f, 0.0f, 0.0f, 1.0f);

				gfx.SetTex(0, tex.get());

				shader->Apply(&gfx);

	#if 0
				scene.UpdateClient();
				scene.Render();
	#else
				Renderer::I().RenderMesh(mesh);
	#endif

				Renderer::I().MatW().Pop();
				Renderer::I().MatV().Pop();
				Renderer::I().MatP().Pop();
			}
			gfx.SceneEnd();

			gfx.Present();
		}
	}

	system.Shutdown();
}

void TestArchive()
{
	Ar ar;

	ar["a"] = 0;
	ar["b"] = 1;
	ar["c"] = 2;

	ar.Begin("s", 0);
	ar["a"] = 0;
	ar["b"] = 1;
	ar["c"] = 2;
	ar.End();

	ar.Begin("s", 1);
	ar["a"] = 0;
	ar["b"] = 1;
	ar["c"] = 2;
	ar.End();

	ar.Write("ar.txt");

	std::string text;
	ar.WriteToString(text);
	Ar ar2;
	ar2.ReadFromString(text);
	ar2.Write("ar2.txt");
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

void TestEngine2()
{
	//std::vector<BrickInfo> bricks = draw_map();

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
		FASSERT(0);
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
			Engine* engine = new Engine(game);

			engine->Initialize(role, localConnect);

			TestEventHandler eventHandler;

			EventManager::I().AddEventHandler(&eventHandler);

			bool captured = false;
			bool renderStats = true;

			bool stop = false;

			Timer t;

#if 1
			while (!eventHandler.m_stop)
			//while (t.Time_get() < 10.0f)
			//while (t.Time_get() < 20.0f)
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

					gfx.Clear(BUFFER_ALL, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

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
#else
			while (!eventHandler.m_stop)
			{
				Display* display = gfx.GetDisplay();

				display->Update();

				engine->Update();

				engine->Render();
			}
#endif

			EventManager::I().RemoveEventHandler(&eventHandler);

			engine->Shutdown();

			delete engine;
		}

		system->Shutdown();

		delete system;

		delete game;
	}
}

void TestGraphics2()
{
	//const int displaySize[2] = { 1920, 1080 };
	const int displaySize[2] = { 800, 600 };
	GFX_DEVICE gfx;
	gfx.InitializeV1(displaySize[0], displaySize[1], false);

	Renderer::I().Initialize();

	{

	ShTexR texRC = new ResTexR(displaySize[0], displaySize[1]);
	ShTexD texRD = new ResTexD(displaySize[0], displaySize[1]);
	ShTexD texD = new ResTexD(2048, 2048);

	std::vector<Mesh*> meshes;

	Mesh* mesh1 = new Mesh();
	Mesh* mesh2 = new Mesh();
	Mesh* mesh3 = new Mesh();
	Mesh* mesh4 = new Mesh();

	ShapeBuilder sb;

	sb.PushRotation(Vec3(0.0f, Calc::mPI / 2.0f, 0.0f));
		sb.CreateDonut(&g_alloc, 10, 10, 0.75f, 0.25f, *mesh1, FVF_XYZ | FVF_NORMAL);
	sb.Pop();
	sb.PushScaling(Vec3(1.0f, 1.0f, 0.05f));
		sb.CreateCube(&g_alloc, *mesh2, FVF_XYZ | FVF_NORMAL);
	sb.Pop();
	sb.PushScaling(Vec3(0.2f, 0.2f, 0.2f));
		sb.CreateCube(&g_alloc, *mesh3, FVF_XYZ | FVF_NORMAL);
	sb.Pop();
	sb.PushTranslation(Vec3(-0.2f, 0.0f, -0.2f));
		sb.PushScaling(Vec3(0.1f, 0.1f, 0.1f));
			sb.CreateCube(&g_alloc, *mesh4, FVF_XYZ | FVF_NORMAL);
		sb.Pop();
	sb.Pop();

	//meshes.push_back(mesh1);
	meshes.push_back(mesh2);
	meshes.push_back(mesh3);
	meshes.push_back(mesh4);

	for (int i = 0; i < 100; ++i)
	{
		const float x = Calc::Random(-1.0f, +1.0f);
		const float y = Calc::Random(-1.0f, +1.0f);
		const float z = Calc::Random(-0.05f, +0.05f);

		const float rx = 0.0f;
		const float ry = 0.0f;
		const float rz = Calc::Random(0.0f, 2.0f * Calc::mPI);

		const float sx = Calc::Random(0.25f, 1.0f);
		const float sy = Calc::Random(0.5f, 2.5f);

		Mesh* mesh = new Mesh();

		sb.PushTranslation(Vec3(x, y, z));
			sb.PushScaling(Vec3(0.05f, 0.05f, 0.25f));
				sb.PushRotation(Vec3(rx, ry, rz));
					sb.PushScaling(Vec3(sx, sx, sy));
						sb.CreateCube(&g_alloc, *mesh, FVF_XYZ | FVF_NORMAL);
						//sb.CreateDonut(20, 10, 0.75f, 0.25f, *mesh, FVF_XYZ | FVF_NORMAL);
					sb.Pop();
				sb.Pop();
			sb.Pop();
		sb.Pop();

		meshes.push_back(mesh);
	}

	Mesh lightMesh;

	sb.PushScaling(Vec3(0.05f, 0.05f, 0.25f));
		sb.CreateDonut(&g_alloc, 20, 10, 1.0f, 0.5f, lightMesh, FVF_XYZ | FVF_NORMAL);
		sb.CalculateNormals(lightMesh);
	sb.Pop();

	for (size_t i = 0; i < meshes.size(); ++i)
		sb.CalculateNormals(*meshes[i]);

	Timer t;

	int pow1 = 0;
	const int pow2 = 4;

	bool stop = false;

	while (stop == false)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_F1)
				{
					pow1 = (pow1 + 1) % (7 + 1);

					const int size = 1 << (pow1 + pow2);

					texD->SetSize(size, size);
				}
				else
				{
					stop = true;
				}
			}
		}

		gfx.SetRTM(texRC.get(), 0, 0, 0, 1, texRD.get());

		gfx.RS(RS_DEPTHTEST, 1);
		gfx.RS(RS_DEPTHTEST_FUNC, CMP_LE);

		Renderer::I().SetGraphicsDevice(&gfx);

		Mat4x4 matLtW;
		Mat4x4 matLtV;
		Mat4x4 matLtP;

		Mat4x4 matObW;
		Mat4x4 matObWT;
		Mat4x4 matObWR;

		Mat4x4 matCmW;
		Mat4x4 matCmV;
		Mat4x4 matCmP;

		Vec3 cmPos1 = Vec3(+1.0f, 0.0f, 0.0f);
		Vec3 cmPos2 = Vec3(0.0f, 0.0f, 2.5f);

		Vec3 ltPos1 = Vec3(-1.0f, 0.0f, 1.5f);
		Vec3 ltPos2 = Vec3(0.0f, 0.0f, 2.5f);

		Vec3 obPos = Vec3(0.0f, 0.0f, 2.5f);

		cmPos1 = Vec3(+1.0f, 0.0f, -sin(t.Time_get() / 2.0f) * 2.0f);
		ltPos1[0] = sin(t.Time_get() / 2.321f);
		ltPos1[1] = cos(t.Time_get() / 2.321f);

		matObWT.MakeTranslation(obPos);
		matObWR.MakeRotationY(t.Time_get() / 2.123f);
		Mat4x4 matObWR2;
		matObWR2.MakeRotationZ(t.Time_get() / 3.321f);
		matObWR = matObWR * matObWR2;
		matObW = matObWT * matObWR;

		////////////////////////////////////////////
		// Light view.
		////////////////////////////////////////////

		matLtW = matObW;
		matLtV.MakeLookat(ltPos1, ltPos2, Vec3(0.0f, 1.0f, 0.0f));
		matLtP.MakePerspectiveLH(Calc::mPI / 1.5f, 1.0f, 0.2f, 10.0f);

		Renderer::I().MatW().Push(matLtW);
		Renderer::I().MatV().Push(matLtV);
		Renderer::I().MatP().Push(matLtP);

		gfx.SetRTM(0, 0, 0, 0, 0, texD.get());
		gfx.Clear(BUFFER_DEPTH, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
		glColorMask(0, 0, 0, 0);
		gfx.RS(RS_CULL, CULL_CCW);
		for (size_t i = 0; i < meshes.size(); ++i)
			Renderer::I().RenderMesh(*meshes[i]);
		gfx.RS(RS_CULL, CULL_NONE);
		glColorMask(1, 1, 1, 1);
		gfx.SetRTM(texRC.get(), 0, 0, 0, 1, texRD.get());

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		////////////////////////////////////////////
		// Camera view.
		////////////////////////////////////////////

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
		Vec4 eyePlaneS(1.0f, 0.0f, 0.0f, 0.0f); glTexGenfv(GL_S, GL_EYE_PLANE, &eyePlaneS[0]);
		Vec4 eyePlaneT(0.0f, 1.0f, 0.0f, 0.0f); glTexGenfv(GL_T, GL_EYE_PLANE, &eyePlaneT[0]);
		Vec4 eyePlaneR(0.0f, 0.0f, 1.0f, 0.0f); glTexGenfv(GL_R, GL_EYE_PLANE, &eyePlaneR[0]);
		Vec4 eyePlaneQ(0.0f, 0.0f, 0.0f, 1.0f); glTexGenfv(GL_Q, GL_EYE_PLANE, &eyePlaneQ[0]);
		glEnable(GL_TEXTURE_GEN_S);
		glEnable(GL_TEXTURE_GEN_T);
		glEnable(GL_TEXTURE_GEN_R);
		glEnable(GL_TEXTURE_GEN_Q);

		matCmW = matObW;
		matCmV.MakeLookat(cmPos1, cmPos2, Vec3(0.0f, 1.0f, 0.0f));
		matCmP.MakePerspectiveLH(Calc::mPI / 2.0f, displaySize[1] / (float)displaySize[0], 0.2f, 10.0f);

		Renderer::I().MatV().Push(matCmV);

		glMatrixMode(GL_MODELVIEW);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		const float position[4] = { ltPos1[0], ltPos1[1], ltPos1[2], 1.0f };
		const float ambient[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		const float specular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

		Renderer::I().MatW().Push(matCmW);
		Renderer::I().MatP().Push(matCmP);

		gfx.Clear(BUFFER_ALL, 0.2f, 0.2f, 0.2f, 0.0f, 1.0f);

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		// (-1, -1)..(+1, +1) -> (0, 0)..(1, 1).
		glTranslatef(0.5f, 0.5f, 0.5f);
		glScalef(0.5f, 0.5f, 0.5f);
		// CamView -> LightProj.
		glMultMatrixf(matLtP.m_v);
		glMultMatrixf(matLtV.m_v);
		glMultMatrixf(matCmV.CalcInv().m_v);

		gfx.SetTex(0, texD.get());
		for (size_t i = 0; i < meshes.size(); ++i)
			Renderer::I().RenderMesh(*meshes[i]);
		gfx.SetTex(0, 0);
		Renderer::I().MatW().PushI();
			Renderer::I().MatW().PushTranslation(ltPos1[0], ltPos1[1], ltPos1[2]);
				Renderer::I().RenderMesh(lightMesh);
			Renderer::I().MatW().Pop();
		Renderer::I().MatW().Pop();

		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();

		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
		glDisable(GL_TEXTURE_GEN_R);
		glDisable(GL_TEXTURE_GEN_Q);

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		gfx.Present();
	}

	for (size_t i = 0; i < meshes.size(); ++i)
		delete meshes[i];
	meshes.clear();

	}

	Renderer::I().SetGraphicsDevice(0);
	Renderer::I().Shutdown();

	gfx.SetRTM(0, 0, 0, 0, 0, 0);
	gfx.SetVB(0);
	gfx.SetIB(0);
	gfx.SetVS(0);
	gfx.SetPS(0);

	gfx.Shutdown();
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
		FASSERT(fabsf(dot1) < 0.01f);
		FASSERT(fabsf(dot2) < 0.01f);
		FASSERT(tan1.CalcSizeSq() != 0.0f);
		FASSERT(tan2.CalcSizeSq() != 0.0f);

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

class VertFrustum
{
public:
	Vec3 m_vertices[8];
	Mx::Plane m_planes[6];

	VertFrustum()
	{
		const float s1 = 1.0f;
		const float s2 = 4.0f;

		m_vertices[0] = Vec3(-s1, -1.0f, -1.0f);
		m_vertices[1] = Vec3(+s1, -1.0f, -1.0f);
		m_vertices[2] = Vec3(+s1, +1.0f, -1.0f);
		m_vertices[3] = Vec3(-s1, +1.0f, -1.0f);

		m_vertices[4] = Vec3(-s2, -2.0f, +1.0f);
		m_vertices[5] = Vec3(+s2, -2.0f, +1.0f);
		m_vertices[6] = Vec3(+s2, +2.0f, +1.0f);
		m_vertices[7] = Vec3(-s2, +2.0f, +1.0f);

		m_planes[0].SetupCW(m_vertices[0], m_vertices[1], m_vertices[2]);
		m_planes[1].SetupCW(m_vertices[7], m_vertices[6], m_vertices[5]);
		m_planes[2].SetupCW(m_vertices[0], m_vertices[4], m_vertices[5]);
		m_planes[3].SetupCW(m_vertices[1], m_vertices[5], m_vertices[6]);
		m_planes[4].SetupCW(m_vertices[2], m_vertices[6], m_vertices[7]);
		m_planes[5].SetupCW(m_vertices[3], m_vertices[7], m_vertices[4]);

		Validate();
	}

	void Transform(const Mat4x4& m)
	{
		for (int i = 0; i < 8; ++i)
		{
			m_vertices[i] = m * m_vertices[i];
		}

		Validate();
	}

	void Validate(const Mx::Plane* planes, int planeCount, const Vec3* vertices, int vertexCount) const 
	{
		Vec3 mid(0.0f, 0.0f, 0.0f);

		for (int i = 0; i < 8; ++i)
			mid += m_vertices[i] / 8.0f;

		for (int i = 0; i < 6; ++i)
		{
			FASSERT(m_planes[i] * mid < 0.0f);
		}
	}

	void Validate() const
	{
		Validate(m_planes, 6, m_vertices, 8);
	}

	std::vector<Mx::Plane> Extrude(const Vec3& direction) const
	{
		//const Vec3 sweep = direction * 50.0f;
		const Vec3 sweep = direction * 1e9f;

		std::vector<Mx::Plane> planes;

		bool vis[6];

		for (int i = 0; i < 6; ++i)
			vis[i] = m_planes[i].m_normal * direction >= 0.0f;

		struct
		{
			int v1, v2;
			int p1, p2;
		} edge[12] =
		{
			{ 0, 1, 0, 2 },
			{ 1, 2, 0, 3 },
			{ 2, 3, 0, 4 },
			{ 3, 0, 0, 5 },

			{ 0, 4, 5, 2 },
			{ 1, 5, 2, 3 },
			{ 2, 6, 3, 4 },
			{ 3, 7, 4, 5 },

			{ 4, 5, 1, 2 },
			{ 5, 6, 1, 3 },
			{ 6, 7, 1, 4 },
			{ 7, 4, 1, 5 }
		};

		Vec3 mid(0.0f, 0.0f, 0.0f);
		for (int i = 0; i < 8; ++i)
			mid += m_vertices[i] / 8.0f;

		int added = 0;

		for (int i = 0; i < 12; ++i)
		{
			const Vec3& v1 = m_vertices[edge[i].v1];
			const Vec3& v2 = m_vertices[edge[i].v2];
			const Mx::Plane& plane1 = m_planes[edge[i].p1];
			const Mx::Plane& plane2 = m_planes[edge[i].p2];
			const bool vis1 = vis[edge[i].p1];
			const bool vis2 = vis[edge[i].p2];

			if (vis1 != vis2)
			{
				Vec3 vd = v2 - v1;
				Vec3 n = vd % direction;
				n.Normalize();
				bool discard =
					plane1.m_normal * n == 0.0f ||
					plane2.m_normal * n == 0.0f;
				if (discard == false)
				{
					Mx::Plane plane(n, n * v1);

					if (plane * mid > 0.0f)
						plane = -plane;

					planes.push_back(plane);
					added++;
				}
			}
		}

		printf("added %d planes\n", added);

		for (int i = 0; i < 6; ++i)
		{
			const bool vis = m_planes[i].m_normal * direction >= 0.0f;

			if (vis)
				planes.push_back(Mx::Plane(m_planes[i].m_normal, m_planes[i].m_distance + m_planes[i].m_normal * sweep));
			else
				planes.push_back(m_planes[i]);
		}

		Validate(&planes[0], planes.size(), m_vertices, 8);

		return planes;
	}
};

void DrawVolume(const Mx::Plane* planes, int planeCount)
{
	for (int i = 0; i < planeCount; ++i)
	{
		ClipPoly p;

		p.SetupFromPlane(planes[i]);

		for (int j = 0; j < planeCount; ++j)
		{
			if (i != j)
			{
				p.ClipByPlane_KeepFront(planes[j]);
			}
		}

		//

		if (p.m_vertexCount >= 3)
		{
			sDebugRender.DrawPoly(p.m_vertices, p.m_vertexCount, 0.1f, 0.1f, 0.1f, 1.0f);
			sDebugRender.DrawPolyLine(p.m_vertices, p.m_vertexCount, 1.0f, 1.0f, 1.0f, 1.0f);
		}
	}
}

void TestGraphics3()
{
	SystemDefault system;
	system.RegisterFileSystems();

	//const int displaySize[2] = { 1920, 1080 };
	const int displaySize[2] = { 512, 512 };

	GFX_DEVICE gfx;
	GraphicsOptions options(
		displaySize[0],
		displaySize[1],
		displaySize[0] == 1920,
		true);
	gfx.Initialize(options);

	{

	Renderer::I().Initialize();
	Renderer::I().SetGraphicsDevice(&gfx);

	ShTexR texR[3];
	ShTexD texD;
	ShTexR texL;
	ShTexR texC;
	texR[0] = new ResTexR(displaySize[0], displaySize[1], TEXR_COLOR   ); // normal
	texR[1] = new ResTexR(displaySize[0], displaySize[1], TEXR_COLOR   ); // albedo
	texR[2] = new ResTexR(displaySize[0], displaySize[1], TEXR_COLOR   ); // shading params
	texD    = new ResTexD(displaySize[0], displaySize[1]               ); // depth buffer
	texL    = new ResTexR(displaySize[0], displaySize[1], TEXR_COLOR   ); // light accum
	texC    = new ResTexR(displaySize[0], displaySize[1], TEXR_COLOR   ); // composite

	ResLoaderTex loader;
	ShTex tex = loader.Load("data/textures/nveye-color.png");

	ShVS geometryVS  = new ResVS(); FVERIFY(geometryVS->Load("data/shaders/deferred_geometry_vs.shader"));
	ShPS geometryPS  = new ResPS(); FVERIFY(geometryPS->Load("data/shaders/deferred_geometry_ps.shader"));
	ShVS lightVS     = new ResVS(); FVERIFY(lightVS->Load("data/shaders/deferred_light_vs.shader"));
	ShPS lightPS     = new ResPS(); FVERIFY(lightPS->Load("data/shaders/deferred_light_ps.shader"));
	ShVS compositeVS = new ResVS(); FVERIFY(compositeVS->Load("data/shaders/deferred_composite_vs.shader"));
	ShPS compositePS = new ResPS(); FVERIFY(compositePS->Load("data/shaders/deferred_composite_ps.shader"));

	std::vector<Mesh*> meshes;

	Mesh* mesh1 = new Mesh();
	Mesh* mesh2 = new Mesh();
	Mesh* mesh3 = new Mesh();
	Mesh* mesh4 = new Mesh();

	ShapeBuilder sb;
	sb.PushRotation(Vec3(0.0f, Calc::mPI / 2.0f, 0.0f));
		sb.CreateDonut(&g_alloc, 10, 10, 0.75f, 0.25f, *mesh1, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
	sb.Pop();
	sb.PushScaling(Vec3(1.0f, 1.0f, 0.05f));
		sb.CreateCube(&g_alloc, *mesh2, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
	sb.Pop();
	sb.PushScaling(Vec3(0.2f, 0.2f, 0.2f));
		sb.CreateCube(&g_alloc, *mesh3, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
	sb.Pop();
	sb.PushTranslation(Vec3(-0.2f, 0.0f, -0.2f));
		sb.PushScaling(Vec3(0.1f, 0.1f, 0.1f));
			sb.CreateCube(&g_alloc, *mesh4, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
		sb.Pop();
	sb.Pop();

	meshes.push_back(mesh1);
	meshes.push_back(mesh2);
	meshes.push_back(mesh3);
	meshes.push_back(mesh4);

	for (int i = 0; i < 100; ++i)
	{
		const float x = Calc::Random(-1.0f, +1.0f);
		const float y = Calc::Random(-1.0f, +1.0f);
		const float z = Calc::Random(-0.05f, +0.05f);

		const float rx = 0.0f;
		const float ry = 0.0f;
		const float rz = Calc::Random(0.0f, 2.0f * Calc::mPI);

		Mesh* mesh = new Mesh();

		sb.PushTranslation(Vec3(x, y, z));
			sb.PushScaling(Vec3(0.05f, 0.05f, 0.25f));
				sb.PushRotation(Vec3(rx, ry, rz));
					//sb.CreateCube(&g_alloc, *mesh, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
				sb.CreateDonut(&g_alloc, 20, 10, 0.75f, 0.25f, *mesh, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
				sb.Pop();
			sb.Pop();
		sb.Pop();

		meshes.push_back(mesh);
	}

	ProcTexcoordMatrix2DAutoAlign procTexCoord;
	for (size_t i = 0; i < meshes.size(); ++i)
	{
		sb.CalculateNormals(*meshes[i]);
		sb.CalculateTexcoords(*meshes[i], 0, &procTexCoord);
	}

	float rx = 0.0f;
	float ry = 0.0f;

	Timer t;

	int mode = 0;

	bool stop = false;

	while (stop == false)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				if (e.key.keysym.sym == SDLK_F1)
					mode++;
			}
			if (e.type == SDL_MOUSEMOTION)
			{
				rx += e.motion.xrel;
				ry += e.motion.yrel;
			}
		}

		gfx.SceneBegin();

		////////////////////////////////////////////
		// DRAW G-BUFFER
		////////////////////////////////////////////

		gfx.SetRTM(
			texR[0].get(),
			texR[1].get(),
			texR[2].get(), 0, 3, texD.get());

		gfx.RS(RS_DEPTHTEST, 1);
		gfx.RS(RS_DEPTHTEST_FUNC, CMP_LE);

		Mat4x4 matObW;
		Mat4x4 matObWT;
		Mat4x4 matObWR;

		Mat4x4 matCmW;
		Mat4x4 matCmV;
		Mat4x4 matCmP;

		Vec3 cmPos1 = Vec3(0.0f, 0.0f, 0.0f);
		Vec3 cmPos2 = Vec3(0.0f, 0.0f, 2.5f);

		Vec3 obPos = Vec3(0.0f, 0.0f, 2.5f);

		//cmPos1 = Vec3(0.0f, 0.0f, -sin(t.Time_get() / 2.0f) * 2.0f);
		cmPos1 = Vec3(0.0f, 0.0f, -1.0f);

		matObWT.MakeTranslation(obPos);
		matObWR.MakeRotationY(t.Time_get() / 2.123f);
		Mat4x4 matObWR2;
		matObWR2.MakeRotationZ(t.Time_get() / 3.321f);
		matObWR = matObWR * matObWR2;
		matObWR.MakeIdentity();
		matObW = matObWT * matObWR;

		matCmW = matObW;
		matCmV.MakeLookat(cmPos1, cmPos2, Vec3(0.0f, 1.0f, 0.0f));
		matCmP.MakePerspectiveLH(Calc::mPI / 2.0f, 480.0f / 640.0f, 0.2f, 10.0f);

		Renderer::I().MatP().Push(matCmP);
		Renderer::I().MatV().Push(matCmV);
		Renderer::I().MatW().Push(matCmW);

		gfx.Clear(BUFFER_ALL, 0.2f, 0.2f, 0.2f, 0.0f, 1.0f);

		geometryVS->p["INPUT_w"] =
			Renderer::I().MatW().Top();
		geometryVS->p["INPUT_wvp"] =
			Renderer::I().MatP().Top() *
			Renderer::I().MatV().Top() *
			Renderer::I().MatW().Top();
		geometryPS->p["INPUT_albedoTex"] = tex.get();

		gfx.SetVS(geometryVS.get());
		gfx.SetPS(geometryPS.get());

		for (size_t i = 0; i < meshes.size(); ++i)
			Renderer::I().RenderMesh(*meshes[i]);

		gfx.SetVS(0);
		gfx.SetPS(0);

		Renderer::I().MatW().Pop();

		//

		Mat4x4 matR;
		Mat4x4 matS;
		matR.MakeRotationY(t.Time_get());
		matS.MakeScaling(0.1f, 0.1f, 0.1f);

		Renderer::I().MatW().Push(matR * matS);

		geometryVS->p["INPUT_w"] =
			Renderer::I().MatW().Top();
		geometryVS->p["INPUT_wvp"] =
			Renderer::I().MatP().Top() *
			Renderer::I().MatV().Top() *
			Renderer::I().MatW().Top();

		gfx.SetVS(geometryVS.get());
		gfx.SetPS(geometryPS.get());

		Mat4x4 frustumMat;
		frustumMat.MakeIdentity();
		Frustum frustum;
		frustum.Setup(frustumMat, 0.1f, 0.5f);
		//DrawVolume(frustum.m_planes, 6);

		VertFrustum vf;
		Mat4x4 matD;
		{
			Mat4x4 matR1;
			Mat4x4 matR2;
			Mat4x4 matR3;
			/*matR1.MakeRotationX(t.Time_get() / 5.0f);
			matR2.MakeRotationY(t.Time_get() / 7.0f);
			matR3.MakeRotationZ(t.Time_get() / 11.0f);*/
			matR1.MakeRotationX(rx / 50.0f);
			matR2.MakeRotationY(ry / 50.0f);
			matR3.MakeIdentity();
			matD = matR3 * matR2 * matR1;
		}
		Vec3 dir;
		mode %= 4;
		if (mode == 0)
			dir = matD * Vec3(1.0f, 1.0f, 1.0f);
		if (mode == 1)
			dir = Vec3(1.0f, 0.0f, 0.0f);
		if (mode == 2)
			dir = Vec3(0.0f, 1.0f, 0.0f);
		if (mode == 3)
			dir = Vec3(0.0f, 0.0f, 1.0f);

		dir.Normalize();

		std::vector<Mx::Plane> planes = vf.Extrude(dir);

		for (size_t i = 0; i < planes.size(); ++i)
			planes[i] = -planes[i];

		gfx.RS(RS_DEPTHTEST, 0);
		gfx.RS(RS_BLEND, 1);
		gfx.RS(RS_BLEND_SRC, BLEND_ONE);
		gfx.RS(RS_BLEND_DST, BLEND_ONE);

		DrawVolume(&planes[0], planes.size());

		gfx.RS(RS_BLEND, 0);
		gfx.RS(RS_DEPTHTEST, 1);

		gfx.SetVS(0);
		gfx.SetPS(0);

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		gfx.RS(RS_DEPTHTEST, 0);

#if 1
		////////////////////////////////////////////
		// ACCUMULATE LIGHTS
		////////////////////////////////////////////

		gfx.SetRTM(texL.get(), 0, 0, 0, 1, 0);
		gfx.Clear(BUFFER_COLOR, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

		Mat4x4 matP;
		Mat4x4 matV;
		Mat4x4 matW;

		matP.MakeOrthoLH(-1.0f, +1.0f, -1.0f, +1.0f, 0.1f, 10.0f);
		matV.MakeTranslation(0.0f, 0.0f, 1.0f);
		matW.MakeIdentity();

		Renderer::I().MatW().Push(matW);
		Renderer::I().MatV().Push(matV);
		Renderer::I().MatP().Push(matP);

		lightVS->p["INPUT_wvp"] =
			Renderer::I().MatP().Top() *
			Renderer::I().MatV().Top() *
			Renderer::I().MatW().Top();
		lightPS->p["INPUT_normalTex"] = texR[0].get();
		lightPS->p["INPUT_paramsTex"] = texR[2].get();

		gfx.SetVS(lightVS.get());
		gfx.SetPS(lightPS.get());

		gfx.RS(RS_BLEND, 1);
		gfx.RS(RS_BLEND_SRC, BLEND_ONE);
		gfx.RS(RS_BLEND_DST, BLEND_ONE);

		for (int i = 0; i < 2; ++i)
			Renderer::I().RenderQuad();

		gfx.RS(RS_BLEND, 0);

		gfx.SetVS(0);
		gfx.SetPS(0);

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		////////////////////////////////////////////
		// RENDER COMPOSITE IMAGE
		////////////////////////////////////////////

		{
		gfx.SetRTM(texC.get(), 0, 0, 0, 1, 0);
		gfx.Clear(BUFFER_ALL, 0.2f, 0.2f, 0.2f, 0.0f, 1.0f);

		Mat4x4 matP;
		Mat4x4 matV;
		Mat4x4 matW;

		matP.MakeOrthoLH(-1.0f, +1.0f, -1.0f, +1.0f, 0.1f, 10.0f);
		matV.MakeTranslation(0.0f, 0.0f, 1.0f);
		matW.MakeIdentity();

		Renderer::I().MatW().Push(matW);
		Renderer::I().MatV().Push(matV);
		Renderer::I().MatP().Push(matP);

		compositeVS->p["INPUT_wvp"] =
			Renderer::I().MatP().Top() *
			Renderer::I().MatV().Top() *
			Renderer::I().MatW().Top();
		compositePS->p["INPUT_albedoTex"] = texR[1].get();
		compositePS->p["INPUT_lightTex"] = texL.get();
		gfx.SetVS(compositeVS.get());
		gfx.SetPS(compositePS.get());

		Renderer::I().RenderQuad();

		gfx.SetVS(0);
		gfx.SetPS(0);

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();
		}
#endif

		gfx.SceneEnd();

		const int src = (int)t.Time_get() % 3;

		//gfx.Resolve(texR[src].get());
		gfx.Resolve(texC.get());

		gfx.Present();
	}

	for (size_t i = 0; i < meshes.size(); ++i)
		delete meshes[i];
	meshes.clear();

	Renderer::I().SetGraphicsDevice(0);
	Renderer::I().Shutdown();

	}

	gfx.SetRTM(0, 0, 0, 0, 0, 0);
	gfx.SetVB(0);
	gfx.SetIB(0);
	gfx.SetVS(0);
	gfx.SetPS(0);

	gfx.Shutdown();
}

void TestShaders()
{
	GFX_DEVICE gfx;
	gfx.InitializeV1(1024, 768, false);

	ShVS vs(new ResVS());
	FVERIFY(vs->Load("shaders/test/vs.cg"));

	ShPS ps(new ResPS());
	FVERIFY(ps->Load("shaders/test/ps.cg"));
	ShPS psPlax(new ResPS());
	FVERIFY(psPlax->Load("shaders/test/ps_plax.cg"));

	ShTex tex(new ResTex());
	tex->SetSize(16, 16);
	for (int i = 0; i < 16; ++i)
		for (int j = 0; j < 16; ++j)
			tex->SetPixel(i, j, Color(Calc::Random(0.0f, 1.0f), Calc::Random(0.0f, 1.0f), 1.0f, 1.0f));

	ShTexCR texCR(new ResTexCR());
	texCR->SetSize(512, 512);

	Mesh mesh;
	ShapeBuilder sb;
	sb.CreateCube(&g_alloc, mesh, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
	if (1)
	{
		sb.CalculateNormals(mesh);
		ProcTexcoordMatrix2DAutoAlign procTex;
		Mat4x4 temp;
		temp.MakeScaling(1.0f, 1.0f, 1.0f);
		procTex.SetMatrix(temp);
		sb.CalculateTexcoords(mesh, 0, &procTex);
	}
	sb.ConvertToIndexed(&g_alloc, mesh, mesh);
	sb.CalculateNormals(mesh);
	Mesh mesh2;
	sb.CreateDonut(&g_alloc, 30, 20, 0.75f, 0.25f, mesh2, FVF_XYZ | FVF_NORMAL);
	sb.ConvertToIndexed(&g_alloc, mesh2, mesh2);
	sb.CalculateNormals(mesh2);
	//sb.Copy(mesh2, mesh);

	Mat4x4 matW;
	Mat4x4 matV;
	Mat4x4 matP;

	Timer t;

	bool stop = false;

	while (!stop)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
				stop = true;
		}

		Renderer::I().SetGraphicsDevice(&gfx);

		Mat4x4 t1;
		Mat4x4 t2;
		t1.MakeRotationX(t.Time_get() * 1.234f / 4.0f);
		t2.MakeRotationY(t.Time_get() * 2.345f / 4.0f);
		matW = t1 * t2;
		matV.MakeTranslation(0.0f, 0.0f, 2.5f);
		matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, 2.0f, 10.0f);

		Renderer::I().MatW().Push(matW);
		Renderer::I().MatV().Push(matV);
		Renderer::I().MatP().Push(matP);

		gfx.RS(RS_DEPTHTEST, 1);
		gfx.RS(RS_DEPTHTEST_FUNC, CMP_LE);

		gfx.SetRT(texCR->m_faces[CUBE_X_NEG].get()); gfx.Clear(BUFFER_ALL, 1.0f, sin(t.Time_get() * 1.0f), 0.0f, 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(texCR->m_faces[CUBE_X_POS].get()); gfx.Clear(BUFFER_ALL, 0.0f, 1.0f, sin(t.Time_get() * 1.1f), 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(texCR->m_faces[CUBE_Y_NEG].get()); gfx.Clear(BUFFER_ALL, sin(t.Time_get() * 1.2f), 0.0f, 1.0f, 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(texCR->m_faces[CUBE_Y_POS].get()); gfx.Clear(BUFFER_ALL, 1.0f, 1.0f, sin(t.Time_get() * 1.3f), 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(texCR->m_faces[CUBE_Z_NEG].get()); gfx.Clear(BUFFER_ALL, sin(t.Time_get() * 1.4f), 1.0f, 1.0f, 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(texCR->m_faces[CUBE_Z_POS].get()); gfx.Clear(BUFFER_ALL, 1.0f, sin(t.Time_get() * 1.5f), 1.0f, 1.0f, 1.0f); Renderer::I().RenderMesh(mesh2);
		gfx.SetRT(0);

		gfx.Clear(BUFFER_ALL, 0.0f, 0.0f, 0.1f, 0.0f, 1.0f);

		vs->p["wvp"] = matP * matV * matW;
		vs->p["col"] = Vec4(1.0f, 0.0f, 0.0f, 1.0f);
		ps->p["tex"] = texCR.get();
		psPlax->p["tex"] = tex.get();
		psPlax->p["dmap"] = tex.get();
		psPlax->p["t"] = t.Time_get();
		gfx.SetVS(vs.get());
		gfx.SetPS(ps.get());
		gfx.SetPS(psPlax.get());

		Renderer::I().RenderMesh(mesh);

		gfx.SetVS(0);
		gfx.SetPS(0);

		//gfx.Copy(texCR->m_faces[CUBE_Z_NEG].get());

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		gfx.Present();
	}
}

void TestShapeBuilder_GetEdges()
{
	Mesh mesh;

	ShapeBuilder sb;

	sb.CreateCube(&g_alloc, mesh, FVF_XYZ);
	sb.ConvertToIndexed(&g_alloc, mesh, mesh);

	EdgeTable edges;

	sb.GetEdges(&g_alloc, mesh, edges);

	printf("EdgeCount: %d.\n", edges.size());

	sb.RemoveTJunctions(&g_alloc, mesh, mesh);

	Mesh mesh2;
	mesh2.Initialize(&g_alloc, PT_TRIANGLE_LIST, false, 5, FVF_XYZ, 6);
	mesh2.GetVB()->position[0] = Vec3(0.0f, 0.0f, 0.0f);
	mesh2.GetVB()->position[1] = Vec3(1.0f, 1.0f, 0.0f);
	mesh2.GetVB()->position[2] = Vec3(0.0f, 1.0f, 0.0f);
	mesh2.GetVB()->position[3] = Vec3(0.5f, 1.0f, 0.0f);
	mesh2.GetVB()->position[4] = Vec3(0.5f, 2.0f, 0.0f);
	mesh2.GetIB()->index[0] = 0;
	mesh2.GetIB()->index[1] = 1;
	mesh2.GetIB()->index[2] = 2;
	mesh2.GetIB()->index[3] = 3;
	mesh2.GetIB()->index[4] = 1;
	mesh2.GetIB()->index[5] = 4;
	sb.RemoveTJunctions(&g_alloc, mesh2, mesh2);

	printf("TestShapeBuilder_GetEdges done. press any key\n");
	getc(stdin);
}

void TestShapeBuilder_Patches()
{
	Mesh mesh;
	PatchMesh patchMesh;

	patchMesh.SetSize(2, 2);
	for (int px = 0; px < 2; ++px)
	{
		for (int py = 0; py < 2; ++py)
		{
			Patch& patch = patchMesh.GetPatch(px, py);
			for (int x = 0; x < 4; ++x)
			{
				for (int y = 0; y < 4; ++y)
				{
					float vx = px * 3.0f + x * 4.0f / 4.0f;
					float vy = py * 3.0f + y * 4.0f / 4.0f;
					patch.v[x][y][0] = vx - 3.0f;
					patch.v[x][y][1] = vy - 3.0f;
					patch.v[x][y][2] = sin((vx + vy)) * 0.2f;
				}
			}
		}
	}
	ShapeBuilder sb;
	sb.UpdatePatch(patchMesh);

	sb.CreatePatch(&g_alloc, patchMesh, 20, 20, mesh, FVF_XYZ | FVF_NORMAL);
	sb.ConvertToIndexed(&g_alloc, mesh, mesh);
	sb.CalculateNormals(mesh);

	GFX_DEVICE gfx;
	gfx.InitializeV1(1024, 768, false);

	Timer t;

	bool stop = false;

	while (!stop)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
				stop = true;
		}

		Renderer::I().SetGraphicsDevice(&gfx);

		Mat4x4 matW;
		Mat4x4 matV;
		Mat4x4 matP;

		matW.MakeRotationX(t.Time_get());
		matV.MakeTranslation(0.0f, 0.0f, 2.0f);
		matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, 0.01f, 100.0f);

		Renderer::I().MatW().Push(matW);

		glMatrixMode(GL_MODELVIEW);
		glShadeModel(GL_SMOOTH);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		float position[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
		float ambient[4]  = { 0.0f, 0.0f, 0.0f, 0.0f };
		float diffuse[4]  = { 1.0f, 1.0f, 1.0f, 1.0f };
		float specular[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_POSITION, position);
		glLightfv(GL_LIGHT0, GL_AMBIENT,  ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

		Renderer::I().MatV().Push(matV);
		Renderer::I().MatP().Push(matP);

		gfx.Clear(BUFFER_ALL, 0.0f, 0.25f, 0.0f, 0.0f, 1.0f);

		gfx.RS(RS_DEPTHTEST, 1);
		gfx.RS(RS_DEPTHTEST_FUNC, CMP_LE);

		Renderer::I().RenderMesh(mesh);

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		gfx.Present();
	}
}

void TestShadowCube()
{
	GFX_DEVICE gfx;

	gfx.InitializeV1(1024, 768, false);
	gfx.RS(RS_DEPTHTEST, 1);
	gfx.RS(RS_DEPTHTEST_FUNC, CMP_LE);

	Renderer::I().SetGraphicsDevice(&gfx);

	ShTexCR cube(new ResTexCR());

	//cube->SetTarget(TEXR_DEPTH);
	//cube->SetSize(256, 256);
	cube->SetSize(512, 512);
	//cube->SetSize(1024, 1024);

	ShVS shVS(new ResVS()); FVERIFY(shVS->Load("sh_vs.cg"));
	ShPS shPS(new ResPS()); FVERIFY(shPS->Load("sh_ps.cg"));
	ShVS shcVS(new ResVS()); FVERIFY(shcVS->Load("shc_vs.cg"));
	ShPS shcPS(new ResPS()); FVERIFY(shcPS->Load("shc_ps.cg"));

	Mesh mesh;
	Mesh mesh2;

	ShapeBuilder sb;
	//sb.CreateCube(mesh, FVF_XYZ | FVF_NORMAL);
	sb.CreateDonut(&g_alloc, 20, 10, 0.75f, 0.25f, mesh, FVF_XYZ | FVF_NORMAL);
	sb.CalculateNormals(mesh);

	sb.PushScaling(Vec3(10.0f, 0.1f, 10.0f));
		sb.CreateCube(&g_alloc, mesh2, FVF_XYZ | FVF_NORMAL);
		sb.CalculateNormals(mesh2);
	sb.Pop();

	Timer t;

	// TODO: Rotate cube view matrix to prevent perfect alignment with surfaces.

	bool stop = false;

	//while (t.Time_get() < 4.0f)
	while (!stop)
	{
		Vec3 cmPos(0.0f, 4.0f, -4.0f);
		static Vec3 ltPos(0.0f, 0.0f, 0.0f);
		Vec3 obPos(0.0f, 0.0f, 0.0f);

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				if (e.key.state == SDL_PRESSED)
				{
					if (e.key.keysym.sym == SDLK_w) ltPos[2] += 0.1f;
					if (e.key.keysym.sym == SDLK_s) ltPos[2] -= 0.1f;
					if (e.key.keysym.sym == SDLK_a) ltPos[0] -= 0.1f;
					if (e.key.keysym.sym == SDLK_d) ltPos[0] += 0.1f;
					if (e.key.keysym.sym == SDLK_q) ltPos[1] += 0.1f;
					if (e.key.keysym.sym == SDLK_z) ltPos[1] -= 0.1f;
				}
			}
		}

		/*
		cmPos = Vec3(
			sin(t.Time_get() / 2.0f) * 4.0f,
			4.0f,
			cos(t.Time_get() / 2.0f) * 4.0f);*/

#if 0
#if 0
		ltPos = Vec3(
			sin(t.Time_get() * 2.0f) * 0.5f,
			sin(t.Time_get() / 2.0f) * 1.5f,
			cos(t.Time_get() * 2.0f) * 0.5f - 2.0f);
#else
		ltPos = Vec3(
			sin(t.Time_get() * 2.0f) * 0.5f,
			//sin(t.Time_get() / 2.0f) * 0.5f,
			cos(t.Time_get() * 2.0f) * 0.5f,
			cos(t.Time_get() / 2.0f) * 0.5f);
#endif
#endif

		Mat4x4 matW;
		Mat4x4 matV;
		Mat4x4 matP;

		matW.MakeTranslation(obPos);
		{ Mat4x4 matWR; matWR.MakeRotationX(t.Time_get() / 5.0f * 1.234f); matW = matW * matWR; }
		{ Mat4x4 matWR; matWR.MakeRotationY(t.Time_get() / 5.0f * 1.345f); matW = matW * matWR; }
		{ Mat4x4 matWR; matWR.MakeRotationZ(t.Time_get() / 5.0f * 1.456f); matW = matW * matWR; }

		gfx.SetVS(shVS.get());
		gfx.SetPS(shPS.get());

		gfx.RS(RS_CULL, CULL_CW);

		matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, 0.05f, 20.0f);

		Renderer::I().MatW().Push(matW);
		Renderer::I().MatP().Push(matP);

		for (int i = 0; i < 6; ++i)
		{
			gfx.SetRT(cube->GetFace((CUBE_FACE)i).get());
			gfx.Clear(BUFFER_ALL, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

			matV = Renderer::I().GetCubeSSMMatrix((CUBE_FACE)i, ltPos);// * matVT;

			Renderer::I().MatV().Push(matV);

			shVS->p["wvp"] =
				Renderer::I().MatP().Top() *
				Renderer::I().MatV().Top() *
				Renderer::I().MatW().Top();
			shVS->p["w"] =
				Renderer::I().MatW().Top();
			shPS->p["lt_pos"] = ltPos;

			gfx.SetVS(shVS.get());
			gfx.SetPS(shPS.get());

			Renderer::I().RenderMesh(mesh);
			Renderer::I().RenderMesh(mesh2);

			Renderer::I().MatV().Pop();
		}

		Renderer::I().MatW().Pop();
		Renderer::I().MatP().Pop();

		gfx.RS(RS_CULL, CULL_NONE);
		gfx.SetVS(0);
		gfx.SetPS(0);

		gfx.SetRT(0);

		gfx.Clear(BUFFER_ALL, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);

		// Calc matV, matP.
		matV.MakeLookat(cmPos, obPos, Vec3(0.0f, 1.0f, 0.0f));
		matP.MakePerspectiveLH(Calc::mPI / 2.0f, 1.0f, 0.01f, 100.0f);

		Renderer::I().MatW().Push(matW);
		Renderer::I().MatV().Push(matV);
		Renderer::I().MatP().Push(matP);

		shcVS->p["wvp"] =
			Renderer::I().MatP().Top() *
			Renderer::I().MatV().Top() *
			Renderer::I().MatW().Top();
		shcVS->p["w"] =
			Renderer::I().MatW().Top();
		shcPS->p["tex"] = cube.get();
		shcPS->p["lt_pos"] = ltPos;

		gfx.SetVS(shcVS.get());
		gfx.SetPS(shcPS.get());

		Renderer::I().RenderMesh(mesh);
		Renderer::I().RenderMesh(mesh2);

		gfx.SetVS(0);
		gfx.SetPS(0);

		gfx.SetTex(0, 0);

		Renderer::I().MatW().Pop();

		glBegin(GL_LINES);
		glVertex3f(ltPos[0], ltPos[1], ltPos[2]);
		glVertex3f(ltPos[0], ltPos[1] + 10.0f, ltPos[2]);
		glEnd();

		Renderer::I().MatW().Pop();
		Renderer::I().MatV().Pop();
		Renderer::I().MatP().Pop();

		gfx.Present();
	}
}
