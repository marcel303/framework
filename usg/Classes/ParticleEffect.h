#pragma once

#include "Forward.h"
#include "libgg_forward.h"
#include "SpriteGfx.h"
#include "Types.h"

#define MAX_PARTICLES 600

class Particle;

typedef void (*ParticleUpdateCB)(Particle* particle, float dt);
typedef void (*ParticleRenderCB)(const Particle& particle);
typedef void (*ParticleForEachCB)(const Particle& particle);

class Particle
{
public:
	Particle();
	
	void Setup(float x, float y, float life, float sx, float sy, float angle);
	
	ParticleUpdateCB m_UpdateCB;
	ParticleRenderCB m_RenderCB;
	
	// -------------------------
	// Logic
	// -------------------------
	float m_Life;
	float m_LifeBeginRcp;
	
	// -------------------------
	// Drawing
	// -------------------------
	Vec2F m_Position;
	float m_Angle;
	float m_Sx;
	float m_Sy;
	const Atlas_ImageInfo* m_Image;
	const VectorShape* m_VectorShape;
	SpriteColor m_Color;
	
	// -------------------------
	// Behaviour
	// -------------------------
#ifdef PSP
	Vec2F m_CustomData[4];
#else
	char m_CustomData[32];
#endif
	
	inline void* Data_get()
	{
		return m_CustomData;
	}
	
	inline const void* Data_get() const
	{
		return m_CustomData;
	}
};

extern void Particle_Default_Setup(Particle* particle, float x, float y, float life, float sx, float sy, float angle, float speed);
extern void Particle_Default_Update(Particle* particle, float dt);
extern void Particle_RotMove_Setup(Particle* particle, float x, float y, float life, float sx, float sy, float angle, float vx, float vy, float vAngle);
extern void Particle_RotMove_Update(Particle* particle, float dt);

class ParticleEffect
{
public:
	ParticleEffect();
	void Initialize(int dataSetId);

	void Update(float dt);
	void Render(SpriteGfx* spriteGfx);
	
	Particle& Allocate(const Atlas_ImageInfo* image, const VectorShape* vectorShape, ParticleUpdateCB updateCB);
	
	void Clear();
	
	void ForEach(ParticleForEachCB cb);
				 
private:
	Sprite m_Sprite;
	Particle m_Particles[MAX_PARTICLES];
	int m_ParticleIndex;
	int m_DataSetId;
};
