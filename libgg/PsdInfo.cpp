#include "PsdInfo.h"
#include "PsdLog.h"
#include "Stream.h"

void PsdInfo::Read(Stream* stream)
{
	mHeaderInfo.Read(this, stream);
	mColorModeData.Read(this, stream);
	mImageResourceList.Read(this, stream);
	mLayerAndMaskInfo.Read(this, stream);
	mImageData.Read(this, stream);
	
	PSD_LOG_DBG("read %d/%d", stream->Position_get(), stream->Length_get());
}

void PsdInfo::Write(Stream* stream)
{
	mLayerAndMaskInfo.WritePrepare();
	
	mHeaderInfo.Write(this, stream);
	mColorModeData.Write(this, stream);
	mImageResourceList.Write(this, stream);
	mLayerAndMaskInfo.Write(this, stream);
	mImageData.Write(this, stream);

	PSD_LOG_DBG("write %d/%d", stream->Position_get(), stream->Length_get());
}
