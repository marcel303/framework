#include "image.h"
#include "vfxNodePictureCpu.h"

VfxNodePictureCpu::VfxNodePictureCpu()
	: VfxNodeBase()
	, image()
	, currentFilename()
	, imageData(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_ImageCpu, &image);
}

VfxNodePictureCpu::~VfxNodePictureCpu()
{
	setImage(nullptr);
}

void VfxNodePictureCpu::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (filename != nullptr)
	{
		setImage(filename);
	}
}

void VfxNodePictureCpu::tick(const float dt)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (filename == nullptr)
	{
		setImage(nullptr);
	}
	else if (filename != currentFilename)
	{
		setImage(filename);
	}
}

void VfxNodePictureCpu::setImage(const char * filename)
{
	if (filename != nullptr)
	{
		currentFilename = filename;
		
		imageData = loadImage(filename);

		if (imageData != nullptr)
		{
			image.setDataRGBA8((uint8_t*)imageData->getLine(0), imageData->sx, imageData->sy, 16, 0);
		}
		else
		{
			image.reset();
		}
	}
	else
	{
		currentFilename.clear();
		
		delete imageData;
		imageData = nullptr;
		
		image.reset();
	}
}

void VfxNodePictureCpu::getDescription(VfxNodeDescription & d)
{
	d.add("image:");
	d.add(image);
}
