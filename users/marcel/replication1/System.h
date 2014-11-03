#ifndef SYSTEM_H
#define SYSTEM_H
#pragma once

class Engine;
class Game;
class GraphicsDevice;
class SoundDevice;

class System
{
public:
	System();
	~System();

	void Initialize(Game* game, int width, int height, bool fullscreen);
	void Shutdown();

	Engine* GetEngine();
	GraphicsDevice* GetGraphicsDevice();
	SoundDevice* GetSoundDevice();

	// System setup.
	virtual void RegisterFileSystems();
	virtual void RegisterResourceLoaders();
	virtual GraphicsDevice* CreateGraphicsDevice();
	virtual SoundDevice* CreateSoundDevice();

private:
	GraphicsDevice* m_gfx;
	SoundDevice* m_sfx;
	Game* m_game;
	Engine* m_engine;
};

#endif
