#include "binaural.h"
#include "binaural_cipic.h"
#include <map>
#include <memory>
#include <string>

namespace binaural
{
	enum Side
	{
		kSide_Left,
		kSide_Right
	};
	
	struct File
	{
		std::shared_ptr<SoundData> l;
		std::shared_ptr<SoundData> r;
	};
	
	static bool parseFilename(
		const char * filename,
		int & azimuth,
		Side & side)
	{
		std::string name = filename;
		
		int beginOffset = 0;
		int endOffset = 0;
		
		for (int i = 0; i < name.size(); ++i)
			if (name[i] == '\\' || name[i] == '/')
				beginOffset = i + 1;
		for (int i = 0; i < name.size(); ++i)
			if (name[i] == '.')
				endOffset = i;
		
		if (beginOffset >= endOffset)
			return false;
		
		name = name.substr(beginOffset, endOffset - beginOffset);
		
		int offset = 0;
		
		bool negativeAzimuth = false;
		
		if (offset + 3 <= name.size() && name[offset + 0] == 'n' && name[offset + 1] == 'e' && name[offset + 2] == 'g')
		{
			negativeAzimuth = true;
			offset += 3;
		}
		
		int begin = offset;
		while (offset < name.size() && name[offset] != 'a')
			offset++;
		if (offset == name.size())
			return false;
		if (parseInt32(name.substr(begin, offset - begin), azimuth) == false)
			return false;
		
		if (negativeAzimuth)
			azimuth = -azimuth;
		
		if (offset == name.size() || name[offset] != 'a')
			return false;
		offset++;
		if (offset == name.size() || name[offset] != 'z')
			return false;
		offset++;
		
		const std::string sideText = name.substr(offset);
		if (sideText == "left")
			side = kSide_Left;
		else if (sideText == "right")
			side = kSide_Right;
		else
			return false;
		
		return true;
	}

	//

	bool loadHRIRSampleSet_Cipic(const char * path, HRIRSampleSet & sampleSet)
	{
		std::vector<std::string> files;
		listFiles(path, true, files);
		
		debugTimerBegin("load_hrir_from_audio");
		
		std::map<int, File> filesByAzimuth;
		
		for (auto & filename : files)
		{
			int azimuth;
			Side side;
			
			if (parseFilename(filename.c_str(), azimuth, side) == false)
			{
				continue;
			}
			
			SoundData * soundData = loadSound(filename.c_str());
			
			if (soundData == nullptr)
			{
				debugLog("failed to load sound data");
			}
			else if (soundData->numChannels != 200 || soundData->sampleSize != 2 || soundData->numSamples != 50)
			{
				debugLog("sound data is not CIPIC encoded");
				
				delete soundData;
				soundData = nullptr;
			}
			else
			{
				File & file = filesByAzimuth[azimuth];
				
				//debugLog("sample: azimuth=%d, side=%c, file=%s", azimuth, side == kSide_Left ? 'L' : 'R', filename.c_str());
				
				if (side == kSide_Left)
				{
					debugAssert(file.l.get() == nullptr);
					file.l = std::shared_ptr<SoundData>(soundData);
				}
				
				if (side == kSide_Right)
				{
					debugAssert(file.r.get() == nullptr);
					file.r = std::shared_ptr<SoundData>(soundData);
				}
			}
		}
		
		sampleSet.samples.reserve(1250);
		
		int numAdded = 0;
		
		for (auto & fileItr : filesByAzimuth)
		{
			const int azimuth = fileItr.first;
			const File & file = fileItr.second;
			
			if (file.l == nullptr || file.r == nullptr)
			{
				debugLog("left or right ear HRIR is missing");
				
				continue;
			}
			
			//debugLog("sample: azimuth=%d, elevation=%d", azimuth, elevation);
			
			/*

			the CIPIC data set is stored in a peculiar format. each wave file contains 200 channels
			of PCM data, each 50 samples long. the filenames document the azimuth. the files themselves
			contain the HRIRs for 50 elevations. except for storing the samples for 50 elevations in 50
			channels, 200 samples in length (like a normal representation wold), the channels and samples
			are transposed. meaning, we get 200 channels of 50 samples in length

			the elevation for a channel within the wave file is giving by the equation
				elevation = -45 + 5.625 * channel (where channel = 0..49)

			azimuth goes from -90 to +90. elevation from -90 to +270

			az/el
			(0, 0) = in front
			(0, 90) = overhead
			(0, 180) = behind
			(0, 270) = below
			(90, 0) = right
			(-90, 0) = left

			*/
			
			for (int i = 0; i < 50; ++i)
			{
				short sampleData[200 * 2];
				
				const short * __restrict lSamples = (short*)file.l->sampleData;
				const short * __restrict rSamples = (short*)file.r->sampleData;
				
				for (int c = 0; c < 200; ++c)
				{
					sampleData[c * 2 + 0] = lSamples[c + i * 200];
					sampleData[c * 2 + 1] = rSamples[c + i * 200];
				}
				
				// transpose the sound data ..
				
				SoundData soundData;
				soundData.numChannels = 2;
				soundData.sampleSize = 2;
				soundData.numSamples = 200;
				soundData.sampleData = sampleData;
				
				const float elevation = -45 + 5.625 * i;
				
				float x, y, z;
				elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
				float newElevation;
				float newAzimuth;
				cartesianToElevationAndAzimuth(x, y, z, newElevation, newAzimuth);
				
				if (sampleSet.addHrirSampleFromSoundData(soundData, newElevation, newAzimuth, false))
				{
					numAdded++;
				}
				
				soundData.sampleData = nullptr;
			}
		}
		
		filesByAzimuth.clear();
		
		debugTimerEnd("load_hrir_from_audio");
		
		debugLog("done loading sample set. %d samples added in total", numAdded);
		
		return numAdded > 0;
	}
}
