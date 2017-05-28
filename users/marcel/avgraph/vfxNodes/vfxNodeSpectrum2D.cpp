#include "framework.h"
#include "vfxNodeSpectrum2D.h"

VfxNodeSpectrum2D::VfxNodeSpectrum2D()
	: VfxNodeBase()
	, texture()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_ImageCpu);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeSpectrum2D::~VfxNodeSpectrum2D()
{
	freeTexture();
}

void VfxNodeSpectrum2D::tick(const float dt)
{
	// todo : analyze image. create/update texture

	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	
	if (image == nullptr)
	{
		freeTexture();
	}
	else if (texture.isChanged(image->sx, image->sy, GL_R32F))
	{
		if (image->sx == 0 || image->sy == 0)
			freeTexture();
		else
			allocateTexture(image->sx, image->sy);
	}
	
	if (texture.id != 0)
	{
		float * values = (float*)_mm_malloc(sizeof(float) * image->sx * image->sy, 16);
		for (int i = 0; i < image->sx * image->sy; ++i)
			values[i] = random(-1.f, +1.f);
		
		texture.upload(values, 16, image->sx, GL_RED, GL_FLOAT);
		
		_mm_free(values);
		values = nullptr;
	}
}

void VfxNodeSpectrum2D::init(const GraphNode & node)
{
	const VfxImageCpu * image = getInputImageCpu(kInput_Image, nullptr);
	
	if (image != nullptr)
	{
		allocateTexture(image->sx, image->sy);
	}
}

void VfxNodeSpectrum2D::allocateTexture(const int sx, const int sy)
{
	freeTexture();
	
	texture.allocate(sx, sy, GL_R32F);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
	
	imageOutput.texture = texture.id;
}

void VfxNodeSpectrum2D::freeTexture()
{
	texture.free();
	
	imageOutput.texture = 0;
}
