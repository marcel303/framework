#include "binaural.h"
#include "binaural_ircam.h"

namespace binaural
{
	void splitString(const std::string & str, std::vector<std::string> & result, const char delimiter)
	{
		int start = -1;
		
		for (size_t i = 0; i <= str.size(); ++i)
		{
			const char c = i < str.size() ? str[i] : delimiter;
			
			if (start == -1)
			{
				// found start
				if (c != delimiter)
					start = i;
			}
			else if (c == delimiter)
			{
				// found end
				result.push_back(str.substr(start, i - start));
				start = -1;
			}
		}
	}
	
	static bool parseFilename(
		const char * filename,
		int & subjectId,
		int & radius,
		int & elevation,
		int & azimuth)
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
		
		// IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav;
		
		auto logConventionMismatch = []()
		{
			debugLog("error: filename doesn't match conversion: IRC_<subject_ID>_<status>_R<radius>_T<azimuth>_P<elevation>.wav");
		};
		
		std::vector<std::string> parts;
		splitString(name, parts, '_');
		
		if (parts.size() != 6)
		{
			logConventionMismatch();
			return false;
		}
		
		debugAssert(parts[0] == "IRC");
		debugAssert(parts[3][0] == 'R');
		debugAssert(parts[4][0] == 'T');
		debugAssert(parts[5][0] == 'P');
		
		if (parts[0] != "IRC")
		{
			logConventionMismatch();
			return false;
		}
		
		if (parseInt32(parts[1], subjectId) == false)
		{
			logConventionMismatch();
			return false;
		}
		
		if (parts[3][0] != 'R' || parseInt32(parts[3].substr(1), radius) == false)
		{
			logConventionMismatch();
			return false;
		}
		
		if (parts[4][0] != 'T' || parseInt32(parts[4].substr(1), azimuth) == false)
		{
			logConventionMismatch();
			return false;
		}
		
		if (parts[5][0] != 'P' || parseInt32(parts[5].substr(1), elevation) == false)
		{
			logConventionMismatch();
			return false;
		}
		
		if (radius < 0 || azimuth < 0 || elevation < 0)
		{
			debugAssert("radius, azimuth or elevation is < 0");
			return false;
		}
		
		return true;
	}

	bool loadHRIRSampleSet_Ircam(const char * path, HRIRSampleSet & sampleSet)
	{
		std::vector<std::string> files;
		listFiles(path, false, files);
		
		debugTimerBegin("load_hrir_from_audio");
		
		sampleSet.samples.reserve(sampleSet.samples.size() + files.size());
		
		int numAdded = 0;
		
		for (auto & filename : files)
		{
			int subjectId;
			int radius;
			int elevation;
			int azimuth;
			
			if (parseFilename(filename.c_str(), subjectId, radius, elevation, azimuth) == false)
			{
				continue;
			}
			
			//debugLog("sample: subjectId=%d, radius=%d, azimuth=%d, elevation=%d", subjectId, radius, azimuth, elevation);
			
			SoundData * soundData = loadSound(filename.c_str());
			
			if (soundData == nullptr)
			{
				debugLog("failed to load sound data");
			}
			else
			{
				if (sampleSet.addHrirSampleFromSoundData(*soundData, elevation, azimuth, false))
				{
					numAdded++;
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
