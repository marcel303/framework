#pragma once

#include <vector>
#include "AudioOutput.h"
#include "AudioMixer.h"
#include "AudioStreamVorbis.h"
#include "FixedSizeString.h"

class AudioSheet
{
public:
	std::vector<int> mFadeIn;
	std::vector<int> mFadeOut;
};

class AudioManager
{
public:
	AudioManager()
	{
	}
	
	void Initialize()
	{
		mAudioOutput.Initialize(2, 44100); // fixme!
		mAudioOutput.Play();
	}
	
	void Shutdown()
	{
		mAudioOutput.Shutdown();
	}
	
	void Update()
	{
		Layer& mainLayer = mLayers[0];
		
		if (mainLayer.isSet)
		{
			AudioSheet& mainSheet = *mainLayer.sheet;
			
			int layerPosition[kMaxLayers][2];
			bool layerHasLooped[kMaxLayers];
			
			for (int i = 0; i < 4; ++i)
				if (mLayers[i].isSet)
					layerPosition[i][0] = mLayers[i].vorbisStream.Position_get();
			
			UpdateAudioOutput();
			
			for (int i = 0; i < 4; ++i)
			{
				if (mLayers[i].isSet)
				{
					layerPosition[i][1] = mLayers[i].vorbisStream.Position_get();
					layerHasLooped[i] = mLayers[i].vorbisStream.HasLooped_get();
				}
			}
			
			// actually provided some samples?
			
			int mainPosition1 = layerPosition[0][0];
			int mainPosition2 = layerPosition[0][1];
			bool mainHasLooped = layerHasLooped[0];
			
#if 0
#warning
			if (CheckSyncList(mainSheet.mFadeIn, mainPosition1, mainPosition2, mainHasLooped))
			{
				//LOG_DBG("sync check passed", 0);
			}
#endif
			
			if (mainPosition1 != mainPosition2)
			{
				// check for layer activation
				
				for (int i = 0; i < kMaxLayers; ++i)
				{
					Layer& layer = mLayers[i];
					
					if (layer.isSet)
					{
						if (layer.isActive && !layer.isActiveEffective)
						{
							// check if we hit a sync point
							
							std::vector<int>& syncList = mainSheet.mFadeIn;
							
							if (CheckSyncList(syncList, mainPosition1, mainPosition2, mainHasLooped))
							{
								LOG_DBG("audio layer activated. layerIndex=%d", i);
								
								AudioRamp ramp;
								ramp.MakeFade(mAudioMixer.GetRampValue(i), 255, kAudioRampMaxSize);
								mAudioMixer.SetRamp(i, &ramp);
								
								layer.isActiveEffective = layer.isActive;
							}
						}
					}
				}
			}

			// check for layer deactivation
			
			for (int i = 0; i < kMaxLayers; ++i)
			{
				// actually provided some samples?
				
				int layerPosition1 = layerPosition[i][0];
				int layerPosition2 = layerPosition[i][1];
				
				if (layerPosition1 != layerPosition2)
				{
					Layer& layer = mLayers[i];
					
					if (layer.isSet)
					{
						if (!layer.isActive && layer.isActiveEffective)
						{
							AudioSheet& layerSheet = *layer.sheet;
							
							// check if we hit a sync point
							
							std::vector<int>& syncList = layerSheet.mFadeOut;
							
							if (CheckSyncList(syncList, layerPosition1, layerPosition2, layerHasLooped[i]))
							{
								LOG_DBG("audio layer deactivated. layerIndex=%d", i);
								
								AudioRamp ramp;
								ramp.MakeFade(mAudioMixer.GetRampValue(i), 0, kAudioRampMaxSize);
								mAudioMixer.SetRamp(i, &ramp);
								
								layer.isActiveEffective = layer.isActive;
							}
						}
					}
				}
			}
		}
	}
	
	void AudioLayerData_set(int layerIndex, const char* fileName, AudioSheet* sheet)
	{
		Assert(layerIndex >= 0 && layerIndex <= kMaxLayers-1);
		
		Layer& layer = mLayers[layerIndex];
		
		//
		
		layer.vorbisStream.Close();
		
		//
		
		layer.isSet = true;
		layer.fileName = fileName;
		layer.sheet = sheet;
		layer.isActive = true;
		layer.isActiveEffective = true;
		
		//
		
		layer.vorbisStream.Open(fileName, true);
		
		mAudioMixer.SetStream(layerIndex, &layer.vorbisStream);
	}
	
	bool AudioLayerIsActive_get(int layerIndex)
	{
		Assert(layerIndex >= 0 && layerIndex <= kMaxLayers-1);
		return mLayers[layerIndex].isActive;
	}
	
	void AudioLayerIsActive_set(int layerIndex, bool isActive)
	{
		Assert(layerIndex >= 0 && layerIndex <= kMaxLayers-1);
		
		Layer& layer = mLayers[layerIndex];
		
		layer.isActive = isActive;
		
		if (layer.isSet)
		{
			LOG_DBG("changed layer activation. channelIndex=%d, isActive=%d", layerIndex, isActive ? 1 : 0);
		}
		else
		{
			LOG_DBG("warning: attempt to change layer activation for unused layer index %d", layerIndex);
		}
	}
	
private:
	const static int kMaxLayers = 4;
	
	void UpdateAudioOutput()
	{
		mAudioOutput.Update(&mAudioMixer);
	}
	
	bool CheckSyncList(std::vector<int>& syncList, int position1, int position2, bool hasLooped)
	{
		return true;
		
		for (size_t j = 0; j < syncList.size(); ++j)
		{
			int position = syncList[j];
			
			if (hasLooped)
			{
				if (position >= position1)
				{
					LOG_DBG("sync check passed. looped=1, position1=%d, position2=%d, position=%d", position1, position2, position);
					return true;
				}
				if (position < position2)
				{
					LOG_DBG("sync check passed. looped=1, position1=%d, position2=%d, position=%d", position1, position2, position);
					return true;
				}
			}
			else
			{
				if (position >= position1 && position < position2)
				{
					LOG_DBG("sync check passed. looped=0, position1=%d, position2=%d, position=%d", position1, position2, position);
					return true;
				}
			}
		}
		
		return false;
	}
	
	class Layer
	{
	public:
		Layer()
			: isSet(false)
			, sheet(0)
			, isActive(false)
			, isActiveEffective(false)
		{
		}
		
		bool isSet;
		FixedSizeString<256> fileName;
		AudioSheet* sheet;
		AudioStream_Vorbis vorbisStream;
		bool isActive;
		bool isActiveEffective;
	};
	
	Layer mLayers[kMaxLayers];
	
	AudioOutput_OpenAL mAudioOutput;
	AudioMixer mAudioMixer;
};
