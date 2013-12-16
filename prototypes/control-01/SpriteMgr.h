#pragma once

#include <allegro.h>
#include "types.h"

class ISprite
{
public:
	virtual void Render(BITMAP* buffer, Vec2 pos, float rot) = 0;
};

class SpriteMgr
{
public:
	SpriteMgr();

	void Initialize();

	ISprite* Load(const std::string& fileName);
};

class SpriteAllegro : public ISprite
{
public:
	virtual void Render(BITMAP* buffer, Vec2 pos, float rot);

	BITMAP* m_Bitmap;
};

extern SpriteMgr g_SpriteMgr;
