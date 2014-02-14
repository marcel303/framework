#pragma once

/* --------------------------------------------------------------------------------
// FBX file reader
// --------------------------------------------------------------------------------

	This work is licensed under a Creative Commons Attribution 3.0 Unported License.
		http://creativecommons.org/licenses/by/3.0/
	
	Original code by Marcel Smit [marcel303 (at) gmail.com]
	
You are free to:

	Share — copy and redistribute the material in any medium or format
	Adapt — remix, transform, and build upon the material for any purpose, even commercially.
	The licensor cannot revoke these freedoms as long as you follow the license terms.

Under the following terms:

	Attribution — You must give appropriate credit, provide a link to the license, and indicate if changes were made. You may do so in any reasonable manner, but not in any way that suggests the licensor endorses you or your use.
	No additional restrictions — You may not apply legal terms or technological measures that legally restrict others from doing anything the license permits.

Notices:

	You do not have to comply with the license for elements of the material in the public domain or where your use is permitted by an applicable exception or limitation.
	No warranties are given. The license may not give you all of the permissions necessary for your intended use. For example, other rights such as publicity, privacy, or moral rights may limit how you use the material.

*/

#include <stdint.h>
#include <string>
#include <vector>

// --------------------------------------------------------------------------------

class FbxRecord;
class FbxReader;
class FbxValue;

/* --------------------------------------------------------------------------------
// FbxRecord
// --------------------------------------------------------------------------------

FbxRecord is similar to an XML element node, in that it may have child and sibling nodes and has a list of
properties (attributes). firstChild and nextSibling provide access to neighboring records. Each record may
have zero or more unnamed properties. Each property has a distinct type (int, string, etc), but easy access
is provided through the varying value type FbxValue. Access to a record's properties is provided through
captureProperties<T>, which collects all properties into a single std::vector of type T.

Example:
	FbxReader reader;
	reader.openFromMemory(bytes, numBytes);
	
	for (FbxRecord record = reader.firstRecord("Objects"); record.isValid(); record = record.nextSibling("Objects"))
	{
		std::vector<float> propertiesAsFloat = record.captureProperties<float>();
		
		std::vector<FbxValue> propertiesAsVarying = record.captureProperties<FbxValue>();
	}
*/

class FbxRecord
{
	friend class FbxReader;
	
	const FbxReader * m_reader;
	
	size_t m_startOffset;
	size_t m_endOffset;
	size_t m_parentEndOffset;
	size_t m_numProperties;
	size_t m_propertyListOffset;
	size_t m_propertyListLen;
	
	explicit FbxRecord();
	explicit FbxRecord(const FbxReader & reader, size_t startOffset, size_t parentEndOffset);
	
	void read();
	void findFirstSibling(const char * name);
	
public:
	std::string name;
	
	bool isValid() const;
	FbxRecord firstChild(const char * name = 0) const;
	FbxRecord nextSibling(const char * name = 0) const;
	template <typename T> inline std::vector<T> captureProperties() const;
	template <typename T> inline T captureProperty(int index) const;
	void capturePropertiesAsInt(std::vector<int> & result) const;
	void capturePropertiesAsFloat(std::vector<float> & result) const;
	
	inline size_t getStartOffset() const { return m_startOffset; }
	inline size_t getEndOffset() const { return m_endOffset; }
	inline size_t getNumProperties() const { return m_numProperties; }
};

/* --------------------------------------------------------------------------------
// FbxValue
// --------------------------------------------------------------------------------

FbxValue is used to capture property values. FbxValue is a varying type, which means it may be a string,
int, float, etc. FbxValue::type specifies the type. Conversion is provided through get<T>(value) methods.

Example:
	std::vector<FbxValue> properties = record.captureProperties<FbxValue>();
	
	for (size_t i = 0; i < properties.size(); ++i)
	{
		const float v = get<float>(properties[i]);
		
		printf("value: %g\n", v);
	}
*/

class FbxValue
{
	union
	{
		bool Bool;
		int64_t Int;
		double Real;
		char * String;
	};
	
public:
	enum TYPE
	{
		TYPE_INVALID,
		TYPE_BOOL,
		TYPE_INT,
		TYPE_REAL,
		TYPE_STRING
	};
	
	TYPE type;
	
	explicit FbxValue();
	FbxValue(const FbxValue & value);
	explicit FbxValue(bool value);
	explicit FbxValue(int64_t value);
	explicit FbxValue(double value);
	explicit FbxValue(const char * value);
	~FbxValue();
	
	FbxValue & operator=(const FbxValue & value);

	bool isValid() const;
	bool getBool() const;
	int64_t getInt() const;
	double getDouble() const;
	const char * getString() const;
};

template <typename T> T  inline get(const FbxValue & value);
template <> FbxValue     inline get(const FbxValue & value) { return value; }
template <> bool         inline get(const FbxValue & value) { return value.getBool(); }
template <> int          inline get(const FbxValue & value) { return int(value.getInt()); }
template <> int64_t      inline get(const FbxValue & value) { return value.getInt(); }
template <> float        inline get(const FbxValue & value) { return float(value.getDouble()); }
template <> double       inline get(const FbxValue & value) { return value.getDouble(); }
template <> inline const char * get(const FbxValue & value) { return value.getString(); }
template <> inline std::string  get(const FbxValue & value) { return std::string(value.getString()); }

/* --------------------------------------------------------------------------------
// FbxReader
// --------------------------------------------------------------------------------

FbxReader is used to access FBX file contents. It provides methods for opening FBX files, access to the first top-level
FBX record, and helper functions for reading file contents.

*/

class FbxReader
{
	friend class FbxRecord;
	
	const uint8_t * m_bytes;
	size_t m_numBytes;
	size_t m_firstRecordOffset;
	
	void throwException() const;
	
	template <typename T> void read(size_t & offset, T & result) const;
	template <typename T> void read(size_t & offset, T & result, size_t numBytes) const;
	void skip(size_t & offset, size_t numBytes) const;
	void seek(size_t & offset, size_t newOffset) const;
	template <typename T> void skipArray(size_t & offset) const;
	void readPropertyValue(size_t & offset, FbxValue & value) const;
	
public:
	FbxReader();
	
	void openFromMemory(const void * bytes, size_t numBytes);
	FbxRecord firstRecord(const char * name = 0) const;
};

// --------------------------------------------------------------------------------

template <typename T>
inline std::vector<T> FbxRecord::captureProperties() const
{	
	std::vector<T> result;
	
	if (isValid())
	{
		result.resize(m_numProperties);
		
		size_t offset = m_propertyListOffset;
		
		for (size_t i = 0; i < m_numProperties; ++i)
		{
			FbxValue value;
			
			m_reader->readPropertyValue(offset, value);
			
			result[i] = get<T>(value);
		}
	}
	
	return result;
}

template <typename T>
inline T FbxRecord::captureProperty(int index) const
{
	if (isValid())
	{
		size_t offset = m_propertyListOffset;
		
		for (size_t i = 0; i < m_numProperties; ++i)
		{
			FbxValue value;
			
			m_reader->readPropertyValue(offset, value);
			
			if (int(i) == index)
			{
				return get<T>(value);
			}
		}
	}
	
	return T();
}
