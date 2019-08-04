#pragma once

#include <vector>
#include <string>
#include "libgg_forward.h"
#include "PsdForward.h"
#include "PsdRect.h"

class PsdLayer
{
public:
	PsdLayer();
	~PsdLayer();
	void Setup(const std::string& name, PsdRect rect, const std::string& signature, const std::string& blendKey, uint8_t opacity, uint8_t clipping, uint8_t flags);
	
	void ReadHeader(PsdInfo* pi, Stream* stream);
	void ReadData(PsdInfo* pi, Stream* stream);
	void WritePrepare();
	void WriteHeader(PsdInfo* pi, Stream* stream);
	void WriteData(PsdInfo* pi, Stream* stream);
	
	std::string mName;
	PsdRect mRect;
	uint16_t mChannelCount;
	char mSignature[4];
	char mBlendKey[4];
	uint8_t mOpacity;
	uint8_t mClipping;
	uint8_t mFlags;
	std::vector<PsdChannel*> mChannelList;
	bool mWritePrepared;
};
