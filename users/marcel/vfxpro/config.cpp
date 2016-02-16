#include "config.h"
#include "framework.h"
#include "tinyxml2.h"

using namespace tinyxml2;

Config::Config()
{
	reset();
}

void Config::reset()
{
	audioIn = AudioIn();

	midiEnabled = false;
	memset(midiMapping, -1, sizeof(midiMapping));
}

// tinyxml helper functions

static const char * stringAttrib(const XMLElement * elem, const char * name, const char * defaultValue)
{
	if (elem->Attribute(name))
		return elem->Attribute(name);
	else
		return defaultValue;
}

static bool boolAttrib(const XMLElement * elem, const char * name, bool defaultValue)
{
	if (elem->Attribute(name))
		return elem->BoolAttribute(name);
	else
		return defaultValue;
}

static int intAttrib(const XMLElement * elem, const char * name, int defaultValue)
{
	if (elem->Attribute(name))
		return elem->IntAttribute(name);
	else
		return defaultValue;
}

static float floatAttrib(const XMLElement * elem, const char * name, float defaultValue)
{
	if (elem->Attribute(name))
		return elem->FloatAttribute(name);
	else
		return defaultValue;
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
				midiEnabled = boolAttrib(xmlMidi, "enabled", true);

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
						midiMapping[id] = externalId;
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
	if (!midiEnabled)
		return false;
	if (id < 0 || id >= kMaxMidiMappings)
		return false;
	if (midiMapping[id] == -1)
		return false;
	return true;
}

bool Config::midiIsDown(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return midi.isDown(midiMapping[id]);
}

bool Config::midiWentDown(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return midi.wentDown(midiMapping[id]);
}

bool Config::midiWentUp(int id) const
{
	if (!midiIsMapped(id))
		return false;
	return midi.wentUp(midiMapping[id]);
}

float Config::midiGetValue(int id, float _default) const
{
	if (!midiIsMapped(id))
		return _default;
	return midi.getValue(midiMapping[id]);
}
