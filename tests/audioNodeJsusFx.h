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

#pragma once

#include "audioNodeBase.h"

struct AudioResource_JsusFx;

class JsusFx;
struct JsusFxFileAPI_Basic;
struct JsusFxGfx_Framework;
struct JsusFxPathLibrary_Basic;

struct AudioNodeJsusFx : AudioNodeBase
{
	enum Input
	{
		kInput_Filename,
		kInput_Slider1,
		kInput_Slider2,
		kInput_Slider3,
		kInput_Slider4,
		kInput_Input1,
		kInput_Input2,
		kInput_Input3,
		kInput_Input4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio1,
		kOutput_Audio2,
		kOutput_Audio3,
		kOutput_Audio4,
		kOutput_COUNT
	};
	
	bool preInitialized;
	
	int numAudioInputs;
	int numSliderInputs;
	int numAudioOutputs;
	
	std::vector<float> defaultSliderValues;
	
	std::vector<AudioFloat> audioOutputs;
	
	JsusFxPathLibrary_Basic * pathLibrary;
	JsusFx * jsusFx;
	bool jsusFxIsValid;
	
	JsusFxFileAPI_Basic * jsusFx_fileAPI;
	JsusFxGfx_Framework * jsusFx_gfx;
	
	std::string currentFilename;
	
	bool hasFocus;
	
	AudioResource_JsusFx * resource;
	int resourceVersion;
	
	AudioNodeJsusFx(const bool preInitialized = false);
	~AudioNodeJsusFx();
	
	void load(const char * filename);
	void free();
	
	void clearOutputs();
	bool isSliderConnected(const int index) const;
	void updateImmediateValues();
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	virtual bool tickEditor(const int x, const int y, int & sx, int & sy, bool & inputIsCaptured) override;
	virtual void drawEditor(Surface * surface, const int x, const int y, const int sx, const int sy) override;
};
