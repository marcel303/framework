#pragma once

#include "OpenGLCompat.h"
#include "libgg_forward.h"

TextureRGBA* LoadTexture_TGA(const char* fileName);
void ToTexture(TextureRGBA* texture, GLuint& textureId);
