#include <allegro.h>
#include "Renderer.h"
#include "SpriteMgr.h"

SpriteMgr g_SpriteMgr;

SpriteMgr::SpriteMgr()
{
	Initialize();
}

void SpriteMgr::Initialize()
{
}

ISprite* SpriteMgr::Load(const std::string& fileName)
{
	SpriteAllegro* result = new SpriteAllegro();

	result->m_Bitmap = load_bitmap(fileName.c_str(), 0);

	assert(result->m_Bitmap);

	return result;
}

void SpriteAllegro::Render(BITMAP* buffer, Vec2 pos, float rot)
{
	Vec2 p1(0.0f, 0.0f);
	Vec2 p2(1.0f, 0.0f);

	p1 = g_Renderer.Project(p1);
	p2 = g_Renderer.Project(p2);

	Vec2 d = p2 - p1;

	pos = p1;
	rot = -atan2(d[0], d[1]);

//	pivot_sprite(buffer, m_Bitmap, pos[0], pos[1], 0, 0, RAD2ALLEG(rot) * 65536.0f);
}
