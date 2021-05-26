/*
	Copyright (C) 2020 Marcel Smit
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
#include "vfxNodeYuvToRgb.h"

VFX_ENUM_TYPE(yuvToRgbColorSpace)
{
	elem("NTSC");
	elem("BT.601");
	elem("BT.709");
}

VFX_NODE_TYPE(VfxNodeYuvToRgb)
{
	typeName = "draw.yuvToRgb";
	
	in("y", "image");
	in("u", "image");
	in("v", "image");
	inEnum("colorspace", "yuvToRgbColorSpace");
	out("image", "image");
}

static bool s_yuvToRgb_initialized = false;

static const char * s_yuvToRgbVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_yuvToRgb_ntscPs = R"SHADER(
	include engine/ShaderPS.txt

	shader_in vec2 texcoord;
	uniform sampler2D yTex;
	uniform sampler2D uTex;
	uniform sampler2D vTex;

	void main()
	{
		float y = texture(yTex, texcoord).x;
		float u = texture(uTex, texcoord).x;
		float v = texture(vTex, texcoord).x;
		
		//y = y - 16/255.0;
		u = u - 0.5;
		v = v - 0.5;
		
		float r = +1.16406 * y +0.00000 * u +1.59766 * v;
		float g = +1.16406 * y -0.39063 * u -0.81250 * v;
		float b = +1.16406 * y +2.01563 * u +0.00000 * v;
		
		vec4 color = vec4(r, g, b, 1.0);
		color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
		
		shader_fragColor = color;
	}
)SHADER";

// https://en.wikipedia.org/wiki/Rec._601
static const char * s_yuvToRgb_bt601Ps = R"SHADER(
	include engine/ShaderPS.txt

	shader_in vec2 texcoord;
	uniform sampler2D yTex;
	uniform sampler2D uTex;
	uniform sampler2D vTex;

	void main()
	{
		float y = texture(yTex, texcoord).x;
		float u = texture(uTex, texcoord).x;
		float v = texture(vTex, texcoord).x;
		
		//y = y - 16/255.0;
		u = u - 0.5;
		v = v - 0.5;
		
		float r = +1.00000 * y +0.00000 * u +1.13983 * v;
		float g = +1.00000 * y -0.39465 * u -0.58060 * v;
		float b = +1.00000 * y +2.03211 * u +0.00000 * v;
		
		vec4 color = vec4(r, g, b, 1.0);
		color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
		
		shader_fragColor = color;
	}
)SHADER";

static const char * s_yuvToRgb_bt709Ps = R"SHADER(
	include engine/ShaderPS.txt

	shader_in vec2 texcoord;
	uniform sampler2D yTex;
	uniform sampler2D uTex;
	uniform sampler2D vTex;

	void main()
	{
		float y = texture(yTex, texcoord).x;
		float u = texture(uTex, texcoord).x;
		float v = texture(vTex, texcoord).x;
		
		//y = y - 16/255.0;
		u = u - 0.5;
		v = v - 0.5;
		
		float r = +1.00000 * y +0.00000 * u +1.28033 * v;
		float g = +1.00000 * y -0.21482 * u -0.38059 * v;
		float b = +1.00000 * y +2.12798 * u +0.00000 * v;
		
		vec4 color = vec4(r, g, b, 1.0);
		color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));
		
		shader_fragColor = color;
	}
)SHADER";


VfxNodeYuvToRgb::VfxNodeYuvToRgb()
	: VfxNodeBase()
	, surface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Y, kVfxPlugType_Image);
	addInput(kInput_U, kVfxPlugType_Image);
	addInput(kInput_V, kVfxPlugType_Image);
	addInput(kInput_ColorSpace, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	
	if (s_yuvToRgb_initialized == false)
	{
		s_yuvToRgb_initialized = true;

		shaderSource("yuvToRgb.vs", s_yuvToRgbVs);
		
		shaderSource("yuvToRgbNtsc.ps", s_yuvToRgb_ntscPs);
		shaderSource("yuvToRgbBt601.ps", s_yuvToRgb_bt601Ps);
		shaderSource("yuvToRgbBt709.ps", s_yuvToRgb_bt709Ps);
	}
}

VfxNodeYuvToRgb::~VfxNodeYuvToRgb()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeYuvToRgb::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeYuvToRgb);
	
	const VfxImageBase * y = getInputImage(kInput_Y, nullptr);
	const VfxImageBase * u = getInputImage(kInput_U, nullptr);
	const VfxImageBase * v = getInputImage(kInput_V, nullptr);

	if (isPassthrough ||
		y == nullptr ||
		u == nullptr ||
		v == nullptr ||
		y->getSx() == 0 ||
		y->getSy() == 0)
	{
		delete surface;
		surface = nullptr;
	}
	else
	{
		const int sx = y->getSx();
		const int sy = y->getSy();

		if (surface == nullptr || surface->getWidth() != sx || surface->getHeight() != sy)
		{
			delete surface;
			surface = nullptr;

			surface = new Surface(sx, sy, false, false, SURFACE_RGBA8, 1);
		}
	}
	
	if (surface != nullptr)
		imageOutput.texture = surface->getTexture();
	else
		imageOutput.texture = 0;
}

void VfxNodeYuvToRgb::draw() const
{
	if (surface != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeYuvToRgb);
		
		const VfxImageBase * y = getInputImage(kInput_Y, nullptr);
		const VfxImageBase * u = getInputImage(kInput_U, nullptr);
		const VfxImageBase * v = getInputImage(kInput_V, nullptr);
		const ColorSpace colorSpace = (ColorSpace)getInputInt(kInput_ColorSpace, 0);
		
		pushSurface(surface);
		{
			pushBlend(BLEND_OPAQUE);
			setColor(colorWhite);
			
			const char * filenameVs = "yuvToRgb.vs";
			const char * filenamePs =
				colorSpace == kColorSpace_NTSC ? "yuvToRgbNtsc.ps" :
				colorSpace == kColorSpace_BT601 ? "yuvToRgbBt601.ps" :
				colorSpace == kColorSpace_BT709 ? "yuvToRgbBt709.ps" : "yuvToRgb.ps";
			
			Shader shader(filenamePs, filenameVs, filenamePs);
			setShader(shader);
			{
				shader.setTexture("yTex", 0, y->getTexture(), true, true);
				shader.setTexture("uTex", 1, u->getTexture(), true, true);
				shader.setTexture("vTex", 2, v->getTexture(), true, true);
				
				drawRect(0, 0, surface->getWidth(), surface->getHeight());
			}
			clearShader();
			
			popBlend();
		}
		popSurface();
	}
}
