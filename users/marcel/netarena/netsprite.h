#pragma once

#include <stdint.h>
#include <string>

#define MAX_SPRITES 256
#define SPRITE_ID_INVALID 0xffff

class Sprite;

class NetSprite
{
public:
	NetSprite();

	void set(const char * filename, int x, int y);

	bool enabled;
	Sprite * sprite;
	std::string filename;
};

class NetSpriteManager
{
	uint16_t m_freeList[MAX_SPRITES];
	uint16_t m_numFree;

public:
	NetSprite m_sprites[MAX_SPRITES];

	NetSpriteManager();

	void draw();

	uint16_t alloc();
	void free(uint16_t id);
};
