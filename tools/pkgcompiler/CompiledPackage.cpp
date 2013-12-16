#include <string.h>
#include "CompiledPackage.h"
#include "StreamReader.h"
#include "StreamWriter.h"

PackageItem::PackageItem()
{
	m_Hash = 0;
}

void PackageItem::Setup(Hash hash, const std::string& fileName, int offset, int length)
{
	m_Hash = hash;
	m_FileName = fileName;
	m_Offset = offset;
	m_Length = length;
}

//

PackageIndex::PackageIndex()
{
	m_Items = 0;
	m_ItemCount = 0;
}

PackageIndex::~PackageIndex()
{
	Allocate(0);
}

void PackageIndex::Allocate(int count)
{
	delete[] m_Items;
	m_Items = 0;
	m_ItemCount = 0;
	
	if (count > 0)
	{
		m_Items = new PackageItem[count];
		m_ItemCount = count;
	}
}

bool PackageIndex::ContainsFile(const char* fileName)
{
	PackageItem* item = FindFile(fileName);

	return item != 0;
}

PackageItem* PackageIndex::FindFile(const char* fileName)
{
	std::map<std::string, PackageItem*>::iterator i = m_ItemsByFileName.find(std::string(fileName));

	if (i == m_ItemsByFileName.end())
		return 0;

	return i->second;

#if 0
	Hash hash = HashFunc::Hash_FNV1(fileName, strlen(fileName));

	for (int i = 0; i < m_ItemCount; ++i)
	{
		PackageItem& item = m_Items[i];

		//if (item.m_Hash != hash)
		//	continue;
		if (strcmp(item.m_FileName.c_str(), fileName))
			continue;

		return &item;
	}

	return 0;
#endif
}

//

CompiledPackage::CompiledPackage()
{
	m_DataStream = 0;
}

CompiledPackage::~CompiledPackage()
{
	Clear();
}

void CompiledPackage::Clear()
{
	delete m_DataStream;
	m_DataStream = 0;
}

void CompiledPackage::Load(Stream* stream, bool own)
{
	Clear();

	if (!own)
		throw ExceptionVA("compiled package requires stream ownership");

	m_DataStream = stream;

	StreamReader reader(stream, false);

	int32_t itemCount = reader.ReadInt32();

	m_Index.Allocate(itemCount);

	for (int i = 0; i < itemCount; ++i)
	{
		PackageItem& item = m_Index.m_Items[i];

		item.m_Hash = reader.ReadUInt32();
		item.m_Offset = reader.ReadUInt32();
		item.m_Length = reader.ReadUInt32();
		item.m_FileName = reader.ReadText_Binary();

		m_Index.m_ItemsByFileName.insert(std::pair<std::string, PackageItem*>(item.m_FileName, &item));
	}

	m_DataOffset = m_DataStream->Position_get();
}

void CompiledPackage::Save(Stream* stream)
{
	StreamWriter writer(stream, false);

	// write index

	writer.WriteInt32(m_Index.m_ItemCount);

	for (int i = 0; i < m_Index.m_ItemCount; ++i)
	{
		PackageItem& item = m_Index.m_Items[i];

		writer.WriteUInt32(item.m_Hash);
		writer.WriteUInt32(item.m_Offset);
		writer.WriteUInt32(item.m_Length);
		writer.WriteText_Binary(item.m_FileName.c_str());
	}

	// write data

	StreamExtensions::WriteTo(&m_Data, stream);
}

bool CompiledPackage::ContainsFile(const char* fileName)
{
	return m_Index.ContainsFile(fileName);
}

SubStream CompiledPackage::Open(const char* fileName)
{
	PackageItem* item = m_Index.FindFile(fileName);

	if (item == 0)
		throw ExceptionVA("file not found");

	int offset = m_DataOffset + item->m_Offset;

	m_DataStream->Seek(offset, SeekMode_Begin);

	return SubStream(m_DataStream, false, offset, item->m_Length);
}
