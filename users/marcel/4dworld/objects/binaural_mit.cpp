#include "binaural.h"
#include "binaural_mit.h"
#include <string>

namespace binaural
{
	static bool parseFilename(
		const char * filename,
		int & elevation,
		int & azimuth)
	{
		const std::string name = filename;
		
		int offset = 0;
		
		for (int i = 0; i < name.size(); ++i)
			if (name[i] == '\\' || name[i] == '/')
				offset = i + 1;
		
		if (offset == name.size())
			return false;
		if (name[offset] != 'H')
			return false;
		offset++;
		
		int begin = offset;
		while (offset < name.size() && name[offset] != 'e')
			offset++;
		if (offset == name.size())
			return false;
		if (parseInt32(name.substr(begin, offset - begin), elevation) == false)
			return false;
		offset++;
		
		begin = offset;
		while (offset < name.size() && name[offset] != 'a')
			offset++;
		if (offset == name.size())
			return false;
		if (parseInt32(name.substr(begin, offset - begin), azimuth) == false)
			return false;
		offset++;
		
		return true;
	}

	//

	bool loadHRIRSampleSet_Mit(const char * path, HRIRSampleSet & sampleSet)
	{
		std::vector<std::string> files;
		listFiles(path, true, files);
		
		debugTimerBegin("load_hrir_from_audio");
		
		sampleSet.samples.reserve(sampleSet.samples.size() + files.size());
		
		int numAdded = 0;
		
		for (auto & filename : files)
		{
			// "H0e070a.wav"
			
			int elevation;
			int azimuth;
			
			if (parseFilename(filename.c_str(), elevation, azimuth) == false)
			{
				continue;
			}
			
			//debugLog("sample: azimuth=%d, elevation=%d", azimuth, elevation);
			
			SoundData * soundData = loadSound(filename.c_str());
			
			if (soundData == nullptr)
			{
				debugLog("failed to load sound data");
			}
			else
			{
				if (sampleSet.addHrirSampleFromSoundData(*soundData, elevation, -azimuth, false))
				{
					numAdded++;
				}
				
				if (azimuth != 0)
				{
					if (sampleSet.addHrirSampleFromSoundData(*soundData, elevation, +azimuth, true))
					{
						numAdded++;
					}
				}
				else
				{
					//debugLog("skipping HRIR mirroring for %s", filename.c_str());
				}
				
				if (elevation == 90)
				{
					if (sampleSet.addHrirSampleFromSoundData(*soundData, elevation, -180, false))
					{
						numAdded++;
					}
					
					if (sampleSet.addHrirSampleFromSoundData(*soundData, elevation, +180, false))
					{
						numAdded++;
					}
				}
				
				delete soundData;
				soundData = nullptr;
			}
		}
		
		debugTimerEnd("load_hrir_from_audio");
		
		debugLog("done loading sample set. %d samples added in total", numAdded);
		
		return numAdded > 0;
	}
}
