#ifndef ENTITYSOUND_H
#define ENTITYSOUND_H
#pragma once

#include "Entity.h"
#include "ResSnd.h"
#include "ResSndSrc.h"

class EntitySound : public Entity
{
public:
	EntitySound(const std::string& filename);
	virtual ~EntitySound();
	
	virtual void OnSceneAdd(Scene* scene);
	virtual void OnSceneRemove(Scene* scene);
	virtual void OnTransformChange();

	ShSnd m_snd;
	ShSndSrc m_src;
	std::string m_filename;
};

#endif
