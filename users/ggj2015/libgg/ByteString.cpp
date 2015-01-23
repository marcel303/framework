#include "ByteString.h"
#include "Exception.h"

ByteString::ByteString()
{
}

ByteString::ByteString(int length)
{
	Length_set(length);
}

ByteString ByteString::FromString(const std::string& text)
{
	ByteString result;
	
	result.Length_set((int)text.length());
	
	for (size_t i = 0; i < text.length(); ++i)
		result.m_Bytes[i] = text[i];
	
	return result;
}

int ByteString::Length_get() const
{
	return (int)m_Bytes.size();
}

void ByteString::Length_set(int length)
{
	m_Bytes.clear();
	
	m_Bytes.reserve(length);
	
	for (int i = 0; i < length; ++i)
		m_Bytes.push_back(0);
}

ByteString ByteString::Extract(int offset, int length)
{
	if (offset + length > Length_get())
		throw ExceptionVA("index out of bounds");
	
	ByteString result(length);
	
	for (int i = 0; i < length; ++i)
		result.m_Bytes[i] = m_Bytes[offset + i];
	
	return result;
}

std::string ByteString::ToString() const
{
	std::string result;
	
	for (size_t i = 0; i < m_Bytes.size(); ++i)
		result.push_back(m_Bytes[i]);
	
	return result;
}

bool ByteString::Equals(const ByteString& other)
{
	if (Length_get() != other.Length_get())
		return false;
	
	for (int i = 0; i < Length_get(); ++i)
		if (m_Bytes[i] != other.m_Bytes[i])
			return false;
	
	return true;
}
