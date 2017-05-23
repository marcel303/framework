#include "image.h"
#include "vfxNodePictureCpu.h"

VfxNodePictureCpu::VfxNodePictureCpu()
	: VfxNodeBase()
	, image()
	, imageData(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &image);
}

VfxNodePictureCpu::~VfxNodePictureCpu()
{
	delete imageData;
	imageData = nullptr;
}

void VfxNodePictureCpu::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (filename != nullptr)
	{
		imageData = loadImage(filename);

		if (imageData != nullptr)
		{
			image.setDataRGBA8((uint8_t*)imageData->getLine(0), imageData->sx, imageData->sy, 0);
		}
	}
}

void VfxNodePictureCpu::tick(const float dt)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (filename == nullptr)
	{
		delete imageData;
		imageData = nullptr;

		image.reset();
	}
	else
	{
		// todo : detect if the filename has changed
	}
}
