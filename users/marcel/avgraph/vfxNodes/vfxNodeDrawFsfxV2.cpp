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
#include "Path.h"
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxNodeDrawFsfxV2.h"

static std::vector<std::string> s_shaderList;
static bool shaderListIsInitialized = false;

void getFsfxShaderList(std::vector<std::string> & shaderList)
{
	if (shaderListIsInitialized == false)
	{
		shaderListIsInitialized = true;
		auto files = listFiles("fsfx", true);
		
		for (auto & file : files)
		{
			if (Path::GetExtension(file, true) == "ps")
				s_shaderList.push_back(Path::StripExtension(file));
		}
	}
	
	shaderList = s_shaderList;
}

static const char * toShaderName(const int index)
{
	if (index < 0 || index >= s_shaderList.size())
		return nullptr;
	else
		return s_shaderList[index].c_str();
}

VFX_NODE_TYPE(VfxNodeFsfxV2)
{
	typeName = "draw.fsfx-v2";
	displayName = "fsfx";
	
	in("before", "any");
	inEnum("shader", "fsfxShader");
	in("image1", "image");
	in("image2", "image");
	in("color1", "color", "ffff");
	in("param1", "float");
	in("param2", "float");
	in("time", "float");
	in("opacity", "float", "1");
	out("any", "any");
}

VfxNodeFsfxV2::VfxNodeFsfxV2()
	: VfxNodeBase()
	, currentShader()
	, currentShaderVersion(0)
	, shader(nullptr)
	, shaderInputs()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Before, kVfxPlugType_DontCare);
	addInput(kInput_Shader, kVfxPlugType_Int);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addInput(kInput_Color1, kVfxPlugType_Color);
	addInput(kInput_Param1, kVfxPlugType_Float);
	addInput(kInput_Param2, kVfxPlugType_Float);
	addInput(kInput_Time, kVfxPlugType_Float);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, nullptr);
}

VfxNodeFsfxV2::~VfxNodeFsfxV2()
{
	delete shader;
	shader = nullptr;
}

void VfxNodeFsfxV2::loadShader(const char * filename)
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
			
			int socketIndex = VfxNodeBase::inputs.size();

			for (int i = 0; i < uniformCount; ++i)
			{
				const GLsizei bufferSize = 32;
				char name[bufferSize];
				
				GLsizei length;
				GLint size;
				GLenum type;
				
				glGetActiveUniform(shader->getProgram(), i, bufferSize, &length, &size, &type, name);
				
				// built-in?
				if (!strcmp(name, "screenSize") || !strcmp(name, "colormap"))
					continue;
				
				// standardized input?
				if (!strcmp(name, "image1") || !strcmp(name, "image2") || !strcmp(name, "color1") || !strcmp(name, "param1") || !strcmp(name, "param2") || !strcmp(name, "time") || !strcmp(name, "opacity"))
					continue;
				
				const GLint location = glGetUniformLocation(shader->getProgram(), name);
				
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

void VfxNodeFsfxV2::freeShader()
{
	delete shader;
	shader = nullptr;
	
	shaderInputs.clear();
	
	setDynamicInputs(nullptr, 0);
}

void VfxNodeFsfxV2::tick(const float dt)
{
	if (isPassthrough)
	{
		freeShader();
		return;
	}
	
	const char * shaderName = toShaderName(getInputInt(kInput_Shader, 0));
	
	if (shaderName == nullptr)
	{
		currentShader.clear();
		currentShaderVersion = 0;
		
		freeShader();
	}
	else
	{
		if (shaderName != currentShader || (shader && shader->getVersion() != currentShaderVersion))
		{
			freeShader();
			
			//
			
			loadShader(shaderName);
			
			currentShader = shaderName;
			currentShaderVersion = shader->getVersion();
		}
	}
}

void VfxNodeFsfxV2::draw() const
{
	vfxCpuTimingBlock(VfxNodeFsfxV2);
	
	clearEditorIssue();
	
	if (isPassthrough || shader == nullptr || g_currentVfxSurface == nullptr)
	{
		return;
	}
	
	if (shader->isValid())
	{
		vfxGpuTimingBlock(VfxNodeFsfxV2);
		
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
			
			const GLuint surfaceTexture = g_currentVfxSurface->getTexture();

			const VfxImageBase * image1 = getInputImage(kInput_Image1, nullptr);
			const VfxImageBase * image2 = getInputImage(kInput_Image2, nullptr);
			const GLuint texture1 = image1 ? image1->getTexture() : 0;
			const GLuint texture2 = image2 ? image2->getTexture() : 0;
			
			const VfxColor * color = getInputColor(kInput_Color1, &defaultColor);
			const float param1 = getInputFloat(kInput_Param1, 0.f);
			const float param2 = getInputFloat(kInput_Param2, 0.f);
			const float time = getInputFloat(kInput_Time, g_currentVfxGraph->time);
			const float opacity = getInputFloat(kInput_Opacity, 1.f);
			
			shader->setImmediate("screenSize", g_currentVfxSurface->getWidth(), g_currentVfxSurface->getHeight());
			shader->setTexture("colormap", textureSlot++, surfaceTexture, true, false);
			shader->setTexture("image1", textureSlot++, texture1, true, false);
			shader->setTexture("image2", textureSlot++, texture2, true, false);
			shader->setImmediate("color1", color->r, color->g, color->b, color->a);
			shader->setImmediate("param1", param1);
			shader->setImmediate("param2", param2);
			shader->setImmediate("opacity", opacity);
			shader->setImmediate("time", time);

			g_currentVfxSurface->postprocess();
		}
		clearShader();
		popBlend();
	}
	else
	{
		setEditorIssue("shader is invalid");
		
		pushBlend(BLEND_ADD);
		{
			setColor(127, 0, 127);
			drawRect(0, 0, g_currentVfxSurface->getWidth(), g_currentVfxSurface->getHeight());
		}
		popBlend();
	}
}

void VfxNodeFsfxV2::init(const GraphNode & node)
{
	const char * shaderName = toShaderName(getInputInt(kInput_Shader, 0));
	
	if (shaderName)
	{
		loadShader(shaderName);
		
		currentShader = shaderName;
		currentShaderVersion = shader->getVersion();
	}
}

void VfxNodeFsfxV2::getDescription(VfxNodeDescription & d)
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
