#error


#include <stdio.h>
#include "FileStream.h"
#include "Stream.h"

enum FSROOT
{
	FSROOT_BUNDLE,
	FSROOT_DOCUMENTS,
	FSROOT_CACHE,
	FSROOT_TEMP
};

class Stream_File : public Stream
{
public:
	Stream_File();
	Stream_File(const char* fileName, OpenMode mode);
	virtual ~Stream_File();
	void Initialize();
	
	void Open(const char* fileName, OpenMode mode);
	void Close();
	
	virtual int Read(void* out_Bytes, int byteCount);
	virtual void Write(const void* bytes, int byteCount);
	virtual int Length_get();
	virtual int Position_get();
	virtual void Seek(int seek, SeekMode mode);
	virtual bool EOF_get();
	
	FILE* FileHandle_get();
	
private:
	OpenMode m_OpenMode;
	void* m_FileHandle;
	FileStream m_FileStream;
};
