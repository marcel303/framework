#pragma once

#include "OpenGLCompat.h"

class TextureDesc
{
public:
	TextureDesc()
	{
		textureId = 0;
	}
	
	GLuint textureId;
	float s[2];
};
