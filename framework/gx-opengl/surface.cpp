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

#include "gx_render.h"
#include "internal.h"

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

void Surface::setSwizzle(int r, int g, int b, int a)
{
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
	AssertMsg(false, "not implemented. GL_TEXTURE_SWIZZLE_RGBA is not available in non-desktop OpenGL", 0);
#endif

	// restore the previous OpenGL state
	
	glBindTexture(GL_TEXTURE_2D, oldTexture);
	checkErrorGL();
}

void Surface::blitTo(Surface * surface) const
{
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
}

void blitBackBufferToSurface(Surface * surface)
{
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
}

#endif
