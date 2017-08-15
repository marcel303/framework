#include <string.h>
#include "Debugging.h"
#include "Image.h"
#include "PsdChannel.h"
#include "PsdCompression.h"
#include "PsdLog.h"
#include "PsdTypes.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PsdChannel::PsdChannel()
{
	mUsage = 0;
	mBytes = 0;
	mSx = 0;
	mSy = 0;
}

PsdChannel::~PsdChannel()
{
	delete[] mBytes;
	mBytes = 0;
}

void PsdChannel::ReadHeader(StreamReader& reader)
{
	mUsage = SwapU16(reader.ReadUInt16());
	mLength = SwapU32(reader.ReadUInt32());

	Assert(mLength > 0);

	PSD_LOG_DBG("channel: read: usage=%d, length=%d",
		(int)mUsage,
		(int)mLength);
}

void PsdChannel::ReadChannelHeader(Stream* stream, uint16_t compression, int sy)
{
	switch (compression)
	{
		case PsdCompressionType_Raw:
			break;
		case PsdCompressionType_Rle:
		{
			StreamReader reader(stream, false);
			
			mRleHeader.resize(sy);
			
			for (int y = 0; y < sy; ++y)
			{
				const uint16_t size = SwapU16(reader.ReadUInt16());

				Assert(size > 0);
				
				mRleHeader[y] = size;
				
				//PSD_LOG_DBG("read: RLE size: %d", (int)size);
			}
			break;
		}
		default:
			throw ExceptionNA();
	}
}

void PsdChannel::ReadChannelData(Stream* stream, uint16_t compression, PsdImageMode mode, int sx, int sy)
{
	Assert(sx >= 0);
	Assert(sy >= 0);

	StreamReader reader(stream, false);

	mSx = sx;
	mSy = sy;
	
	const int area = mSx * mSy;
	const int byteCount = area;	
	
	mBytes = new uint8_t[byteCount];
	memset(mBytes, 0x00, byteCount);

	switch (compression)
	{
		case PsdCompressionType_Raw:
		{
			PSD_LOG_DBG("image data: read: raw");

			switch (mode)
			{
				case PsdImageMode_Rgb:
				{
					for (int i = 0; i < area; ++i)
					{
						uint8_t v = reader.ReadUInt8();

						mBytes[i] = v;
					}
				}
				break;
				default:
					throw ExceptionVA("unknown compression/mode: %d/%d", compression, mode);
			}
			
			break;
		}

		case PsdCompressionType_Rle:
		{
			PSD_LOG_DBG("image data: read: rle");

			int i = 0;
			int packetCount = 0;

			for (int y = 0; y < mSy; ++y)
			{
				const int begin = stream->Position_get();
				
				for (int x = 0; x < mSx;)
				{
					const uint8_t header = reader.ReadUInt8();

					if (header & 0x80)
					{
						// rle

						const int size = 257 - header;

						Assert(size > 0);

						const uint8_t v = reader.ReadUInt8();

						for (int j = 0; j < size; ++j)
						{
							mBytes[i] = v;

							i++;
							x++;
						}
					}
					else
					{
						// raw

						const int size = header + 1;

						Assert(size > 0);

						for (int j = 0; j < size; ++j)
						{
							mBytes[i] = reader.ReadUInt8();

							i++;
							x++;
						}
					}

					packetCount++;
				}
				
				const int end = stream->Position_get();
				
				const int size = end - begin;
				
				Assert(size > 0);
				(void)size;

				PSD_LOG_DBG("RLE size read: %d/%d", size, (int)mRleHeader[y]);
			}

			PSD_LOG_DBG("packet count: %d", packetCount);

			Assert(i == area);
			
			break;
		}
			
		default:
			throw ExceptionVA("unknown compression: %d", compression);
	}
}

void PsdChannel::WritePrepare(uint16_t compression, uint16_t mode, bool merged)
{
	EncodeBytes(compression, mode);

	//

	if (!merged)
	{
		StreamWriter writer(&mChannelData, false);
		writer.WriteUInt16(SwapU16(compression));
		WriteChannelHeader(&mChannelData, compression);
	}
	
	WriteChannelData(&mChannelData);

	//

	mLength = mChannelData.Length_get();

	Assert(mLength > 0);
}

void PsdChannel::WriteChannelHeader(Stream* stream, uint16_t compression)
{
	StreamWriter writer(stream, false);

	Assert(
		compression == PsdCompressionType_Raw ||
		compression == PsdCompressionType_Rle);

	switch (compression)
	{
		case PsdCompressionType_Raw:
			break;
		case PsdCompressionType_Rle:
		{
			Assert((int)mRleHeader.size() == mSy);

			for (size_t i = 0; i < mRleHeader.size(); ++i)
			{
				const uint16_t size = mRleHeader[i];
				
				//PSD_LOG_DBG("write: RLE size: %d", (int)size);
				
				writer.WriteUInt16(SwapU16(size));
			}
			break;
		}
		default:
			throw ExceptionNA();
	}
}

void PsdChannel::WriteChannelData(Stream* stream)
{
	mRleData.Seek(0, SeekMode_Begin);

	mRleData.StreamTo(stream, mRleData.Length_get());
}

void PsdChannel::WriteHeader(Stream* stream)
{
	Assert(mLength > 0);
	
	StreamWriter writer(stream, false);

	writer.WriteUInt16(SwapU16(mUsage));
	writer.WriteUInt32(SwapU32(mLength));

	PSD_LOG_DBG("channel: write: usage=%d, length=%d",
		(int)mUsage,
		(int)mLength);
}

void PsdChannel::Write(Stream* stream)
{
	mChannelData.Seek(0, SeekMode_Begin);

	mChannelData.StreamTo(stream, mChannelData.Length_get());
}

void PsdChannel::EncodeBytes(uint16_t compression, uint16_t mode)
{
	StreamWriter writer(&mRleData, false);

	const int area = mSx * mSy;

	switch (compression)
	{
		case PsdCompressionType_Raw: // raw data
		{
			PSD_LOG_DBG("image data: write: raw");

			switch (mode)
			{
				case PsdImageMode_Rgb: // rgb
				{
					for (int i = 0; i < area; ++i)
					{
						writer.WriteUInt8(mBytes[i]);
					}

					break;
				}
				default:
					throw ExceptionVA("unknown compression/mode: %d/%d", compression, mode);
			}
			break;
		}
		case PsdCompressionType_Rle: // rle data
		{
			PSD_LOG_DBG("image data: write: rle");

			PsdCompression::RleCompress(mBytes, mSx, mSy, &mRleData, mRleHeader);

			break;
		}
		default:
			throw ExceptionVA("unknown compression: %d", compression);
	}
}

void PsdChannel::Setup(int sx, int sy, int bpp, uint16_t usage)
{
	Assert(sx >= 0);
	Assert(sy >= 0);
	Assert(bpp == 8);
	
	const int area = sx * sy;

	mBytes = new uint8_t[area];
	mSx = sx;
	mSy = sy;

	mUsage = usage;
	mLength = 0;
}

void PsdChannel::Setup(const Image& image, uint16_t usage)
{
	Setup(image.m_Sx, image.m_Sy, 8, usage);
	
	int index = -1;

	if (usage == 0)
		index = 0;
	if (usage == 1)
		index = 1;
	if (usage == 2)
		index = 2;
	if (usage == 65535)
		index = 3;

	if (index >= 0)
	{
		for (int y = 0; y < image.m_Sy; ++y)
		{
			const ImagePixel* srcLine = image.GetLine(y);
			uint8_t* dstLine = mBytes + y * mSx;
			
			for (int x = 0; x < image.m_Sx; ++x)
			{
				if (index == 3)
				{
					dstLine[x] = (uint8_t)srcLine[x].rgba[index];
				}
				else if (srcLine[x].rgba[3] == 0)
				{
					dstLine[x] = 0;
				}
				else
				{
					int v1 = srcLine[x].rgba[index];
					int v2 = srcLine[x].rgba[3];

					if (v1 > v2)
						v1 = v2;
					
					const int c = v1 * 255 / v2;
					
					Assert(c >= 0 && c <= 255);
					
					dstLine[x] = c;
				}
			}
		}
	}
	else
	{
		memset(mBytes, 0, mSx * mSy);
	}
}
