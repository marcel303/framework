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
#include "vfxGraph.h"
#include "vfxNodeFsfx.h"

// todo : use percentage of screen size for surface size instead of width/height

extern const int GFX_SX;
extern const int GFX_SY;

VFX_NODE_TYPE(fsfx, VfxNodeFsfx)
{
	typeName = "draw.fsfx";
	
	in("image", "image");
	in("shader", "string");
	in("width", "int");
	in("height", "int");
	in("color1", "color");
	in("color2", "color");
	in("param1", "float");
	in("param2", "float");
	in("param3", "float");
	in("param4", "float");
	in("opacity", "float", "1");
	in("image1", "image");
	in("image2", "image");
	out("image", "image");
}

VfxNodeFsfx::VfxNodeFsfx()
	: VfxNodeBase()
	, surface(nullptr)
	, image(nullptr)
{
	image = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Shader, kVfxPlugType_String);
	addInput(kInput_Width, kVfxPlugType_Int);
	addInput(kInput_Height, kVfxPlugType_Int);
	addInput(kInput_Color1, kVfxPlugType_Color);
	addInput(kInput_Color2, kVfxPlugType_Color);
	addInput(kInput_Param1, kVfxPlugType_Float);
	addInput(kInput_Param2, kVfxPlugType_Float);
	addInput(kInput_Param3, kVfxPlugType_Float);
	addInput(kInput_Param4, kVfxPlugType_Float);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
}

VfxNodeFsfx::~VfxNodeFsfx()
{
	delete image;
	image = nullptr;
	
	delete surface;
	surface = nullptr;
}

void VfxNodeFsfx::allocateSurface(const int _sx, const int _sy)
{
	const int sx = _sx ? _sx : GFX_SX;
	const int sy = _sy ? _sy : GFX_SY;
	
	if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight())
	{
		delete surface;
		surface = nullptr;
		
		surface = new Surface(sx, sy, true);
		
		surface->clear();
		surface->swapBuffers();
		surface->clear();
		surface->swapBuffers();
	}
}

void VfxNodeFsfx::tick(const float dt)
{
	const int sx = getInputInt(kInput_Width, 0);
	const int sy = getInputInt(kInput_Height, 0);

	allocateSurface(sx, sy);
}

void VfxNodeFsfx::draw() const
{
	vfxCpuTimingBlock(VfxNodeFsfx);
	
	if (isPassthrough)
	{
		const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
		const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
		image->texture = inputTexture;
		return;
	}
	
	const char * shaderName = getInputString(kInput_Shader, nullptr);
	
	if (shaderName == nullptr)
	{
		// todo : warn ?
	}
	else
	{
		const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
		const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
		Shader shader(shaderName);
		
		if (shader.isValid())
		{
			vfxGpuTimingBlock(VfxNodeFsfx);
			
			pushBlend(BLEND_OPAQUE);
			setShader(shader);
			{
				// fixme : remove this code. it's here to test enumerating uniforms so we can dynamically add node type definitions for shaders at some point ..
				static bool isFirst = true;
				
				if (isFirst)
				{
					isFirst = false;
					
					GLsizei uniformCount = 0;
					glGetProgramiv(shader.getProgram(), GL_ACTIVE_UNIFORMS, &uniformCount);

					for (int i = 0; i < uniformCount; ++i)
					{
						const GLsizei bufferSize = 32;
						char name[bufferSize];
						
						GLsizei length;
						GLint size;
						GLenum type;
						
						glGetActiveUniform(shader.getProgram(), i, bufferSize, &length, &size, &type, name);
						
						const bool supported = type == GL_FLOAT || type == GL_INT || type == GL_BOOL || type == GL_SAMPLER_2D;
						
						logDebug("uniform %d. supported=%d, name=%s, type=%d, size=%d", i, supported, name, type, size);
					}
				}
				
				const VfxImageBase * image1 = getInputImage(kInput_Image1, nullptr);
				const VfxImageBase * image2 = getInputImage(kInput_Image2, nullptr);
				const GLuint texture1 = image1 ? image1->getTexture() : 0;
				const GLuint texture2 = image2 ? image2->getTexture() : 0;
				
				shader.setImmediate("screenSize", surface->getWidth(), surface->getHeight());
				shader.setTexture("colormap", 0, inputTexture, true, false);
				shader.setImmediate("param1", getInputFloat(kInput_Param1, 0.f));
				shader.setImmediate("param2", getInputFloat(kInput_Param2, 0.f));
				shader.setImmediate("param3", getInputFloat(kInput_Param3, 0.f));
				shader.setImmediate("param4", getInputFloat(kInput_Param4, 0.f));
				shader.setImmediate("opacity", getInputFloat(kInput_Opacity, 1.f));
				shader.setTexture("texture1", 1, texture1, true, false);
				shader.setTexture("texture2", 2, texture2, true, false);
				shader.setImmediate("time", g_currentVfxGraph->time);
				surface->postprocess();
			}
			clearShader();
			popBlend();
		}
		else
		{
			pushSurface(surface);
			{
				if (inputImage == nullptr)
				{
					pushBlend(BLEND_OPAQUE);
					setColor(0, 127, 0);
					drawRect(0, 0, framework.windowSx, framework.windowSy);
					popBlend();
				}
				else
				{
					pushBlend(BLEND_OPAQUE);
					gxSetTexture(inputTexture);
					setColorMode(COLOR_ADD);
					setColor(127, 0, 127);
					drawRect(0, 0, framework.windowSx, framework.windowSy);
					setColorMode(COLOR_MUL);
					gxSetTexture(0);
					popBlend();
				}
			}
			popSurface();
		}
	}
	
	image->texture = surface->getTexture();
}

void VfxNodeFsfx::init(const GraphNode & node)
{
	const int sx = getInputInt(kInput_Width, 0);
	const int sy = getInputInt(kInput_Height, 0);
	
	allocateSurface(sx, sy);
}
