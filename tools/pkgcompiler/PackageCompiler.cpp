#include "FileStream.h"
#include "Hash.h"
#include "PackageCompiler.h"
#include "Path.h"

void PackageCompiler::Compile(std::vector<std::string> srcList, std::string dst, CompiledPackage& out_Package)
{
	PackageIndex& index = out_Package.m_Index;

	index.Allocate((int)srcList.size());

	for (size_t i = 0; i < srcList.size(); ++i)
	{
		PackageItem& item = index.m_Items[i];

		std::string fileName = srcList[i];
		Hash hash = HashFunc::Hash_FNV1(fileName.c_str(), fileName.size());
		int offset = out_Package.m_Data.Position_get();

		FileStream stream;
		stream.Open(fileName.c_str(), OpenMode_Read);
		int length = stream.Length_get();
		StreamExtensions::WriteTo(&stream, &out_Package.m_Data);

		item.Setup(hash, Path::GetFileName(fileName), offset, length);
	}
}
