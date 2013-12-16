#include "BinaryData.h"
#include "Exception.h"

BinaryData::BinaryData()
{
	Initialize();
}

BinaryData::~BinaryData()
{
	delete[] m_Bytes;
	m_Bytes = 0;
	m_ByteCount = 0;
}

void BinaryData::Initialize()
{
	m_Bytes = 0;
	m_ByteCount = 0;
}

void BinaryData::Load(Stream* stream)
{
	m_ByteCount = stream->Length_get();
	
	m_Bytes = new uint8_t[m_ByteCount];
	
	if (stream->Read(m_Bytes, m_ByteCount) != m_ByteCount)
		throw Exception("unable to read %d of binary data", m_ByteCount);
}
