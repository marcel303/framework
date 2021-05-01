#pragma once

#include "component.h"

#include "binauralizer.h"

#include <mutex>

struct AudioEmitterComponent : Component<AudioEmitterComponent>
{
	struct Mutex : binaural::Mutex
	{
		std::mutex mutex;
		
		virtual void lock() override final { mutex.lock(); }
		virtual void unlock() override final { mutex.unlock(); }
	};
	
	float outputBuffer[256]; // todo : max buffer size
	
	Mutex mutex;
	
	binaural::Binauralizer binauralizer;
	
	virtual bool init() override final
	{
		memset(outputBuffer, 0, sizeof(outputBuffer));
		
		binauralizer.init(nullptr, &mutex);
		
		return true;
	}
};

struct AudioEmitterComponentMgr : ComponentMgr<AudioEmitterComponent>
{
	std::mutex mutex;
	
	virtual AudioEmitterComponent * createComponent(const int id) override final
	{
		mutex.lock();
		
		auto * component = ComponentMgr<AudioEmitterComponent>::createComponent(id);
		
		mutex.unlock();
		
		// todo : register for audio processing
		
		return component;
	}
	
	virtual void destroyComponent(const int id) override final
	{
		// todo : unregister from audio processing
	
		mutex.lock();
		
		ComponentMgr<AudioEmitterComponent>::destroyComponent(id);
		
		mutex.unlock();
	}
	
	virtual void tick(const float dt) override final
	{
		// todo : update audio processing properties
	}
};

extern AudioEmitterComponentMgr g_audioEmitterComponentMgr;

#if defined(DEFINE_COMPONENT_TYPES)

#include "componentType.h"

struct AudioEmitterComponentType : ComponentType<AudioEmitterComponent>
{
	AudioEmitterComponentType()
		: ComponentType("AudioEmitterComponent", &g_audioEmitterComponentMgr)
	{
	}
};

#endif
