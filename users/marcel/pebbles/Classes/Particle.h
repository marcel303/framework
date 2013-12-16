#pragma once

typedef bool (*ParticleSegmentCb)(void* obj, class Particle* p, float x1, float y1, float x2, float y2);

class Particle
{
public:
	void Setup(float x, float y, float vx, float vy, float reflection, float falloff);
	
	void Update(float dt, ParticleSegmentCb cb, void* cbObj);
	void Render();
	
	void Reflect(float planeX, float planeY);
	
	bool mIsAlive;
	
public:
	void Reset();
	
	float mPosX;
	float mPosY;
	float mSpeedX;
	float mSpeedY;
	float mAccelX;
	float mAccelY;
	float mReflection;
	float mFalloff;
};

//

class ParticleMgr
{
public:
	ParticleMgr(float gravX, float gravY, int particleCount, ParticleSegmentCb segmentCb, void* cbObj);
	~ParticleMgr();
	
	Particle& Allocate();
	
	void Update(float dt);
	void Render();
	
private:
	float mGravX;
	float mGravY;
	Particle* mParticleList;
	int mParticleCount;
	int mParticleIdx;
	ParticleSegmentCb mSegmentCb;
	void* mCbObj;
};
