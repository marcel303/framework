#pragma once

#include <Box2D/Box2D.h>
#include <vector>
#include "Forward.h"

class Sim
{
public:
	Sim();
	~Sim();
	
	void Setup();
	void Update(float dt);
	void RenderGL(SpriteGfx* gfx);
	
	b2World* world;
	std::vector<b2Body*> bodyList;
};
