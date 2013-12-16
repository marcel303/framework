#if 0

#include "Exception.h"
#include "Stream_File.h"

#define USE_NATIVE 0

Stream_File::Stream_File()
{
	Initialize();
}

Stream_File::Stream_File(const char* fileName, OpenMode mode)
{
	Initialize();
	
	Open(fileName, mode);
}

Stream_File::~Stream_File()
{
#if USE_NATIVE
	if (m_FileHandle)
		Close();
#endif
}

void Stream_File::Initialize()
{
#if USE_NATIVE
	m_FileHandle = 0;
	m_OpenMode = OpenMode_Read;
#endif
}

void Stream_File::Open(const char* fileName, OpenMode mode)
{
#if USE_NATIVE
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName];
	const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	const NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw Exception();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
	
	if (!path)
		throw Exception("path does not exist: %s", fileName);
	
	NSFileHandle* fileHandle = 0;
	
	if (mode == OpenMode_Read)
		fileHandle = [NSFileHandle fileHandleForReadingAtPath:path];
	if (mode == OpenMode_Write)
		fileHandle = [NSFileHandle fileHandleForWritingAtPath:path];

	if (!fileHandle)
		throw Exception("unable to open file: %s", fileName);
	
	m_FileHandle = fileHandle;
	m_OpenMode = mode;
#else
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
	const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	const NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw ExceptionNA();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
	
	if (!path)
		throw ExceptionVA("path does not exist: %s", fileName);
	
	const char* temp = [path cStringUsingEncoding:NSASCIIStringEncoding];
	
	m_FileStream.Open(temp, mode);
#endif
}

void Stream_File::Close()
{
#if USE_NATIVE
	if (!m_FileHandle)
		throw std::exception();
	
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	[fileHandle closeFile];
	
	m_FileHandle = 0;
#else
	m_FileStream.Close();
#endif
}

int Stream_File::Read(void* out_Bytes, int byteCount)
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	const NSData* data = [fileHandle readDataOfLength:byteCount];
	
	[data getBytes:out_Bytes];
	
	const int length = data.length;
	
	return length;
#else
	return m_FileStream.Read(out_Bytes, byteCount);
#endif
}

void Stream_File::Write(const void* bytes, int byteCount)
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	NSData* data = [NSData dataWithBytesNoCopy:(void*)bytes length:byteCount];
	
	try
	{
		[fileHandle writeData:data];
	}
	catch (...)
	{
		throw Exception("unable to write to file");
	}
#else
	m_FileStream.Write(bytes, byteCount);
#endif
}

int Stream_File::Length_get()
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	const int p1 = (int)[fileHandle offsetInFile];
	
	[fileHandle seekToEndOfFile];
	
	const int p2 = (int)[fileHandle offsetInFile];
	
	[fileHandle seekToFileOffset:p1];
	
	return p2 - p1;
#else
	return m_FileStream.Length_get();
#endif
}

int Stream_File::Position_get()
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	return (int)[fileHandle offsetInFile];
#else
	return m_FileStream.Position_get();
#endif
}

void Stream_File::Seek(int seek, SeekMode mode)
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	if (mode == SeekMode_Begin)
	{
		[fileHandle seekToFileOffset:seek];
	}
	
	if (mode == SeekMode_Offset)
	{
		const int p = Position_get() + seek;
		
		[fileHandle seekToFileOffset:p];
	}
#else
	m_FileStream.Seek(seek, mode);
#endif
}

bool Stream_File::EOF_get()
{
#if USE_NATIVE
	NSFileHandle* fileHandle = (NSFileHandle*)m_FileHandle;
	
	const int p1 = (int)[fileHandle offsetInFile];
	
	[fileHandle seekToEndOfFile];
	
	const int p2 = (int)[fileHandle offsetInFile];
	
	[fileHandle seekToFileOffset:p1];
	
	return p1 == p2;
#else
	return m_FileStream.EOF_get();
#endif
}

FILE* Stream_File::FileHandle_get()
{
#if USE_NATIVE
	throw ExceptionVA("operation not supported");
#else
	return m_FileStream.FileHandle_get();
#endif
}

#endif
