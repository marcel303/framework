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

#include <SDL2/SDL_opengl.h>
#include "framework.h"
#include "jpegLoader.h"
#include "MemAlloc.h"
#include "openglImageCapture.h"
#include "openglTexture.h"
#include "StringEx.h"
#include <limits.h>

OpenglImageCapture::OpenglImageCapture()
	: textures()
	, saveIndex(0)
{
}

OpenglImageCapture::~OpenglImageCapture()
{
	if (!textures.empty())
	{
		glDeleteTextures(textures.size(), &textures[0]);
		checkErrorGL();
	}
}

void OpenglImageCapture::recordFramebuffer(const int sx, const int sy)
{
	GLuint texture = 0;
	glGenTextures(1, &texture);
	checkErrorGL();
	
	if (texture != 0)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		{
			glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 0, 0, sx, sy, 0);
			checkErrorGL();
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		
		textures.push_back(texture);
	}
}

void OpenglImageCapture::saveImageSequence(const char * filenameFormat, const bool flush)
{
	for (auto texture : textures)
	{
		GLint sx = 0;
		GLint sy = 0;
		
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
		{
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
			checkErrorGL();
			
			if (sx > 0 && sy > 0)
			{
				void * pixels = MemAlloc(sx * sy * 4, 256);
				
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
		glBindTexture(GL_TEXTURE_2D, 0);
		checkErrorGL();
	}
	
	if (flush && !textures.empty())
	{
		glDeleteTextures(textures.size(), &textures[0]);
		textures.clear();
	}
}
