/*
	Copyright (C) 2017 Marcel Smit
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
#include "gx_render.h"

#if ENABLE_OPENGL

#include "internal.h"
#include <algorithm>

#endif

extern int s_backingScale; // todo : can this be exposed/determined more nicely?

void Surface::construct()
{
	m_backingScale = 1;
	
	m_bufferId = 0;
	
	m_colorTarget[0] = nullptr;
	m_colorTarget[1] = nullptr;
	m_depthTarget[0] = nullptr;
	m_depthTarget[1] = nullptr;
	
	m_name = "Surface";
}

void Surface::destruct()
{
	if (m_properties.colorTarget.enabled)
	{
		for (int i = 0; i < (m_properties.colorTarget.doubleBuffered ? 2 : 1); ++i)
		{
			if (m_colorTarget[i] != nullptr)
			{
				delete m_colorTarget[i];
				m_colorTarget[i] = nullptr;
			}
		}
	}
	
	m_colorTarget[0] = nullptr;
	m_colorTarget[1] = nullptr;
	
	if (m_properties.depthTarget.enabled)
	{
		for (int i = 0; i < (m_properties.depthTarget.doubleBuffered ? 2 : 1); ++i)
		{
			if (m_depthTarget[i] != nullptr)
			{
				delete m_depthTarget[i];
				m_depthTarget[i] = nullptr;
			}
		}
	}
	
	m_depthTarget[0] = nullptr;
	m_depthTarget[1] = nullptr;
	
	m_properties = SurfaceProperties();
	m_backingScale = 1;
	
	m_bufferId = 0;
}

Surface::Surface()
{
	construct();
}

Surface::Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer, bool doubleBuffered, const int backingScale)
{
	construct();
	
	init(sx, sy, highPrecision ? SURFACE_RGBA16F : SURFACE_RGBA8, withDepthBuffer, doubleBuffered, backingScale);
}

Surface::Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format, const int backingScale)
{
	construct();

	init(sx, sy, format, withDepthBuffer, doubleBuffered, backingScale);
}

Surface::~Surface()
{
	destruct();
}

void Surface::swapBuffers()
{
	fassert(m_properties.colorTarget.doubleBuffered || m_properties.depthTarget.doubleBuffered);

	m_bufferId = (m_bufferId + 1) % 2;
}

bool Surface::init(const SurfaceProperties & properties)
{
	fassert(m_colorTarget[0] == nullptr && m_depthTarget[0] == nullptr);
	
	m_properties = properties;

	const int sx = properties.dimensions.width  / framework.minification;
	const int sy = properties.dimensions.height / framework.minification;
	
	//
	
	bool result = true;
	
	if (properties.dimensions.backingScale != 0)
		m_backingScale = properties.dimensions.backingScale;
	else
		m_backingScale = s_backingScale;
	
	const int backingSx = sx * m_backingScale;
	const int backingSy = sy * m_backingScale;
	
	// allocate color target backing storage

	if (properties.colorTarget.enabled)
	{
		ColorTargetProperties targetProperties;
		targetProperties.dimensions.width = backingSx;
		targetProperties.dimensions.height = backingSy;
		targetProperties.format = properties.colorTarget.format;
		
		for (int i = 0; result && i < (properties.colorTarget.doubleBuffered ? 2 : 1); ++i)
		{
			m_colorTarget[i] = new ColorTarget();
			result &= m_colorTarget[i]->init(targetProperties);
		}
		
		if (result && properties.colorTarget.doubleBuffered == false)
		{
			m_colorTarget[1] = m_colorTarget[0];
		}
	}
	
	// allocate depth target backing storage
	
	if (properties.depthTarget.enabled)
	{
		DepthTargetProperties targetProperties;
		targetProperties.dimensions.width = backingSx;
		targetProperties.dimensions.height = backingSy;
		targetProperties.format = properties.depthTarget.format;
		targetProperties.enableTexture = true;
		
		for (int i = 0; result && i < (properties.depthTarget.doubleBuffered ? 2 : 1); ++i)
		{
			m_depthTarget[i] = new DepthTarget();
			result &= m_depthTarget[i]->init(targetProperties);
		}
		
		if (result && properties.depthTarget.doubleBuffered == false)
		{
			m_depthTarget[1] = m_depthTarget[0];
		}
	}
	
	if (!result)
	{
		logError("failed to init surface. calling destruct()");
		
		destruct();
	}
	
	// set color swizzle
	
	setSwizzle(
		properties.colorTarget.swizzle[0],
		properties.colorTarget.swizzle[1],
		properties.colorTarget.swizzle[2],
		properties.colorTarget.swizzle[3]);
	
	return result;
}

bool Surface::init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered, const int backingScale)
{
	SurfaceProperties properties;
	
	properties.dimensions.width = sx;
	properties.dimensions.height = sy;
	properties.dimensions.backingScale = backingScale;
	properties.colorTarget.enabled = true;
	properties.colorTarget.format = format;
	properties.colorTarget.doubleBuffered = doubleBuffered;
	
	if (withDepthBuffer)
	{
		properties.depthTarget.enabled = true;
		properties.depthTarget.format = DEPTH_FLOAT32;
		properties.depthTarget.doubleBuffered = false;
	}
	
	return init(properties);
}

void Surface::free()
{
	destruct();
}

#if ENABLE_OPENGL

// todo : perhaps use GxTextures internally

static GLint toOpenGLTextureSwizzle(const int value)
{
	if (value == GX_SWIZZLE_ZERO)
		return GL_ZERO;
	else if (value == GX_SWIZZLE_ONE)
		return GL_ONE;
	else if (value == 0)
		return GL_RED;
	else if (value == 1)
		return GL_GREEN;
	else if (value == 2)
		return GL_BLUE;
	else if (value == 3)
		return GL_ALPHA;
	else
		return GL_INVALID_ENUM;
}

#endif

void Surface::setSwizzle(int r, int g, int b, int a)
{
#if ENABLE_METAL
	Assert(false);
#elif ENABLE_OPENGL
	fassert(m_properties.colorTarget.enabled);
	if (m_properties.colorTarget.enabled == false)
		return;
	
#if USE_LEGACY_OPENGL
	return; // sorry; not supported!
#endif
	
	// capture previous OpenGL state
	
	GLuint oldTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oldTexture);
	checkErrorGL();
	
#if ENABLE_DESKTOP_OPENGL
	// set swizzle on both targets

	GLint swizzleMask[4] =
	{
		toOpenGLTextureSwizzle(r),
		toOpenGLTextureSwizzle(g),
		toOpenGLTextureSwizzle(b),
		toOpenGLTextureSwizzle(a)
	};

	for (int i = 0; i < (m_properties.colorTarget.doubleBuffered ? 2 : 1); ++i)
	{
		glBindTexture(GL_TEXTURE_2D, m_colorTarget[i]->getTextureId());
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		checkErrorGL();
	}
#else
	// todo : gles : swizzle mask ?
#endif

	// restore the previous OpenGL state
	
	glBindTexture(GL_TEXTURE_2D, oldTexture);
	checkErrorGL();
#endif
}

void Surface::setClearColor(int r, int g, int b, int a)
{
	setClearColorf(
		r / 255.f,
		g / 255.f,
		b / 255.f,
		a / 255.f);
}

void Surface::setClearColorf(float r, float g, float b, float a)
{
	for (int i = 0; i < 2; ++i)
		if (m_colorTarget[i] != nullptr)
			m_colorTarget[i]->setClearColor(r, g, b, a);
}

void Surface::setClearDepth(float d)
{
	for (int i = 0; i < 2; ++i)
		if (m_depthTarget[i] != nullptr)
			m_depthTarget[i]->setClearDepth(d);
}

void Surface::setName(const char * name)
{
	m_name = name;
}

const char * Surface::getName() const
{
	return m_name.c_str();
}

ColorTarget * Surface::getColorTarget()
{
	return m_colorTarget[m_bufferId];
}

DepthTarget * Surface::getDepthTarget()
{
	return m_depthTarget[m_bufferId];
}

GxTextureId Surface::getTexture() const
{
	if (m_colorTarget[m_bufferId] == nullptr)
		return 0;
	else
		return m_colorTarget[m_bufferId]->getTextureId();
}

bool Surface::hasDepthTexture() const
{
	return m_depthTarget[0] != 0;
}

GxTextureId Surface::getDepthTexture() const
{
	if (m_depthTarget[m_bufferId] == nullptr)
		return 0;
	else
		return m_depthTarget[m_bufferId]->getTextureId();
}

int Surface::getWidth() const
{
	return m_properties.dimensions.width;
}

int Surface::getHeight() const
{
	return m_properties.dimensions.height;
}

int Surface::getBackingScale() const
{
	return m_backingScale;
}

SURFACE_FORMAT Surface::getFormat() const
{
	fassert(m_properties.colorTarget.enabled);
	return m_properties.colorTarget.format;
}

static const float s255 = 1.f / 255.f;

inline float scale255(const float v)
{
	return v * s255;
}

void Surface::clear(int r, int g, int b, int a)
{
	clearf(scale255(r), scale255(g), scale255(b), scale255(a));
}

void Surface::clearf(float r, float g, float b, float a)
{
	const Color oldClearColor = getColorTarget()->getClearColor();
	
	getColorTarget()->setClearColor(r, g, b, a);
	pushRenderPass(getColorTarget(), true, nullptr, false, "Surface::clear");
	{
		// target gets cleared during push
	}
	popRenderPass();
	
	getColorTarget()->setClearColor(
		oldClearColor.r,
		oldClearColor.g,
		oldClearColor.b,
		oldClearColor.a);
}

void Surface::clearDepth(float d)
{
	const float oldClearDepth = getDepthTarget()->getClearDepth();
	
	getDepthTarget()->setClearDepth(d);
	pushRenderPass(nullptr, false, getDepthTarget(), true, "Surface::clearDepth");
	{
		// target gets cleared during push
	}
	popRenderPass();
	
	getDepthTarget()->setClearDepth(oldClearDepth);
}

void Surface::clearAlpha()
{
	setAlphaf(0.f);
}

void Surface::setAlpha(int a)
{
	setAlphaf(scale255(a));
}

void Surface::setAlphaf(float a)
{
	pushSurface(this);
	{
		pushBlend(BLEND_OPAQUE);
		pushColorWriteMask(0, 0, 0, 1);
		{
			setColorf(1.f, 1.f, 1.f, a);
			drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
		}
		popColorWriteMask();
		popBlend();
	}
	popSurface();
}

void Surface::mulf(float r, float g, float b, float a)
{
	pushSurface(this);
	{
		pushBlend(BLEND_MUL);
		setColorf(r, g, b, a);
		drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
		popBlend();
	}
	popSurface();
}

void Surface::postprocess()
{
	swapBuffers();

	pushSurface(this);
	pushDepthTest(false, DEPTH_EQUAL, false); // todo : surface push should set depth state, blend mode (anything affecting how to draw to the surface)
	{
		drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
	}
	popDepthTest();
	popSurface();
}

void Surface::postprocess(Shader & shader)
{
	swapBuffers();
	
	pushSurface(this);
	pushDepthTest(false, DEPTH_EQUAL, false);
	{
		setShader(shader);
		{
			drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
		}
		clearShader();
	}
	popDepthTest();
	popSurface();
}

void Surface::invert()
{
	pushSurface(this);
	{
		pushBlend(BLEND_INVERT);
		setColorf(1.f, 1.f, 1.f, 1.f);
		drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
		popBlend();
	}
	popSurface();
}

void Surface::invertColor()
{
	pushColorWriteMask(1, 1, 1, 0);
	{
		invert();
	}
	popColorWriteMask();
}

void Surface::invertAlpha()
{
	pushColorWriteMask(0, 0, 0, 1);
	{
		invert();
	}
	popColorWriteMask();
}

void Surface::gaussianBlur(const float strengthH, const float strengthV, const int _kernelSize)
{
	const int kernelSize = _kernelSize < 0 ? int(ceilf(std::max(strengthH, strengthV))) : _kernelSize;
	
	if (kernelSize == 0)
		return;
	
	if (strengthH > 0.f)
	{
		pushBlend(BLEND_OPAQUE);
		setShader_GaussianBlurH(getTexture(), kernelSize, strengthH);
		postprocess();
		clearShader();
		popBlend();
	}
	
	if (strengthV > 0.f)
	{
		pushBlend(BLEND_OPAQUE);
		setShader_GaussianBlurV(getTexture(), kernelSize, strengthV);
		postprocess();
		clearShader();
		popBlend();
	}
}

void Surface::blitTo(Surface * surface) const
{
#if ENABLE_METAL
	Assert(false);
	// todo : metal_copy_texture_to_texture(..);
#elif ENABLE_OPENGL
	int oldReadBuffer = 0;
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();
	
	// create framebuffer object for the source of the blit operaration
	GLuint srcFramebuffer = 0;
	glGenFramebuffers(1, &srcFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, srcFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTarget[m_bufferId]->getTextureId(), 0);
	checkErrorGL();
	
	// create framebuffer object for the destination of the blit operaration
	GLuint dstFramebuffer = 0;
	glGenFramebuffers(1, &dstFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, surface->m_colorTarget[m_bufferId]->getTextureId(), 0);
	checkErrorGL();
	
	glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFramebuffer);
	checkErrorGL();

	glBlitFramebuffer(
		0, 0, getWidth() * m_backingScale, getHeight() * m_backingScale,
		0, 0, surface->getWidth() * surface->m_backingScale, surface->getHeight() * surface->m_backingScale,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	checkErrorGL();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
	
	glDeleteFramebuffers(1, &srcFramebuffer);
	glDeleteFramebuffers(1, &dstFramebuffer);
	checkErrorGL();
#endif
}

void Surface::blit(BLEND_MODE blendMode) const
{
	pushBlend(blendMode);
	pushColorMode(COLOR_IGNORE);
	{
		gxSetTexture(getTexture());
		drawRect(0, 0, getWidth(), getHeight());
		gxSetTexture(0);
	}
	popColorMode();
	popBlend();
}

void blitBackBufferToSurface(Surface * surface)
{
#if ENABLE_METAL
	Assert(false);
#elif ENABLE_OPENGL
	int drawableSx;
	int drawableSy;
	SDL_GL_GetDrawableSize(globals.currentWindow->getWindow(), &drawableSx, &drawableSy);
	
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();
	
	// create framebuffer object for the destination of the blit operaration
	GLuint dstFramebuffer = 0;
	glGenFramebuffers(1, &dstFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, dstFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, surface->getColorTarget()->getTextureId(), 0);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFramebuffer);
	checkErrorGL();

	glBlitFramebuffer(
		0, 0, drawableSx, drawableSy,
		0, 0, surface->getWidth(), surface->getHeight(),
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
	
	glDeleteFramebuffers(1, &dstFramebuffer);
	checkErrorGL();
#endif
}
