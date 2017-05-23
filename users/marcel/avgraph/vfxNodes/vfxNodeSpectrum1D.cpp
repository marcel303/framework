#include "vfxNodeSpectrum1D.h"

VfxNodeSpectrum1D::VfxNodeSpectrum1D()
	: VfxNodeBase()
	, texture(0)
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
	
	if (false)
	{
		allocateTexture(windowSize);
	}
	
	float * values = (float*)alloca(sizeof(float) * windowSize);
	for (int i = 0; i < windowSize; ++i)
		values[i] = random(-1.f, +1.f);
	
	// capture current OpenGL states before we change them
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	GLint restoreUnpack;
	glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
	checkErrorGL();
	
	glBindTexture(GL_TEXTURE_2D, texture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	checkErrorGL();
	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, windowSize, 1, GL_RED, GL_FLOAT, values);
	checkErrorGL();
	
	// restore previous OpenGL states
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	checkErrorGL();
}

void VfxNodeSpectrum1D::init(const GraphNode & node)
{
	const int windowSize = getInputInt(kInput_Size, 64);
	
	allocateTexture(windowSize);
}

void VfxNodeSpectrum1D::allocateTexture(const int size)
{
	freeTexture();
	
	glGenTextures(1, &texture);
	checkErrorGL();
	
	if (texture == 0)
	{
		return;
	}
	
	// capture current OpenGL states before we change them
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	glBindTexture(GL_TEXTURE_2D, texture);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();
	
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, size, 1);
	checkErrorGL();
	
	GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();

	// set filtering

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	checkErrorGL();
	
	// todo : clear texture contents
	
	// restore previous OpenGL states
			
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}

void VfxNodeSpectrum1D::freeTexture()
{
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
		checkErrorGL();
		
		imageOutput.texture = 0;
	}
}
