#include "framework.h"
#include "vfxNodeComposite.h"

extern int GFX_SX;
extern int GFX_SY;

VfxNodeComposite::VfxNodeComposite()
	: VfxNodeBase()
	, surface(nullptr)
	, image()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image1, kVfxPlugType_Image);
	addInput(kInput_Transform1, kVfxPlugType_Transform);
	addInput(kInput_Image2, kVfxPlugType_Image);
	addInput(kInput_Transform2, kVfxPlugType_Transform);
	addInput(kInput_Image3, kVfxPlugType_Image);
	addInput(kInput_Transform3, kVfxPlugType_Transform);
	addInput(kInput_Image4, kVfxPlugType_Image);
	addInput(kInput_Transform4, kVfxPlugType_Transform);
	addOutput(kOutput_Image, kVfxPlugType_Image, &image);
	
	surface = new Surface(GFX_SX, GFX_SY, true);
}

VfxNodeComposite::~VfxNodeComposite()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeComposite::tick(const float dt)
{
	pushSurface(surface);
	{
		surface->clear();
		
		const int kNumImages = 4;
		
		const VfxImageBase * images[kNumImages] =
		{
			getInputImage(kInput_Image1, nullptr),
			getInputImage(kInput_Image2, nullptr),
			getInputImage(kInput_Image3, nullptr),
			getInputImage(kInput_Image4, nullptr)
		};
		
		const VfxTransform defaultTransform;
		
		const VfxTransform * transforms[kNumImages] =
		{
			&getInputTransform(kInput_Transform1, defaultTransform),
			&getInputTransform(kInput_Transform2, defaultTransform),
			&getInputTransform(kInput_Transform3, defaultTransform),
			&getInputTransform(kInput_Transform4, defaultTransform)
		};
		
		for (int i = 0; i < kNumImages; ++i)
		{
			if (images[i] == nullptr)
				continue;
			
			gxPushMatrix();
			{
				gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
				gxMultMatrixf(transforms[i]->matrix.m_v);
				gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);
				
				gxSetTexture(images[i]->getTexture());
				{
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
				}
				gxSetTexture(0);
			}
			gxPopMatrix();
		}
	}
	popSurface();
	
	image.texture = surface->getTexture();
}
