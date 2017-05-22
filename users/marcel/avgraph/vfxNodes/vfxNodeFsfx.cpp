#include "vfxGraph.h"
#include "vfxNodeFsfx.h"

extern const int GFX_SX;
extern const int GFX_SY;

VfxNodeFsfx::VfxNodeFsfx()
	: VfxNodeBase()
	, surface(nullptr)
	, image(nullptr)
{
	image = new VfxImage_Texture();
	
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Shader, kVfxPlugType_String);
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

void VfxNodeFsfx::draw() const
{
	if (isPassthrough)
	{
		const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
		const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
		image->texture = inputTexture;
		return;
	}
	
	const std::string & shaderName = getInputString(kInput_Shader, "");
	
	if (shaderName.empty())
	{
		// todo : warn ?
	}
	else
	{
		const VfxImageBase * inputImage = getInputImage(kInput_Image, nullptr);
		const GLuint inputTexture = inputImage != nullptr ? inputImage->getTexture() : surface->getTexture();
		Shader shader(shaderName.c_str());
		
		if (shader.isValid())
		{
			pushBlend(BLEND_OPAQUE);
			setShader(shader);
			{
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
	const int w = getInputInt(kInput_Width, 0);
	const int h = getInputInt(kInput_Height, 0);
	
	const int sx = w ? w : GFX_SX;
	const int sy = h ? h : GFX_SY;
	
	surface = new Surface(sx, sy, true);
	
	surface->clear();
	surface->swapBuffers();
	surface->clear();
	surface->swapBuffers();
}
