#include "Debugging.h"
#include "PsdLog.h"
#include "PsdResource_DisplayInfo.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdDisplayInfo::PsdDisplayInfo()
{
	mColorSpace = PsdColorSpace_Gray;
	mColor[0] = mColor[1] = mColor[2] = mColor[3] = 0;
	mOpacity = 0;
	mKind = false;
}

PsdDisplayInfo::~PsdDisplayInfo()
{
}

void PsdDisplayInfo::Setup(const std::string& name, PsdColorSpace colorSpace, int r, int g, int b, int a, int opacity, bool kind)
{
	Assert(colorSpace == PsdColorSpace_Rgb);
	Assert(r >= 0 && r <= 65535);
	Assert(g >= 0 && g <= 65535);
	Assert(b >= 0 && b <= 65535);
	Assert(a >= 0 && a <= 65535);
	Assert(opacity >= 0 && opacity <= 100);
	
	PsdImageResource::Setup("8BIM", PsdResourceId_DisplayInfo, name);
	
	mColorSpace = colorSpace;
	mColor[0] = r;
	mColor[1] = g;
	mColor[2] = b;
	mColor[3] = a;
	mOpacity = opacity;
	mKind = kind;
}

void PsdDisplayInfo::ReadResource(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	const PsdColorSpace colorSpace = (PsdColorSpace)SwapU16(reader.ReadUInt16());
	uint16_t color[4];
	for (int i = 0; i < 4; ++i)
		color[i] = SwapU16(reader.ReadUInt16());
	const uint16_t opacity = SwapU16(reader.ReadUInt16());
	bool kind = reader.ReadUInt8() != 0x00;
	const uint8_t padding = reader.ReadUInt8();
	(void)padding;

	Setup("", colorSpace, color[0], color[1], color[2], color[3], opacity, kind);
	
	PSD_LOG_DBG("display_info: read: %d, %d, %d, %d",
		(int)mColorSpace,
		(int)mOpacity,
		(int)mKind,
		(int)padding);
}

void PsdDisplayInfo::WriteResource(PsdInfo* pi, Stream* stream)
{
	StreamWriter writer(stream, false);

	const uint8_t padding = 0;
	
	writer.WriteUInt16(SwapU16(mColorSpace));
	for (int i = 0; i < 4; ++i)
		writer.WriteUInt16(SwapU16(mColor[i]));
	writer.WriteUInt16(SwapU16(mOpacity));
	writer.WriteUInt8(mKind ? 0x01 : 0x00);
	writer.WriteUInt8(padding);

	PSD_LOG_DBG("display_info: write: %d, %d, %d, %d",
		(int)mColorSpace,
		(int)mOpacity,
		(int)mKind,
		(int)padding);
}
