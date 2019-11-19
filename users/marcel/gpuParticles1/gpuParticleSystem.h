#pragma once

#include "framework.h"

struct GpuParticleSystem
{
	Surface p; // xy = particle position, zw = particle velocity
	
	GxTextureId particleTexture = 0;
	
	int numParticles = 0;
	
	struct
	{
		float velocitySize = 1.f;
		float colorSize = 1.f;
	} dimensions;
	
	struct
	{
		Vec2 origin;
		float strength = 1.f;
	} gravity;
	
	struct
	{
		float strength = 1.f;
	} repulsion;
	
	struct
	{
		float strength = 1.f;
	} flow;
	
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
	
	Color baseColor = colorWhite;
	
	void init(const int numParticles, const int sx, const int sy);
	void shut();
	
	GxTextureId generateParticleTexture() const;
	
	void setBounds(const float minX, const float minY, const float maxX, const float maxY);
	
	void drawParticleVelocity() const;
	void drawParticleColor() const;
	
	void updateParticles(const GxTextureId flowfield);
};
