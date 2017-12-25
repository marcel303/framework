/*
Copyright (C) 2017 Marcel Smit
marcel303@gmail.com
https://www.facebook.com/marcel.smit981

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "Log.h"
#include "rtmidi/RtMidi.h"
#include "vfxNodeMidi.h"

// todo : interpret NOTE_OFF and NOTE_ON messages

static const uint8_t NOTE_OFF = 0x80;
static const uint8_t NOTE_ON = 0x90;
static const uint8_t POLYPHONIC_KEY_PRESSURE = 0xa0;
static const uint8_t CONTROLLER_CHANGE = 0xb0;
static const uint8_t PITCH_BEND = 0xe0;

/*

todo : send reset all controllers message on connect and expose it in the node ?

https://www.midi.org/specifications/item/table-1-summary-of-midi-message
Reset All Controllers. When Reset All Controllers is received, all controller values are reset to their default values. (See specific Recommended Practices for defaults).

*/


VFX_NODE_TYPE(VfxNodeMidi)
{
	typeName = "midi";
	
	in("port", "int");
	out("key", "float");
	out("value", "float");
	out("trigger", "trigger");
	out("values", "channel");
}

VfxNodeMidi::VfxNodeMidi()
	: VfxNodeBase()
	, currentPort(-1)
	, midiIn(nullptr)
	, keyOutput(0.f)
	, valueOutput(0.f)
	, valueData()
	, valueChannel()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Port, kVfxPlugType_Int);
	addOutput(kOutput_Key, kVfxPlugType_Float, &keyOutput);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_ValueChannel, kVfxPlugType_Channel, &valueChannel);
	
	memset(valueData, 0, sizeof(valueData));
	valueChannel.setData(valueData, false, 256);
}

VfxNodeMidi::~VfxNodeMidi()
{
	close();
}

void VfxNodeMidi::close()
{
	try
	{
		delete midiIn;
		midiIn = nullptr;
		
		memset(valueData, 0, sizeof(valueData));
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to free MIDI input: %s", e.what());
	}
}

void VfxNodeMidi::tick(const float dt)
{
	const int port = getInputInt(kInput_Port, 0);
	
	if (isPassthrough || port < 0)
	{
		currentPort = -1;
		
		close();
		return;
	}
	
	try
	{
		if (port != currentPort)
		{
			currentPort = port;
			
			midiIn = new RtMidiIn(RtMidi::UNSPECIFIED, "Vfx Midi", 1024);
			
			for (int i = 0; i < midiIn->getPortCount(); ++i)
			{
				auto name = midiIn->getPortName();
				
				LOG_DBG("available MIDI port: %d: %s", i, name.c_str());
			}
			
			if (port >= midiIn->getPortCount())
			{
				close();
			}
			else
			{
				midiIn->openPort(port);
			}
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to open MIDI port: %s", e.what());
		
		close();
	}
	
	if (midiIn != nullptr)
	{
		try
		{
			std::vector<uint8_t> message;
			
			do
			{
				midiIn->getMessage(&message);
				
				if (message.empty() == false)
				{
					//LOG_DBG("received message!");
					
					const uint8_t b = message[0];
					
					int channel;
					int event;
					
					if ((b & 0xf0) != 0xf0)
					{
						channel = b & 0x0f;
						event = b & 0xf0;
					}
					else
					{
						channel = 0;
						event = b;
					}
					
					if (event == CONTROLLER_CHANGE)
					{
						const int key = message[1];
						const int value = message[2];
						
						keyOutput = key;
						valueOutput = value / 127.f;
						
						if (key >= 0 && key < 256)
							valueData[key] = valueOutput;
						
						trigger(kOutput_Trigger);
					}
					else if (event == NOTE_ON)
					{
						const int key = message[1];
						const int value = message[2];
						
						keyOutput = key;
						valueOutput = value / 127.f;
						
						if (key >= 0 && key < 256)
							valueData[key] = valueOutput;
						
						trigger(kOutput_Trigger);
					}
					else if (event == NOTE_OFF)
					{
						// todo : check note off
						const int key = message[1];
						const int value = message[2];
						
						keyOutput = key;
						valueOutput = 0.f;
						
						if (key >= 0 && key < 256)
							valueData[key] = valueOutput;
						
						trigger(kOutput_Trigger);
					}
					else if (event == PITCH_BEND)
					{
					
					}
					else
					{
						LOG_DBG("unknown MIDI event: %x", event);
					}
				}
			}
			while (message.empty() == false);
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to process MIDI input: %s", e.what());
		}
	}
}
