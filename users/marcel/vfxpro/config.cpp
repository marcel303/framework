#include "config.h"
#include "framework.h"
#include "tinyxml2.h"
#include "xml.h"
#include <string.h>

//

using namespace tinyxml2;

//

extern int GFX_SX;
extern int GFX_SY;

//

Config::Midi::Midi()
{
	memset(this, 0, sizeof(*this));
	memset(mapping, -1, sizeof(mapping));
}

Config::AudioIn::AudioIn()
{
	memset(this, 0, sizeof(*this));

	deviceIndex = -1;
}

Config::Display::Display()
	: sx(0)
	, sy(0)
	, fullscreen(false)
	, showTestImage(false)
	, showScaleOverlay(false)
	, gamma(1.f)
{
}

//

Config::Config()
{
	reset();
}

void Config::reset()
{
	midi = Midi();
	audioIn = AudioIn();
	display = Display();
}

bool Config::load(const char * filename)
{
	bool result = true;

	XMLDocument xmlDoc;
		
	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load config: %s", filename);

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

			//

			const XMLElement * xmlDisplay = xmlSettings->FirstChildElement("display");

			if (xmlDisplay == nullptr)
			{
				logWarning("missing <display> element");
			}
			else
			{
				display.sx = intAttrib(xmlDisplay, "sx", GFX_SX);
				display.sy = intAttrib(xmlDisplay, "sy", GFX_SY);
				display.fullscreen = boolAttrib(xmlDisplay, "fullscreen", false);
				display.showTestImage = boolAttrib(xmlDisplay, "show_testimage", false);
				display.showScaleOverlay = boolAttrib(xmlDisplay, "show_scaleoverlay", false);
				display.gamma = floatAttrib(xmlDisplay, "gamma", 1.f);
				display.mirror = boolAttrib(xmlDisplay, "mirror", false);
			}


			//

			const XMLElement * xmlDebug = xmlSettings->FirstChildElement("debug");

			if (xmlDebug == nullptr)
			{
				logWarning("missing <debug> element");
			}
			else
			{
				debug.showMessages = boolAttrib(xmlDebug, "show_messages", false);
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
