#if !defined(IPHONEOS)
	#include <GL/glew.h>
#endif

#include "framework.h"

#if ENABLE_OPENGL

#include "renderTarget.h"

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
#endif

static GLenum translateColorFormat(const SURFACE_FORMAT format)
{
	GLenum glFormat = GL_INVALID_ENUM;

	if (format == SURFACE_RGBA8)
		glFormat = GL_RGBA8;
	if (format == SURFACE_RGBA8_SRGB)
		glFormat = GL_SRGB8_ALPHA8;
	if (format == SURFACE_RGBA16F)
		glFormat = GL_RGBA16F;
	if (format == SURFACE_RGBA32F)
		glFormat = GL_RGBA32F;
	if (format == SURFACE_R8)
		glFormat = GL_R8;
	if (format == SURFACE_R16F)
		glFormat = GL_R16F;
	if (format == SURFACE_R32F)
		glFormat = GL_R32F;
	if (format == SURFACE_RG8)
		glFormat = GL_RG8;
	if (format == SURFACE_RG16F)
		glFormat = GL_RG16F;
	if (format == SURFACE_RG32F)
		glFormat = GL_RG32F;
	
	return glFormat;
}

ColorTarget::~ColorTarget()
{
	free();
}

bool ColorTarget::init(const ColorTargetProperties & in_properties)
{
	bool result = true;
	
	//
	
	free();
	
	//
	
	properties = in_properties;
	
	// allocate storage

	fassert(m_colorTextureId == 0);
	glGenTextures(1, &m_colorTextureId);
	checkErrorGL();

	if (m_colorTextureId == 0)
		result = false;
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_colorTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		const GLenum glFormat = translateColorFormat(properties.format);

	#if USE_LEGACY_OPENGL
		GLenum uploadFormat = GL_INVALID_ENUM;
		GLenum uploadType = GL_INVALID_ENUM;

		if (properties.format == SURFACE_RGBA8)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_UNSIGNED_BYTE;
		}
		if (properties.format == SURFACE_RGBA16F)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_RGBA32F)
		{
			uploadFormat = GL_RGBA;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_R8)
		{
			uploadFormat = GL_RED;
			uploadType = GL_UNSIGNED_BYTE;
		}
		if (properties.format == SURFACE_R16F)
		{
			uploadFormat = GL_RED;
			uploadType = GL_FLOAT;
		}
		if (properties.format == SURFACE_R32F)
		{
			uploadFormat = GL_RED;
			uploadType = GL_FLOAT;
		}

		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, properties.dimensions.width, properties.dimensions.height, 0, uploadFormat, uploadType, nullptr);
		checkErrorGL();
	#else
		glTexStorage2D(GL_TEXTURE_2D, 1, glFormat, properties.dimensions.width, properties.dimensions.height);
		checkErrorGL();
	#endif

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
	// todo : properly restore previous texture
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}
	
	if (result == false)
	{
		if (m_colorTextureId != 0)
		{
			glDeleteTextures(1, &m_colorTextureId);
			m_colorTextureId = 0;
		}
	}
	
	return result;
}

void ColorTarget::free()
{
	if (m_colorTextureId != 0)
	{
		glDeleteTextures(1, &m_colorTextureId);
		m_colorTextureId = 0;
	}
}

//

static GLenum translateDepthFormat(const DEPTH_FORMAT format)
{
	GLenum glFormat = GL_INVALID_ENUM;

	if (format == DEPTH_FLOAT16)
		glFormat = GL_DEPTH_COMPONENT16;
	
#if ENABLE_DESKTOP_OPENGL
	// todo : gles : float32 depth format ?
	if (format == DEPTH_FLOAT32)
		glFormat = GL_DEPTH32F_STENCIL8;
#endif

	return glFormat;
}

DepthTarget::~DepthTarget()
{
	free();
}

bool DepthTarget::init(const DepthTargetProperties & in_properties)
{
	bool result = true;
	
	//
	
	free();
	
	//
	
	properties = in_properties;
	
	fassert(m_depthTextureId == 0);
	glGenTextures(1, &m_depthTextureId);
	checkErrorGL();

	if (m_depthTextureId == 0)
		result = false;
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_depthTextureId);
		checkErrorGL();

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		const GLenum glFormat = translateDepthFormat(properties.format);

	#if USE_LEGACY_OPENGL
		GLenum uploadFormat = GL_DEPTH_COMPONENT;
		GLenum uploadType = GL_FLOAT;

		glTexImage2D(GL_TEXTURE_2D, 0, glFormat, properties.dimensions.width, properties.dimensions.height, 0, uploadFormat, uploadType, 0);
		checkErrorGL();
	#else
		glTexStorage2D(GL_TEXTURE_2D, 1, glFormat, properties.dimensions.width, properties.dimensions.height);
		checkErrorGL();
	#endif

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
	// todo : properly restore previous texture
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}
	
	if (result == false)
	{
		if (m_depthTextureId != 0)
		{
			glDeleteTextures(1, &m_depthTextureId);
			m_depthTextureId = 0;
		}
	}
	
	return result;
}

void DepthTarget::free()
{
	if (m_depthTextureId != 0)
	{
		glDeleteTextures(1, &m_depthTextureId);
		m_depthTextureId = 0;
	}
}

#endif
