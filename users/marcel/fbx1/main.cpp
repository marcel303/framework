#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

// FBX file reader

enum PROPTYPE
{
	PROP_BOOL,
	PROP_BOOL_ARRAY,
	PROP_INT,
	PROP_INT_ARRAY,
	PROP_FLOAT,
	PROP_FLOAT_ARRAY,
	PROP_STRING,
	PROP_RAW
};

struct FbxRecord
{
	int32_t endOffset;
	int32_t numProperties;
	int32_t propertyListLen;
	int8_t nameLen;
	
	std::string name;
	
	bool isNULL() const
	{
		const char * bytes = (const char*)this;
		for (int i = 0; i < 13; ++i)
			if (bytes[i])
				return false;
		return true;
	}
};

class FbxReceiver
{
public:
	virtual void recordBegin(const FbxRecord & record) = 0;
	virtual void recordEnd() = 0;
	virtual void property(int index, PROPTYPE type, const void * value) = 0;
};

static int tab = 0;

static void * start = 0;

static FbxReceiver * receiver = 0;

template <typename T> T read(void *& p, T & t)
{
	T * v = (T*)p;
	t = *v++;
	p = v;
}

template <typename T> void read(void *& p, T & t, int n)
{
	char * v = (char*)p;
	memcpy(&t, v, n);
	v += n;
	p = v;
}

void skip(void *& p, int n)
{
	char * v = (char*)p;
	v += n;
	p = v;
}

void log(const char * fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	
	char tabs[128];
	
	for (int i = 0; i < tab; ++i)
		tabs[i] = '\t';
	tabs[tab] = 0;
	
	char temp[1024];
	
	vsprintf(temp, fmt, va);
	
	printf("%s%s", tabs, temp);
	
	va_end(va);
}

template <typename T> void readArray(void *& p)
{
	int32_t arrayLength;
	int32_t encoding;
	int32_t compressedLength;
	
	read(p, arrayLength);
	read(p, encoding);
	read(p, compressedLength);
	
	log("arrayLength=%d, encoding=%d, compressedLength=%d\n",
		arrayLength,
		encoding,
		compressedLength);
	
	int length;
	
	if (encoding == 0)
	{
		length = sizeof(T) * arrayLength;
	}
	
	if (encoding == 1)
	{
		// todo: run deflate
		
		length = compressedLength;
	}
	
	skip(p, length);
}

void readRecord(void *& p, FbxRecord & r)
{
	read(p, r.endOffset);
	read(p, r.numProperties);
	read(p, r.propertyListLen);
	read(p, r.nameLen);
	
	char * name = (char*)alloca(r.nameLen + 1);
	read(p, name[0], r.nameLen);
	name[r.nameLen] = 0;
	
	r.name = name;
}

int distance(void * p1, void * p2)
{
	return reinterpret_cast<uintptr_t>(p2) - reinterpret_cast<uintptr_t>(p1);
}

bool parseNode(void *& p, const FbxRecord & r)
{
	receiver->recordBegin(r);
	
	log("node: endOffset=%d, numProperties=%d, propertyListLen=%d, nameLen=%d, name=%s\n",
		r.endOffset,
		r.numProperties,
		r.propertyListLen,
		(int32_t)r.nameLen,
		r.name.c_str());
	
	tab++;
	
	// properties
	
	for (int i = 0; i < r.numProperties; ++i)
	{
		char type;
		
		read(p, type);
		
		switch (type)
		{
			// single value
			
			case 'Y':
			{
				int16_t v;
				read(p, v);
				log("int16: %d\n", (int)v);
				int t = v;
				receiver->property(i, PROP_INT, &t);
				break;
			}
			case 'C':
			{
				int8_t v;
				read(p, v);
				log("bool: %d\n", (int)v);
				bool t = v != 0;
				receiver->property(i, PROP_BOOL, &t);
				break;
			}
			case 'I':
			{
				int32_t v;
				read(p, v);
				log("int32: %d\n", v);
				int t = v;
				receiver->property(i, PROP_INT, &t);
				break;
			}
			case 'F':
			{
				float v;
				read(p, v);
				log("single: %f\n", v);
				float t = v;
				receiver->property(i, PROP_FLOAT, &t);
				break;
			}
			case 'D':
			{
				double v;
				read(p, v);
				log("double: %f\n", (float)v);
				float t = (float)v;
				receiver->property(i, PROP_FLOAT, &t);
				break;
			}
			case 'L':
			{
				int64_t v;
				read(p, v);
				log("int64: %lld\n", v);
				int t = (int)v;
				receiver->property(i, PROP_INT, &t);
				break;
			}
			
			// arrays
			
			case 'f':
			{
				readArray<float>(p);
				break;
			}
			case 'd':
			{
				readArray<double>(p);
				break;
			}
			case 'l':
			{
				readArray<int64_t>(p);
				break;
			}
			case 'i':
			{
				readArray<int32_t>(p);
				break;
			}
			case 'b':
			{
				readArray<int8_t>(p);
				break;
			}
			
			// special
			
			case 'S':
			{
				int32_t length;
				read(p, length);
				char * str = (char*)alloca(length + 1);
				read(p, str[0], length);
				str[length] = 0;
				log("string: %s\n", str);
				receiver->property(i, PROP_STRING, str);
				break;
			}
			
			case 'R':
			{
				log("raw\n");
				int32_t length;
				read(p, length);
				
				uint8_t * raw = (uint8_t*)alloca(length);
				read(p, raw[0], length);
				
				#if 1
				char * temp = (char*)alloca(length * 2 + 1);
				for (int j = 0; j < length; ++j)
					sprintf(temp + j * 2, "%0x", raw[j]);
				temp[length * 2] = 0;
				log("%s\n", temp);
				#endif
				
				receiver->property(i, PROP_RAW, raw);
				break;
			}
			
			default:
			{
				assert(false);
				break;
			}
		}
	}
	
	tab--;
	
	// nested records
	
	while (distance(start, p) != r.endOffset)
	{		
		assert(distance(start, p) < r.endOffset);
		
		void * t = p;
		
		FbxRecord temp;
		
		readRecord(p, temp);
		
		if (temp.isNULL())
		{
			log("EOR\n");
			
			assert(distance(start, p) == r.endOffset);
			
			break;
		}
		else
		{
			tab++;
	
			parseNode(p, temp);
			
			tab--;
		}
	}
	
	receiver->recordEnd();
}

void parseFbx(void * p, int length)
{
	start = p;
	
	// parse header
	
	{
		const char magic[21] = "Kaydara FBX Binary  ";
		const char * bytes = (const char*)p;
		for (int i = 0; i < sizeof(magic); ++i)
			if (bytes[i] != magic[i])
				log("magic doesn't match\n");
		skip(p, sizeof(magic));
		
		const char * special = (const char*)p;
		log("special bytes: %x, %x\n", special[0], special[1]);
		skip(p, 2);
		
		uint32_t version;
		read(p, version);
		log("version: %d\n", version);
	}
	
	while (distance(start, p) != length)
	{
		void * t = p;
		
		FbxRecord temp;
		
		readRecord(p, temp);
		
		if (temp.isNULL())
		{
			log("EOF\n");
			
			break;
		}
		else
		{
			log("startDistance: %d/%d\n", distance(start, p), length);
		
			parseNode(p, temp);
			
			log("endDistance: %d/%d\n", distance(start, p), length);
		}
	}
}

//

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
		STATE_MODEL_MAYBE,
		STATE_MODEL,
		STATE_VERTS,
		STATE_POLYS,
		STATE_UNKNOWN
	};
	
	STATE state[maxDepth];
	int depth;
	
	MyMesh * currentMesh;
	
public:
	std::vector<MyMesh*> meshes;
	
	MyReceiver()
	{
		depth = 0;
		state[depth] = STATE_ROOT;
		
		currentMesh = 0;
	}
	
	virtual void recordBegin(const FbxRecord & record)
	{
		STATE oldState = state[depth];
		
		depth++;
		
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
					newState = STATE_MODEL_MAYBE;
				}
				break;
			}
			
			case STATE_MODEL_MAYBE:
			{
				break;
			}
			
			case STATE_MODEL:
			{
				if (record.name == "Vertices")
				{
					newState = STATE_VERTS;
					
					currentMesh->numVertices = record.numProperties;
					currentMesh->vertices = new float[record.numProperties * 3];
				}
				if (record.name == "PolygonVertexIndex")
				{
					newState = STATE_POLYS;
					
					currentMesh->numIndices = record.numProperties;
					currentMesh->indices = new int[record.numProperties];
				}
				break;
			}
			
			case STATE_VERTS:
			{
				break;
			}
			
			case STATE_POLYS:
			{
				break;
			}

			case STATE_UNKNOWN:
			{
				break;
			}
			
			default:
			{
				assert(false);
				break;
			}
		}
		
		state[depth] = newState;
		
		log("state change %d -> %d\n", oldState, newState);
	}
	
	virtual void recordEnd()
	{
		STATE oldState = state[depth];
		
		switch (oldState)
		{
			case STATE_MODEL:
			{
				meshes.push_back(currentMesh);
				currentMesh = 0;
				break;
			}
		}
		
		depth--;
	}
	
	virtual void property(int index, PROPTYPE type, const void * value)
	{
		STATE oldState = state[depth];
		
		STATE newState = oldState;
		
		switch (oldState)
		{
			case STATE_MODEL_MAYBE:
			{
				if (index == 1 && type == PROP_STRING)
				{
					const char * str = (const char*)value;
					
					if (!strcmp(str, "Mesh"))
					{
						log("STATE_MODEL_MAYBE -> STATE_MODEL !!\n");
						
						newState = STATE_MODEL;
						
						currentMesh = new MyMesh();
					}
				}
				
				break;
			}
			
			case STATE_VERTS:
			{
				if (type == PROP_FLOAT)
					currentMesh->vertices[index] = *(float*)value;
				break;
			}
			
			case STATE_POLYS:
			{
				if (type == PROP_INT)
					currentMesh->indices[index] = *(int*)value;
				break;
			}
		}
		
		state[depth] = newState;
	}
};

int main(int argc, const char * argv[])
{
	MyReceiver r;
	
	receiver = &r;
	
	// read file contents
	
	FILE * file = fopen("test.fbx", "rb");
	const int p1 = ftell(file);
	fseek(file, 0, SEEK_END);
	const int p2 = ftell(file);
	fseek(file, 0, SEEK_SET);
	const int size = p2 - p1;
	void * bytes = malloc(size);
	fread(bytes, size, 1, file);
	fclose(file);
	
	// parse
	
	parseFbx(bytes, size);
	
	free(bytes);
	
	// show result
	
	for (int i = 0; i < r.meshes.size(); ++i)
	{
		MyMesh * mesh = r.meshes[i];
		
		log("mesh: numVertices=%d, numIndices=%d\n", mesh->numVertices, mesh->numIndices);
		
		for (int i = 0; i < mesh->numIndices; ++i)
		{
			int index = mesh->indices[i];
			bool end = false;
			
			if (index < 0)
			{
				index = ~index;
				end = true;
			}
			
			log("[%d] (%+4.2f %+4.2f %+4.2f) ",
				index,
				mesh->vertices[index*3+0],
				mesh->vertices[index*3+1],
				mesh->vertices[index*3+2]);
			
			if (end)
				log("\n");
		}
	}
	
	return 0;
}
