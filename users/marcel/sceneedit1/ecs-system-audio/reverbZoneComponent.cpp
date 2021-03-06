#include "reverbZoneComponent.h"

#include "componentDraw.h"

#include "sceneNodeComponent.h"

#include "zita-rev1.h"

void ReverbZoneComponent::drawGizmo(ComponentDraw & draw) const
{
	const auto kOutlineColor = draw.makeColor(127, 127, 255);
	
	if (draw.isSelected)
	{
		draw.pushMatrix();
		{
			draw.multMatrix(draw.sceneNodeComponent->objectToWorld);
			
			draw.color(kOutlineColor);
			draw.lineCube(Vec3(), boxExtents);
		}
		draw.popMatrix();
	}
}
				
ReverbZoneComponentMgr g_reverbZoneComponentMgr;

ReverbZoneComponent * ReverbZoneComponentMgr::createComponent(const int id)
{
	auto * component = ComponentMgr<ReverbZoneComponent>::createComponent(id);
	
	component->dirty = true;
	
	commands_mutex.lock();
	{
		Command command;
		command.type = kCommandType_AddZone;
		command.addZone.id = id;
		commands.push_back(command);
	}
	commands_mutex.unlock();
	
	return component;
}

void ReverbZoneComponentMgr::destroyComponent(const int id)
{
	commands_mutex.lock();
	{
		Command command;
		command.type = kCommandType_RemoveZone;
		command.removeZone.id = id;
		commands.push_back(command);
	}
	commands_mutex.unlock();
	
	ComponentMgr<ReverbZoneComponent>::destroyComponent(id);
}

void ReverbZoneComponentMgr::tickAlways()
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
				command.type = kCommandType_UpdateZoneParams;
				command.updateZone.id = i;
				command.updateZone.enabled = comp->enabled;
				command.updateZone.boxExtents = comp->boxExtents;
				command.updateZone.preDelay = comp->preDelay;
				command.updateZone.t60Low = comp->t60Low;
				command.updateZone.t60Mid = comp->t60Mid;
				command.updateZone.t60CrossoverFrequency = comp->t60CrossoverFrequency;
				command.updateZone.dampingFrequency = comp->dampingFrequency;
				command.updateZone.eq1Gain = comp->eq1Gain;
				command.updateZone.eq2Gain = comp->eq2Gain;
				
				commands.push_back(command);
			}
			
			{
				auto * sceneNodeComp = g_sceneNodeComponentMgr.getComponent(comp->componentSet->id);
				
				Command command;
				command.type = kCommandType_UpdateZoneTransform;
				command.updateZoneTransform.id = i;
				command.updateZoneTransform.worldToObject = sceneNodeComp->objectToWorld.CalcInv();
				
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

void ReverbZoneComponentMgr::onAudioThreadBegin(const int frameRate, const int bufferSize)
{
	audioThreadIsActive = true;
	
	commands_mutex.lock();
	{
		Command command;
		command.type = kCommandType_UpdateAudioParams;
		command.updateAudioParams.frameRate = frameRate;
		command.updateAudioParams.bufferSize = bufferSize;
		
		commands.push_back(command);
	}
	commands_mutex.unlock();
}

void ReverbZoneComponentMgr::onAudioThreadEnd()
{
	audioThreadIsActive = false;
}

void ReverbZoneComponentMgr::onAudioThreadProcess()
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
				
			case kCommandType_AddZone:
				{
				#if defined(DEBUG)
					bool found = false;
					
					for (auto i = zones.begin(); i != zones.end(); ++i)
					{
						auto & zone = *i;
						if (zone.id == command.addZone.id)
						{
							found = true;
							break;
						}
					}
					
					Assert(found == false);
				#endif
				
					zones.emplace_back(Zone());
					auto & zone = zones.back();
					zone.id = command.addZone.id;
					zone.reverb = new ZitaRev1::Reverb();
					zone.reverb->init(audioFrameRate, false);
					zone.reverb->set_opmix(1.f);
					zone.reverb->set_rgxyz(0.f);
					zone.inputBuffer = new float[audioBufferSize];
				}
				break;
				
			case kCommandType_RemoveZone:
				{
					bool found = false;
					
					for (auto i = zones.begin(); i != zones.end(); ++i)
					{
						auto & zone = *i;
						if (zone.id == command.removeZone.id)
						{
							delete zone.inputBuffer;
							zone.inputBuffer = nullptr;
							
							delete zone.reverb;
							zone.reverb = nullptr;
							
							zones.erase(i);
							found = true;
							break;
						}
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateZoneParams:
				{
					bool found = false;
					
					for (auto & zone : zones)
					{
						if (zone.id != command.updateZone.id)
							continue;
						
						auto & params = command.updateZone;
						
						zone.enabled = params.enabled;
						zone.boxExtents = params.boxExtents;
						
						auto & reverb = *zone.reverb;
						reverb.set_delay(params.preDelay);
						reverb.set_rtlow(params.t60Low);
						reverb.set_rtmid(params.t60Mid);
						reverb.set_xover(params.t60CrossoverFrequency);
						reverb.set_fdamp(params.dampingFrequency);
						reverb.set_eq1( 160.f, params.eq1Gain);
						reverb.set_eq2(2500.f, params.eq2Gain);
						
						found = true;
						break;
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateZoneTransform:
				{
					bool found = false;
					
					for (auto & zone : zones)
					{
						if (zone.id != command.updateZoneTransform.id)
							continue;
						
						zone.worldToObject = command.updateZoneTransform.worldToObject;
						zone.hasTransform = true;
						
						found = true;
						break;
					}
					
					Assert(found);
				}
				break;
				
			case kCommandType_UpdateAudioParams:
				{
					audioFrameRate = command.updateAudioParams.frameRate;
					audioBufferSize = command.updateAudioParams.bufferSize;
					
					for (auto & zone : zones)
					{
						// re-initialize reverb
						
						zone.reverb->shut();
						zone.reverb->init(audioFrameRate, false);
						
						// re-allocate input buffer
						
						delete zone.inputBuffer;
						zone.inputBuffer = nullptr;
						
						zone.inputBuffer = new float[audioBufferSize];
					}
				}
				break;
			}
		}
		
		commands.clear();
	}
	commands_mutex.unlock();
}
