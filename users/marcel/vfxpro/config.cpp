#include "config.h"
#include "framework.h"
#include "tinyxml2.h"
#include "xml.h"

using namespace tinyxml2;

Config::Config()
{
	reset();
}

void Config::reset()
{
	midi = Midi();
	audioIn = AudioIn();
}

bool Config::load(const char * filename)
{
	bool result = true;

	XMLDocument xmlDoc;
		
	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

		result = false;
	}
	else
	{
		const XMLElement * xmlSettings = xmlDoc.FirstChildElement("settings");

		if (xmlSettings == 0)
		{
			logError("missing <settings> element");

			result = false;
		}
		else
		{
			const XMLElement * xmlMidi = xmlSettings->FirstChildElement("midi");

			if (xmlMidi == 0)
			{
				logWarning("missing <midi> element");
			}
			else
			{
				midi.enabled = boolAttrib(xmlMidi, "enabled", true);
				midi.deviceIndex = intAttrib(xmlMidi, "device_index", 0);

				for (const XMLElement * xmlMap = xmlMidi->FirstChildElement("map"); xmlMap; xmlMap = xmlMap->NextSiblingElement("map"))
				{
					const int id = intAttrib(xmlMap, "id", -1);
					const int externalId = intAttrib(xmlMap, "external_id", -1);

					if (id < 0 || id >= kMaxMidiMappings || externalId < 0 || externalId >= 256)
					{
						logError("invalid MIDI map. id=%d, external_id=%d", id, externalId);
					}
					else
					{
						midi.mapping[id] = externalId;
					}
				}
			}

			//

			const XMLElement * xmlAudioIn = xmlSettings->FirstChildElement("audio_in");

			if (xmlAudioIn == nullptr)
			{
				logWarning("missing <audio_in> element");
			}
			else
			{
				audioIn.enabled = boolAttrib(xmlAudioIn, "enabled", true);
				audioIn.deviceIndex = intAttrib(xmlAudioIn, "device_index", -1);
				audioIn.numChannels = intAttrib(xmlAudioIn, "num_channels", 2);
				audioIn.sampleRate = intAttrib(xmlAudioIn, "sample_rate", 48000);
				audioIn.bufferLength = intAttrib(xmlAudioIn, "buffer_length", 2048);
				audioIn.volume = intAttrib(xmlAudioIn, "volume", 100) / 100.f;

				audioIn.bufferLength *= audioIn.numChannels;
			}
		}
	}

	return result;
}

bool Config::midiIsMapped(int id) const
{
	if (!midi.enabled)
		return false;
	if (id < 0 || id >= kMaxMidiMappings)
		return false;
	if (midi.mapping[id] == -1)
		return false;
	return true;
}

bool Config::midiIsDown(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return ::midi.isDown(midi.mapping[id]);
}

bool Config::midiWentDown(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return ::midi.wentDown(midi.mapping[id]);
}

bool Config::midiWentUp(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return ::midi.wentUp(midi.mapping[id]);
}

float Config::midiGetValue(int id, float _default) const
{
	if (!midiIsMapped(id))
		return _default;
	return ::midi.getValue(midi.mapping[id], _default);
}
