#include <assert.h>
#include <stdint.h>
#include <string>

// FBX file reader

class FbxNext;
class FbxReceiver;
class FbxRecord;
class FbxParser;

struct FbxRecord
{
	int32_t endOffset;
	int32_t numProperties;
	int32_t propertyListLen;
	int8_t nameLen;
	
	std::string name;
	
	bool isNULL() const
	{
		return endOffset == 0;
	}
};

class FbxValue
{
	union
	{
		bool Bool;
		int64_t Int;
		double Real;
	};
	
	std::string String;
	
	static const std::string kEmptyString;
	
public:
	enum TYPE
	{
		TYPE_NULL,
		TYPE_BOOL,
		TYPE_INT,
		TYPE_REAL,
		TYPE_STRING
	};
	
	TYPE type;
	
	explicit FbxValue()
	{
		type = TYPE_NULL;
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
	
	bool isNULL() const
	{
		return type == TYPE_NULL;
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

template <typename T> T         get(const FbxValue & value);
template <> bool                get(const FbxValue & value) { return value.getBool(); }
template <> int                 get(const FbxValue & value) { return int(value.getInt()); }
template <> int64_t             get(const FbxValue & value) { return value.getInt(); }
template <> float               get(const FbxValue & value) { return float(value.getDouble()); }
template <> double              get(const FbxValue & value) { return value.getDouble(); }
template <> const std::string & get(const FbxValue & value) { return value.getString(); }

struct FbxNext
{
	bool recurse;
	bool properties;
	
	FbxNext()
	{
		recurse = true;
		properties = false;
	}
};

class FbxReceiver
{
public:
	virtual void recordBegin(const FbxRecord & record, FbxNext & next) = 0;
	virtual void recordEnd() = 0;
	virtual void property(int index, const FbxValue & value) = 0;
};

class FbxParser
{
	const uint8_t * m_bytes;
	const uint8_t * m_bytePtr;
	int m_numBytes;
	FbxReceiver * m_receiver;
	
	void parseRecord(const FbxRecord & record);
	
	template <typename T> void read(T & result)
	{
		result = *(T*)m_bytePtr;
		m_bytePtr += sizeof(T);
	}
	
	template <typename T> void read(T & result, int numBytes)
	{
		memcpy(&result, m_bytePtr, numBytes);
		m_bytePtr += numBytes;
	}
	
	void skip(int numBytes)
	{
		m_bytePtr += numBytes;
	}
	
	template <typename T> void skipArray()
	{
		// array description
		
		int32_t arrayLength;
		int32_t encoding;
		int32_t compressedLength;
		
		read(arrayLength);
		read(encoding);
		read(compressedLength);
		
		// calculate length
		
		int length;
		if (encoding == 0) // raw
			length = sizeof(T) * arrayLength;
		if (encoding == 1) // deflate
			length = compressedLength;
		
		skip(length);
	}
	
	void readRecord(FbxRecord & record)
	{
		read(record.endOffset);
		read(record.numProperties);
		read(record.propertyListLen);
		read(record.nameLen);
		
		char * name = (char*)alloca(record.nameLen + 1);
		read(name[0], record.nameLen);
		name[record.nameLen] = 0;
		record.name = name;
	}
	
	void readValue(FbxValue & value)
	{
		// property type
		
		char type;
		read(type);
		
		switch (type)
		{
			// scalars
			
		#define READ_SCALAR(code, type, valueType) case code: { type v; read(v); value = FbxValue(valueType(v)); break; }
			
			READ_SCALAR('C', int8_t, bool)
			READ_SCALAR('Y', int16_t, int64_t)
			READ_SCALAR('I', int32_t, int64_t)
			READ_SCALAR('L', int64_t, int64_t)
			READ_SCALAR('F', float, double)
			READ_SCALAR('D', double, double)
			
		#undef READ_SCALAR
			
			// arrays
			
		#define SKIP_ARRAY(code, type) case code: { skipArray<type>(); break; }
			
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
				read(length);
				char * str = (char*)alloca(length + 1);
				read(str[0], length);
				str[length] = 0;
				value = FbxValue(str);
				break;
			}
			
			case 'R':
			{
				int32_t length;
				read(length);
				
			#if 1
				skip(length);
			#else
				uint8_t * raw = (uint8_t*)alloca(length);
				read(raw[0], length);
			#endif
				break;
			}
		}
	}
	
	int distance() const
	{
		return m_bytePtr - m_bytes;
	}
	
public:
	FbxParser()
	{
		m_bytes = 0;
		m_bytePtr = 0;
		m_numBytes = 0;
		m_receiver = 0;
	}
	
	void parse(const void * bytes, int numBytes, FbxReceiver & receiver);
	template <typename T> void capture(const FbxRecord & record, T * buffer);
};

void FbxParser::parseRecord(const FbxRecord & record)
{
	FbxNext next;
	
	m_receiver->recordBegin(record, next);
	
	if (next.properties)
	{
		// properties
		
		for (int i = 0; i < record.numProperties; ++i)
		{
			FbxValue value;
			
			readValue(value);
			
			m_receiver->property(i, value);
		}
	}
	else
	{
		skip(record.propertyListLen);
	}
	
	if (next.recurse)
	{
		// nested records
		
		while (distance() != record.endOffset)
		{		
			assert(distance() < record.endOffset);
			
			FbxRecord childRecord;
			readRecord(childRecord);
			
			if (childRecord.isNULL())
			{
				// end of record
				
				assert(distance() == record.endOffset);
				
				break;
			}
			else
			{
				parseRecord(childRecord);
			}
		}
	}
	else
	{
		skip(m_bytes + record.endOffset - m_bytePtr);
	}
	
	m_receiver->recordEnd();
}

void FbxParser::parse(const void * bytes, int numBytes, FbxReceiver & receiver)
{
	m_bytes = (uint8_t*)bytes;
	m_numBytes = numBytes;
	m_bytePtr = (uint8_t*)bytes;
	m_receiver = &receiver;
	
	// header: identifier
	
	const char kMagic[21] = "Kaydara FBX Binary  ";
	char magic[sizeof(kMagic)];
	read(magic[0], sizeof(kMagic));
	for (size_t i = 0; i < sizeof(kMagic); ++i)
	{
		if (magic[i] != kMagic[i])
		{
			// not a binary FBX file
			
			return;
		}
	}
	
	// header: unknown
	
	char special[2];
	read(special[0], 2); // should be 0x1a 0x00
	
	// header: version
	
	int32_t version;
	read(version);
	
	// top level records
	
	while (distance() != m_numBytes)
	{
		FbxRecord record;
		
		readRecord(record);
		
		if (record.isNULL())
		{
			// end of file
			
			break;
		}
		else
		{
			parseRecord(record);
		}
	}
}

template <typename T>
void FbxParser::capture(const FbxRecord & record, T * buffer)
{
	const uint8_t * oldBytePtr = m_bytePtr;
	{
		for (int i = 0; i < record.numProperties; ++i)
		{
			FbxValue value;
			
			readValue(value);
			
			buffer[i] = get<T>(value);
		}
	}
	m_bytePtr = oldBytePtr;
}

// usage example

#include <stdio.h>
#include <stdlib.h>
#include <vector>

class MyMesh
{
public:
	int numVertices;
	float * vertices;
	
	int numIndices;
	int * indices;
	
	MyMesh()
	{
		numVertices = 0;
		vertices = 0;
		
		numIndices = 0;
		indices = 0;
	}
	
	~MyMesh()
	{
		delete [] vertices;
		delete [] indices;
	}
};

class MyReceiver : public FbxReceiver
{
	static const int maxDepth = 128;
	
	enum STATE
	{
		STATE_ROOT,
		STATE_OBJECTS,
		STATE_MODEL,
		STATE_MODEL_MESH,
		/*
		STATE_MODEL_NORMALS,
		STATE_MODEL_UVS,
		STATE_MODEL_MATERIALS,
		STATE_MODEL_TEXTURES,
		*/
		STATE_UNKNOWN
	};
		
	int m_logIndent;
	
	FbxParser * m_parser;
	
	STATE m_state[maxDepth];
	int m_depth;
	
	MyMesh * m_currentMesh;
	
public:
	std::vector<MyMesh*> meshes;
	
	MyReceiver(FbxParser & parser)
	{
		m_logIndent = 0;
		
		m_parser = &parser;
		
		m_depth = 0;
		m_state[0] = STATE_ROOT;
		
		m_currentMesh = 0;
	}
	
	virtual void recordBegin(const FbxRecord & record, FbxNext & next)
	{
		log("node: endOffset=%d, numProperties=%d, propertyListLen=%d, nameLen=%d, name=%s\n",
			record.endOffset,
			record.numProperties,
			record.propertyListLen,
			(int32_t)record.nameLen,
			record.name.c_str());
		
		m_logIndent++;
		
		STATE oldState = m_state[m_depth];
		
		m_depth++;
		
		STATE newState = STATE_UNKNOWN;
		
		switch (oldState)
		{
			case STATE_ROOT:
			{
				if (record.name == "Objects")
				{
					newState = STATE_OBJECTS;
				}
				break;
			}
			
			case STATE_OBJECTS:
			{
				if (record.name == "Model")
				{
					newState = STATE_MODEL;
					next.properties = true;
				}
				if (record.name == "Pose")
				{
					//
				}
				if (record.name == "Material")
				{
					//
				}
				if (record.name == "Texture")
				{
					//
				}
				break;
			}
			
			case STATE_MODEL_MESH:
			{
				if (record.name == "Vertices")
				{
					delete [] m_currentMesh->vertices;
					m_currentMesh->numVertices = record.numProperties;
					m_currentMesh->vertices = new float[record.numProperties];
					m_parser->capture<float>(record, m_currentMesh->vertices);
					next.recurse = false;
				}
				if (record.name == "PolygonVertexIndex")
				{
					delete [] m_currentMesh->indices;
					m_currentMesh->numIndices = record.numProperties;
					m_currentMesh->indices = new int[record.numProperties];
					m_parser->capture<int>(record, m_currentMesh->indices);
					next.recurse = false;
				}
				/*
				if (record.name == "LayerElementNormal")
				{
					newState = STATE_MODEL_NORMALS;
				}
				if (record.name == "LayerElementUV")
				{
					newState = STATE_MODEL_UVS;
				}
				if (record.name == "LayerElementMaterial")
				{
					newState = STATE_MODEL_MATERIALS;
				}
				if (record.name == "LayerElementTexture")
				{
					newState = STATE_MODEL_TEXTURES;
				}
				*/
				break;
			}
			
			/*
			case STATE_MODEL_NORMALS:
			{
				if (record.name == "Normals")
				{
					m_parser->capture<float>(..);
					next.recurse = false;
				}
				break;
			}
			
			case STATE_MODEL_UVS:
			{
				if (record.name == "UV")
				{
					m_parser->capture<float>(..);
					next.recurse = false;
				}
				if (record.name == "UVIndex")
				{
					m_parser->capture<int>(..);
					next.recurse = false;
				}
				break;
			}
			
			case STATE_MODEL_MATERIALS:
			{
				if (record.name == "Materials")
				{
					next.recurse = false;
				}
				break;
			}
			
			case STATE_MODEL_TEXTURES:
			{
				if (record.name == "TextureId")
				{
					next.recurse = false;
				}
				break;
			}
			*/
			
			default:
			{
				break;
			}
		}
		
		m_state[m_depth] = newState;
		
		log("state change %d -> %d\n", oldState, newState);
		
		if (newState == STATE_UNKNOWN)
		{
			#if 0
			// speed up: skip unknown record types
			next.recurse = false;
			next.properties = false;
			#else
			// traverse the entire file for logging purposes
			next.recurse = true;
			next.properties = true;
			#endif
		}
	}
	
	virtual void recordEnd()
	{
		m_logIndent--;
		
		STATE oldState = m_state[m_depth];
		
		switch (oldState)
		{
			case STATE_MODEL_MESH:
			{
				meshes.push_back(m_currentMesh);
				m_currentMesh = 0;
				break;
			}
			
			default:
			{
				break;
			}
		}
		
		m_depth--;
	}
	
	virtual void property(int index, const FbxValue & value)
	{
		// log the property value
		
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
			
			case FbxValue::TYPE_NULL:
				log("(NULL)\n");
				break;
		}
		
		// process
		
		STATE oldState = m_state[m_depth];
		
		STATE newState = oldState;
		
		switch (oldState)
		{
			case STATE_MODEL:
			{
				if (index == 1 && value.getString() == "Mesh")
				{
					log("STATE_MODEL -> STATE_MODEL_MESH\n");
					
					newState = STATE_MODEL_MESH;
					
					m_currentMesh = new MyMesh();
				}
				
				break;
			}
			
			default:
			{
				break;
			}
		}
		
		m_state[m_depth] = newState;
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
	
	FbxParser parser;
	MyReceiver receiver(parser);
	parser.parse(bytes, numBytes, receiver);
	
	free(bytes);
	
	// show result
	
	for (size_t i = 0; i < receiver.meshes.size(); ++i)
	{
		const MyMesh * mesh = receiver.meshes[i];
		
		receiver.log("mesh: numVertices=%d, numIndices=%d\n", mesh->numVertices, mesh->numIndices);
		
		for (int i = 0; i < mesh->numIndices; ++i)
		{
			int index = mesh->indices[i];
			
			bool end = false;
			
			if (index < 0)
			{
				index = ~index;
				end = true;
			}
			
			receiver.log("[%d] (%+4.2f %+4.2f %+4.2f) ",
				index,
				mesh->vertices[index*3+0],
				mesh->vertices[index*3+1],
				mesh->vertices[index*3+2]);
			
			if (end)
				receiver.log("\n");
		}
	}
	
	return 0;
}
