#include "Debugging.h"
#include "FileStream.h"
#include "ImageLoader_Photoshop.h"
#include "PsdChannel.h"
#include "PsdCompression.h"
#include "PsdForward.h"
#include "PsdImageData.h"
#include "PsdInfo.h"
#include "PsdLayer.h"
#include "PsdLog.h"
#include "PsdRect.h"
#include "PsdResource.h"
#include "PsdResourceList.h"
#include "PsdResource_ChannelNames.h"
#include "PsdResource_DisplayInfo.h"
#include "PsdResource_ResolutionInfo.h"
#include "PsdTypes.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"

void ImageLoader_Photoshop::Load(Image& image, const std::string& fileName)
{
	FileStream stream;

	stream.Open(fileName.c_str(), OpenMode_Read);

	PsdInfo psdInfo;

	psdInfo.Read(&stream);
}

void ImageLoader_Photoshop::Save(const Image& image, const std::string& fileName)
{
}

void ImageLoader_Photoshop::SaveMultiLayer(Image** images, int imageCount, const std::string& fileName)
{
	if (imageCount < 1)
		throw ExceptionVA("image count must be > 0");

	Image merged;

	const Image* image0 = images[0];

	merged.SetSize(image0->m_Sx, image0->m_Sy);

	for (int i = 0; i < imageCount; ++i)
	{
		Image* image = images[i];

		for (int y = 0; y < merged.m_Sy; ++y)
		{
			const ImagePixel* srcLine = image->GetLine(y);
			ImagePixel* dstLine = merged.GetLine(y);

			for (int x = 0; x < merged.m_Sx; ++x)
			{
				for (int j = 0; j < 4; ++j)
				{
					dstLine[x].rgba[j] =
						(dstLine[x].rgba[j] * (255 - srcLine[x].rgba[3]) >> 8) +
						srcLine[x].rgba[j];
				}
			}
		}
	}

	FileStream stream;

	stream.Open(fileName.c_str(), OpenMode_Write);

	// build PSD file
	
	PsdInfo psdInfo;
	
	psdInfo.mHeaderInfo.Setup("8BPS", 1, 4, image0->m_Sx, image0->m_Sy, 8, PsdImageMode_Rgb);
	
	psdInfo.mImageData.Setup(&merged, PsdCompressionType_Rle);
	
	psdInfo.mLayerAndMaskInfo.Setup(false);
	
#if 1
	// add resolution info resource
	PsdResolutionInfo* resolutionInfo = new PsdResolutionInfo();
	resolutionInfo->Setup("", 72, 1, PsdResolutionUnit_Inch, 72, 1, PsdResolutionUnit_Inch);
	psdInfo.mImageResourceList.mResourceList.push_back(resolutionInfo);
#endif
	
#if 1
	// add display info resource
	PsdDisplayInfo* displayInfo = new PsdDisplayInfo();
	displayInfo->Setup("", PsdColorSpace_Rgb, 255, 255, 255, 255, 100, false);
	psdInfo.mImageResourceList.mResourceList.push_back(displayInfo);
#endif

#if 0
	// add channel names resource
	PsdResource_ChannelNames* channelNames = new PsdResource_ChannelNames();
	channelNames->Setup("");
	channelNames->Add("Alpha");
	channelNames->Add("R");
	channelNames->Add("G");
	channelNames->Add("B");
	psdInfo.mImageResourceList.mResourceList.push_back(channelNames);
#endif
	
	for (int j = 0; j < imageCount; ++j)
	{
		const Image* image = images[j];

		PsdLayer* layer = new PsdLayer();
		
		layer->Setup(String::Format("Layer %d", j), PsdRect(0, 0, image->m_Sx, image->m_Sy), "8BIM", "norm", 255, 0, 0);
		for (int i = 0; i < 4; ++i)
		{
			PsdChannel* channel = new PsdChannel();
			channel->Setup(*image, i - 1);
			layer->mChannelList.push_back(channel);
		}
		
		psdInfo.mLayerAndMaskInfo.mLayerList.push_back(layer);
	}
	
	psdInfo.Write(&stream);
	
	stream.Close();
}
