#pragma once

#include <math.h>
#include <vector>
#include "Calc.h"
#include "Surface.h"

class Particle;

typedef double (*ParticleDensity_getCB)(Particle* p, double x, double y);
typedef Pixel (*ColorRampCB)(double density);

class Modifier
{
public:
	Modifier()
	{
		mData = 0;
	}
	
	virtual void Update(Particle* p, double dt) = 0;
	
private:
	void* mData;
	
	friend class ModifierList;
};

class ModifierList
{
public:
	ModifierList()
	{
	}
	
	void Add(Modifier* modifier)
	{
		mModifierList.push_back(modifier);
	}
	
	void Update(Particle* p, double dt)
	{
		for (size_t i = 0; i < mModifierList.size(); ++i)
			mModifierList[i]->Update(p, dt);
	}
	
private:	
	std::vector<Modifier*> mModifierList;
};

class Particle
{
public:
	Particle();
	void Setup(double x, double y, double vx, double vy, double life);
	void Update(double dt);
	
	ParticleDensity_getCB Density_get;
	
	inline bool IsDead_get() const
	{
		return mLife <= 0.0;
	}
	
	inline double LifeFactor_get() const
	{
		return mLife * mLifeRcp;
	}
	
	template <class U>
	inline U& DensityData_get()
	{
		return *((U*)mDensityData);
	}
	
	inline double Distance_get(double x, double y) const
	{
		Vec d = mPosition - Vec(x, y);
		return d.L();
	}
	
	inline ModifierList& ModifierList_get()
	{
		return mModifierList;
	}
	
	Vec mPosition;
	Vec mSpeed;
	Vec mForce;
	double mLife;
	double mLifeRcp;
	
	ModifierList mModifierList;
	uint8_t mDensityData[128];
};
