#ifndef RENDERLIST_H
#define RENDERLIST_H
#pragma once

#include <vector>
#include "Debug.h"
#include "MxPlane.h"
#include "ResShader.h"

class Entity;

enum RENDER_MASK
{
	RM_SOLID   = 0x01,
	RM_BLENDED = 0x02,
	RM_ALL     = 0xFF
};

class RenderList
{
public:
	RenderList();
	~RenderList();

	void Add(Entity* entity);
	void Remove(Entity* entity);

	void Render(int renderMask);

	void SetOverrideShader(ResShader* shader);

	typedef std::vector<Entity*> EntityColl;
	typedef EntityColl::iterator EntityCollItr;

	EntityCollItr Find(Entity* entity);

	EntityColl m_entities;

	ResShader* m_shader;
};

#endif
