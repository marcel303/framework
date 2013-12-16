#pragma once

#include <map>
#include <string>
#include "Hash.h"
#include "MemoryStream.h"
#include "SubStream.h"

class PackageItem
{
public:
	PackageItem();

	void Setup(Hash hash, const std::string& fileName, int offset, int length);
	
	Hash m_Hash;
	std::string m_FileName;
	int m_Offset;
	int m_Length;
};

class PackageIndex
{
public:
	PackageIndex();
	~PackageIndex();
	
	void Allocate(int count);

	bool ContainsFile(const char* fileName);
	PackageItem* FindFile(const char* fileName);
	
	PackageItem* m_Items;
	int m_ItemCount;

	std::map<std::string, PackageItem*> m_ItemsByFileName;
};

class CompiledPackage
{
public:
	CompiledPackage();
	~CompiledPackage();

	void Clear();

	void Load(Stream* stream, bool own);
	void Save(Stream* stream);

	bool ContainsFile(const char* fileName);
	SubStream Open(const char* fileName);
	
	PackageIndex m_Index;
	MemoryStream m_Data;
	int m_DataOffset;
	Stream* m_DataStream;
};
