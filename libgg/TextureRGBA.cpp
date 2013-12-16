#include "Exception.h"
#include "TextureRGBA.h"

TextureRGBA::TextureRGBA(int sx, int sy, uint8_t* bytes, bool isOwner)
{
	m_Sx = sx;
	m_Sy = sy;
	m_Bytes = bytes;
	m_IsOwner = isOwner;
}

TextureRGBA::~TextureRGBA()
{
	if (m_IsOwner)
	{
		delete[] m_Bytes;
		m_Bytes = 0;
	}
}

int TextureRGBA::Sx_get() const
{
	return m_Sx;
}

int TextureRGBA::Sy_get() const
{
	return m_Sy;
}

uint8_t* TextureRGBA::Bytes_get()
{
	return m_Bytes;
}

const uint8_t* TextureRGBA::Bytes_get() const
{
	return m_Bytes;
}

void TextureRGBA::DisownBytes()
{
	if (m_IsOwner == false)
		throw ExceptionVA("unable to disown bytes. i am not the owner");

	m_IsOwner = false;
}
