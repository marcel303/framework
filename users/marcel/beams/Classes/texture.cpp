#include "FileStream.h"
#include "ImageLoader_Tga.h"
#include "paths.h"
#include "StreamReader.h"
#include "texture.h"
#include "TextureRGBA.h"

TextureRGBA* LoadTexture_TGA(const char* fileName)
{
	FileStream stream;
	
	stream.Open(GetResourcePath(fileName).c_str(), OpenMode_Read);
	
	StreamReader reader(&stream, false);
	
	TgaHeader header;
	
	header.Load(&stream);
	
	TgaLoader loader;
	
	uint8_t* bytes;
	
	loader.LoadData(&stream, header, &bytes);
	
	return new TextureRGBA(header.sx, header.sy, bytes, true);
}

void ToTexture(TextureRGBA* texture, GLuint& textureId)
{
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->Sx_get(), texture->Sy_get(), 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->Bytes_get());
	GL_CHECKERROR();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	GL_CHECKERROR();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECKERROR();
}
