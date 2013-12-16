#include <algorithm>
#include <string.h>
#include "Exception.h"
#include "FileStream.h"
#include "Image.h"
#include "ImageLoader_Tiff.h"
#include "Log.h"
#include "MemoryStream.h"
#include "StreamWriter.h"
#include "StringEx.h"

/*
 Baseline TIFF writer:
 
 bpp = 8
 compression = 1 (uncompressed)
 
 //
 
 8 byte image header:
 	2 II or MM. II = little endian
 	2 int16: 42, version
 	4 int32: IFD location, must be word aligned. suggested directly after header
 
 IDF:
 	2 int16: item count
 	12 x count: directory items
 	4 int32: next IFD location (0 if none)
 
 IFD item:
 	2 int16: IFD item tag
    2 int16: field type
 		1 = UNSIGNED BYTE
 		2 = ASCII
 		3 = UNSIGNED SHORT
 		4 = UNSIGNED LONG
 		5 = RATIONAL
    4 int32: field count
 	4 int32: field location or field value (if value fits into 4 bytes)
 	** IFD items must be sorted by tag**
 
 
 */

enum TiffTag
{
	TiffTag_Compression = 259,
	TiffTag_ImageSx = 256,
	TiffTag_ImageSy = 257,
	TiffTag_StripSy = 278,
	TiffTag_StripLocation = 273,
	TiffTag_StripByteCount = 279,
	TiffTag_StripOffset = 273,
	TiffTag_BitsPerSample = 258,
	TiffTag_SamplesPerPixel = 277,
	TiffTag_PhotometricInterpretation = 262,
	TiffTag_ResolutionSx = 282,
	TiffTag_ResolutionSy = 283,
	TiffTag_ResolutionUnit = 296,
	TiffTag_ExtraSamples = 338,
	TiffTag_FileType = 254,
	TiffTag_PageNumber = 297,
	TiffTag_PageName = 285
};

enum TiffFieldType
{
	TiffFieldType_UInt8 = 1,
	TiffFieldType_UInt16 = 3,
	TiffFieldType_UInt32 = 4,
	TiffFieldType_String = 2,
	TiffFieldType_Rational = 5
};

enum TiffCompressionType
{
	TiffCompressionType_None = 1
};

enum TiffPhotometricInterpretation
{
	TiffPhotometricInterpretation_Rgb = 2
};

enum TiffResolutionUnit
{
	TiffResolutionUnit_Inch = 2
};

enum TiffFileType
{
	TiffFileType_Page = 1 << 1,
	TiffFileType_Thumbnail = 1 << 0
};

class TiffHeader
{
public:
	TiffHeader()
	{
		mEndianness[0] = mEndianness[1] = 'I';
		mVersion = 42;
		mDirectoryLocation = 8;
	}
	
	void Write(Stream* stream)
	{
		StreamWriter writer(stream, false);
		
		LOG_DBG("header: write: endianness=%c%c, version=%d, directory=%d",
				mEndianness[0],
				mEndianness[1],
				mVersion,
				mDirectoryLocation);
		
		writer.WriteInt8(mEndianness[0]);
		writer.WriteInt8(mEndianness[1]);
		
		writer.WriteInt16(mVersion);
		writer.WriteUInt32(mDirectoryLocation);
	}
	
	char mEndianness[2];
	int16_t mVersion;
	int32_t mDirectoryLocation;
};

class TiffValue
{
public:
	TiffValue(TiffFieldType type, int value)
	{
		switch (type)
		{
			case TiffFieldType_UInt8:
				value8 = value;
				break;
			case TiffFieldType_UInt16:
				value16 = value;
				break;
			case TiffFieldType_UInt32:
				value32 = value;
				break;
			case TiffFieldType_String:
				this->text = text;
				break;
			default:
				throw ExceptionNA();
		}
	}
	
	TiffValue(TiffFieldType type, int value1, int value2)
	{
		switch (type)
		{
			case TiffFieldType_Rational:
				rational[0] = value1;
				rational[1] = value2;
				break;
			default:
				throw ExceptionNA();
		}
	}
	
	union
	{
		uint8_t value8;
		uint16_t value16;
		uint32_t value32;
		uint32_t rational[2];
		char text;
	};
};

class TiffDirectoryEntry
{
public:
	void Setup(TiffTag tag, TiffFieldType type)
	{
		SetupEx(tag, type, 0);
	}
	
	void Setup(TiffTag tag, TiffFieldType type, int value)
	{
		SetupEx(tag, type, &value);
	}
	
	void Setup(TiffTag tag, TiffFieldType type, int value1, int value2)
	{
		SetupEx(tag, type, 0);
		
		Add(TiffValue(type, value1, value2));
	}
	
	void SetupEx(TiffTag tag, TiffFieldType type, int* value)
	{
		mTag = tag;
		mFieldType = type;
		mValueCount = 0;
		
		if (value)
		{
			Add(TiffValue(type, *value));
		}
	}
	
	void Setup(TiffTag tag, TiffFieldType type, const std::string& text)
	{
		mTag = tag;
		mFieldType = type;
		mValueCount = 0;
		
		for (size_t i = 0; i < text.length(); ++i)
			Add(TiffValue(type, text[i]));
		Add(TiffValue(type, 0));
	}
	
	void Add(TiffValue value)
	{
		mValueList.push_back(value);
		
		mValueCount = (int)mValueList.size();
	}
	
	void Write(Stream* stream, Stream* contentStream, int offsetValues, int offsetImage)
	{
		LOG_DBG("entry: write: tag=%d, field_type=%d, value_count=%d", (int)mTag, (int)mFieldType, (int)mValueCount);
		
		StreamWriter writer(stream, false);
		StreamWriter contentWriter(contentStream, false);
		
		writer.WriteInt16(mTag);
		writer.WriteInt16(mFieldType);
		writer.WriteInt32(mValueCount);
		
		if (mTag == TiffTag_StripOffset)
			mValueList[0].value32 = offsetImage;
		
		if (IsInline_get())
		{
			int position = stream->Position_get();
			
			writer.WriteInt32(0);
			stream->Seek(position, SeekMode_Begin);

			for (int i = 0; i < mValueCount; ++i)
			{
				WriteValue(writer, 0, true);
			}
			
			stream->Seek(position + 4, SeekMode_Begin);
		}
		else
		{
			int location = offsetValues + contentStream->Position_get();
			
			writer.WriteInt32(location);
			
			for (int i = 0; i < mValueCount; ++i)
			{
				WriteValue(contentWriter, i, false);
			}
		}
		
		// word align
		
		if (stream->Position_get() % 2)
			writer.WriteUInt8(0);
	}
						
	void WriteValue(StreamWriter& writer, int index, bool isInline)
	{
		LOG_DBG("write_value: tag=%03d, type=%d, index=%d, inline=%d", (int)mTag, (int)mFieldType, index, isInline ? 1 : 0);
		
		switch (mFieldType)
		{
			case TiffFieldType_UInt8:
				writer.WriteInt8(mValueList[index].value8);
				break;
			case TiffFieldType_UInt16:
				writer.WriteInt16(mValueList[index].value16);
				break;
			case TiffFieldType_UInt32:
				writer.WriteInt32(mValueList[index].value32);
				break;
			case TiffFieldType_Rational:
				writer.WriteInt32(mValueList[index].rational[0]);
				writer.WriteInt32(mValueList[index].rational[1]);
				break;
			case TiffFieldType_String:
				writer.WriteInt8(mValueList[index].text);
				break;
			default:
				throw ExceptionNA();
		}
	}
	
	inline int FieldSize_get() const
	{
		switch (mFieldType)
		{
			case TiffFieldType_UInt8:
				return 1;
			case TiffFieldType_UInt16:
				return 2;
			case TiffFieldType_UInt32:
				return 4;
			case TiffFieldType_Rational:
				return 8;
			case TiffFieldType_String:
				return 1;
			default:
				throw ExceptionNA();
		}
	}
	
	inline int ValueSize_get() const
	{
		return FieldSize_get() * mValueCount;
	}
	
	inline bool IsInline_get() const
	{
		return mValueCount == 1 && ValueSize_get() <= 4;
	}
	
	static TiffDirectoryEntry MakeImageSx(int sx);
	static TiffDirectoryEntry MakeImageSy(int sy);
	static TiffDirectoryEntry MakeCompression(TiffCompressionType type);
	static TiffDirectoryEntry MakeStripSy(int sy);
	static TiffDirectoryEntry MakeStripOffset(int offset);
	static TiffDirectoryEntry MakeStripByteCount(int byteCount);
	static TiffDirectoryEntry MakeBitsPerSample(int samplesPerPixel, int bitsPerSample);
	static TiffDirectoryEntry MakeSamplesPerPixel(int samplesPerPixel);
	static TiffDirectoryEntry MakePhotometricInterpretation(TiffPhotometricInterpretation mode);
	static TiffDirectoryEntry MakeResolutionUnit(TiffResolutionUnit unit);
	static TiffDirectoryEntry MakeResolutionSx(int sxNom, int sxDenom);
	static TiffDirectoryEntry MakeResolutionSy(int syNom, int syDenom);
	static TiffDirectoryEntry MakeExtraSamples(int extraSamples);
	static TiffDirectoryEntry MakeFileType(int fileType);
	static TiffDirectoryEntry MakePageNumber(int pageNumber, int pageCount);
	static TiffDirectoryEntry MakePageName(const std::string& name);
	
	inline bool operator<(const TiffDirectoryEntry& e) const
	{
		return mTag < e.mTag;
	}
	
	TiffTag mTag;
	TiffFieldType mFieldType;
	int32_t mValueCount;
	std::vector<TiffValue> mValueList;
};

TiffDirectoryEntry TiffDirectoryEntry::MakeImageSx(int sx)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ImageSx, TiffFieldType_UInt16, sx);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeImageSy(int sy)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ImageSy, TiffFieldType_UInt16, sy);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeCompression(TiffCompressionType type)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_Compression, TiffFieldType_UInt16, type);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeStripSy(int sy)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_StripSy, TiffFieldType_UInt16, sy);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeStripOffset(int offset)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_StripOffset, TiffFieldType_UInt32, offset);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeStripByteCount(int byteCount)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_StripByteCount, TiffFieldType_UInt32, byteCount);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeBitsPerSample(int samplerPerPixel, int bitsPerSample)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_BitsPerSample, TiffFieldType_UInt16);
	for (int i = 0; i < samplerPerPixel; ++i)
		result.Add(TiffValue(TiffFieldType_UInt16, bitsPerSample));
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeSamplesPerPixel(int samplesPerPixel)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_SamplesPerPixel, TiffFieldType_UInt16, samplesPerPixel);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakePhotometricInterpretation(TiffPhotometricInterpretation mode)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_PhotometricInterpretation, TiffFieldType_UInt16, mode);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeResolutionUnit(TiffResolutionUnit unit)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ResolutionUnit, TiffFieldType_UInt16, unit);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeResolutionSx(int sxNom, int sxDenom)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ResolutionSx, TiffFieldType_Rational, sxNom, sxDenom);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeResolutionSy(int syNom, int syDenom)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ResolutionSy, TiffFieldType_Rational, syNom, syDenom);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeExtraSamples(int extraSamples)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_ExtraSamples, TiffFieldType_UInt16, extraSamples);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakeFileType(int fileType)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_FileType, TiffFieldType_UInt32, fileType);
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakePageNumber(int pageNumber, int pageCount)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_PageNumber, TiffFieldType_UInt16);
	result.Add(TiffValue(TiffFieldType_UInt16, pageNumber));
	result.Add(TiffValue(TiffFieldType_UInt16, pageCount));
	return result;
}

TiffDirectoryEntry TiffDirectoryEntry::MakePageName(const std::string& name)
{
	TiffDirectoryEntry result;
	result.Setup(TiffTag_PageName, TiffFieldType_String, name);
	return result;
}

class TiffDirectory
{
public:
	void Add(TiffDirectoryEntry entry)
	{
		mEntries.push_back(entry);
	}
	
/*	void WriteReserve(Stream* stream)
	{
		int byteCount = 2 + mEntries.size() * 12 + 4;
		
		uint8_t* bytes = new uint8_t[byteCount];
		
		memset(bytes, 0, byteCount);
		
		stream->Write(bytes, byteCount);
		
		delete[] bytes;
	}*/
	
	void Write(Stream* stream, int offsetValues, int offsetImage, int offsetNext)
	{
		LOG_DBG("directory: write: entry_count=%d", (int)mEntries.size());
		
		Sort();
		
		StreamWriter writer(stream, false);
		
		MemoryStream valueContents;
		
		// write entry count
		
		writer.WriteInt16((int16_t)mEntries.size());
		
		// write entries
		
		for (std::vector<TiffDirectoryEntry>::iterator i = mEntries.begin(); i != mEntries.end(); ++i)
			i->Write(stream, &valueContents, offsetValues, offsetImage);
		
		// write next directory location
		
		writer.WriteUInt32(offsetNext);
		
		// write values
		
		stream->Seek(offsetValues, SeekMode_Begin);
//		stream->Seek(valueLocation, SeekMode_Begin);
		valueContents.Seek(0, SeekMode_Begin);
		valueContents.StreamTo(stream, valueContents.Length_get());
	}
	
	void Sort()
	{
		std::sort(mEntries.begin(), mEntries.end());
	}
	
	std::vector<TiffDirectoryEntry> mEntries;
};

void ImageLoader_Tiff::Load(Image& image, const std::string& fileName)
{
	throw ExceptionVA("not implemented");
}

static void WritePicture(Stream* stream, const Image& image, int pageNumber, int pageCount, bool expectNext)
{	
	const int begin = stream->Position_get();

	// calculate space requirements
	
	const int reservedForDirectory = 1024;
	const int reservedForValues = 1024;
	const int reservedForImage = image.m_Sx * image.m_Sy * 4;
	
	// calculate section offsets
	
	const int offsetValues = begin + reservedForDirectory;
	const int offsetImage = offsetValues + reservedForValues;
	const int offsetNext = offsetImage + reservedForImage;
	
	LOG_DBG("write_picture: begin_directory=%d, begin_values=%d, begin_image=%d, begin_next=%d", begin, offsetValues, offsetImage, offsetNext);
	
	// allocate space
	
	const int reserved = reservedForDirectory + reservedForValues + reservedForImage;
	
	uint8_t* bytes = new uint8_t[reserved];
	memset(bytes, 0, reserved);
	stream->Write(bytes, reserved);
	delete[] bytes;
		
	// build directory
	
	TiffDirectory directory;
	
	directory.Add(TiffDirectoryEntry::MakeImageSx(image.m_Sx));
	directory.Add(TiffDirectoryEntry::MakeImageSy(image.m_Sy));
	directory.Add(TiffDirectoryEntry::MakeCompression(TiffCompressionType_None));
	directory.Add(TiffDirectoryEntry::MakeStripSy(image.m_Sy));
	directory.Add(TiffDirectoryEntry::MakeStripOffset(0));
	directory.Add(TiffDirectoryEntry::MakeStripByteCount(image.m_Sx * image.m_Sy * 4));
	directory.Add(TiffDirectoryEntry::MakeBitsPerSample(4, 8));
	directory.Add(TiffDirectoryEntry::MakeSamplesPerPixel(4));
	directory.Add(TiffDirectoryEntry::MakePhotometricInterpretation(TiffPhotometricInterpretation_Rgb));
	directory.Add(TiffDirectoryEntry::MakeResolutionUnit(TiffResolutionUnit_Inch));
	directory.Add(TiffDirectoryEntry::MakeResolutionSx(image.m_Sx, 1));
	directory.Add(TiffDirectoryEntry::MakeResolutionSy(image.m_Sy, 1));
	directory.Add(TiffDirectoryEntry::MakeExtraSamples(1));
	directory.Add(TiffDirectoryEntry::MakeFileType(TiffFileType_Page));
	directory.Add(TiffDirectoryEntry::MakePageNumber(pageNumber, pageCount));
	directory.Add(TiffDirectoryEntry::MakePageName(String::Format("page %d of %d", pageNumber, pageCount)));
	
	// write directory
	
	stream->Seek(begin, SeekMode_Begin);
	directory.Write(stream, offsetValues, offsetImage, expectNext ? offsetNext : 0);
	
	// write image data
	
	LOG_DBG("write_image: sx=%d, sy=%d", image.m_Sx, image.m_Sy);
	
	stream->Seek(offsetImage, SeekMode_Begin);
	
	StreamWriter writer(stream, false);
	
	for (int y = 0; y < image.m_Sy; ++y)
	{
		const ImagePixel* line = image.GetLine(y);
		
		for (int x= 0; x < image.m_Sx; ++x)
		{
			writer.WriteUInt8((uint8_t)line[x].r);
			writer.WriteUInt8((uint8_t)line[x].g);
			writer.WriteUInt8((uint8_t)line[x].b);
			writer.WriteUInt8((uint8_t)line[x].a);
		}
	}
}

void ImageLoader_Tiff::Save(const Image& image, const std::string& fileName)
{
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Write);
	
	TiffHeader header;
	
	header.Write(&stream);

	WritePicture(&stream, image, 0, 2, true);
	WritePicture(&stream, image, 1, 2, false);
}
