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

extern int VFXGRAPH_SX;
extern int VFXGRAPH_SY;

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
	, imageOutput()
{
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
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeFsfx::~VfxNodeFsfx()
{
	delete surface;
	surface = nullptr;
	
	delete shader;
	shader = nullptr;
}

void VfxNodeFsfx::allocateSurface(const int sx, const int sy)
{
	Assert(surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight());
	
	freeSurface();
	
	if (sx > 0 && sy > 0)
	{
		surface = new Surface(sx, sy, true);
		
		surface->clear();
		surface->swapBuffers();
		surface->clear();
		surface->swapBuffers();
	}
}

void VfxNodeFsfx::freeSurface()
{
	delete surface;
	surface = nullptr;
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
			std::vector<DynamicInput> inputs;
			
			auto immediateInfos = shader->getImmediateInfos();
			
			std::sort(immediateInfos.begin(), immediateInfos.end(),
				[](GxImmediateInfo & u1, GxImmediateInfo & u2)
				{
					return u1.name < u2.name; 
				});
			
			int socketIndex = VfxNodeBase::inputs.size();

			for (auto & info : immediateInfos)
			{
				const GX_IMMEDIATE_TYPE type = info.type;
				const std::string & name = info.name;
				const GxImmediateIndex index = info.index;
				
				// built-in?
				if (name == "screenSize" ||
					name == "colormap" ||
					name == "drawColor" ||
					name == "drawSkin")
					continue;
				
				// standardized input?
				if (name == "image1" ||
					name == "image2" ||
					name == "color1" ||
					name == "param1" ||
					name == "param2" ||
					name == "time" ||
					name == "opacity")
					continue;
				
				//logDebug("immediate %d. name=%s, type=%d, size=%d", i, name, type, size);
				
				if (type == GX_IMMEDIATE_FLOAT)
				{
					ShaderInput shaderInput;
					shaderInput.type = type;
					shaderInput.socketType = kVfxPlugType_Float;
					shaderInput.socketIndex = socketIndex++;
					shaderInput.immediateIndex = index;
					shaderInputs.push_back(shaderInput);
					
					DynamicInput input;
					input.name = name;
					input.type = kVfxPlugType_Float;
					inputs.push_back(input);
				}
				else if (type == GX_IMMEDIATE_VEC2 || type == GX_IMMEDIATE_VEC3 || type == GX_IMMEDIATE_VEC4)
				{
					if (String::StartsWith(name, "color"))
					{
						ShaderInput shaderInput;
						shaderInput.type = type;
						shaderInput.socketType = kVfxPlugType_Color;
						shaderInput.socketIndex = socketIndex++;
						shaderInput.immediateIndex = index;
						shaderInputs.push_back(shaderInput);
					
						DynamicInput input;
						input.name = name;
						input.type = kVfxPlugType_Color;
						inputs.push_back(input);
					}
					else
					{
						ShaderInput shaderInput;
						shaderInput.type = type;
						shaderInput.socketType = kVfxPlugType_Float;
						shaderInput.socketIndex = socketIndex;
						shaderInput.immediateIndex = index;
						shaderInputs.push_back(shaderInput);
					
						const int numElems = type == GX_IMMEDIATE_VEC2 ? 2 : type == GX_IMMEDIATE_VEC3 ? 3 : 4;
						
						char elementNames[4] = { 'x', 'y', 'z', 'w' };
						
						for (int i = 0; i < numElems; ++i)
						{
							DynamicInput input;
							input.name = String::Format("%s.%c", name.c_str(), elementNames[i]);
							input.type = kVfxPlugType_Float;
							inputs.push_back(input);
						}
						
						socketIndex += numElems;
					}
				}
			// todo : add getTextureInfos() to shader
				/*
				else if (type == GL_SAMPLER_2D)
				{
					ShaderInput shaderInput;
					shaderInput.type = type;
					shaderInput.socketType = kVfxPlugType_Image;
					shaderInput.socketIndex = socketIndex++;
					shaderInput.uniformLocation = location;
					shaderInputs.push_back(shaderInput);
					
					DynamicInput input;
					input.name = name;
					input.type = kVfxPlugType_Image;
					inputs.push_back(input);
				}
				*/
				else
				{
				}
			}
			
			if (inputs.empty())
				setDynamicInputs(nullptr, 0);
			else
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
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const char * shaderName = getInputString(kInput_Shader, nullptr);
	
	if (isPassthrough || shaderName == nullptr)
	{
		freeSurface();
		freeShader();
		
		currentShader.clear();
		currentShaderVersion = 0;
		
		return;
	}
	
	const int sx = image ? image->getSx() : getInputInt(kInput_Width, g_currentVfxSurface ? g_currentVfxSurface->getWidth() : VFXGRAPH_SX);
	const int sy = image ? image->getSy() : getInputInt(kInput_Height, g_currentVfxSurface ? g_currentVfxSurface->getHeight() : VFXGRAPH_SY);
	
	if (surface == nullptr || sx != surface->getWidth() || sy != surface->getHeight())
	{
		allocateSurface(sx, sy);
	}
	
	//
	
	if (shaderName != currentShader || (shader != nullptr && shader->getVersion() != currentShaderVersion))
	{
		loadShader(shaderName);
		
		currentShader = shaderName;
		currentShaderVersion = shader->getVersion();
	}
}

void VfxNodeFsfx::draw() const
{
	vfxCpuTimingBlock(VfxNodeFsfx);
	
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	
	clearEditorIssue();
	
	if (isPassthrough || surface == nullptr || shader == nullptr)
	{
		if (isPassthrough)
			imageOutput.texture = image != nullptr ? image->getTexture() : 0;
		else
			imageOutput.texture = 0;
		
		return;
	}
	
	if (shader->isValid())
	{
		vfxGpuTimingBlock(VfxNodeFsfx);
		
		setShader(*shader);
		{
			int textureSlot = 0;
			
			for (auto & shaderInput : shaderInputs)
			{
				if (shaderInput.type == GX_IMMEDIATE_FLOAT)
				{
					shader->setImmediate(shaderInput.immediateIndex, getInputFloat(shaderInput.socketIndex, 0.f));
				}
				else if (shaderInput.type == GX_IMMEDIATE_VEC2)
				{
					if (shaderInput.socketType == kVfxPlugType_Color)
					{
						const VfxColor defaultColor(0.f, 0.f, 0.f, 0.f);
						const VfxColor * color = getInputColor(shaderInput.socketIndex, &defaultColor);
						
						shader->setImmediate(
							shaderInput.immediateIndex,
							color->r,
							color->g);
					}
					else
					{
						shader->setImmediate(
							shaderInput.immediateIndex,
							getInputFloat(shaderInput.socketIndex + 0, 0.f),
							getInputFloat(shaderInput.socketIndex + 1, 0.f));
					}
				}
				else if (shaderInput.type == GX_IMMEDIATE_VEC3)
				{
					if (shaderInput.socketType == kVfxPlugType_Color)
					{
						const VfxColor defaultColor(0.f, 0.f, 0.f, 0.f);
						const VfxColor * color = getInputColor(shaderInput.socketIndex, &defaultColor);
						
						shader->setImmediate(
							shaderInput.immediateIndex,
							color->r,
							color->g,
							color->b);
					}
					else
					{
						shader->setImmediate(
							shaderInput.immediateIndex,
							getInputFloat(shaderInput.socketIndex + 0, 0.f),
							getInputFloat(shaderInput.socketIndex + 1, 0.f),
							getInputFloat(shaderInput.socketIndex + 2, 0.f));
					}
				}
				else if (shaderInput.type == GX_IMMEDIATE_VEC4)
				{
					if (shaderInput.socketType == kVfxPlugType_Color)
					{
						const VfxColor defaultColor(0.f, 0.f, 0.f, 0.f);
						const VfxColor * color = getInputColor(shaderInput.socketIndex, &defaultColor);
						
						shader->setImmediate(
							shaderInput.immediateIndex,
							color->r,
							color->g,
							color->b,
							color->a);
					}
					else
					{
						shader->setImmediate(
							shaderInput.immediateIndex,
							getInputFloat(shaderInput.socketIndex + 0, 0.f),
							getInputFloat(shaderInput.socketIndex + 1, 0.f),
							getInputFloat(shaderInput.socketIndex + 2, 0.f),
							getInputFloat(shaderInput.socketIndex + 3, 0.f));
					}
				}
			// todo : add getTextureInfos() to shader
				/*
				else if (shaderInput.type == GL_SAMPLER_2D)
				{
					const VfxImageBase * image = getInputImage(shaderInput.socketIndex, nullptr);
					
					shader->setTextureUniform(
						shaderInput.uniformLocation,
						textureSlot++,
						image ? image->getTexture() : 0);
				}
				*/
				else
				{
					Assert(false);
				}
			}
			
			const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
			
			const GLuint imageTexture = image != nullptr ? image->getTexture() : surface->getTexture();
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
			shader->setTexture("colormap", textureSlot++, imageTexture, true, false);
			shader->setTexture("image1", textureSlot++, texture1, true, false);
			shader->setTexture("image2", textureSlot++, texture2, true, false);
			shader->setImmediate("color1", color->r, color->g, color->b, color->a);
			shader->setImmediate("param1", param1);
			shader->setImmediate("param2", param2);
			shader->setImmediate("opacity", opacity);
			shader->setImmediate("time", time);
			
			pushBlend(BLEND_OPAQUE);
			{
				pushTransform();
				setTransform(TRANSFORM_SCREEN);
				surface->postprocess();
				popTransform();
			}
			popBlend();
		}
		clearShader();
	}
	else
	{
		setEditorIssue("shader is invalid");
		
		surface->clear(127, 0, 127, 255);
	}
	
	imageOutput.texture = surface->getTexture();
}

void VfxNodeFsfx::getDescription(VfxNodeDescription & d)
{
	d.add("shader constants:");
	if (dynamicInputs.empty())
		d.add("(none)");
	for (size_t i = 0; i < dynamicInputs.size(); ++i)
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
	
	std::vector<std::string> errorMessages;
	if (shader && shader->getErrorMessages(errorMessages))
	{
		d.newline();
		d.add("error messages:");
		for (auto & errorMessage : errorMessages)
			d.add("%s", errorMessage.c_str());
	}
}
