#include <cstdio>
#include <stdio.h>
#include <sys/stat.h>
#include "Exception.h"
#include "FileStream.h"
#include "FixedSizeString.h"
#include "StringEx.h"

#ifdef PSP
#include <kernel.h>
#endif

FileStream::FileStream()
{
#ifdef PSP
	m_FileId = -1;
	m_FileLength = -1;
#else
	m_File = 0;
#endif
}

FileStream::FileStream(const char* fileName, OpenMode mode)
{
#ifdef PSP
	m_FileId = -1;
	m_FileLength = -1;
#else
	m_File = 0;
#endif

	Open(fileName, mode);
}

FileStream::~FileStream()
{
	if (IsOpen_get())
		Close();
}

void FileStream::Open(const char* fileName, OpenMode mode)
{
#ifdef PSP
	m_FileMode = mode;

	if (mode == OpenMode_Read)
	{
		SceIoStat stats;

		sceIoGetstat(fileName, &stats);

		m_FileLength = stats.st_size;
	}

	int flag = 0;

	if (mode == OpenMode_Read)
		flag = SCE_O_RDONLY;
	if (mode == OpenMode_Write)
		flag = SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC;
	if (mode == OpenMode_Append)
		flag = /*SCE_O_APPEND*/SCE_O_RDWR;

	if (flag == 0)
		throw ExceptionVA("unknown open mode: %d", (int)mode);

	m_FileId = sceIoOpen(fileName, flag, 0);

	if (m_FileId < 0)
		throw ExceptionVA("unable to open %s", fileName);

	if (mode == OpenMode_Append)
		Seek(Length_get(), SeekMode_Begin);
#else
	FixedSizeString<8> modeStr;

	if (mode & OpenMode_Read)
		modeStr += "r";
	if ((mode & OpenMode_Write) && !(mode & OpenMode_Append))
		modeStr += "w";
	if (mode & OpenMode_Append)
	{
		if (!Exists(fileName))
		{
			FileStream temp;
			temp.Open(fileName, OpenMode_Write);
			temp.Close();
		}
		modeStr += "r+";
	}

	if (mode & OpenMode_Text)
		modeStr += "t";
	else
		modeStr += "b";

	m_File = fopen(fileName, modeStr.c_str());
	
	if (!m_File)
		throw ExceptionVA("unable to open %s", fileName);
	
	if (mode & OpenMode_Append)
		Seek(Length_get(), SeekMode_Begin);
#endif
}

void FileStream::Close()
{
#ifdef PSP
	if (m_FileId < 0)
		throw ExceptionVA("file not open");

	// todo: flush
	// todo: close
	if (sceIoClose(m_FileId) != SCE_KERNEL_ERROR_OK)
		throw ExceptionVA("unable to close file");
	m_FileId = -1;
#else
	if (!m_File)
		throw ExceptionVA("file not open");
	
	fflush(m_File);
	fclose(m_File);
	m_File = 0;
#endif
}

void FileStream::Write(const void* bytes, int byteCount)
{
#if 0
	printf("==W==: %d x %d\n", Position_get(), byteCount);
#endif

	if (byteCount == 0)
		return;
	
#ifdef PSP
	int count = sceIoWrite(m_FileId, bytes, byteCount);

	if (count != byteCount)
		throw ExceptionVA(String::FormatC("failed to write %d bytes to stream", byteCount).c_str());
#else
	int count = (int)fwrite(bytes, byteCount, 1, m_File);

	if (count != 1)
		throw ExceptionVA(String::FormatC("failed to write %d bytes to stream", byteCount).c_str());
#endif
}

int FileStream::Read(void* bytes, int byteCount)
{
#ifdef PSP
	SceSSize count = sceIoRead(m_FileId, bytes, byteCount);

	if (count < 0)
		throw ExceptionVA("file read error");

	return (int)count;
#else
	int count = (int)fread(bytes, 1, byteCount, m_File);

	return count;
#endif
}

int FileStream::Length_get()
{
#ifdef PSP
	if (m_FileMode == OpenMode_Read)
		return m_FileLength;

	int position = Position_get();
	
	sceIoLseek(m_FileId, 0, SEEK_END);
	
	int length = Position_get();
	
	Seek(position, SeekMode_Begin);

	return length;
#else
//	filelength(fileno(m_File));
	
	int position = Position_get();
	
	fseek(m_File, 0, SEEK_END);
	
	int length = Position_get();
	
	Seek(position, SeekMode_Begin);
	
	return length;
#endif
}

int FileStream::Position_get()
{
#ifdef PSP
	int result =(int)sceIoLseek(m_FileId, 0, SCE_SEEK_CUR);

	if (result < 0)
		throw ExceptionVA("unable to determine file stream position");

	return result;
#else
	return static_cast<int>(ftell(m_File));
#endif
}

void FileStream::Seek(int seek, SeekMode mode)
{
#ifdef PSP
	if (mode == SeekMode_Begin)
	{
		sceIoLseek(m_FileId, seek, SCE_SEEK_SET);
	}

	if (mode == SeekMode_Offset)
	{
		if (seek < 0)
		{
			Seek(Position_get() + seek, SeekMode_Begin);
		}
		else
		{
			sceIoLseek(m_FileId, seek, SCE_SEEK_CUR);
		}
	}
#else
	if (mode == SeekMode_Begin)
	{
		fseek(m_File, seek, SEEK_SET);
	}
	
	if (mode == SeekMode_Offset)
	{
		if (seek < 0)
		{
			Seek(Position_get() + seek, SeekMode_Begin);
		}
		else
		{
			fseek(m_File, seek, SEEK_CUR);
		}
	}
#endif
}

bool FileStream::EOF_get()
{
	return Position_get() >= Length_get();
}

bool FileStream::IsOpen_get() const
{
#ifdef PSP
	return m_FileId >= 0;
#else
	return m_File != 0;
#endif
}

bool FileStream::Exists(const char* fileName)
{
#ifdef PSP
	SceUID file = sceIoOpen(fileName, SCE_O_RDONLY, 0);

	if (file >= 0)
	{
		sceIoClose(file);

		return true;
	}
	else
	{
		return false;
	}
#else
	FILE* file = fopen(fileName, "rb");
	
	if (file)
	{
		fclose(file);
		
		return true;
	}
	else
	{
		return false;
	}
#endif
}

void FileStream::Delete(const char* fileName)
{
#ifdef PSP
	sceIoRemove(fileName);
#else
	remove(fileName);
#endif
}

FILE* FileStream::FileHandle_get()
{
#ifdef PSP
	throw ExceptionVA("not yet implemented");
#else
	return m_File;
#endif
}
