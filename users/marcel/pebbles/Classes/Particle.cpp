#include <cmath>
#include "Particle.h"
#include "render.h"

#define SIZE 5.0f

void Particle::Setup(float x, float y, float vx, float vy, float reflection, float falloff)
{
	Reset();
	
	mPosX = x;
	mPosY = y;
	mSpeedX = vx;
	mSpeedY = vy;
	mAccelX = 0.0f;
	mAccelY = 0.0f;
	mReflection = reflection;
	mFalloff = falloff;
}

void Particle::Update(float dt, ParticleSegmentCb cb, void* cbObj)
{
	mSpeedX += mAccelX * dt;
	mSpeedY += mAccelY * dt;
	
	float x = mPosX + mSpeedX * dt;
	float y = mPosY + mSpeedY * dt;
	
	if (cb(cbObj, this, mPosX, mPosY, x, y))
	{
		mPosX = x;
		mPosY = y;
	}
	
	mSpeedX *= std::pow(mFalloff, dt);
	mSpeedY *= std::pow(mFalloff, dt);

	mAccelX = 0.0f;
	mAccelY = 0.0f;
}

void Particle::Render()
{
	gRender->Quad(mPosX - SIZE, mPosY - SIZE, mPosX + SIZE, mPosY + SIZE, Color(1.0f, 1.0f, 1.0f));
}

void Particle::Reflect(float planeX, float planeY)
{
	float d = mSpeedX * planeX + mSpeedY * planeY;
	
	mSpeedX -= planeX * d * mReflection;
	mSpeedY -= planeY * d * mReflection;
}

void Particle::Reset()
{
	mIsAlive = true;
}

//

ParticleMgr::ParticleMgr(float gravX, float gravY, int particleCount, ParticleSegmentCb segmentCb, void* cbObj)
{
	mGravX = gravX;
	mGravY = gravY;
	mParticleList = new Particle[particleCount];
	mParticleCount = particleCount;
	mParticleIdx = -1;
	mSegmentCb = segmentCb;
	mCbObj = cbObj;
}

ParticleMgr::~ParticleMgr()
{
	delete[] mParticleList;
	mParticleList = 0;
}

Particle& ParticleMgr::Allocate()
{
	mParticleIdx++;
	
	if (mParticleIdx == mParticleCount)
		mParticleIdx = 0;
	
	return mParticleList[mParticleIdx];
}

void ParticleMgr::Update(float dt)
{
	for (int i = 0; i < mParticleCount; ++i)
	{
		if (mParticleList[i].mIsAlive)
		{
			mParticleList[i].mAccelX += mGravX;
			mParticleList[i].mAccelY += mGravY;
			mParticleList[i].Update(dt, mSegmentCb, mCbObj);
		}
	}
}

void ParticleMgr::Render()
{
	for (int i = 0; i < mParticleCount; ++i)
		if (mParticleList[i].mIsAlive)
			mParticleList[i].Render();
}
