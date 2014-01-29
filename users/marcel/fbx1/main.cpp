#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

// FBX file reader

static int tab = 0;

static void * start = 0;

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
	//assert(false);
	
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

struct Record
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

void readRecord(void *& p, Record & r)
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

bool parseNode(void *& p)
{
	Record r;
	
	readRecord(p, r);
	
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
				break;
			}
			case 'C':
			{
				int8_t v;
				read(p, v);
				log("bool: %d\n", (int)v);
				break;
			}
			case 'I':
			{
				int32_t v;
				read(p, v);
				log("int32: %d\n", v);
				break;
			}
			case 'F':
			{
				float v;
				read(p, v);
				log("single: %f\n", v);
				break;
			}
			case 'D':
			{
				double v;
				read(p, v);
				log("double: %f\n", (float)v);
				break;
			}
			case 'L':
			{
				int64_t v;
				read(p, v);
				log("int64: %d\n", (int)v);
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
				break;
			}
			
			case 'R':
			{
				log("raw\n");
				int32_t length;
				read(p, length);
				
				#if 1
				uint8_t * raw = (uint8_t*)alloca(length);
				read(p, raw[0], length);
				char * temp = (char*)alloca(length * 2 + 1);
				for (int i = 0; i < length; ++i)
					sprintf(temp + i * 2, "%0x", raw[i]);
				temp[length * 2] = 0;
				log("%s\n", temp);
				#else
				skip(p, length);
				#endif
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
		
		Record temp;
		
		readRecord(p, temp);
		
		if (temp.isNULL())
		{
			log("EOR\n");
			
			break;
		}
		else
		{
			p = t;
			
			tab++;
	
			parseNode(p);
			
			tab--;
		}
	}
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
	
	//skip(p, 6);
	
	while (distance(start, p) != length)
	{
		void * t = p;
		
		Record temp;
		
		readRecord(p, temp);
		
		p = t;
		
		if (temp.isNULL())
		{
			log("EOF\n");
			
			break;
		}
		else
		{
			log("startDistance: %d/%d\n", distance(start, p), length);
		
			parseNode(p);
			
			log("endDistance: %d/%d\n", distance(start, p), length);
		}
	}
}

int main(int argc, const char * argv[])
{
	FILE * file = fopen("test.fbx", "rb");
	const int p1 = ftell(file);
	fseek(file, 0, SEEK_END);
	const int p2 = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	const int size = p2 - p1;
	
	void * bytes = malloc(size);
	
	fread(bytes, size, 1, file);
	
	fclose(file);
	
	parseFbx(bytes, size);
	
	free(bytes);
	
	return 0;
}
