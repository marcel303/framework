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

#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include "framework.h"
#include "jpegLoader.h"
#include "MemAlloc.h"
#include "openglImageCapture.h"
#include "StringEx.h"
#include <limits.h>

OpenglImageCapture::OpenglImageCapture()
	: textures()
	, saveIndex(0)
{
}

OpenglImageCapture::~OpenglImageCapture()
{
	flushRecordedTextures();
}

void OpenglImageCapture::recordFramebuffer(const int sx, const int sy)
{
// todo : add method to gx-metal to read-back the frame buffer
//        preferably async ..

	GxTexture * texture = new GxTexture();
	texture->allocate(sx, sy, GX_RGBA8_UNORM);
	
	if (texture->id != 0)
	{
		// capture current OpenGL states before we change them

		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		checkErrorGL();
		
		// capture framebuffer contents
		
		glBindTexture(GL_TEXTURE_2D, texture->id);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sx, sy);
		checkErrorGL();
		
		// restore previous OpenGL states

		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		checkErrorGL();
		
		textures.push_back(texture);
	}
}

void OpenglImageCapture::flushRecordedTextures()
{
	for (auto *& texture : textures)
	{
		texture->free();
		
		delete texture;
		texture = nullptr;
	}
	
	textures.clear();
}

void OpenglImageCapture::saveImageSequence(const char * filenameFormat, const bool flush)
{
	// capture current OpenGL states before we change them

	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	for (auto * texture : textures)
	{
		const int sx = texture->sx;
		const int sy = texture->sy;
		
		if (texture->sx > 0 && texture->sy > 0)
		{
			void * pixels = MemAlloc(sx * sy * 4, 256);
			
			glBindTexture(GL_TEXTURE_2D, texture->id);
			checkErrorGL();
			
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			checkErrorGL();
			
		#ifdef WIN32
			char filename[_MAX_PATH];
		#else
			char filename[PATH_MAX];
		#endif
			sprintf_s(filename, sizeof(filename), filenameFormat, saveIndex);
			
			if (saveImage_turbojpeg(filename, pixels, sx * sy * 4, sx, sy, true, 100, true))
			{
				saveIndex++;
			}
			
			MemFree(pixels);
		}
	}
	
	if (flush)
	{
		flushRecordedTextures();
	}
	
	// restore previous OpenGL states

	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
}
