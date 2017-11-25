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

#include "openglTexture.h"
#include "vfxNodeBase.h"

struct VfxNodeSpectrum2D : VfxNodeBase
{
	enum ImageChannel
	{
		kImageChannel_R,
		kImageChannel_G,
		kImageChannel_B,
		kImageChannel_A,
	};
	
	enum OutputMode
	{
		kOutputMode_Channel1And2,
		kOutputMode_Channel1,
		kOutputMode_Channel2,
		kOutputMode_Length,
		kOutputMode_LengthSq
	};
	
	enum Input
	{
		kInput_Image,
		kInput_ImageChannel,
		kInput_OutputMode,
		kInput_Normalize,
		kInput_Scale,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image1,
		kOutput_Image2,
		kOutput_Channels,
		kOutput_COUNT
	};
	
	OpenglTexture texture1;
	OpenglTexture texture2;
	float * dreal;
	float * dimag;
	
	VfxImage_Texture image1Output;
	VfxImage_Texture image2Output;
	VfxChannels channelsOutput;

	VfxNodeSpectrum2D();
	virtual ~VfxNodeSpectrum2D() override;
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void allocateTextures(const int sx, const int sy);
	void freeTextures();
};
