#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>

// FBX file reader

class FbxRecord;
class FbxReader;

/* --------------------------------------------------------------------------------
// FbxRecord
// --------------------------------------------------------------------------------

FbxRecord is similar to an XML element node. firstChild and nextSibling provide access to neighboring records.
Each record may have zero or more unnamed properties. Each property has a distinct type (int, string, etc), but easy access
is provided through the varying type FbxValue, and implicit conversion through captureProperties.

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
	
	int32_t m_startOffset;
	int32_t m_propertyListOffset;
	int32_t m_parentEndOffset;
	
	explicit FbxRecord();
	explicit FbxRecord(const FbxReader & reader, int32_t startOffset, int32_t parentEndOffset);
	
	void read();
	
public:

	// header values
	int32_t endOffset;
	int32_t numProperties;
	int32_t propertyListLen;
	int8_t nameLen;
	std::string name;
	
	bool isValid() const;
	FbxRecord firstChild(const char * name = 0) const;	
	FbxRecord nextSibling(const char * name = 0) const;
	template <typename T> std::vector<T> captureProperties() const;
};

/* --------------------------------------------------------------------------------
// FbxValue
// --------------------------------------------------------------------------------

FbxValue is used to capture property values. FbxValue is a varying type, which means it may be a string,
int, float, etc. FbxValue::type specifies the type. Conversion is provided through get<T>(value).

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
	static const std::string kEmptyString;
	
	union
	{
		bool Bool;
		int64_t Int;
		double Real;
	};
	
	std::string String;
	
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
	
	explicit FbxValue()
	{
		type = TYPE_INVALID;
	}
	explicit FbxValue(bool value)
	{
		type = TYPE_BOOL;
		Bool = value;
	}
	explicit FbxValue(int64_t value)
	{
		type = TYPE_INT;
		Int = value;
	}
	explicit FbxValue(double value)
	{
		type = TYPE_REAL;
		Real = value;
	}
	explicit FbxValue(const char * value)
	{
		type = TYPE_STRING;
		String = value;
	}
	
	//
	
	bool isValid() const
	{
		return type != TYPE_INVALID;
	}
	
	bool getBool() const
	{
		if (type == TYPE_BOOL)
			return Bool;
		if (type == TYPE_INT)
			return bool(Int);
		return false;
	}
	
	int64_t getInt() const
	{
		if (type == TYPE_BOOL)
			return int64_t(Bool);
		if (type == TYPE_INT)
			return Int;
		if (type == TYPE_REAL)
			return int64_t(Real);
		return 0;
	}
	
	double getDouble() const
	{
		if (type == TYPE_BOOL)
			return double(Bool);
		if (type == TYPE_INT)
			return double(Int);
		if (type == TYPE_REAL)
			return Real;
		return 0.0;
	}
	
	const std::string & getString() const
	{
		if (type == TYPE_STRING)
			return String;
		return kEmptyString;
	}
};

const std::string FbxValue::kEmptyString;

template <typename T> T get(const FbxValue & value);
template <> FbxValue    get(const FbxValue & value) { return value; }
template <> bool        get(const FbxValue & value) { return value.getBool(); }
template <> int         get(const FbxValue & value) { return int(value.getInt()); }
template <> int64_t     get(const FbxValue & value) { return value.getInt(); }
template <> float       get(const FbxValue & value) { return float(value.getDouble()); }
template <> double      get(const FbxValue & value) { return value.getDouble(); }
template <> std::string get(const FbxValue & value) { return value.getString(); }

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
	int m_numBytes;
	int32_t m_firstRecordOffset;
	
	void throwException() const
	{
		throw std::exception();
	}
	
	template <typename T> void read(int & offset, T & result) const
	{
		if (offset + int(sizeof(T)) > m_numBytes)
			throwException();
		result = *(T*)&m_bytes[offset];
		offset += sizeof(T);
	}
	
	template <typename T> void read(int & offset, T & result, int numBytes) const
	{
		if (offset + numBytes > m_numBytes)
			throwException();
		memcpy(&result, &m_bytes[offset], numBytes);
		offset += numBytes;
	}
	
	void skip(int & offset, int numBytes) const
	{
		if (offset + numBytes > m_numBytes)
			throwException();
		offset += numBytes;
	}
	
	void seek(int & offset, int newOffset) const
	{
		if (newOffset < 0 || newOffset > m_numBytes)
			throwException();
		offset = newOffset;
	}
	
	template <typename T> void skipArray(int & offset) const
	{
		// array description
		
		int32_t arrayLength;
		int32_t encoding;
		int32_t compressedLength;
		
		read(offset, arrayLength);
		read(offset, encoding);
		read(offset, compressedLength);
		
		// calculate length
		
		int length;
		if (encoding == 0) // raw
			length = sizeof(T) * arrayLength;
		if (encoding == 1) // deflate
			length = compressedLength;
		
		skip(offset, length);
	}
	
	void readValue(int & offset, FbxValue & value) const
	{
		// property type
		
		char type;
		read(offset, type);
		
		switch (type)
		{
			// scalars
			
		#define READ_SCALAR(code, type, valueType) case code: { type v; read(offset, v); value = FbxValue(valueType(v)); break; }
			
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
				read(offset, length);
				char * str = (char*)alloca(length + 1);
				read(offset, str[0], length);
				str[length] = 0;
				value = FbxValue(str);
				break;
			}
			
			case 'R':
			{
				int32_t length;
				read(offset, length);
				
			#if 1
				skip(offset, length);
			#else
				uint8_t * raw = (uint8_t*)alloca(length);
				read(offset, raw[0], length);
			#endif
				break;
			}
		}
	}
	
public:
	FbxReader()
	{
		m_bytes = 0;
		m_numBytes = 0;
		m_firstRecordOffset = 0;
	}
	
	void openFromMemory(const void * bytes, int numBytes);
	FbxRecord firstRecord(const char * name = 0) const;
};

//

FbxRecord::FbxRecord()
	: m_reader(0)
	, endOffset(0)
{
}

FbxRecord::FbxRecord(const FbxReader & reader, int32_t startOffset, int32_t parentEndOffset)
	: m_reader(&reader)
	, m_startOffset(startOffset)
	, m_parentEndOffset(parentEndOffset)
{
	read();
}

void FbxRecord::read()
{
	int offset = 0;
	
	m_reader->seek(offset, m_startOffset);
	
	m_reader->read(offset, endOffset);
	m_reader->read(offset, numProperties);
	m_reader->read(offset, propertyListLen);
	m_reader->read(offset, nameLen);
	
	char * name = (char*)alloca(nameLen + 1);
	m_reader->read(offset, name[0], nameLen);
	name[nameLen] = 0;
	this->name = name;
	
	m_propertyListOffset = offset;
}

bool FbxRecord::isValid() const
{
	return endOffset != 0;
}

FbxRecord FbxRecord::firstChild(const char * name) const
{
	assert(isValid());
	
	int offset = m_propertyListOffset + propertyListLen;
	
	while (offset != endOffset)
	{
		assert(offset < endOffset);
			
		FbxRecord childRecord(*m_reader, offset, endOffset);
		
		if (!childRecord.isValid())
		{
			break;
		}
		else
		{
			if (name == 0 || childRecord.name == name)
			{
				return childRecord;
			}
			
			offset = childRecord.endOffset;
		}
	}
	
	return FbxRecord();
}

FbxRecord FbxRecord::nextSibling(const char * name) const
{
	assert(isValid());
	
	int offset = endOffset;
	
	while (offset != m_parentEndOffset)
	{
		assert(offset < m_parentEndOffset);
		
		FbxRecord childRecord(*m_reader, offset, m_parentEndOffset);
		
		if (!childRecord.isValid())
		{
			break;
		}
		else
		{
			if (name == 0 || childRecord.name == name)
			{
				return childRecord;
			}
			
			offset = childRecord.endOffset;
		}
	}
	
	return FbxRecord();
}

template <typename T>
std::vector<T> FbxRecord::captureProperties() const
{
	assert(isValid());
	
	std::vector<T> result;
	
	if (isValid())
	{
		result.resize(numProperties);
		
		int offset = m_propertyListOffset;
		
		for (int i = 0; i < numProperties; ++i)
		{
			FbxValue value;
			
			m_reader->readValue(offset, value);
			
			result[i] = get<T>(value);
		}
	}
	
	return result;
}

//

void FbxReader::openFromMemory(const void * bytes, int numBytes)
{
	m_bytes = (uint8_t*)bytes;
	m_numBytes = numBytes;
	
	int offset = 0;
	
	// identifier string
	
	static const char kMagic[21] = "Kaydara FBX Binary  ";
	char magic[sizeof(kMagic)];
	read(offset, magic[0], sizeof(kMagic));
	if (strcmp(magic, kMagic))
		return; // not a binary FBX file
	
	// unknown bytes. should be (0x1a, 0x00)
	
	char unknown[2];
	read(offset, unknown[0], 2);
	
	// version number
	
	int32_t version;
	read(offset, version);
	
	// remember the offset of the first top-level record
	
	m_firstRecordOffset = offset;
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

// usage example

#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

class FbxFileLogger
{
	int m_logIndent;
	
	const FbxReader * m_reader;
	
public:
	FbxFileLogger(const FbxReader & reader)
	{
		m_logIndent = 0;
		
		m_reader = &reader;
	}
	
	void dumpFileContents()
	{
		for (FbxRecord record = m_reader->firstRecord(); record.isValid(); record = record.nextSibling())
		{
			dumpRecord(record);
		}
	}
	
	void dumpRecord(const FbxRecord & record)
	{
		log("node: endOffset=%d, numProperties=%d, propertyListLen=%d, nameLen=%d, name=%s\n",
			record.endOffset,
			record.numProperties,
			record.propertyListLen,
			(int32_t)record.nameLen,
			record.name.c_str());
		
		m_logIndent++;
		
		std::vector<FbxValue> properties = record.captureProperties<FbxValue>();
		
		for (size_t i = 0; i < properties.size(); ++i)
		{
			dumpProperty(properties[i]);
		}
		
		for (FbxRecord childRecord = record.firstChild(); childRecord.isValid(); childRecord = childRecord.nextSibling())
		{
			dumpRecord(childRecord);
		}
		
		m_logIndent--;
	}
	
	void dumpProperty(const FbxValue & value)
	{
		switch (value.type)
		{
			case FbxValue::TYPE_BOOL:
				log("bool: %d\n", get<bool>(value));
				break;
			case FbxValue::TYPE_INT:
				log("int: %lld\n", get<int64_t>(value));
				break;
			case FbxValue::TYPE_REAL:
				log("float: %f\n", get<float>(value));
				break;
			case FbxValue::TYPE_STRING:
				log("string: %s\n", value.getString().c_str());
				break;
			
			case FbxValue::TYPE_INVALID:
				log("(invalid)\n");
				break;
		}
	}
	
	void log(const char * fmt, ...)
	{
		va_list va;
		va_start(va, fmt);
		
		char tabs[128];
		for (int i = 0; i < m_logIndent; ++i)
			tabs[i] = '\t';
		tabs[m_logIndent] = 0;
		
		char temp[1024];
		vsprintf(temp, fmt, va);
		va_end(va);
		
		printf("%s%s", tabs, temp);
	}
};

class Mesh
{
public:
	std::vector<float> vertices;
	std::vector<int> indices;
};

int main()
{
	// read file contents
	
	FILE * file = fopen("test.fbx", "rb");
	const int p1 = ftell(file);
	fseek(file, 0, SEEK_END);
	const int p2 = ftell(file);
	fseek(file, 0, SEEK_SET);
	const int numBytes = p2 - p1;
	void * bytes = malloc(numBytes);
	fread(bytes, numBytes, 1, file);
	fclose(file);
	
	// parse
	
	FbxReader reader;
	reader.openFromMemory(bytes, numBytes);
	
	FbxFileLogger logger(reader);
	logger.dumpFileContents();
	
	std::list<Mesh> meshes;
	
	for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
	{
		// Model, Pose, Material, Texture, ..
		
		for (FbxRecord model = objects.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
		{
			std::vector<std::string> modelProps = model.captureProperties<std::string>();
			
			if (modelProps.size() >= 2 && modelProps[1] == "Mesh")
			{
				printf("Mesh!\n");
				
				meshes.push_back(Mesh());
				
				Mesh & mesh = meshes.back();
				
				const FbxRecord vertices = model.firstChild("Vertices");
				const FbxRecord indices = model.firstChild("PolygonVertexIndex");
				
				if (vertices.isValid())
					mesh.vertices = vertices.captureProperties<float>();
				
				if (indices.isValid())
					mesh.indices = indices.captureProperties<int>();
				
				// LayerElementNormal.Normals
				// LayerElementUV.UV
				// LayerElementMaterial.Materials
				// LayerElementTexture.TextureId
			}
		}
	}
	
	free(bytes);
	
	// show result
	
	for (std::list<Mesh>::iterator i = meshes.begin(); i != meshes.end(); ++i)
	{
		const Mesh & mesh = *i;
		
		logger.log("mesh: numVertices=%d, numIndices=%d\n", int(mesh.vertices.size()), int(mesh.indices.size()));
		
		for (size_t i = 0; i < mesh.indices.size(); ++i)
		{
			int index = mesh.indices[i];
			
			bool end = false;
			
			if (index < 0)
			{
				index = ~index;
				end = true;
			}
			
			logger.log("[%d] (%+4.2f %+4.2f %+4.2f) ",
				index,
				mesh.vertices[index*3+0],
				mesh.vertices[index*3+1],
				mesh.vertices[index*3+2]);
			
			if (end)
				logger.log("\n");
		}
	}
	
	return 0;
}
