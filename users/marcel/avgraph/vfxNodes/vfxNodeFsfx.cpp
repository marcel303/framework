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
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxNodeFsfx.h"

// todo : use percentage of screen size for surface size instead of width/height

extern const int GFX_SX;
extern const int GFX_SY;

VFX_NODE_TYPE(VfxNodeFsfx)
{
	typeName = "draw.fsfx";
	displayName = "fsfx-v1";
	
	in("image", "image");
	in("shader", "string");
	in("width", "int");
	in("height", "int");
	in("image1", "image");
	in("image2", "image");
	in("color1", "color", "ffff");
	in("param1", "float");
	in("param2", "float");
	in("time", "float");
	in("opacity", "float", "1");
	out("image", "image");
}

VfxNodeFsfx::VfxNodeFsfx()
	: VfxNodeBase()
	, surface(nullptr)
	, currentShader()
	, currentShaderVersion(0)
	, shader(nullptr)
	, shaderInputs()
	, image(nullptr)
{
	image = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Shader, kVfxPlugType_String);
	addInput(kInput_Width, kVfxPlugType_Int);
	addInput(kInput_Height, kVfxPlugType_Int);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addInput(kInput_Color1, kVfxPlugType_Color);
	addInput(kInput_Param1, kVfxPlugType_Float);
	addInput(kInput_Param2, kVfxPlugType_Float);
	addInput(kInput_Time, kVfxPlugType_Float);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Image, kVfxPlugType_Image, image);
}

VfxNodeFsfx::~VfxNodeFsfx()
{
	delete image;
	image = nullptr;
	
	delete surface;
	surface = nullptr;
	
	delete shader;
	shader = nullptr;
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

void VfxNodeFsfx::loadShader(const char * filename)
{
	freeShader();
	
	//
	
	if (filename)
	{
		shader = new Shader(filename);
		
		//
		
		if (!shader->isValid())
		{
			logDebug("shader is invalid: %s", filename);
		}
		else
		{
			std::vector<DynamicPlug> inputs;
			
			GLsizei uniformCount = 0;
			glGetProgramiv(shader->getProgram(), GL_ACTIVE_UNIFORMS, &uniformCount);
			checkErrorGL();
			
			int socketIndex = VfxNodeBase::inputs.size();

			for (int i = 0; i < uniformCount; ++i)
			{
				const GLsizei bufferSize = 32;
				char name[bufferSize];
				
				GLsizei length;
				GLint size;
				GLenum type;
				
				glGetActiveUniform(shader->getProgram(), i, bufferSize, &length, &size, &type, name);
				checkErrorGL();
				
				// built-in?
				if (!strcmp(name, "screenSize") || !strcmp(name, "colormap"))
					continue;
				
				// standardized input?
				if (!strcmp(name, "image1") || !strcmp(name, "image2") || !strcmp(name, "color1") || !strcmp(name, "param1") || !strcmp(name, "param2") || !strcmp(name, "time") || !strcmp(name, "opacity"))
					continue;
				
				const GLint location = glGetUniformLocation(shader->getProgram(), name);
				checkErrorGL();
				
				//logDebug("uniform %d. name=%s, type=%d, size=%d", i, name, type, size);
				
				if (type == GL_FLOAT)
				{
					ShaderInput input;
					input.type = type;
					input.socketType = kVfxPlugType_Float;
					input.socketIndex = socketIndex++;
					input.uniformLocation = location;
					shaderInputs.push_back(input);
					
					DynamicPlug plug;
					plug.name = name;
					plug.type = kVfxPlugType_Float;
					inputs.push_back(plug);
				}
				else if (type == GL_FLOAT_VEC2 || type == GL_FLOAT_VEC3 || type == GL_FLOAT_VEC4)
				{
					if (String::StartsWith(name, "color"))
					{
						ShaderInput input;
						input.type = type;
						input.socketType = kVfxPlugType_Color;
						input.socketIndex = socketIndex++;
						input.uniformLocation = location;
						shaderInputs.push_back(input);
					
						DynamicPlug plug;
						plug.name = name;
						plug.type = kVfxPlugType_Color;
						inputs.push_back(plug);
					}
					else
					{
						ShaderInput input;
						input.type = type;
						input.socketType = kVfxPlugType_Float;
						input.socketIndex = socketIndex;
						input.uniformLocation = location;
						shaderInputs.push_back(input);
					
						const int numElems = type == GL_FLOAT_VEC2 ? 2 : type == GL_FLOAT_VEC3 ? 3 : 4;
						
						char elementNames[4] = { 'x', 'y', 'z', 'w' };
						
						for (int i = 0; i < numElems; ++i)
						{
							DynamicPlug plug;
							plug.name = String::Format("%s.%c", name, elementNames[i]);
							plug.type = kVfxPlugType_Float;
							inputs.push_back(plug);
						}
						
						socketIndex += numElems;
					}
				}
				else if (type == GL_SAMPLER_2D)
				{
					ShaderInput input;
					input.type = type;
					input.socketType = kVfxPlugType_Image;
					input.socketIndex = socketIndex++;
					input.uniformLocation = location;
					shaderInputs.push_back(input);
					
					DynamicPlug plug;
					plug.name = name;
					plug.type = kVfxPlugType_Image;
					inputs.push_back(plug);
				}
				else
				{
				}
			}
			
			setDynamicInputs(&inputs[0], inputs.size());
		}
	}
}

void VfxNodeFsfx::freeShader()
{
	delete shader;
	shader = nullptr;
	
	shaderInputs.clear();
	
	setDynamicInputs(nullptr, 0);
}

void VfxNodeFsfx::tick(const float dt)
{
	if (isPassthrough)
	{
		allocateSurface(0, 0);
		
		freeShader();
		
		currentShader.clear();
		currentShaderVersion = 0;
		
		return;
	}
	
	const int sx = getInputInt(kInput_Width, 0);
	const int sy = getInputInt(kInput_Height, 0);

	allocateSurface(sx, sy);
	
	//
	
	const char * shaderName = getInputString(kInput_Shader, nullptr);
	
	if (shaderName == nullptr)
	{
		freeShader();
		
		currentShader.clear();
		currentShaderVersion = 0;
	}
	else
	{
		if (shaderName != currentShader || (shader && shader->getVersion() != currentShaderVersion))
		{
			loadShader(shaderName);
			
			currentShader = shaderName;
			currentShaderVersion = shader->getVersion();
		}
		
		/*
		// todo : remove. test dynamically growing and shrinking list of inputs as a stress test
		
		const int kNumInputs = std::round(std::sin(framework.time) * 4.f + 5.f);
	
		DynamicPlug inputs[kNumInputs];
		
		for (int i = 0; i < kNumInputs; ++i)
		{
			char name[32];
			sprintf_s(name, sizeof(name), "dynamic%d", i + 1);
			
			inputs[i].name = name;
			inputs[i].type = kVfxPlugType_Float;
		}
		
		setDynamicInputs(inputs, kNumInputs);
		*/
	}
}

void VfxNodeFsfx::draw() const
{
	vfxCpuTimingBlock(VfxNodeFsfx);
	
	clearEditorIssue();
	
	if (isPassthrough || shader == nullptr)
	{
		const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
		const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
		image->texture = inputTexture;
		return;
	}
	
	const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
	const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
	
	if (shader->isValid())
	{
		vfxGpuTimingBlock(VfxNodeFsfx);
		
		pushBlend(BLEND_OPAQUE);
		setShader(*shader);
		{
			int textureSlot = 0;
			
			for (auto & shaderInput : shaderInputs)
			{
				if (shaderInput.type == GL_FLOAT)
				{
					shader->setImmediate(shaderInput.uniformLocation, getInputFloat(shaderInput.socketIndex, 0.f));
				}
				else if (shaderInput.type == GL_FLOAT_VEC2)
				{
					shader->setImmediate(
						shaderInput.uniformLocation,
						getInputFloat(shaderInput.socketIndex + 0, 0.f),
						getInputFloat(shaderInput.socketIndex + 1, 0.f));
				}
				else if (shaderInput.type == GL_FLOAT_VEC3)
				{
					shader->setImmediate(
						shaderInput.uniformLocation,
						getInputFloat(shaderInput.socketIndex + 0, 0.f),
						getInputFloat(shaderInput.socketIndex + 1, 0.f),
						getInputFloat(shaderInput.socketIndex + 2, 0.f));
				}
				else if (shaderInput.type == GL_FLOAT_VEC4)
				{
					if (shaderInput.socketType == kVfxPlugType_Color)
					{
						const VfxColor defaultColor(0.f, 0.f, 0.f, 0.f);
						const VfxColor * color = getInputColor(shaderInput.socketIndex, &defaultColor);
						
						shader->setImmediate(
							shaderInput.uniformLocation,
							color->r,
							color->g,
							color->b,
							color->a);
					}
					else
					{
						shader->setImmediate(
							shaderInput.uniformLocation,
							getInputFloat(shaderInput.socketIndex + 0, 0.f),
							getInputFloat(shaderInput.socketIndex + 1, 0.f),
							getInputFloat(shaderInput.socketIndex + 2, 0.f),
							getInputFloat(shaderInput.socketIndex + 3, 0.f));
					}
				}
				else if (shaderInput.type == GL_SAMPLER_2D)
				{
					const VfxImageBase * image = getInputImage(shaderInput.socketIndex, nullptr);
					
					shader->setTextureUniform(
						shaderInput.uniformLocation,
						textureSlot++,
						image ? image->getTexture() : 0);
				}
				else
				{
					Assert(false);
				}
			}
			
			const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
			
			const VfxImageBase * image1 = getInputImage(kInput_Image1, nullptr);
			const VfxImageBase * image2 = getInputImage(kInput_Image2, nullptr);
			const GLuint texture1 = image1 ? image1->getTexture() : 0;
			const GLuint texture2 = image2 ? image2->getTexture() : 0;
			
			const VfxColor * color = getInputColor(kInput_Color1, &defaultColor);
			const float param1 = getInputFloat(kInput_Param1, 0.f);
			const float param2 = getInputFloat(kInput_Param2, 0.f);
			const float time = getInputFloat(kInput_Time, g_currentVfxGraph->time);
			const float opacity = getInputFloat(kInput_Opacity, 1.f);
			
			shader->setImmediate("screenSize", surface->getWidth(), surface->getHeight());
			shader->setTexture("colormap", textureSlot++, inputTexture, true, false);
			shader->setTexture("image1", textureSlot++, texture1, true, false);
			shader->setTexture("image2", textureSlot++, texture2, true, false);
			shader->setImmediate("color1", color->r, color->g, color->b, color->a);
			shader->setImmediate("param1", param1);
			shader->setImmediate("param2", param2);
			shader->setImmediate("opacity", opacity);
			shader->setImmediate("time", time);
			surface->postprocess();
		}
		clearShader();
		popBlend();
	}
	else
	{
		setEditorIssue("shader is invalid");
		
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
				pushColorMode(COLOR_ADD);
				setColor(127, 0, 127);
				drawRect(0, 0, framework.windowSx, framework.windowSy);
				popColorMode();
				gxSetTexture(0);
				popBlend();
			}
		}
		popSurface();
	}
	
	image->texture = surface->getTexture();
}

void VfxNodeFsfx::init(const GraphNode & node)
{
	const int sx = getInputInt(kInput_Width, 0);
	const int sy = getInputInt(kInput_Height, 0);
	
	allocateSurface(sx, sy);
	
	//
	
	const char * shaderName = getInputString(kInput_Shader, nullptr);
	
	if (shaderName)
	{
		loadShader(shaderName);
		
		currentShader = shaderName;
		currentShaderVersion = shader->getVersion();
	}
}

void VfxNodeFsfx::getDescription(VfxNodeDescription & d)
{
	d.add("shader constants:");
	for (int i = 0; i < dynamicInputs.size(); ++i)
	{
		auto & input = dynamicInputs[i];
		
		if (input.type == kVfxPlugType_Float)
			d.add("%s: %f", input.name.c_str(), getInputFloat(kInput_COUNT + i, 0.f));
		else if (input.type == kVfxPlugType_Color)
		{
			const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
			const VfxColor * color = getInputColor(kInput_COUNT + i, &defaultColor);
			d.add("%s: rgb=(%.2f, %.2f, %.2f), alpha=%.2f", input.name.c_str(), color->r, color->g, color->b, color->a);
		}
		else
		{
			Assert(false);
			d.add("%s: n/a", input.name.c_str());
		}
	}
}
