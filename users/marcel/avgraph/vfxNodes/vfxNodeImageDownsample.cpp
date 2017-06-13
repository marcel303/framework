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
#include "vfxNodeImageDownsample.h"

// todo : implement single channel downsample
// todo : add setSwizzle method to Surface class
// todo : swizzle Surface class to RED, RED, RED, ONE for single channel formats ?

const char * s_downsample2x2Vs = R"SHADER(

	include engine/ShaderVS.txt

	shader_out vec2 v_texcoord;

	void main()
	{
		vec4 position = unpackPosition();
		
		position = objectToProjection(position);
		
		vec2 texcoord = unpackTexcoord(0);
		
		gl_Position = position;
		
		v_texcoord = texcoord;
	}

	)SHADER";

const char * s_downsample2x2Ps = R"SHADER(

	include engine/ShaderPS.txt

	shader_in vec2 v_texcoord;

	uniform sampler2D source;

	void main()
	{
		shader_fragColor = texture(source, v_texcoord);
	}

	)SHADER";

VfxNodeImageDownsample::VfxNodeImageDownsample()
	: VfxNodeBase()
	, surface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Size, kVfxPlugType_Int);
	addInput(kInput_Channel, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	
	shaderSource("image.downsample2x2.vs", s_downsample2x2Vs);
	shaderSource("image.downsample2x2.ps", s_downsample2x2Ps);
}

VfxNodeImageDownsample::~VfxNodeImageDownsample()
{
	freeImage();
}

void VfxNodeImageDownsample::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeImageDownsample);
	
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const DownsampleSize downsampleSize = (DownsampleSize)getInputInt(kInput_Size, 0);
	const DownsampleChannel channel = (DownsampleChannel)getInputInt(kInput_Channel, kDownsampleChannel_All);

	if (image == nullptr || image->getSx() == 0 || image->getSy() == 0)
	{
		freeImage();
	}
	else
	{
		vfxGpuTimingBlock(VfxNodeImageDownsample);
		
		const int pixelSize = downsampleSize == kDownsampleSize_2x2 ? 2 : 4;
		const int sx = std::max(1, image->getSx() / pixelSize);
		const int sy = std::max(1, image->getSy() / pixelSize);
		
		if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight())
		{
			allocateImage(sx, sy);
		}

		pushSurface(surface);
		{
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("image.downsample2x2");
				setShader(shader);
				{
					shader.setTexture("source", 0, image->getTexture(), true, true);
					
					const int x1 = 0;
					const int y1 = 0;
					const int x2 = surface->getWidth();
					const int y2 = surface->getHeight();
					
					const float u1 = .5f / float(image->getSx());
					const float v1 = .5f / float(image->getSy());
					const float u2 = (sx * pixelSize + .5f) / float(image->getSx());
					const float v2 = (sy * pixelSize + .5f) / float(image->getSy());
					
					gxBegin(GL_QUADS);
					{
						gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
						gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
						gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
						gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
					}
					gxEnd();
				}
				clearShader();
			}
			popBlend();
		}
		popSurface();
		
		Assert(imageOutput.texture == surface->getTexture());
	}
}

void VfxNodeImageDownsample::allocateImage(const int sx, const int sy)
{
	freeImage();

	// todo : use the correct surface format
	
	surface = new Surface(sx, sy, false, false, SURFACE_RGBA8);
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeImageDownsample::freeImage()
{
	delete surface;
	surface = nullptr;

	imageOutput.texture = 0;
}

