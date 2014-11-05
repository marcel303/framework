#include "Engine.h"
#include "GraphicsDevice.h"
#include "Mem.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "SoundDevice.h"
#include "System.h"

System::System()
{
	m_game = 0;
	m_engine = 0;
	m_gfx = 0;
	m_sfx = 0;
}

System::~System()
{
	Assert(m_gfx == 0);
}

void System::Initialize(Game* game, int width, int height, bool fullscreen)
{
	m_game = game;

	RegisterFileSystems();

	RegisterResourceLoaders();

	m_gfx = CreateGraphicsDevice();
	m_sfx = CreateSoundDevice();

	if (m_gfx)
		m_gfx->InitializeV1(width, height, fullscreen);

	if (m_sfx)
	{
		m_sfx->SetWorldScale(100.0f);
		m_sfx->Initialize();
	}

	Renderer::I().Initialize();

	Renderer::I().SetGraphicsDevice(m_gfx);
	Renderer::I().SetSoundDevice(m_sfx);

	ResMgr::I().Initialize();

	m_engine = new Engine(m_game);
}

void System::Shutdown()
{
	SAFE_FREE(m_engine);

	ResMgr::I().Shutdown();

	Renderer::I().SetGraphicsDevice(0);
	Renderer::I().SetSoundDevice(0);

	Renderer::I().Shutdown();

	m_gfx->Shutdown();
	m_sfx->Shutdown();

	SAFE_FREE(m_gfx);
	SAFE_FREE(m_sfx);

	// TODO: Remove resouce loaders.

	// TODO: Remove file systems.
}

Engine* System::GetEngine()
{
	return m_engine;
}

GraphicsDevice* System::GetGraphicsDevice()
{
	return m_gfx;
}

SoundDevice* System::GetSoundDevice()
{
	return m_sfx;
}

void System::RegisterFileSystems()
{
}

void System::RegisterResourceLoaders()
{
}

GraphicsDevice* System::CreateGraphicsDevice()
{
	return 0;
}

SoundDevice* System::CreateSoundDevice()
{
	return 0;
}
