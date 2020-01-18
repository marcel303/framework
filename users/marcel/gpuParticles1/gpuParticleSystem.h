#pragma once

#include "framework.h"

struct GpuParticleSystem
{
	Surface p; // xy = particle position, zw = particle velocity
	
	GxTextureId particleTexture = 0;
	
	int numParticles = 0;
	
	struct Dimensions
	{
		float velocitySize = 1.f;
		float colorSize = 1.f;
	} dimensions;
	
	struct Gravity
	{
		Vec2 origin;
		float strength = 1.f;
	} gravity;
	
	struct Repulsion
	{
		float strength = 1.f;
	} repulsion;
	
	struct Flow
	{
		float strength = 1.f;
	} flow;
	
	float drag = 0.f;
	
	struct Bounds
	{
		enum Mode
		{
			kMode_Disabled,
			kMode_Bounce,
			kMode_Wrap
		};
		
		bool enabled = true;
		Mode xMode = kMode_Bounce;
		Mode yMode = kMode_Bounce;
		
		Vec2 min;
		Vec2 max;
	} bounds;
	
	struct DrawColor
	{
		float lightAmount = 1.f;
		float sizeThreshold = 0.f;
	} drawColor;
	
	Color baseColor = colorWhite;
	
	void init(const int numParticles, const int sx, const int sy);
	void shut();
	
	GxTextureId generateParticleTexture() const;
	
	void setBounds(const float minX, const float minY, const float maxX, const float maxY);
	
	void drawParticleVelocity(const int numParticles = 0) const;
	void drawParticleColor(const int numParticles = 0) const;
	
	void updateParticles(const GxTextureId flowfield, const float dt);
};
