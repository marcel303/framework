#include "FileStream.h"
#include "FileStreamExtends.h"
#include "MemoryStream.h"
#include "StreamReader.h"

bool FileStreamExtents::ContentsAreEqual(Stream* src, Stream* dst)
{
	StreamReader srcReader(src, false);
	StreamReader dstReader(dst, false);
	
	int srcLength = src->Length_get();
	int dstLength = dst->Length_get();
	
	if (srcLength != dstLength)
		return false;
	
	uint8_t* srcBytes = srcReader.ReadAllBytes();
	uint8_t* dstBytes = dstReader.ReadAllBytes();

	int length = srcLength;
	
	bool result = true;
	
	for (int i = 0; i < length; ++i)
	{
		if (srcBytes[i] != dstBytes[i])
			result = false;
	}
	
	delete[] srcBytes;
	srcBytes = 0;
	delete[] dstBytes;
	dstBytes = 0;
	
	return result;
}

void FileStreamExtents::OverwriteIfChanged(MemoryStream* src, const std::string& fileName)
{
	src->Seek(0, SeekMode_Begin);
	
	FileStream dst;
	
	bool overwrite = false;

	if (!FileStream::Exists(fileName.c_str()))
	{
		overwrite = true;
	}
	else
	{
		dst.Open(fileName.c_str(), OpenMode_Read);
		
		overwrite = !ContentsAreEqual(src, &dst);

		dst.Close();
	}

	if (overwrite)
	{
		//printf("%s has changed - overwrite\n", fileName.c_str());
		
		src->Seek(0, SeekMode_Begin);
		dst.Open(fileName.c_str(), OpenMode_Write);
		
		src->CopyTo(&dst);
	}
}
