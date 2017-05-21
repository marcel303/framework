#include "textureatlas.h"

TextureAtlas::TextureAtlas()
	: a()
	, texture(0)
{
}

TextureAtlas::~TextureAtlas()
{
	init(0, 0);
}

void TextureAtlas::init(const int sx, const int sy)
{
	a.init(sx, sy);
	
	if (texture != 0)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
		checkErrorGL();
	}
	
	if (sx > 0 || sy > 0)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, sx, sy);
		checkErrorGL();
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
	}
}

BoxAtlasElem * TextureAtlas::tryAlloc(const uint8_t * values, const int sx, const int sy)
{
	auto e = a.tryAlloc(sx, sy);
	
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
			glTexSubImage2D(GL_TEXTURE_2D, 0, e->x, e->y, sx, sy, GL_RED, GL_UNSIGNED_BYTE, values);
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

void TextureAtlas::clearTexture(GLuint texture, float r, float g, float b, float a)
{
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_FRAMEBUFFER, (GLint*)&oldBuffer);
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
	glGetIntegerv(GL_FRAMEBUFFER, (GLint*)&oldBuffer);
	checkErrorGL();
	
	//
	
	GLuint newTexture = 0;
	
	glGenTextures(1, &newTexture);
	glBindTexture(GL_TEXTURE_2D, newTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, sx, sy);
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
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
	//
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
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
	glGetIntegerv(GL_FRAMEBUFFER, (GLint*)&oldBuffer);
	checkErrorGL();
	
	//
	
	GLuint newTexture = 0;
	
	glGenTextures(1, &newTexture);
	glBindTexture(GL_TEXTURE_2D, newTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, a.sx, a.sy);
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
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	checkErrorGL();
	
	glDeleteTextures(1, &texture);
	texture = 0;
	checkErrorGL();
	
	//
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	texture = newTexture;
	
	return true;
}
