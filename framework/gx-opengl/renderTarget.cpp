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
	else if (format == SURFACE_RGBA8_SRGB)
		glFormat = GL_SRGB8_ALPHA8;
	else if (format == SURFACE_RGBA16F)
		glFormat = GL_RGBA16F;
	else if (format == SURFACE_RGBA32F)
		glFormat = GL_RGBA32F;
	else if (format == SURFACE_R8)
		glFormat = GL_R8;
	else if (format == SURFACE_R16F)
		glFormat = GL_R16F;
	else if (format == SURFACE_R32F)
		glFormat = GL_R32F;
	else if (format == SURFACE_RG8)
		glFormat = GL_RG8;
	else if (format == SURFACE_RG16F)
		glFormat = GL_RG16F;
	else if (format == SURFACE_RG32F)
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
		// capture current OpenGL states before we change them

		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		// allocate texture storage
		
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
		
		// restore previous OpenGL states

		glBindTexture(GL_TEXTURE_2D, restoreTexture);
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

	if (format == DEPTH_UNORM16)
		glFormat = GL_DEPTH24_STENCIL8; // GL_DEPTH_COMPONENT16 support is less common than DEPTH24
#if ENABLE_DESKTOP_OPENGL
	// todo : gles : float32 depth format ?
	else if (format == DEPTH_FLOAT32)
		glFormat = GL_DEPTH32F_STENCIL8;
#endif

	Assert(glFormat != GL_INVALID_ENUM);

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
		// capture current OpenGL states before we change them

		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		checkErrorGL();
		
		// restore previous OpenGL states

		glBindTexture(GL_TEXTURE_2D, restoreTexture);
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
