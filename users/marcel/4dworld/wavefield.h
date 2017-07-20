#pragma once

#include "soundmix.h"
#include <list>
#include <SDL2/SDL.h> // fixme

struct SDL_mutex;

template <typename Command>
struct CommandQueue
{
	SDL_mutex * mutex;
	
	std::list<Command> commandList;

	CommandQueue()
		: mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}

	~CommandQueue()
	{
		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}
	}

	void push(const Command & command)
	{
		SDL_LockMutex(mutex);
		{
			commandList.push_back(command);
		}
		SDL_UnlockMutex(mutex);
	}

	bool pop(Command & command)
	{
		bool result = false;

		SDL_LockMutex(mutex);
		{
			if (commandList.empty() == false)
			{
				result = true;

				command = commandList.front();

				commandList.pop_front();
			}
		}
		SDL_UnlockMutex(mutex);

		return result;
	}
};

//

struct Wavefield1D
{
	const static int kMaxElems = 2048;
	
	int numElems;
	
	ALIGN32 double p[kMaxElems];
	ALIGN32 double v[kMaxElems];
	ALIGN32 double f[kMaxElems];
	ALIGN32 double d[kMaxElems];
	
	Wavefield1D();
	
	void init(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	
	float sample(const float x) const;
};

struct AudioSourceWavefield1D : AudioSource
{
	Wavefield1D m_wavefield;
	double m_sampleLocation;
	double m_sampleLocationSpeed;
	bool m_closedEnds;
	
	AudioSourceWavefield1D();
	
	void init(const int numElems);

	void tick(const double dt);

	virtual void generate(float * __restrict samples, const int numSamples) override;
};

//

struct Wavefield2D
{
	static const int kMaxElems = 64;
	
	int numElems;
	
	ALIGN32 double p[kMaxElems][kMaxElems];
	ALIGN32 double v[kMaxElems][kMaxElems];
	ALIGN32 double f[kMaxElems][kMaxElems];
	ALIGN32 double d[kMaxElems][kMaxElems];
	
	void init(const int numElems);
	void shut();
	
	Wavefield2D();
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	void tickForces(const double dt, const double c, const bool _closedEnds);
	void tickVelocity(const double dt, const double vRetainPerSecond, const double pRetainPerSecond);
	
	void doGaussianImpact(const int _x, const int _y, const int _radius, const double strength);
	float sample(const float x, const float y) const;
	
	void copyFrom(const Wavefield2D & other, const bool copyP, const bool copyV, const bool copyF);
};

struct AudioSourceWavefield2D : AudioSource
{
	struct Command
	{
		int x;
		int y;
		int radius;
		
		double strength;
		
		Command()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	//
	
	Wavefield2D m_wavefield;
	double m_sampleLocation[2];
	double m_sampleLocationSpeed[2];
	bool m_slowMotion;
	CommandQueue<Command> m_commandQueue;
	
	AudioSourceWavefield2D();
	
	void init(const int numElems);
	
	void tick(const double dt);
	
	virtual void generate(float * __restrict samples, const int numSamples) override;
};
