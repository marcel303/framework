#include "PsdLog.h"
#include "PsdResource_ResolutionInfo.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdResolutionInfo::PsdResolutionInfo()
{
	mXppi = 0;
	mXppiUnit = 0;
	mSxUnit = 0;
	mYppi = 0;
	mYppiUnit = 0;
	mSyUnit = 0;
}

PsdResolutionInfo::~PsdResolutionInfo()
{
}

void PsdResolutionInfo::Setup(const std::string& name, int xppi, int xppiUnit, PsdResolutionUnit sxUnit, int yppi, int yppiUnit, PsdResolutionUnit syUnit)
{
	PsdImageResource::Setup("8BIM", PsdResourceId_ResolutionInfo, name);

	mXppi = xppi;
	mXppiUnit = xppiUnit;
	mSxUnit = sxUnit;
	mYppi = yppi;
	mYppiUnit = yppiUnit;
	mSyUnit = syUnit;
}

void PsdResolutionInfo::ReadResource(PsdInfo* pi, Stream* stream)
{
	StreamReader reader(stream, false);

	const uint16_t xppi = SwapU16(reader.ReadUInt16());
	const uint32_t xppiUnit = SwapU32(reader.ReadUInt32());
	const PsdResolutionUnit sxUnit = (PsdResolutionUnit)SwapU16(reader.ReadUInt16());
	const uint16_t yppi = SwapU16(reader.ReadUInt16());
	const uint32_t yppiUnit = SwapU32(reader.ReadUInt32());
	const PsdResolutionUnit syUnit = (PsdResolutionUnit)SwapU16(reader.ReadUInt16());
	
	Setup("", xppi, xppiUnit, sxUnit, yppi, yppiUnit, syUnit);

	PSD_LOG_DBG("resolution_info: read: %d/%lu/%d, %d/%lu/%d",
		(int)mXppi, mXppiUnit, (int)mSxUnit,
		(int)mYppi, mYppiUnit, (int)mSyUnit);
}

void PsdResolutionInfo::WriteResource(PsdInfo* pi, Stream* stream)
{
	StreamWriter writer(stream, false);

	writer.WriteUInt16(SwapU16(mXppi));
	writer.WriteUInt32(SwapU32(mXppiUnit));
	writer.WriteUInt16(SwapU16(mSxUnit));
	writer.WriteUInt16(SwapU16(mYppi));
	writer.WriteUInt32(SwapU32(mYppiUnit));
	writer.WriteUInt16(SwapU16(mSyUnit));

	PSD_LOG_DBG("resolution_info: write: %d/%d/%d, %d/%d/%d",
		(int)mXppi, (int)mXppiUnit, (int)mSxUnit,
		(int)mYppi, (int)mYppiUnit, (int)mSyUnit);
}
