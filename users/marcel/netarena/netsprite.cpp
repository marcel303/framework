#include "Debugging.h"
#include "framework.h"
#include "netsprite.h"

NetSprite::NetSprite()
	: enabled(false)
	, sprite(0)
{
}

void NetSprite::set(const char * filename, int x, int y)
{
	if (sprite)
		delete sprite;
	enabled = true;
	sprite = new Sprite(filename);
	sprite->x = x;
	sprite->y = y;
	this->filename = filename;
}

//

NetSpriteManager::NetSpriteManager()
	: m_numFree(MAX_SPRITES)
{
	for (int i = 0; i < MAX_SPRITES; ++i)
		m_freeList[i] = i;
}

void NetSpriteManager::draw()
{
	setColor(255, 255, 255);

	for (int i = 0; i < MAX_SPRITES; ++i)
	{
		const NetSprite & sprite = m_sprites[i];

		if (sprite.enabled)
		{
			sprite.sprite->draw();
		}
	}
}

uint16_t NetSpriteManager::alloc()
{
	if (m_numFree == 0)
		return SPRITE_ID_INVALID;
	return m_freeList[--m_numFree];
}

void NetSpriteManager::free(uint16_t id)
{
	Assert(id != SPRITE_ID_INVALID);
	if (id != SPRITE_ID_INVALID)
	{
		NetSprite & sprite = m_sprites[id];
		sprite.enabled = false;
		m_freeList[m_numFree++] = id;
	}
}
