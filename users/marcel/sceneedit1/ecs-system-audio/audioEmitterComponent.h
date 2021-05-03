#pragma once

#include "component.h"

#include "binauralizer.h"

#include "Mat4x4.h"

#include <mutex>

struct AudioEmitterComponent : Component<AudioEmitterComponent>
{
	bool dirty = false;
	
	virtual bool init() override final
	{
		return true;
	}
	
	virtual const char * getGizmoTexturePath() const override
	{
		return "gizmo-audio-emitter.png";
	}
	
	virtual void propertyChanged(void * address) override final
	{
		dirty = true;
	}
};

struct AudioEmitterComponentMgr : ComponentMgr<AudioEmitterComponent>
{
	// the audio emitter component mgr maintains a copy of the emitters accessible exclusively on the audio thread. this allows us to
	// add and remove audio emitter components without blocking the audio thread and vice versa processing the audio thread without
	// blocking the main thread. to manage the copy commands are used to schedule additions, removals and updates on the audio thread
	
	enum CommandType
	{
		kCommandType_None,
		kCommandType_AddEmitter,
		kCommandType_RemoveEmitter,
		kCommandType_UpdateEmitterParams,    // should be infrequent
		kCommandType_UpdateEmitterTransform, // should be infrequent, but could be each frame when parented to an animated node
		kCommandType_UpdateAudioParams
	};
	
	struct Command
	{
		CommandType type = kCommandType_None;
		
		struct
		{
			int id;
		} addEmitter;
		
		struct
		{
			int id;
		} removeEmitter;
		
		struct
		{
			int id;
			bool enabled;
		} updateEmitter;
		
		struct
		{
			int id;
			Mat4x4 objectToWorld;
		} updateEmitterTransform;
		
		struct
		{
			int bufferSize;
			int frameRate;
		} updateAudioParams;
	};
	
	struct Emitter
	{
		int id = -1;
		
		bool enabled = false;
		
		Mat4x4 objectToWorld;
		bool hasTransform = false;
	
		float * outputBuffer = nullptr;
		
		binaural::Mutex_Dummy * mutex = nullptr;
		
		binaural::Binauralizer * binauralizer = nullptr;
	};
	
	std::vector<Emitter> emitters;
	int audioBufferSize = 0;
	int audioFrameRate = 0;
	
	std::vector<Command> commands;
	std::mutex commands_mutex;
	
	bool audioThreadIsActive = false;
	
	virtual AudioEmitterComponent * createComponent(const int id) override final
	{
		auto * component = ComponentMgr<AudioEmitterComponent>::createComponent(id);
		
		component->dirty = true;
	
		commands_mutex.lock();
		{
			Command command;
			command.type = kCommandType_AddEmitter;
			command.addEmitter.id = id;
			commands.push_back(command);
		}
		commands_mutex.unlock();
		
		return component;
	}
	
	virtual void destroyComponent(const int id) override final
	{
		commands_mutex.lock();
		{
			Command command;
			command.type = kCommandType_RemoveEmitter;
			command.removeEmitter.id = id;
			commands.push_back(command);
		}
		commands_mutex.unlock();
	
		ComponentMgr<AudioEmitterComponent>::destroyComponent(id);
	}
	
	virtual void tickAlways() override final;
	
	void onAudioThreadBegin(const int frameRate, const int bufferSize);
	void onAudioThreadEnd();
	void onAudioThreadProcess();
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
