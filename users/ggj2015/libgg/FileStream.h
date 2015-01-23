#pragma once

#include <stdio.h>
#include "Stream.h"

#ifdef PSP
#include <kerneltypes.h>
#endif

class FileStream : public Stream
{
public:
	FileStream();
	FileStream(const char* fileName, OpenMode mode);
	virtual ~FileStream();

	virtual void Open(const char* fileName, OpenMode mode);
	virtual void Close();

	virtual void Write(const void* bytes, int byteCount);
	virtual int Read(void* bytes, int byteCount);

	virtual int Length_get();
	virtual int Position_get();
	virtual void Seek(int seek, SeekMode mode);
	virtual bool EOF_get();
	virtual bool IsOpen_get() const;

	static bool Exists(const char* fileName);
	static void Delete(const char* fileName);
	
	FILE* FileHandle_get();
	
private:
#ifdef PSP
	SceUID m_FileId;
	int m_FileLength;
	OpenMode m_FileMode;
#else
	FILE* m_File;
#endif
};
