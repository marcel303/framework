#include "vfxNodeLeapMotion.h"
#include "leap/Leap.h"

class LeapListener : public Leap::Listener
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

public:
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
	// todo : read latest Leap Motion state
}
