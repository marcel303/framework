#include "vfxNodeDeepbelief.h"

#include "FileStream.h"

VfxNodeDeepbelief::VfxNodeDeepbelief()
	: VfxNodeBase()
	, deepbelief()
	, networkFilename()
	, result()
	, labelOutput()
	, certaintyOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Network, kVfxPlugType_String);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addInput(kInput_Treshold, kVfxPlugType_Float);
	addInput(kInput_ShowResult, kVfxPlugType_Bool);
	addOutput(kOutput_Label, kVfxPlugType_String, &labelOutput);
	addOutput(kOutput_Certainty, kVfxPlugType_Float, &certaintyOutput);
}

void VfxNodeDeepbelief::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDeepbelief);

	const char * newNetworkFilename = getInputString(kInput_Network, "");
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	const float treshold = getInputFloat(kInput_Treshold, .01f);

	if (newNetworkFilename != networkFilename)
	{
		networkFilename = newNetworkFilename;
		
		if (FileStream::Exists(newNetworkFilename))
		{
			deepbelief.init(newNetworkFilename);
		}
	}

	if (image != nullptr && deepbelief.isInitialized)
	{
		if (image->isInterleaved)
		{
			deepbelief.process(image->channel[0].data, image->sx, image->sy, image->numChannels, image->channel[0].pitch, treshold);
		}
	}
	
	if (deepbelief.isInitialized)
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

void VfxNodeDeepbelief::draw() const
{
	vfxCpuTimingBlock(VfxNodeDeepbelief);

	const bool showResult = getInputBool(kInput_ShowResult, false);
}

void VfxNodeDeepbelief::getDescription(VfxNodeDescription & d)
{
	d.add("result:");
	
	for (auto & p : result.predictions)
	{
		d.add("certainty: %1.3f, label: %s", p.certainty, p.label.c_str());
	}

	d.newline();
	d.add("classification took %.2fms", result.classificationTime / 1000.0);
}
