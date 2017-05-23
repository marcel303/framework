#include "vfxNodeSpectrum2D.h"

VfxNodeSpectrum2D::VfxNodeSpectrum2D()
	: VfxNodeBase()
	, texture(0)
	, textureSx(0)
	, textureSy(0)
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
		return;
	}
	
	//

	if (texture == 0 || textureSx != image->sx || textureSy != image->sy)
	{
		allocateTexture(image->sx, image->sy);
	}
	
	float * values = (float*)alloca(sizeof(float) * image->sx * image->sy);
	for (int i = 0; i < image->sx * image->sy; ++i)
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
	
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->sx, image->sy, GL_RED, GL_FLOAT, values);
	checkErrorGL();
	
	// restore previous OpenGL states
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
	checkErrorGL();
	
	imageOutput.texture = texture;
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
	
	glGenTextures(1, &texture);
	checkErrorGL();
	
	if (texture == 0)
	{
		return;
	}
	
	textureSx = sx;
	textureSy = sy;
	
	// capture current OpenGL states before we change them
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	glBindTexture(GL_TEXTURE_2D, texture);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();
	
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, sx, sy);
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

void VfxNodeSpectrum2D::freeTexture()
{
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
	textureSx = 0;
	textureSy = 0;
}
