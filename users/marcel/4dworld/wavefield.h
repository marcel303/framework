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

struct WaterSim1D
{
	const static int kMaxElems = 2048;
	
	int numElems;
	
	double p[kMaxElems];
	double v[kMaxElems];
	double f[kMaxElems];
	
	WaterSim1D();
	
	void init(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	
	float sample(const float x) const;
};

struct AudioSourceWavefield1D : AudioSource
{
	WaterSim1D m_waterSim;
	double m_sampleLocation;
	double m_sampleLocationSpeed;
	bool m_closedEnds;
	
	AudioSourceWavefield1D();
	
	void init(const int numElems);

	void tick(const double dt);

	virtual void generate(float * __restrict samples, const int numSamples) override;
};

//

struct WaterSim2D
{
	static const int kMaxElems = 128;
	
	int numElems;
	
	ALIGN16 double p[kMaxElems][kMaxElems];
	ALIGN16 double v[kMaxElems][kMaxElems];
	ALIGN16 double f[kMaxElems][kMaxElems];
	ALIGN16 double d[kMaxElems][kMaxElems];
	
	void init(const int numElems);
	void shut();
	
	WaterSim2D();
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	
	void doGaussianImpact(const int _x, const int _y, const int _radius, const double strength);
	float sample(const float x, const float y) const;
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
	
	WaterSim2D m_waterSim;
	double m_sampleLocation[2];
	double m_sampleLocationSpeed[2];
	bool m_slowMotion;
	CommandQueue<Command> m_commandQueue;
	
	AudioSourceWavefield2D();
	
	void init(const int numElems);
	
	void tick(const double dt);
	
	virtual void generate(float * __restrict samples, const int numSamples) override;
};
