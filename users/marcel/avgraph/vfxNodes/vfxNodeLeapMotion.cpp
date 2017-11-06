/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#if ENABLE_LEAPMOTION

#include "vfxNodeLeapMotion.h"
#include "leap/Leap.h"
#include <SDL2/SDL.h>

VFX_NODE_TYPE(VfxNodeLeapMotion)
{
	typeName = "leap";
	
	out("leftHandX", "float");
	out("leftHandY", "float");
	out("leftHandZ", "float");
	out("rightHandX", "float");
	out("rightHandY", "float");
	out("rightHandZ", "float");
}

struct LeapListener : public Leap::Listener
{
	struct State
	{
		float leftHandPos[3];
		float rightHandPos[3];
		
		State()
		{
			memset(this, 0, sizeof(*this));
		}
	};
	
	SDL_mutex * mutex;
	State state;
	
	LeapListener()
		: mutex(nullptr)
		, state()
	{
		mutex = SDL_CreateMutex();
	}

	~LeapListener()
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	virtual void onFrame(const Leap::Controller & controller)
	{
		SDL_LockMutex(mutex);
		{
			auto frame = controller.frame(0);
			auto hands = frame.hands();
			
			for (auto & hand : hands)
			{
				if (hand.isLeft())
				{
					auto palmPosition = hand.palmPosition();
					
					state.leftHandPos[0] = palmPosition.x;
					state.leftHandPos[1] = palmPosition.y;
					state.leftHandPos[2] = palmPosition.z;
				}
				else
				{
					auto palmPosition = hand.palmPosition();
					
					state.rightHandPos[0] = palmPosition.x;
					state.rightHandPos[1] = palmPosition.y;
					state.rightHandPos[2] = palmPosition.z;
				}
			}
		}
		SDL_UnlockMutex(mutex);
	}
	
	State getState() const
	{
		State result;
		
		SDL_LockMutex(mutex);
		{
			result = state;
		}
		SDL_UnlockMutex(mutex);
		
		return state;
	}
};

VfxNodeLeapMotion::VfxNodeLeapMotion()
	: VfxNodeBase()
	, leftHandX(0.f)
	, leftHandY(0.f)
	, leftHandZ(0.f)
	, rightHandX(0.f)
	, rightHandY(0.f)
	, rightHandZ(0.f)
	, leapController(nullptr)
	, leapListener(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addOutput(kOutput_LeftHandX, kVfxPlugType_Float, &leftHandX);
	addOutput(kOutput_LeftHandY, kVfxPlugType_Float, &leftHandY);
	addOutput(kOutput_LeftHandZ, kVfxPlugType_Float, &leftHandZ);
	addOutput(kOutput_RightHandX, kVfxPlugType_Float, &rightHandX);
	addOutput(kOutput_RightHandY, kVfxPlugType_Float, &rightHandY);
	addOutput(kOutput_RightHandZ, kVfxPlugType_Float, &rightHandZ);
}

VfxNodeLeapMotion::~VfxNodeLeapMotion()
{
	leapController->removeListener(*leapListener);

	delete leapController;
	leapController = nullptr;

	delete leapListener;
	leapListener = nullptr;
}

void VfxNodeLeapMotion::init(const GraphNode & node)
{
	leapController = new Leap::Controller();
	leapController->setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	leapListener = new LeapListener();
	leapController->addListener(*leapListener);
}

void VfxNodeLeapMotion::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeLeapMotion);
	
	const bool isConnected = leapController->isConnected();
	
	if (isConnected)
	{
		// read latest Leap Motion state
		
		const LeapListener::State state = leapListener->getState();
		
		leftHandX = state.leftHandPos[0];
		leftHandY = state.leftHandPos[1];
		leftHandZ = state.leftHandPos[2];
		
		rightHandX = state.rightHandPos[0];
		rightHandY = state.rightHandPos[1];
		rightHandZ = state.rightHandPos[2];
	}
	else
	{
		leftHandX = 0.f;
		leftHandY = 0.f;
		leftHandZ = 0.f;
		
		rightHandX = 0.f;
		rightHandY = 0.f;
		rightHandZ = 0.f;
	}
	
	setOuputIsValid(kOutput_LeftHandX, isConnected);
	setOuputIsValid(kOutput_LeftHandY, isConnected);
	setOuputIsValid(kOutput_LeftHandZ, isConnected);
	setOuputIsValid(kOutput_RightHandX, isConnected);
	setOuputIsValid(kOutput_RightHandY, isConnected);
	setOuputIsValid(kOutput_RightHandZ, isConnected);
}

void VfxNodeLeapMotion::getDescription(VfxNodeDescription & d)
{
	d.add("Leap Motion connected: %d", leapController->isConnected());
	d.add("Leap Motion has focus: %d", leapController->hasFocus());
}

#endif
