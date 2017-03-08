#include "leapstate.h"

#if ENABLE_LEAPMOTION

#include <SDL2/SDL.h>

XLeapState g_leapState;

LeapListener::LeapListener()
	: mutex(nullptr)
	, shadowState()
{
	mutex = SDL_CreateMutex();
}

LeapListener::~LeapListener()
{
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

void LeapListener::tick()
{
	SDL_LockMutex(mutex);
	{
		g_leapState = shadowState;
	}
	SDL_UnlockMutex(mutex);
}

void LeapListener::onFrame(const Leap::Controller & controller)
{
	XLeapState leapState = shadowState;
	
	auto frame = controller.frame(0);
	auto hands = frame.hands();
	
	// deallocate unused hands
	
	for (int i = 0; i < 2; ++i)
	{
		bool used = false;
		
		for (auto hand = hands.begin(); hand != hands.end(); ++hand)
			if (leapState.hands[i].id == (*hand).id())
				used = true;
		
		if (used == false)
		{
			leapState.hands[i] = XLeapState::Hand();
		}
	}
	
	// update hands, allocate as necessary
	
	for (auto hand = hands.begin(); hand != hands.end(); ++hand)
	{
		const auto & h = *hand;
		
		XLeapState::Hand * usedHand = nullptr;
		XLeapState::Hand * freeHand = nullptr;
		
		for (int i = 0; i < 2; ++i)
		{
			if (h.id() == leapState.hands[i].id)
				usedHand = &leapState.hands[i];
			if (freeHand == nullptr && leapState.hands[i].active == false)
				freeHand = &leapState.hands[i];
		}
		
		if (usedHand == nullptr)
			usedHand = freeHand;
		
		if (usedHand != nullptr)
		{
			usedHand->active = true;
			usedHand->id = h.id();
			
			for (int i = 0; i < 3; ++i)
			{
				usedHand->position[i] = h.palmPosition()[i];
				usedHand->velocity[i] = h.palmVelocity()[i];
			}
			
			const auto fingers = h.fingers();
			
			for (auto finger = fingers.begin(); finger != fingers.end(); ++finger)
			{
				const auto & f = *finger;
				
				for (int i = 0; i < 3; ++i)
				{
					usedHand->fingers[f.type()].position[i] = f.tipPosition()[i];
					usedHand->fingers[f.type()].velocity[i] = f.tipVelocity()[i];
				}
			}
		}
	}
	
	SDL_LockMutex(mutex);
	{
		shadowState = leapState;
	}
	SDL_UnlockMutex(mutex);
}

#endif
