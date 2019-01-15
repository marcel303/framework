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

#include <GL/glew.h> // GL_FRAMEBUFFER. todo : remove in favor of a Framework-provided texture object
#include "framework.h"
#include "textureatlas.h"
#include <string.h>

TextureAtlas::TextureAtlas()
	: a()
	, texture(nullptr)
	, format(GX_UNKNOWN_FORMAT)
	, filter(false)
	, clamp(false)
	, swizzleMask()
{
}

TextureAtlas::~TextureAtlas()
{
	shut();
}

void TextureAtlas::init(const int sx, const int sy, const GX_TEXTURE_FORMAT _format, const bool _filter, const bool _clamp, const GLint * _swizzleMask)
{
	a.init(sx, sy);
	
	format = _format;
	
	filter = _filter;
	clamp = _clamp;
	
	if (_swizzleMask != nullptr)
	{
		memcpy(swizzleMask, _swizzleMask, sizeof(swizzleMask));
	}
	else
	{
		swizzleMask[0] = 0;
		swizzleMask[1] = 1;
		swizzleMask[2] = 2;
		swizzleMask[3] = 3;
	}
	
	if (texture != nullptr)
	{
		delete texture;
		texture = nullptr;
	}
	
	texture = allocateTexture(sx, sy);
}

void TextureAtlas::shut()
{
	init(0, 0, GX_R32_FLOAT, false, false, nullptr);
}

BoxAtlasElem * TextureAtlas::tryAlloc(const uint8_t * values, const int sx, const int sy, const int border)
{
	auto e = a.tryAlloc(sx + border * 2, sy + border * 2);
	
	if (e != nullptr)
	{
		if (values != nullptr)
		{
			texture->uploadArea(values, 1, 0, sx, sy, e->x + border, e->y + border);
		}
		
		return e;
	}
	else
	{
		return nullptr;
	}
}

void TextureAtlas::free(BoxAtlasElem *& e)
{
	if (e != nullptr)
	{
		Assert(e->isAllocated);
		
		texture->clearAreaToZero(e->x, e->y, e->sx, e->sy);
	
		a.free(e);
	}
}

GxTexture * TextureAtlas::allocateTexture(const int sx, const int sy)
{
	GxTexture * newTexture = nullptr;
	
	if (sx > 0 || sy > 0)
	{
		newTexture = new GxTexture();
		newTexture->allocate(sx, sy, format, filter, clamp);
		newTexture->setSwizzle(swizzleMask[0], swizzleMask[1], swizzleMask[2], swizzleMask[3]);
	}
	
	return newTexture;
}

bool TextureAtlas::makeBigger(const int sx, const int sy)
{
	if (sx < a.sx || sy < a.sy)
		return false;
	
	// update texture
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//
	
	GxTexture * newTexture = allocateTexture(sx, sy);
	
	newTexture->clearf(0, 0, 0, 0);
	
// todo : add method to copy an area from one GxTexture to another

	glBindTexture(GL_TEXTURE_2D, newTexture->id);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->id, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, a.sx, a.sy);
	checkErrorGL();
	
	//
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	a.makeBigger(sx, sy);
	
	return true;
}

bool TextureAtlas::optimize()
{
	BoxAtlasElem elems[a.kMaxElems];
	memcpy(elems, a.elems, sizeof(elems));
	
	if (a.optimize() == false)
	{
		return false;
	}
	
	// update texture
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//
	
	GxTexture * newTexture = allocateTexture(a.sx, a.sy);
	
	newTexture->clearf(0, 0, 0, 0);
	
// todo : add method to copy a list of areas from one GxTexture to another

	glBindTexture(GL_TEXTURE_2D, newTexture->id);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->id, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkErrorGL();
	
	for (int i = 0; i < a.kMaxElems; ++i)
	{
		auto & eSrc = elems[i];
		auto & eDst = a.elems[i];
		
		Assert(eSrc.isAllocated == eDst.isAllocated);
		
		if (eSrc.isAllocated)
		{
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, eDst.x, eDst.y, eSrc.x, eSrc.y, eSrc.sx, eSrc.sy);
			checkErrorGL();
		}
	}
	
	//
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	checkErrorGL();
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	return true;
}

bool TextureAtlas::makeBiggerAndOptimize(const int sx, const int sy)
{
	BoxAtlasElem elems[a.kMaxElems];
	memcpy(elems, a.elems, sizeof(elems));
	
	if (a.makeBigger(sx, sy) == false)
	{
		return false;
	}
	
	if (a.optimize() == false)
	{
		return false;
	}
	
	// update texture
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	//
	
	GxTexture * newTexture = allocateTexture(sx, sy);
	
	newTexture->clearf(0, 0, 0, 0);
	
	glBindTexture(GL_TEXTURE_2D, newTexture->id);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->id, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	checkErrorGL();
	
	for (int i = 0; i < a.kMaxElems; ++i)
	{
		auto & eSrc = elems[i];
		auto & eDst = a.elems[i];
		
		Assert(eSrc.isAllocated == eDst.isAllocated);
		
		if (eSrc.isAllocated)
		{
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, eDst.x, eDst.y, eSrc.x, eSrc.y, eSrc.sx, eSrc.sy);
			checkErrorGL();
		}
	}
	
	//
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	checkErrorGL();
	
	delete texture;
	texture = nullptr;
	
	//
	
	texture = newTexture;
	
	return true;
}
