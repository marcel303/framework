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

#include "framework.h"
#include "vfxNodePicture.h"

VFX_NODE_TYPE(VfxNodePicture)
{
	typeName = "picture";
	
	in("source", "string");
	out("image", "image");
}

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
	vfxCpuTimingBlock(VfxNodePictureCpu);
	
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (isPassthrough || filename == nullptr)
	{
		image->texture = 0;
	}
	else
	{
		image->texture = getTexture(filename);
	}
}

void VfxNodePicture::tick(const float dt)
{
	const char * filename = getInputString(kInput_Source, nullptr);
	
	if (isPassthrough || filename == nullptr)
	{
		image->texture = 0;
	}
	else
	{
		image->texture = getTexture(filename);
	}
}
