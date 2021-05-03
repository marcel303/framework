#include "audioEmitterComponent.h"

#include "sceneNodeComponent.h"

AudioEmitterComponentMgr g_audioEmitterComponentMgr;

void AudioEmitterComponentMgr::tickAlways()
{
	// queue update-params command
	
	commands_mutex.lock();
	{
		for (int i = 0; i < numComponents; ++i)
		{
			auto * comp = components[i];
			
			if (comp == nullptr)
				continue;
				
			if (comp->dirty)
			{
				comp->dirty = false;
				
				Command command;
				command.type = kCommandType_UpdateEmitterParams;
				command.updateEmitter.id = i;
				command.updateEmitter.enabled = comp->enabled;
				
				commands.push_back(command);
			}
			
			{
				auto * sceneNodeComp = comp->componentSet->find<SceneNodeComponent>();
				
				Command command;
				command.type = kCommandType_UpdateEmitterTransform;
				command.updateEmitterTransform.id = i;
				command.updateEmitterTransform.objectToWorld = sceneNodeComp->objectToWorld;
				
				commands.push_back(command);
			}
		}
	}
	commands_mutex.unlock();
	
	if (audioThreadIsActive == false)
	{
		// process commands on the main thread, to avoid the command buffer from filling up endlessly
		
		onAudioThreadProcess();
	}
}

void AudioEmitterComponentMgr::onAudioThreadBegin(const int frameRate, const int bufferSize)
{
	Assert(audioThreadIsActive == false);
	
	audioThreadIsActive = true;
	
	commands_mutex.lock();
	{
		Command command;
		command.type = kCommandType_UpdateAudioParams;
		command.updateAudioParams.bufferSize = bufferSize;
		command.updateAudioParams.frameRate = frameRate;
		
		commands.push_back(command);
	}
	commands_mutex.unlock();
}

void AudioEmitterComponentMgr::onAudioThreadEnd()
{
	Assert(audioThreadIsActive == true);
	
	audioThreadIsActive = false;
}

void AudioEmitterComponentMgr::onAudioThreadProcess()
{
	// execute commands from the command queue
	
	commands_mutex.lock();
	{
		for (auto & command : commands)
		{
			switch (command.type)
			{
			case kCommandType_None:
				Assert(false);
				break;
				
			case kCommandType_AddEmitter:
				{
				#if defined(DEBUG)
					bool found = false;
					
					for (auto i = emitters.begin(); i != emitters.end(); ++i)
					{
						auto & emitter = *i;
						if (emitter.id == command.addEmitter.id)
						{
							found = true;
							break;
						}
					}
					
					Assert(found == false);
				#endif
				
					emitters.emplace_back(Emitter());
					auto & emitter = emitters.back();
					emitter.id = command.addEmitter.id;
					emitter.mutex = new binaural::Mutex_Dummy();
					emitter.binauralizer = new binaural::Binauralizer();
					emitter.binauralizer->init(nullptr, emitter.mutex);
					emitter.outputBuffer = new float[audioBufferSize];
				}
				break;
				
			case kCommandType_RemoveEmitter:
				{
					bool found = false;
					
					for (auto i = emitters.begin(); i != emitters.end(); ++i)
					{
						auto & emitter = *i;
						if (emitter.id == command.removeEmitter.id)
						{
							delete emitter.outputBuffer;
							emitter.outputBuffer = nullptr;
							
							delete emitter.binauralizer;
							emitter.binauralizer = nullptr;
							
							delete emitter.mutex;
							emitter.mutex = nullptr;
							
							emitters.erase(i);
							found = true;
							break;
						}
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateEmitterParams:
				{
					bool found = false;
					
					for (auto & emitter : emitters)
					{
						if (emitter.id != command.updateEmitter.id)
							continue;
						
						auto & params = command.updateEmitter;
						
						emitter.enabled = params.enabled;
						
						found = true;
						break;
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateEmitterTransform:
				{
					bool found = false;
					
					for (auto & emitter : emitters)
					{
						if (emitter.id != command.updateEmitterTransform.id)
							continue;
						
						emitter.objectToWorld = command.updateEmitterTransform.objectToWorld;
						emitter.hasTransform = true;
						
						found = true;
						break;
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateAudioParams:
				{
					audioBufferSize = command.updateAudioParams.bufferSize;
					audioFrameRate = command.updateAudioParams.frameRate;
					
					for (auto & emitter : emitters)
					{
						delete emitter.outputBuffer;
						emitter.outputBuffer = nullptr;
						
						emitter.outputBuffer = new float[audioBufferSize];
					}
				}
				break;
			}
		}
		
		commands.clear();
	}
	commands_mutex.unlock();
}
