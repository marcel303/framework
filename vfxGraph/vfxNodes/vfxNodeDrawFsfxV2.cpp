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
#include "Path.h"
#include "StringEx.h"
#include "vfxGraph.h"
#include "vfxNodeDrawFsfxV2.h"

static bool initialized = false;

static std::vector<std::string> s_shaderList;
static bool shaderListIsInitialized = false;

void getFsfxShaderList(std::vector<std::string> & shaderList)
{
	if (shaderListIsInitialized == false)
	{
		shaderListIsInitialized = true;
		auto files = listFiles("fsfx", true);
		std::sort(files.begin(), files.end());
		
		for (auto & file : files)
		{
			if (Path::GetExtension(file, true) == "ps")
				s_shaderList.push_back(file);
		}
		
		std::sort(files.begin(), files.end());
	}
	
	shaderList = s_shaderList;
}

// todo : create a vfxgraph-fsfx system, which requires explicit initialization. add registered systems to VfxGraphContext. when system is not present, let behavior be passthrough mode
// todo : allow to specify a custom location for FSFX shaders (getFsfxShaderList). ties in with registering FSFX object to VfxGraphContext
// todo : add a library of built-in shaders
// todo : automatically register FSFX objects when linked to a FSFX shader libraries? add support for multiple shader libraries?

VFX_ENUM_TYPE(fsfxShader)
{
	enumName = "fsfxShader";
	
	getElems = []() -> std::vector<Elem>
	{
		std::vector<std::string> shaderList;
		getFsfxShaderList(shaderList);
		
		std::vector<Elem> elems;
		
		for (auto & shader : shaderList)
		{
			Elem elem;
			elem.name = shader;
			elem.valueText = shader;
			
			elems.push_back(elem);
		}
		
		return elems;
	};
}

static const char * s_fsfxCommonInc = R"SHADER(
	include engine/ShaderPS.txt
	include engine/ShaderUtil.txt

	uniform vec4 params;

	uniform sampler2D colormap;
	uniform sampler2D image1;
	uniform sampler2D image2;
	uniform vec4 color1;
	uniform float param1;
	uniform float param2;
	uniform float time;
	uniform float opacity;

	shader_in vec2 texcoord;

#if !defined(__METAL_VERSION__)
	vec4 fsfx();
#endif

	float fsfxOpacity = 1.0;

	void main()
	{
		vec4 color = fsfx();
		
		color = applyColorPost(color, params.z);
		
		fsfxOpacity *= opacity;
		
		if (fsfxOpacity != 1.0)
		{
			vec4 baseColor = texture(colormap, texcoord);
			
			color = mix(baseColor, color, fsfxOpacity);
		}
		
		shader_fragColor = color;
	}
)SHADER";

VFX_NODE_TYPE(VfxNodeFsfxV2)
{
	typeName = "draw.fsfx-v2";
	displayName = "fsfx";
	
	in("before", "draw", "", "draw");
	inEnum("shader", "fsfxShader");
	in("image1", "image");
	in("image2", "image");
	in("color1", "color", "ffff");
	in("param1", "float");
	in("param2", "float");
	in("time", "float");
	in("opacity", "float", "1");
	out("any", "draw", "draw");
	out("image", "image");
}

VfxNodeFsfxV2::VfxNodeFsfxV2()
	: VfxNodeBase()
	, currentShader()
	, currentShaderVersion(0)
	, shader(nullptr)
	, shaderInputs()
	, gaussianKernel(nullptr)
	, imageSurface(nullptr)
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Draw, kVfxPlugType_Draw);
	addInput(kInput_Shader, kVfxPlugType_String);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addInput(kInput_Color1, kVfxPlugType_Color);
	addInput(kInput_Param1, kVfxPlugType_Float);
	addInput(kInput_Param2, kVfxPlugType_Float);
	addInput(kInput_Time, kVfxPlugType_Float);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
	
	if (initialized == false)
	{
		initialized = true;
		
		shaderSource("fsfx/common.inc", s_fsfxCommonInc);
	}
}

VfxNodeFsfxV2::~VfxNodeFsfxV2()
{
	freeShader();
	
	delete imageSurface;
	imageSurface = nullptr;
}

void VfxNodeFsfxV2::loadShader(const char * filename)
{
	freeShader();
	
	//
	
	clearEditorIssue();
	
	if (filename)
	{
		shader = new Shader(filename, "fsfx/fsfx.vs", filename);
		
		//
		
		if (!shader->isValid())
		{
			logDebug("shader is invalid: %s", filename);
			
			setEditorIssue("shader is invalid");
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
				
				// special?
				if (String::StartsWith(name, "gaussianKernel"))
				{
					if (gaussianKernel == nullptr)
					{
						gaussianKernel = new ShaderBuffer();
						makeGaussianKernel(60, *gaussianKernel);
					}
					
					continue;
				}
				
				// built-in?
				if (name == "screenSize" ||
					name == "params" ||
					name == "drawColor" ||
					name == "drawSkin")
					continue;
				
				// standardized input?
				if (name == "color1" ||
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
				else
				{
					logWarning("unknown GX immediate type: %d", type);
				}
			}
			
			auto textureInfos = shader->getTextureInfos();
			
			for (auto & info : textureInfos)
			{
				const GX_IMMEDIATE_TYPE type = info.type;
				const std::string & name = info.name;
				const GxImmediateIndex index = info.index;
				
				// built-in?
				if (name == "colormap")
					continue;
				
				// standardized input?
				if (name == "image1" ||
					name == "image2")
					continue;
				
				if (type == GX_IMMEDIATE_TEXTURE_2D)
				{
					ShaderInput shaderInput;
					shaderInput.type = type;
					shaderInput.socketType = kVfxPlugType_Image;
					shaderInput.socketIndex = socketIndex++;
					shaderInput.immediateIndex = index;
					shaderInputs.push_back(shaderInput);
					
					DynamicInput input;
					input.name = name;
					input.type = kVfxPlugType_Image;
					inputs.push_back(input);
				}
			}
			
			if (inputs.empty())
				setDynamicInputs(nullptr, 0);
			else
				setDynamicInputs(inputs.data(), inputs.size());
		}
	}
}

void VfxNodeFsfxV2::freeShader()
{
	if (gaussianKernel != nullptr)
		gaussianKernel->free();
		
	delete gaussianKernel;
	gaussianKernel = nullptr;
	
	delete shader;
	shader = nullptr;
	
	shaderInputs.clear();
	
	setDynamicInputs(nullptr, 0);
}

void VfxNodeFsfxV2::updateImageOutput(Surface * source) const
{
	if (source != nullptr && outputs[kOutput_Image].isReferenced())
	{
		if (imageSurface == nullptr || imageSurface->getWidth() != source->getWidth() || imageSurface->getHeight() != source->getHeight())
		{
			delete imageSurface;
			imageSurface = nullptr;
			
			imageSurface = new Surface(
				source->getWidth(),
				source->getHeight(),
				source->getFormat() == SURFACE_RGBA16F, false, false, 1);
		}
		
		source->blitTo(imageSurface);
		
		imageOutput.texture = imageSurface->getTexture();
	}
	else
	{
		delete imageSurface;
		imageSurface = nullptr;
		
		imageOutput.texture = 0;
	}
}

void VfxNodeFsfxV2::tick(const float dt)
{
	if (isPassthrough)
	{
		freeShader();
		
		currentShader.clear();
		currentShaderVersion = 0;
		
		return;
	}
	
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
	}
}

void VfxNodeFsfxV2::draw() const
{
	vfxCpuTimingBlock(VfxNodeFsfxV2);
	
	if (isPassthrough || shader == nullptr || g_currentVfxSurface == nullptr)
	{
		updateImageOutput(g_currentVfxSurface);
		
		return;
	}
	
	if (shader->isValid())
	{
		vfxGpuTimingBlock(VfxNodeFsfxV2);
		
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
				else if (shaderInput.type == GX_IMMEDIATE_TEXTURE_2D)
				{
					const VfxImageBase * image = getInputImage(shaderInput.socketIndex, nullptr);
					
					shader->setTexture(
						shaderInput.immediateIndex,
						textureSlot++,
						image ? image->getTexture() : 0,
						true,
						false);
				}
				else
				{
					Assert(false);
				}
			}
			
			const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
			
			const GxTextureId surfaceTexture = g_currentVfxSurface->getTexture();

			const VfxImageBase * image1 = getInputImage(kInput_Image1, nullptr);
			const VfxImageBase * image2 = getInputImage(kInput_Image2, nullptr);
			const GxTextureId texture1 = image1 ? image1->getTexture() : 0;
			const GxTextureId texture2 = image2 ? image2->getTexture() : 0;
			
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
			
			if (gaussianKernel != nullptr)
			{
				shader->setBuffer("gaussianKernel", *gaussianKernel);
				shader->setImmediate("gaussianKernelSize", 60.f);
			}
			
			pushBlend(BLEND_OPAQUE);
			{
				pushTransform();
				{
					setTransform(TRANSFORM_SCREEN);
					
				// fixme : the pop/push here causes a lot of extra gpu work ..
					popSurface();
					{
						g_currentVfxSurface->postprocess();
					}
					pushSurface(g_currentVfxSurface);
				}
				popTransform();
			}
			popBlend();
		}
		clearShader();
	}
	else
	{
		g_currentVfxSurface->clear(127, 0, 127, 255);
	}
	
	updateImageOutput(g_currentVfxSurface);
}

void VfxNodeFsfxV2::getDescription(VfxNodeDescription & d)
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
		else if (input.type == kVfxPlugType_Image)
		{
			const VfxImageBase * image = getInputImage(kInput_COUNT + i, nullptr);
			if (image != nullptr)
				d.add(input.name.c_str(), *image);
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
