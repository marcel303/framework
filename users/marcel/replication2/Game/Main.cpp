#if defined(DEBUG) && 0
#define GFX_W 320
#define GFX_H 240
#define GFX_FS false
#else
#define GFX_W 800
#define GFX_H 600
#define GFX_FS false
#endif

#define GFX_DEVICE GraphicsDeviceGL

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

static void TestReplication();

#if LIBNET_ENABLE_NET_STATS
template <class T>
void RenderStatValue(NetStatsValue<T> & value, const std::string & name, int x, int y, int w, int h)
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

	const T min = (T)0;
	const T max = value.CalculateMax();

	T diff = max - min;

	if (diff == 0) diff = 1;

	glBegin(GL_LINES);
	{
		for (int i = 0; i < value.GetSize() - 1; ++i)
		{
			const int offset1 = i + 0;
			const int offset2 = i + 1;

			const int x1 = (i + 0) * w / (value.GetSize() - 1);
			const int x2 = (i + 1) * w / (value.GetSize() - 1);

			const T value1 = (diff - (value.GetValue(offset1) - min)) * h / diff;
			const T value2 = (diff - (value.GetValue(offset2) - min)) * h / diff;

			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(static_cast<float>(x + x1), static_cast<float>(y + value1 + 0.01f), 1.0f);
			glColor3f(0.0f, 1.0f, 0.0f);
			glVertex3f(static_cast<float>(x + x2), static_cast<float>(y + value2 + 0.01f), 1.0f);
		}
	}
	glEnd();

	glColor3f(1.0f, 1.0f, 1.0f);

	Renderer::I().MatW().Pop();
	Renderer::I().MatV().Pop();
	Renderer::I().MatP().Pop();
}
#endif

class TestEventHandler : public EventHandler
{
public:
	TestEventHandler(Engine * engine)
		: EventHandler()
		, m_engine(engine)
		, m_stop(false)
	{
	}

	virtual void OnEvent(Event& event)
	{
		if (event.type == EVT_QUIT)
			m_stop = true;
		if (event.type == EVT_KEY && event.key.key == IK_g && event.key.state)
		{
			Scene * scene = m_engine->m_serverScene;

			std::vector<ShEntity> players = scene->FindEntitiesByClassName("Player");

			for (size_t i = 0; i < players.size(); ++i)
			{
				Entity * player = players[i].get();
				
				const int numBricks = 10 / (1 + players.size()/2);

				for (int j = 0; j < numBricks; ++j)
				{
					Vec3 position = player->GetPosition();
					float a = Calc::Random(Calc::m2PI);
					float x = position[0] + std::sin(a) * 10.0f;
					float z = position[2] + std::cos(a) * 10.0f;
					float size = 10.0f + Calc::Random(0.0f, 10.0f);

					EntityBrick* brick = new EntityBrick();
					brick->Initialize(Vec3(x, 0.0f, z), Vec3(5.0f, size, 5.0f), false);
					scene->AddEntityQueued(ShEntity(brick));
				}
			}
		}
	}

	Engine * m_engine;
	bool m_stop;
};

int main(int argc, char* argv[])
{
	Calc::Initialize();

	bool cfg_fullscreen;
	bool cfg_server;
	bool cfg_client;

	char c;

	if (argc > 1)
	{
		c = argv[1][0];
	}
	else
	{
		printf("S: server\n");
		printf("C: client\n");
		printf("B: client + server\n");

		c = getc(stdin);
	}

	c = tolower(c);

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

			TestEventHandler eventHandler(engine);

			EventManager::I().AddEventHandler(&eventHandler);

			ShShader blitShader = new ResShader();
			blitShader->m_vs = RESMGR.GetVS("data/shaders/blit_vs.cg");
			blitShader->m_ps = RESMGR.GetPS("data/shaders/blit_ps.cg");

			const size_t numClients = engine->m_clientClients.size();
			const size_t numDividesX = (int)std::ceil(std::sqrt((float)numClients));
			const size_t numDividesY = (int)std::ceil((float)numClients / numDividesX);

			ResTexR rtc(GFX_W / numDividesX, GFX_H / numDividesY);
			ResTexD rtd(GFX_W / numDividesX, GFX_H / numDividesY);

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

				Renderer::I().SetGraphicsDevice(&gfx);

				gfx.SceneBegin();
				{
					gfx.Clear(BUFFER_ALL, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f);

					for (size_t i = 0; i < engine->m_clientClients.size(); ++i)
					{
						Client * client = engine->m_clientClients.at(i);

						gfx.SetRTM(&rtc, 0, 0, 0, 1, &rtd);

						Player * player = 0;

						if (!client->m_clientScene->m_activeEntity.expired())
						{
							player = static_cast<Player*>(client->m_clientScene->m_activeEntity.lock().get());
						}

						Mat4x4 matW;
						Mat4x4 matV;
						Mat4x4 matP;

						if (player)
						{
							matW.MakeIdentity();
							matV = player->GetCamera().CalcInv();
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

						//

						client->m_clientScene->Render();

						Renderer::I().GetGraphicsDevice()->SetVS(0);
						Renderer::I().GetGraphicsDevice()->SetPS(0);

					#if LIBNET_ENABLE_NET_STATS
						if (renderStats)
						{
							RenderStatValue<int>(NetStat_PacketsSent,          "NET: Packets sent",      0,   0,   295, 45);
							RenderStatValue<int>(NetStat_PacketsReceived,      "NET: Packets received",  300, 0,   295, 45);
							RenderStatValue<int>(NetStat_BytesSent,            "NET: Bytes sent",        0,   50,  295, 45);
							RenderStatValue<int>(NetStat_BytesReceived,        "NET: Bytes received",    300, 50,  295, 45);
							RenderStatValue<int>(Replication_BytesReceived,    "REP: Bytes received",    0,   100, 295, 45);
							RenderStatValue<int>(Replication_ObjectsCreated,   "REP: Objects created",   300, 100, 295, 45);
							RenderStatValue<int>(Replication_ObjectsDestroyed, "REP: Objects destroyed", 0,   150, 295, 45);
							RenderStatValue<int>(Replication_ObjectsUpdated,   "REP: Objects updated",   300, 150, 295, 45);
							RenderStatValue<int>(Gfx_Fps,                      "GFX: FPS",               0,   250, 295, 45);
						}
					#endif

						//

						Renderer::I().MatW().Pop();
						Renderer::I().MatV().Pop();
						Renderer::I().MatP().Pop();

						gfx.SetRTM(0, 0, 0, 0, 0, 0);

						gfx.SetRT(0);

						size_t x = i % numDividesX;
						size_t y = i / numDividesX;

						matP.MakeOrthoLH(0.f, 1.f, 0.f, 1.f, -1.f, +1.f);
						Renderer::I().MatP().Push(matP);
						Renderer::I().MatP().PushTranslation((float)x / numDividesX, (float)y / numDividesY, 0.f);
						Renderer::I().MatP().PushScaling(1.f / numDividesX, 1.f / numDividesY, 1.f);
						Renderer::I().MatP().PushScaling(.5f, .5f, 1.f);
						Renderer::I().MatP().PushTranslation(1.f, 1.f, 0.f);
						Renderer::I().MatP().PushScaling(1.f, -1.f, 1.f);
						{
							blitShader->m_vs->p["wvp"] =
								Renderer::I().MatP().Top() *
								Renderer::I().MatV().Top() *
								Renderer::I().MatW().Top();
							blitShader->m_ps->p["tex"] = &rtc;
							blitShader->Apply(&gfx);
							Renderer::I().RenderQuad();
						}
						Renderer::I().MatP().Pop();
						Renderer::I().MatP().Pop();
						Renderer::I().MatP().Pop();
						Renderer::I().MatP().Pop();
						Renderer::I().MatP().Pop();
						Renderer::I().MatP().Pop();
					}

					engine->Render();
				}
				gfx.SceneEnd();

				gfx.Present();
			}

			EventManager::I().RemoveEventHandler(&eventHandler);

			engine->Shutdown();
		}

		system->Shutdown();

		delete system;

		delete game;
	}

	printf("done! press ENTER to quit\n");
	char temp[2];
	gets(temp);

	return 0;
}

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
		Assert(vertexCount >= 2);
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
	}
};

DebugRender sDebugRender;
