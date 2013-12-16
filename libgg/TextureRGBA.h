#pragma once

#include <stdint.h>

class TextureRGBA
{
public:
	TextureRGBA(int sx, int sy, uint8_t* bytes, bool isOwner);
	~TextureRGBA();
	
	int Sx_get() const;
	int Sy_get() const;
	
	uint8_t* Bytes_get();
	const uint8_t* Bytes_get() const;

	void DisownBytes();
	
private:
	int m_Sx;
	int m_Sy;
	uint8_t* m_Bytes;
	bool m_IsOwner;
};
