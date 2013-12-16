#pragma once

#include "ParticleEffect.h"
#include "SelectionId.h"

class ParticleGenerator
{
public:
	// particles move in random direction from center, with different directions and speeds
	static void GenerateRandomExplosion(ParticleEffect& e, const Vec2F& pos, float minSpeed, float maxSpeed, float life, int particleCount, const Atlas_ImageInfo* image, float imageScale, ParticleUpdateCB updateCB);
	
	// particles move in random direction from center, with fixed speed
	static void GenerateCircularExplosion(ParticleEffect& e, const Vec2F& pos, float speed, float life, float size, int particleCount, const Atlas_ImageInfo* image, ParticleUpdateCB updateCB);
	
	// particles move in direction from center, with fixed angular steps
	static void GenerateCircularUniformExplosion(ParticleEffect& e, const Vec2F& pos, float baseAngle, float life, float size, int particleCount, const Atlas_ImageInfo* image, float imageScale, ParticleUpdateCB updateCB);
	
	// particles move a direction with a certain spread angle and fixed speed
	static void GenerateSparks(ParticleEffect& e, const Vec2F& pos, const Vec2F& dir, float spread, float speed, float life, float size, int particleCount, const Atlas_ImageInfo* image, ParticleUpdateCB updateCB);
	
	// particles that warp around an entity
	static void GenerateWarp(ParticleEffect& e, const Vec2F& pos, const Vec2F& dir, bool oriented, float spread, float speed1, float speed2, float warp, float falloff, float life, float size, int particleCount, const Atlas_ImageInfo* image, CD_TYPE entityId); 
};
