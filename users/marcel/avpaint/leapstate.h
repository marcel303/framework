#pragma once

#define ENABLE_LEAPMOTION 1

#if ENABLE_LEAPMOTION

#include "leap/Leap.h"
#include "Vec3.h"

struct SDL_mutex;

struct XLeapState
{
	struct Finger
	{
		Vec3 position;
		Vec3 velocity;
		
		Finger()
			: position(0.f, 0.f, 0.f)
			, velocity(0.f, 0.f, 0.f)
		{
		}
	};
	
	struct Hand
	{
		bool active;
		int id;
		Vec3 position;
		Vec3 velocity;
		Finger fingers[5];
		
		Hand()
			: active(false)
			, id(-1)
			, position(0.f, 0.f, 0.f)
			, fingers()
		{
		}
	};
	
	Hand hands[2];
};

class LeapListener : public Leap::Listener
{
	SDL_mutex * mutex;
	XLeapState shadowState;

public:
	LeapListener();
	~LeapListener();

	void tick();
	virtual void onFrame(const Leap::Controller & controller);
};

extern XLeapState g_leapState;

#endif
