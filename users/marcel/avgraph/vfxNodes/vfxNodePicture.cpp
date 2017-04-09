#include "vfxNodePicture.h"

VfxNodePicture::VfxNodePicture()
	: VfxNodeBase()
	, image(nullptr)
{
	image = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
}

VfxNodePicture::~VfxNodePicture()
{
	delete image;
	image = nullptr;
}

void VfxNodePicture::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (filename == nullptr)
	{
		image->texture = 0;
	}
	else
	{
		image->texture = getTexture(filename);
	}
}
