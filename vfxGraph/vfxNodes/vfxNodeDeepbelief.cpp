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

#if ENABLE_DEEPBELIEF

#include "vfxNodeDeepbelief.h"

#include "FileStream.h"

VFX_NODE_TYPE(VfxNodeDeepbelief)
{
	typeName = "deepbelief";
	
	in("network", "string");
	in("image", "image_cpu");
	in("treshold", "float", "0.01");
	in("interval", "float");
	out("label", "string");
	out("certainty", "float");
}

VfxNodeDeepbelief::VfxNodeDeepbelief()
	: VfxNodeBase()
	, deepbelief()
	, networkFilename()
	, updateTimer(0.f)
	, result()
	, labelOutput()
	, certaintyOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Network, kVfxPlugType_String);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Treshold, kVfxPlugType_Float);
	addInput(kInput_UpdateInterval, kVfxPlugType_Float);
	addOutput(kOutput_Label, kVfxPlugType_String, &labelOutput);
	addOutput(kOutput_Certainty, kVfxPlugType_Float, &certaintyOutput);
}

void VfxNodeDeepbelief::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDeepbelief);
	
	if (isPassthrough)
	{
		updateTimer = 0.f;
		result = DeepbeliefResult();
		
		labelOutput.clear();
		certaintyOutput = 0.f;
		
		return;
	}
	
	const char * newNetworkFilename = getInputString(kInput_Network, "");
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const float treshold = getInputFloat(kInput_Treshold, .01f);
	const float updateInterval = getInputFloat(kInput_UpdateInterval, 0.f);

	if (newNetworkFilename != networkFilename)
	{
		networkFilename = newNetworkFilename;
		
		if (FileStream::Exists(newNetworkFilename))
		{
			deepbelief.init(newNetworkFilename);
		}
	}
	
	updateTimer += dt;

	if (image != nullptr && deepbelief.state != nullptr && deepbelief.state->isInitialized && updateTimer >= updateInterval)
	{
		updateTimer = 0.f;
		
		if (image->isInterleaved)
		{
			deepbelief.process(image->channel[0].data, image->sx, image->sy, image->numChannels, image->channel[0].pitch, treshold);
		}
	}
	
	if (deepbelief.state && deepbelief.state->isInitialized)
	{
		if (deepbelief.getResult(result))
		{
			if (!result.predictions.empty())
			{
				labelOutput = result.predictions[0].label;
				certaintyOutput = result.predictions[0].certainty;
			}
		}
	}
}

void VfxNodeDeepbelief::getDescription(VfxNodeDescription & d)
{
	d.add("classification took %.2fms", result.classificationTime);
	d.newline();
	
	d.add("result:");
	
	for (auto & p : result.predictions)
	{
		d.add("certainty: %1.3f, label: %s", p.certainty, p.label.c_str());
	}
}

#endif
