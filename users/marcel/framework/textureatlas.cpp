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

#include "textureatlas.h"

TextureAtlas::TextureAtlas()
	: a()
	, texture(0)
	, internalFormat(GL_R32F)
	, filter(false)
	, clamp(false)
	, swizzleMask()
{
}

TextureAtlas::~TextureAtlas()
{
	shut();
}

void TextureAtlas::init(const int sx, const int sy, const GLenum _internalFormat, const bool _filter, const bool _clamp, const GLint * _swizzleMask)
{
	a.init(sx, sy);
	
	internalFormat = _internalFormat;
	
	filter = _filter;
	clamp = _clamp;
	
	if (_swizzleMask != nullptr)
	{
		memcpy(swizzleMask, _swizzleMask, sizeof(swizzleMask));
	}
	else
	{
		swizzleMask[0] = GL_RED;
		swizzleMask[1] = GL_GREEN;
		swizzleMask[2] = GL_BLUE;
		swizzleMask[3] = GL_ALPHA;
	}
	
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
		checkErrorGL();
	}
	
	texture = allocateTexture(sx, sy);
}

void TextureAtlas::shut()
{
	init(0, 0, GL_R32F, false, false, nullptr);
}

BoxAtlasElem * TextureAtlas::tryAlloc(const uint8_t * values, const int sx, const int sy, const GLenum uploadFormat, const GLenum uploadType, const int border)
{
	auto e = a.tryAlloc(sx + border * 2, sy + border * 2);
	
	if (e != nullptr)
	{
		if (values != nullptr)
		{
			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			checkErrorGL();
			
			glBindTexture(GL_TEXTURE_2D, texture);
			checkErrorGL();
			
			GLint restoreUnpack;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
			checkErrorGL();
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexSubImage2D(GL_TEXTURE_2D, 0, e->x + border, e->y + border, sx, sy, uploadFormat, uploadType, values);
			checkErrorGL();
			
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
			checkErrorGL();
			glBindTexture(GL_TEXTURE_2D, restoreTexture);
			checkErrorGL();
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
		
		uint8_t * zeroes = (uint8_t*)alloca(e->sx * e->sy);
		memset(zeroes, 0, e->sx * e->sy);
		
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
		
		GLint restoreUnpack;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
		checkErrorGL();
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexSubImage2D(GL_TEXTURE_2D, 0, e->x, e->y, e->sx, e->sy, GL_RED, GL_UNSIGNED_BYTE, zeroes);
		checkErrorGL();
		
		glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
		checkErrorGL();
		
		a.free(e);
	}
}

GLuint TextureAtlas::allocateTexture(const int sx, const int sy)
{
	GLuint newTexture = 0;
	
	if (sx > 0 || sy > 0)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glGenTextures(1, &newTexture);
		glBindTexture(GL_TEXTURE_2D, newTexture);
		glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, sx, sy);
		checkErrorGL();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();
		
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		checkErrorGL();
	
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
	}
	
	return newTexture;
}

void TextureAtlas::clearTexture(GLuint texture, float r, float g, float b, float a)
{
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	{
		GLuint frameBuffer = 0;
		
		glGenFramebuffers(1, &frameBuffer);
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
		checkErrorGL();
		{
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}
		glDeleteFramebuffers(1, &frameBuffer);
		frameBuffer = 0;
		checkErrorGL();
	}
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
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
	
	GLuint newTexture = allocateTexture(sx, sy);
	
	glBindTexture(GL_TEXTURE_2D, newTexture);
	checkErrorGL();
	
	clearTexture(newTexture, 0, 0, 0, 0);
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
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
	
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
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
	
	GLuint newTexture = allocateTexture(a.sx, a.sy);
	
	glBindTexture(GL_TEXTURE_2D, newTexture);
	checkErrorGL();
	
	clearTexture(newTexture, 0, 0, 0, 0);
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
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
	
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
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
	
	GLuint newTexture = allocateTexture(sx, sy);
	
	glBindTexture(GL_TEXTURE_2D, newTexture);
	checkErrorGL();
	
	clearTexture(newTexture, 0, 0, 0, 0);
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
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
	
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
	//
	
	texture = newTexture;
	
	return true;
}
