#pragma once

#include "vfxNodeBase.h"
#include <SDL2/SDL_opengl.h>

struct VfxNodeSpectrum1D : VfxNodeBase
{
	enum Input
	{
		kInput_Buffer,
		kInput_Size,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Image,
		kOutput_COUNT
	};
	
	GLuint texture;
	
	VfxImage_Texture imageOutput;

	VfxNodeSpectrum1D();
	virtual ~VfxNodeSpectrum1D() override;
	
	virtual void tick(const float dt) override;
	
	virtual void init(const GraphNode & node) override;
	
	void allocateTexture(const int size);
	void freeTexture();
};
