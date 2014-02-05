#include <list>
#include <stdlib.h>
#include <vector>
#include "fbx.h"

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
	
	// dumpFileContents iterates over the entire file contents and dumps every record and property
	
	void dumpFileContents()
	{
		for (FbxRecord record = m_reader->firstRecord(); record.isValid(); record = record.nextSibling())
		{
			dumpRecord(record);
		}
	}
	
	void dumpRecord(const FbxRecord & record)
	{
		log("node: endOffset=%d, numProperties=%d, name=%s\n",
			(int)record.getEndOffset(),
			(int)record.getNumProperties(),
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

//

class Mesh
{
public:
	std::vector<float> vertices;
	std::vector<int> indices;
};

bool readFile(const char * filename, std::vector<uint8_t> & bytes)
{
	FILE * file = fopen(filename, "rb");
	if (!file)
		return false;
	
	const size_t p1 = ftell(file);
	fseek(file, 0, SEEK_END);
	const size_t p2 = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	const size_t numBytes = p2 - p1;
	bytes.resize(numBytes);
	
	fread(&bytes[0], numBytes, 1, file);
	fclose(file);
	
	return true;
}

int main(int argc, const char * argv[])
{
	// parse command line
	
	bool verbose = false;
	const char * filename = "test.fbx";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-v"))
			verbose = true;
		else
			filename = argv[i];
	}
	
	// read file contents
	
	std::vector<uint8_t> bytes;
	
	if (!readFile(filename, bytes))
	{
		printf("failed to open %s\n", filename);
		return -1;
	}

	// open FBX file
	
	FbxReader reader;
	
	reader.openFromMemory(&bytes[0], bytes.size());
	
	// dump FBX contents
	
	FbxFileLogger logger(reader);
	
	if (verbose)
	{
		logger.dumpFileContents();
	}
	
	// extract meshes
	
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
	
	// dump mesh data
	
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
				// negative indices mark the end of a polygon
				
				index = ~index;
				
				end = true;
			}
			
			logger.log("[%d] (%+4.2f %+4.2f %+4.2f) ",
				index,
				mesh.vertices[index*3+0],
				mesh.vertices[index*3+1],
				mesh.vertices[index*3+2]);
			
			if (end)
			{
				logger.log("\n");
			}
		}
	}
	
	return 0;
}
