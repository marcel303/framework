#pragma once

enum OpenMode
{
	OpenMode_Read = 0x01,
	OpenMode_Write = 0x02,
	OpenMode_Append = 0x04,
	OpenMode_ReadWrite = OpenMode_Read | OpenMode_Write
};

enum SeekMode
{
	SeekMode_Begin,
	SeekMode_Offset
};

class Stream
{
public:
	virtual ~Stream();
	
	virtual void Open(const char* fileName, OpenMode mode) = 0;
	virtual void Close() = 0;
	virtual void Write(const void* bytes, int byteCount) = 0;
	virtual int Read(void* bytes, int byteCount) = 0;
	virtual int Length_get() = 0;
	virtual int Position_get() = 0;
	virtual void Seek(int seek, SeekMode mode) = 0;
	virtual bool EOF_get() = 0;
};

class StreamExtensions
{
public:
	static void StreamTo(Stream* src, Stream* dst, int bufferSize, int length);
	static void StreamTo(Stream* src, Stream* dst, int bufferSize);
	static void WriteTo(Stream* src, Stream* dst);
};
