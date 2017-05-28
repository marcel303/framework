#include "framework.h"
#include "vfxNodeSpectrum1D.h"

VfxNodeSpectrum1D::VfxNodeSpectrum1D()
	: VfxNodeBase()
	, texture()
	, imageOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Buffer, kVfxPlugType_Int);
	addInput(kInput_Size, kVfxPlugType_Int);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);
}

VfxNodeSpectrum1D::~VfxNodeSpectrum1D()
{
	freeTexture();
}

void VfxNodeSpectrum1D::tick(const float dt)
{
	// todo : analyze buffer. create/update texture

	const int buffer = getInputInt(kInput_Buffer, 0);
	const int windowSize = getInputInt(kInput_Size, 64);
	
	if (texture.isChanged(windowSize, 1, GL_R32F))
	{
		if (windowSize <= 0)
			freeTexture();
		else
			allocateTexture(windowSize);
	}
	
	if (texture.id != 0)
	{
		float * values = (float*)alloca(sizeof(float) * windowSize);
		for (int i = 0; i < windowSize; ++i)
			values[i] = random(-1.f, +1.f);
		
		texture.upload(values, 4, windowSize, GL_RED, GL_FLOAT);
	}
}

void VfxNodeSpectrum1D::init(const GraphNode & node)
{
	const int windowSize = getInputInt(kInput_Size, 64);
	
	if (windowSize > 0)
		allocateTexture(windowSize);
}

void VfxNodeSpectrum1D::allocateTexture(const int size)
{
	freeTexture();
	
	texture.allocate(size, 1, GL_R32F);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
	
	imageOutput.texture = texture.id;
}

void VfxNodeSpectrum1D::freeTexture()
{
	texture.free();
	
	imageOutput.texture = 0;
}
