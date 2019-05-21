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

#include "framework.h"

#if ENABLE_METAL

#include "gx_render.h"
#include "internal.h"

#define TODO 0

extern int s_backingScale; // todo : can this be exposed/determined more nicely?

void Surface::construct()
{
	m_backingScale = 1;
	
	m_bufferId = 0;
	
	m_colorTarget[0] = nullptr;
	m_colorTarget[1] = nullptr;
	m_depthTarget[0] = nullptr;
	m_depthTarget[1] = nullptr;
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

Surface::Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer, bool doubleBuffered)
{
	construct();
	
	init(sx, sy, highPrecision ? SURFACE_RGBA16F : SURFACE_RGBA8, withDepthBuffer, doubleBuffered);
}

Surface::Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format)
{
	construct();

	init(sx, sy, format, withDepthBuffer, doubleBuffered);
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

static MTLPixelFormat translateSurfaceColorFormat(const SURFACE_FORMAT format)
{
	MTLPixelFormat metalFormat = MTLPixelFormatInvalid;

	if (format == SURFACE_RGBA8)
		metalFormat = MTLPixelFormatRGBA8Unorm;
	if (format == SURFACE_RGBA16F)
		metalFormat = MTLPixelFormatRGBA16Float;
	if (format == SURFACE_RGBA32F)
		metalFormat = MTLPixelFormatRGBA32Float;
	if (format == SURFACE_R8)
		metalFormat = MTLPixelFormatR8Unorm;
	if (format == SURFACE_R16F)
		metalFormat = MTLPixelFormatR16Float;
	if (format == SURFACE_R32F)
		metalFormat = MTLPixelFormatR32Float;
	if (format == SURFACE_RG16F)
		metalFormat = MTLPixelFormatRG16Float;
	if (format == SURFACE_RG32F)
		metalFormat = MTLPixelFormatRG32Float;
	
	return metalFormat;
}

static MTLPixelFormat translateSurfaceDepthFormat(const DEPTH_FORMAT format)
{
	MTLPixelFormat metalFormat = MTLPixelFormatInvalid;

	if (format == DEPTH_FLOAT16)
		metalFormat = MTLPixelFormatDepth16Unorm;
	if (format == DEPTH_FLOAT32)
		metalFormat = MTLPixelFormatDepth32Float;
	
	return metalFormat;
}

bool Surface::init(const SurfaceProperties & properties)
{
	fassert(m_colorTarget[0] == nullptr);
	
	m_properties = properties;
	
	// todo : honor if colorTarget enabled is false
	// todo : honor depthTarget doubleBuffered flag
	
	const int sx = properties.dimensions.width  / framework.minification;
	const int sy = properties.dimensions.height / framework.minification;
	
	//
	
	bool result = true;
	
	m_backingScale = s_backingScale;
	
	const int backingSx = sx * m_backingScale;
	const int backingSy = sy * m_backingScale;
	
	// allocate color target backing storage

	if (properties.colorTarget.enabled)
	{
		ColorTargetProperties targetProperties;
		targetProperties.dimensions.width = properties.dimensions.width;
		targetProperties.dimensions.height = properties.dimensions.height;
		targetProperties.format = properties.colorTarget.format;
		
		for (int i = 0; result && i < (properties.colorTarget.doubleBuffered ? 2 : 1); ++i)
		{
			// allocate storage
			
			m_colorTarget[i] = new ColorTarget();
			result &= m_colorTarget[i]->init(targetProperties);
			
		#if TODO
			// set filtering
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			checkErrorGL();
		#endif
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
		targetProperties.dimensions.width = properties.dimensions.width;
		targetProperties.dimensions.height = properties.dimensions.height;
		targetProperties.format = properties.depthTarget.format;
		
		for (int i = 0; result && i < (properties.depthTarget.doubleBuffered ? 2 : 1); ++i)
		{
			m_depthTarget[i] = new DepthTarget();
			result &= m_depthTarget[i]->init(targetProperties);
			
			if (result == false)
				continue;

		#if TODO
			// set filtering

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			checkErrorGL();
		#endif
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

bool Surface::init(int in_sx, int in_sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered)
{
	SurfaceProperties properties;
	
	properties.dimensions.width = in_sx;
	properties.dimensions.height = in_sy;
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

void Surface::setSwizzle(int r, int g, int b, int a)
{
#if TODO
	fassert(m_properties.colorTarget.enabled);
	if (m_properties.colorTarget.enabled == false)
		return;
	
	// capture previous OpenGL state
	
	GLuint oldTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oldTexture);
	checkErrorGL();
	
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
		glBindTexture(GL_TEXTURE_2D, m_colorTexture[i]);
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		checkErrorGL();
	}
	
	// restore the previous OpenGL state
	
	glBindTexture(GL_TEXTURE_2D, oldTexture);
	checkErrorGL();
#endif
}

ColorTarget * Surface::getColorTarget()
{
	return m_colorTarget[m_bufferId];
}

DepthTarget * Surface::getDepthTarget()
{
	return m_depthTarget[m_bufferId];
}

uint32_t Surface::getFramebuffer() const
{
	return 0;
}

GxTextureId Surface::getTexture() const
{
	return m_colorTarget[m_bufferId]->getTextureId();
}

bool Surface::hasDepthTexture() const
{
	return m_depthTarget[0] != 0;
}

GxTextureId Surface::getDepthTexture() const
{
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
	getColorTarget()->setClearColor(r, g, b, a);
	pushRenderPass(getColorTarget(), nullptr, false, "Surface::clear");
	{
	}
	popRenderPass();
}

void Surface::clearDepth(float d)
{
	getDepthTarget()->setClearDepth(d);
	pushRenderPass(nullptr, getDepthTarget(), true, "Surface::clearDepth");
	{
	}
	popRenderPass();
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
		setColorf(1.f, 1.f, 1.f, a);
	#if TODO
		glColorMask(0, 0, 0, 1);
		{
			drawRect(0.f, 0.f, m_properties.dimensions.width, m_properties.dimensions.height);
		}
		glColorMask(1, 1, 1, 1);
	#endif
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
#if TODO
	glColorMask(1, 1, 1, 0);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
#endif
}

void Surface::invertAlpha()
{
#if TODO
	glColorMask(0, 0, 0, 1);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
#endif
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
#if TODO
	int oldReadBuffer = 0;
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebuffer());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surface->getFramebuffer());
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
#if TODO
	int drawableSx;
	int drawableSy;
	SDL_GL_GetDrawableSize(globals.currentWindow->getWindow(), &drawableSx, &drawableSy);
	
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surface->getFramebuffer());
	checkErrorGL();

	glBlitFramebuffer(
		0, 0, drawableSx, drawableSy,
		0, 0, surface->getWidth(), surface->getHeight(),
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
#endif
}

#endif
