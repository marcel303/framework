#include "fbx.h"
#include "miniz.h"
#include <assert.h>
#include <limits>
#include <memory>
#include <string.h>

// --------------------------------------------------------------------------------
// FbxRecord
// --------------------------------------------------------------------------------

FbxRecord::FbxRecord()
	: m_reader(0)
	, m_endOffset(0)
{
}

FbxRecord::FbxRecord(const FbxReader & reader, size_t startOffset, size_t parentEndOffset)
	: m_reader(&reader)
	, m_startOffset(startOffset)
	, m_parentEndOffset(parentEndOffset)
{
	read();
}

void FbxRecord::read()
{
	// read header
	
	size_t offset = 0;
	
	m_reader->seek(offset, m_startOffset);
	
	int32_t endOffset;
	int32_t numProperties;
	int32_t propertyListLen;
	int8_t nameLen;
	
	m_reader->read(true, offset, endOffset);
	m_reader->read(true, offset, numProperties);
	m_reader->read(true, offset, propertyListLen);
	m_reader->read(false, offset, nameLen);
	
	// read name
	
	char * name = (char*)alloca(nameLen + 1);
	m_reader->read(offset, name[0], nameLen);
	name[nameLen] = 0;
	
	// fill members
	
	m_endOffset = endOffset;
	m_numProperties = numProperties;
	m_propertyListLen = propertyListLen;
	m_propertyListOffset = offset;
	this->name = name;
}

void FbxRecord::findFirstSibling(const char * name)
{
	assert(m_startOffset != 0);
	
	// seek to the first node (within the same level) with the given name. if the record
	// cannot be found, the record will be marked as invalid (isValid() will return false)
	
	if (m_startOffset != m_parentEndOffset)
	{
		do
		{
			assert(m_startOffset < m_parentEndOffset);
			
			read();
			
			// FBX null terminator?
			
			if (!isValid())
				return;
			
			// found match?
			
			if (name == 0 || this->name == name)
				return;
			
			// skip to next record
			
			m_startOffset = m_endOffset;
		}
		while (m_endOffset != m_parentEndOffset);
	}
	
	// no record found. mark record invalid
	
	m_endOffset = 0;
}

bool FbxRecord::isValid() const
{
	return m_endOffset != 0;
}

FbxRecord FbxRecord::firstChild(const char * name) const
{
	if (!isValid())
	{
		return *this;
	}
	else
	{
		// start at the end of the property list, which marks the start of the first child record
		
		FbxRecord result(*m_reader, m_propertyListOffset + m_propertyListLen, m_endOffset);
		
		result.findFirstSibling(name);
		
		return result;
	}
}

FbxRecord FbxRecord::nextSibling(const char * name) const
{
	if (!isValid())
	{
		return *this;
	}
	else
	{
		// start at our own end offset
		
		FbxRecord result(*m_reader, m_endOffset, m_parentEndOffset);
		
		result.findFirstSibling(name);
		
		return result;
	}
}

//

void FbxRecord::capturePropertiesAsInt(std::vector<int> & result) const
{	
	if (isValid())
	{
		result.resize(m_numProperties);
		
		size_t offset = m_propertyListOffset;
		
		for (size_t i = 0; i < m_numProperties; ++i)
		{
			size_t startOffset = offset;
			
			// property type
			
			char type;
			m_reader->read(false, offset, type);
			
			if (type == 'Y')
			{
				int16_t v;
				m_reader->read(false, offset, v);
				result[i] = int(v);
			}
			else if (type == 'I')
			{
				m_reader->read(false, offset, result[i]);
			}
			else if (type == 'L')
			{
				int64_t v;
				m_reader->read(false, offset, v);
				result[i] = int(v);
			}
			else
			{
				offset = startOffset;
				
				FbxValue value;
				
				m_reader->readPropertyValue(offset, value);
				
				result[i] = get<int>(value);
			}
		}
	}
	else
	{
		result.clear();
	}
}

void FbxRecord::capturePropertiesAsFloat(std::vector<float> & result) const
{	
	if (isValid())
	{
		result.resize(m_numProperties);
		
		size_t offset = m_propertyListOffset;
		
		for (size_t i = 0; i < m_numProperties; ++i)
		{
			size_t startOffset = offset;
			
			// property type
			
			char type;
			m_reader->read(false, offset, type);
			
			if (type == 'F')
			{
				m_reader->read(false, offset, result[i]);
			}
			else if (type == 'D')
			{
				double v;
				m_reader->read(false, offset, v);
				result[i] = float(v);
			}
			else
			{
				offset = startOffset;
				
				FbxValue value;
				
				m_reader->readPropertyValue(offset, value);
				
				result[i] = get<float>(value);
			}
		}
	}
	else
	{
		result.clear();
	}
}

// --------------------------------------------------------------------------------
// FbxValue
// --------------------------------------------------------------------------------

FbxValue::FbxValue()
{
	type = TYPE_INVALID;
}

FbxValue::FbxValue(const FbxValue & value)
{
	type = TYPE_INVALID;
	*this = value;
}

FbxValue::FbxValue(bool value)
{
	type = TYPE_BOOL;
	Bool = value;
}

FbxValue::FbxValue(int64_t value)
{
	type = TYPE_INT;
	Int = value;
}

FbxValue::FbxValue(double value)
{
	type = TYPE_REAL;
	Real = value;
}

FbxValue::FbxValue(const char * value)
{
	type = TYPE_STRING;

	const int length = strlen(value) + 1;
	String = new char[length];
	memcpy(String, value, length);
}

FbxValue::~FbxValue()
{
	if (type == TYPE_STRING)
		delete [] String;
}

FbxValue & FbxValue::operator=(const FbxValue & value)
{
	if (type == TYPE_STRING)
		delete [] String;

	type = value.type;
	Int = value.Int;

	if (type == TYPE_STRING)
	{
		const int length = strlen(value.String) + 1;
		String = new char[length];
		memcpy(String, value.String, length);
	}

	return *this;
}

bool FbxValue::isValid() const
{
	return type != TYPE_INVALID;
}

bool FbxValue::getBool() const
{
	if (type == TYPE_BOOL)
		return Bool;
	if (type == TYPE_INT)
		return bool(Int != 0);
	return false;
}

int64_t FbxValue::getInt() const
{
	if (type == TYPE_BOOL)
		return int64_t(Bool);
	if (type == TYPE_INT)
		return Int;
	if (type == TYPE_REAL)
		return int64_t(Real);
	return 0;
}

double FbxValue::getDouble() const
{
	if (type == TYPE_BOOL)
		return double(Bool);
	if (type == TYPE_INT)
		return double(Int);
	if (type == TYPE_REAL)
		return Real;
	return 0.0;
}

const char * FbxValue::getString() const
{
	if (type == TYPE_STRING)
		return String;
	return "";
}

bool FbxValue::operator==(const char * str) const
{
	if (type == TYPE_STRING)
		return !strcmp(String, str);
	else
		return false;
}

// --------------------------------------------------------------------------------
// FbxValueArray
// --------------------------------------------------------------------------------

FbxValueArray::FbxValueArray()
{
	type = TYPE_INVALID;
}

bool FbxValueArray::isValid() const
{
	return type != TYPE_INVALID;
}

// --------------------------------------------------------------------------------
// FbxReader
// --------------------------------------------------------------------------------

void FbxReader::throwException() const
{
	throw std::exception();
}

template <typename T> void FbxReader::read(const bool isSize, size_t & offset, T & result) const
{
	if (isSize && m_sizesAre64Bit)
	{
		int64_t result64;
		
		if (offset + sizeof(result64) > m_numBytes)
		{
			memset(&result, 0, sizeof(result));
			throwException();
			return;
		}
		
		result64 = *(int64_t*)&m_bytes[offset];
		offset += sizeof(int64_t);
		
		if (result64 > std::numeric_limits<int32_t>::max())
		{
			memset(&result, 0, sizeof(result));
			throwException();
			return;
		}
		
		result = result64;
	}
	else
	{
		if (offset + sizeof(T) > m_numBytes)
		{
			memset(&result, 0, sizeof(result));
			throwException();
			return;
		}
		
		result = *(T*)&m_bytes[offset];
		offset += sizeof(T);
	}
}

template <typename T> void FbxReader::read(size_t & offset, T & result, size_t numBytes) const
{
	if (offset + numBytes > m_numBytes)
	{
		memset(&result, 0, numBytes);
		throwException();
		return;
	}
	memcpy(&result, &m_bytes[offset], numBytes);
	offset += numBytes;
}

void FbxReader::skip(size_t & offset, size_t numBytes) const
{
	if (offset + numBytes > m_numBytes)
		throwException();
	offset += numBytes;
}

void FbxReader::seek(size_t & offset, size_t newOffset) const
{
	if (newOffset > m_numBytes)
		throwException();
	offset = newOffset;
}

template <typename T> void FbxReader::skipArray(size_t & offset) const
{
	// array description
	
	int32_t arrayLength;
	int32_t encoding;
	int32_t compressedLength;
	
	read(false, offset, arrayLength);
	read(false, offset, encoding);
	read(false, offset, compressedLength);
	
	// calculate length
	
	size_t length;
	
	if (encoding == 0) // raw
		length = sizeof(T) * arrayLength;
	if (encoding == 1) // deflate
		length = compressedLength;
	
	skip(offset, length);
}

static bool decompress(const void * __restrict src, const int32_t srcSize, void * __restrict dst, const int32_t dstSize)
{
	mz_stream stream = { };
	mz_inflateInit(&stream);

	stream.next_in = (const uint8_t*)src;
	stream.avail_in = (int)srcSize;
	
	stream.next_out = (uint8_t*)dst;
	stream.avail_out = (int)dstSize;
	
	bool result = true;
	
	if (mz_inflate(&stream, MZ_FINISH) != Z_STREAM_END)
		result = false;

	if (mz_inflateEnd(&stream) != Z_OK)
		result = false;

	return result;
}

template <typename T> void FbxReader::readArray(size_t & offset, FbxValueArray & valueArray) const
{
	// array description
	
	int32_t arrayLength;
	int32_t encoding;
	int32_t compressedLength;
	
	read(false, offset, arrayLength);
	read(false, offset, encoding);
	read(false, offset, compressedLength);
	
	// read values
	
	std::auto_ptr<T> temp(new T[arrayLength]);
	
	T * __restrict values = temp.get();
	
	if (encoding == 0) // raw
	{
		const size_t numBytes = sizeof(T) * arrayLength;
		
		read(offset, values, numBytes);
	}
	else if (encoding == 1) // deflate
	{
		if (offset + compressedLength > m_numBytes)
		{
			throwException();
		}
		
		if (!decompress(m_bytes + offset, compressedLength, values, sizeof(T) * arrayLength))
		{
			throwException();
		}
		
		offset += compressedLength;
	}
	else
	{
		throwException();
	}
	
	// convert to FbxValueArray
	
	valueArray.values.resize(arrayLength);
	
	for (int i = 0; i < arrayLength; ++i)
	{
		valueArray.values[i] = double(values[i]);
	}
}

void FbxReader::readPropertyValue(size_t & offset, FbxValue & value) const
{
	// property type
	
	char type;
	read(false, offset, type);
	
	switch (type)
	{
		// scalars
		
	#define READ_SCALAR(code, type, valueType) case code: { type v; read(false, offset, v); value = FbxValue(valueType(v)); break; }
		
		READ_SCALAR('C', int8_t, bool)
		READ_SCALAR('Y', int16_t, int64_t)
		READ_SCALAR('I', int32_t, int64_t)
		READ_SCALAR('L', int64_t, int64_t)
		READ_SCALAR('F', float, double)
		READ_SCALAR('D', double, double)
		
	#undef READ_SCALAR
		
		// arrays
		
	#define SKIP_ARRAY(code, type) case code: { skipArray<type>(offset); break; }
		
		SKIP_ARRAY('b', int8_t)
		SKIP_ARRAY('i', int32_t)
		SKIP_ARRAY('l', int64_t)
		SKIP_ARRAY('f', float)
		SKIP_ARRAY('d', double)
		
	#undef SKIP_ARRAY
		
		// special
		
		case 'S':
		{
			int32_t length;
			read(false, offset, length);
			char * str = (char*)alloca(length + 1);
			read(offset, str[0], length);
			str[length] = 0;
			value = FbxValue(str);
			break;
		}
		
		case 'R':
		{
			int32_t length;
			read(false, offset, length);
			
		#if 1
			skip(offset, length);
		#else
			uint8_t * raw = (uint8_t*)alloca(length);
			read(offset, raw[0], length);
		#endif
			break;
		}
		
		default:
		{
			throwException(); // unknown property type
			break;
		}
	}
}

void FbxReader::readPropertyArray(size_t & offset, FbxValueArray & value) const
{
	// property type
	
	char type;
	read(false, offset, type);
	
	switch (type)
	{
		// arrays
		
	#define READ_ARRAY(code, type) case code: { readArray<type>(offset, value); break; }
		
		READ_ARRAY('b', int8_t)
		READ_ARRAY('i', int32_t)
		READ_ARRAY('l', int64_t)
		READ_ARRAY('f', float)
		READ_ARRAY('d', double)
		
	#undef READ_ARRAY
		
		default:
		{
			throwException(); // unknown property type
			break;
		}
	}
}

FbxReader::FbxReader()
{
	m_bytes = 0;
	m_numBytes = 0;
	m_firstRecordOffset = 0;
	
	m_version = 0;
	m_sizesAre64Bit = false;
}

void FbxReader::openFromMemory(const void * bytes, size_t numBytes)
{
	m_bytes = (uint8_t*)bytes;
	m_numBytes = numBytes;
	
	size_t offset = 0;
	
	// identifier string
	
	static const char kMagic[21] = "Kaydara FBX Binary  ";
	char magic[sizeof(kMagic)];
	read(offset, magic[0], sizeof(kMagic));
	if (strcmp(magic, kMagic))
	{
		throwException(); // not a binary FBX file
		return;
	}
	
	// unknown bytes. should be (0x1a, 0x00)
	
	char unknown[2];
	read(offset, unknown[0], 2);
	
	// version number
	
	read(false, offset, m_version);
	
	// remember the offset of the first top-level record
	
	m_firstRecordOffset = offset;
	
	// version 7500+ introduced 64 bit sizes for certain integers, breaking backward compatibility with older formats
	
	if (m_version >= 7500)
		m_sizesAre64Bit = true;
}

FbxRecord FbxReader::firstRecord(const char * name) const
{
	FbxRecord record(*this, m_firstRecordOffset, m_numBytes);
	
	if (name != 0 && record.name != name)
	{
		record = record.nextSibling(name);
	}
	
	return record;
}
