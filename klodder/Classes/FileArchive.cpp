#include "Exception.h"
#include "FileArchive.h"
#include "StreamReader.h"
#include "StreamWriter.h"

#define TYPE_FILE 1

FileArchive::~FileArchive()
{
	Clear();
}

void FileArchive::Clear()
{
	for (size_t i = 0; i < mFileList.size(); ++i)
		delete mFileList[i];
	
	mFileList.clear();
}

void FileArchive::Add(const char* name, Stream* stream, int length)
{
	if (length < 0)
		length = stream->Length_get();
	
	stream->Seek(0, SeekMode_Begin);
	
	FileArchiveMember* member = new FileArchiveMember();

	member->mFileName = name;
	member->mData.StreamFrom(stream, length);
	
	mFileList.push_back(member);
}

void FileArchive::Add(const char* name, Stream* stream)
{
	Add(name, stream, -1);
}

void FileArchive::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	while (!stream->EOF_get())
	{
		uint32_t type = reader.ReadUInt32();
		
		switch (type)
		{
			case TYPE_FILE:
			{
				uint32_t version = reader.ReadUInt32();
				
				switch (version)
				{
					case 1:
					{
						FileArchiveMember* member = new FileArchiveMember();
						
						// read filename
						
						member->mFileName = reader.ReadText_Binary();
						
						// read data
						
						uint32_t length = reader.ReadUInt32();
						
						member->mData.StreamFrom(stream, length);
						
						mFileList.push_back(member);

						LOG_DBG("read: %s, length=%lu", member->mFileName.c_str(), length);
						
						break;
					}
						
					default:
						throw ExceptionVA("unknown archive member version: %d", (int)version);
				}
				
				break;
			}
				
			default:
				throw ExceptionVA("unknown archive element type: %d", (int)type);
		}
	}
}

/*void FileArchive::Save(Stream* stream)
{
	SaveBegin(stream);
	
	for (size_t i = 0; i < mFileList.size(); ++i)
	{
		FileArchiveMember* member = mFileList[i];
		
		member->mData.Seek(0, SeekMode_Begin);
		
		SaveAdd(stream, member->mFileName.c_str(), &member->mData, member->mLength);
	}
	
	SaveEnd(stream);
}*/

void FileArchive::SaveBegin(Stream* stream)
{
	// nop
}

void FileArchive::SaveAdd(Stream* stream, const char* name, Stream* dataStream, int dataLength)
{
	StreamWriter writer(stream, false);
	
	int version = 1;
	
	switch (version)
	{
		case 1:
		{
			// write element type
			
			writer.WriteUInt32(TYPE_FILE);
			
			// write archive member version
			
			writer.WriteUInt32(version);
			
			// write filename
			
			writer.WriteText_Binary(name);
			
			// write file data
			
			uint32_t length = dataLength >= 0 ? dataLength : (dataStream->Length_get() - dataStream->Position_get());
			
			writer.WriteUInt32(length);
			
			StreamExtensions::StreamTo(dataStream, stream, 4096, length);
		
			break;
		}
			
		default:
			throw ExceptionVA("unknown archive version: %d", (int)version);
	}
}

void FileArchive::SaveAdd(Stream* stream, const char* name, Stream* dataStream)
{
	SaveAdd(stream, name, dataStream, -1);
}

void FileArchive::SaveEnd(Stream* stream)
{
}

Stream* FileArchive::GetStream(std::string name)
{
	for (size_t i = 0; i < mFileList.size(); ++i)
		if (mFileList[i]->mFileName == name)
			return &mFileList[i]->mData;

	throw ExceptionVA("stream not found: %s", name.c_str());
}
