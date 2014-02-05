#include <list>
#include <stdlib.h>
#include <vector>
#include "fbx.h"

static void log(int logIndent, const char * fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	
	char tabs[128];
	for (int i = 0; i < logIndent; ++i)
		tabs[i] = '\t';
	tabs[logIndent] = 0;
	
	char temp[1024];
	vsprintf(temp, fmt, va);
	va_end(va);
	
	printf("%s%s", tabs, temp);
}

class FbxFileLogger
{
	const FbxReader * m_reader;
	
public:
	int m_logIndent;
	
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
		log(m_logIndent, "node: endOffset=%d, numProperties=%d, name=%s\n",
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
				log(m_logIndent, "bool: %d\n", get<bool>(value));
				break;
			case FbxValue::TYPE_INT:
				log(m_logIndent, "int: %lld\n", get<int64_t>(value));
				break;
			case FbxValue::TYPE_REAL:
				log(m_logIndent, "float: %f\n", get<float>(value));
				break;
			case FbxValue::TYPE_STRING:
				log(m_logIndent, "string: %s\n", value.getString().c_str());
				break;
			
			case FbxValue::TYPE_INVALID:
				log(m_logIndent, "(invalid)\n");
				break;
		}
	}
};

//

class Mesh
{
public:
	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uvs;
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
	int logIndent = 0;
	
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
	
	log(logIndent, "-- extracting mesh data --\n");
	
	std::list<Mesh> meshes;
	
	for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
	{
		// Model, Pose, Material, Texture, ..
		
		for (FbxRecord model = objects.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
		{
			std::vector<std::string> modelProps = model.captureProperties<std::string>();
			
			if (modelProps.size() >= 2 && modelProps[1] == "Mesh")
			{
				meshes.push_back(Mesh());
				Mesh & mesh = meshes.back();
				
				const FbxRecord vertices = model.firstChild("Vertices");
				const FbxRecord indices = model.firstChild("PolygonVertexIndex");
				
				const FbxRecord normalsLayer = model.firstChild("LayerElementNormal");
				const FbxRecord normals = normalsLayer.firstChild("Normals");
				
				const FbxRecord uvsLayer = model.firstChild("LayerElementUV");
				const FbxRecord uvs = uvsLayer.firstChild("UV");
				
				log(logIndent, "found a mesh! hasVertices: %d, hasNormals: %d, hasUVs: %d\n",
					vertices.isValid(),
					normals.isValid(),
					uvs.isValid());
				
				mesh.vertices = vertices.captureProperties<float>();			
				mesh.indices = indices.captureProperties<int>();
				mesh.normals = normals.captureProperties<float>();
				mesh.uvs = uvs.captureProperties<float>();
				
				// LayerElementNormal.Normals
				// LayerElementUV.UV
				// LayerElementMaterial.Materials
				// LayerElementTexture.TextureId
				
				// make sure our normals and uvs arrays are at least as large as our vertex array.
				// we don't want out of bounds accessed if some of the layer data is missing or incorrect
				
				const size_t numVertices = mesh.vertices.size() / 3;
				if (mesh.normals.size() < numVertices * 3)
					mesh.normals.resize(numVertices * 3);
				if (mesh.uvs.size() < numVertices * 2)
					mesh.uvs.resize(numVertices * 2);
			}
		}
	}
	
	// dump mesh data
	
	log(logIndent, "-- dumping mesh data --\n");
	
	for (std::list<Mesh>::iterator i = meshes.begin(); i != meshes.end(); ++i)
	{
		const Mesh & mesh = *i;
		
		log(logIndent, "mesh: numVertices=%d, numIndices=%d\n", int(mesh.vertices.size()), int(mesh.indices.size()));
		
		logIndent++;
		
		int polygonIndex = ~0;
		
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
			
			if (polygonIndex < 0)
			{
				polygonIndex = ~polygonIndex;
				log(logIndent, "polygon [%d]:\n", polygonIndex);
			}
			
			logIndent++;
			
			log(logIndent, "position = (%+7.2f %+7.2f %+7.2f), normal = (%+5.2f %+5.2f %+5.2f), uv = (%+5.2f %+5.2f)\n",
				index,
				mesh.vertices[index*3+0],
				mesh.vertices[index*3+1],
				mesh.vertices[index*3+2],
				mesh.normals[index*3+0],
				mesh.normals[index*3+1],
				mesh.normals[index*3+2],
				mesh.uvs[index*2+0],
				mesh.uvs[index*2+1]);
			
			logIndent--;
			
			if (end)
			{
				log(logIndent, "\n");
				
				polygonIndex = ~(polygonIndex + 1);
			}
		}
		
		logIndent--;
	}
	
	return 0;
}
