#include <assert.h>
#include <list>
#include <map>
#include <stdlib.h>
#include <stdarg.h>
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
	class Vertex
	{
	public:
		Vertex()
		{
			memset(this, 0, sizeof(Vertex));
		}
		
		inline bool operator<(const Vertex & w) const
		{
			const float * floats1 = reinterpret_cast<const float*>(this);
			const float * floats2 = reinterpret_cast<const float*>(&w);
			
			const int kNumFloats = sizeof(Vertex) / sizeof(float);
			
			for (int i = 0; i < kNumFloats; ++i)
			{
				if (floats1[i] != floats2[i])
					return floats1[i] < floats2[i];
			}
			
			return false;
		}
		
		inline bool operator==(const Vertex & w) const
		{
			const float * floats1 = reinterpret_cast<const float*>(this);
			const float * floats2 = reinterpret_cast<const float*>(&w);
			
			const int kNumFloats = sizeof(Vertex) / sizeof(float);
			
			for (int i = 0; i < kNumFloats; ++i)
			{
				if (floats1[i] != floats2[i])
					return false;
			}
			
			return true;
		}
		
		float px, py, pz;     // position
		float nx, ny, nz;     // normal
		float tx, ty;         // texture
		float cx, cy, cz, cw; // color
	};
	
	std::vector<Vertex> m_vertices;
	std::vector<int> m_indices;
	
	void construct(
		int logIndent,
		const std::vector<float> & vertices,
		const std::vector<int> & vertexIndices,
		const std::vector<float> & normals,
		const std::vector<float> & uvs,
		const std::vector<int> & uvIndices,
		const std::vector<float> & colors,
		const std::vector<int> & colorIndices)
	{
		log(logIndent, "-- running da welding machine! --\n");
		
		logIndent++;
		
		log(logIndent, "input: %d vertices, %d indices, %d normals, %d UVs (%d indices), %d colors (%d indices)\n",
			(int)vertices.size()/3,
			(int)vertexIndices.size(),
			(int)normals.size()/3,
			(int)uvs.size()/2,
			(int)uvIndices.size(),
			(int)colors.size()/3,
			(int)colorIndices.size());
		
		std::map<Vertex, int> weldVertices;
		
		for (size_t i = 0; i < vertexIndices.size(); ++i)
		{
			const bool isEnd = vertexIndices[i] < 0;
			const size_t vertexIndex = isEnd ? ~vertexIndices[i] : vertexIndices[i];
			
			// fill in vertex members with the available data from the various input arrays
			
			Vertex vertex;
			
			// position
			
			if (vertexIndex * 3 < vertices.size())
			{
				vertex.px = vertices[vertexIndex * 3 + 0];
				vertex.py = vertices[vertexIndex * 3 + 1];
				vertex.pz = vertices[vertexIndex * 3 + 2];
			}
			
			// normal
			
			if (i * 3 < normals.size())
			{
				vertex.nx = normals[i * 3 + 0];
				vertex.ny = normals[i * 3 + 1];
				vertex.nz = normals[i * 3 + 2];
			}
			
			// uv
			
			if (uvIndices.size() >= vertexIndices.size())
			{
				// indexed UV
				
				const size_t uvIndex = uvIndices[i];
				
				if (uvIndex * 2 < uvs.size())
				{
					vertex.tx = uvs[uvIndex * 2 + 0];
					vertex.ty = uvs[uvIndex * 2 + 1];
				}
			}
			else if (uvIndices.size() == 0 && i * 2 < uvs.size())
			{
				// non-indexed UV
				
				vertex.tx = uvs[i * 2 + 0];
				vertex.ty = uvs[i * 2 + 1];
			}
			
			// color
			
			if (colorIndices.size() >= vertexIndices.size())
			{
				// indexed color
				
				const size_t colorIndex = colorIndices[i];
				
				if (colorIndex * 4 < colors.size())
				{
					vertex.cx = colors[colorIndex * 4 + 0];
					vertex.cy = colors[colorIndex * 4 + 1];
					vertex.cz = colors[colorIndex * 4 + 2];
					vertex.cw = colors[colorIndex * 4 + 3];
				}
			}
			else if (colorIndices.size() == 0 && i * 4 < colors.size())
			{
				// non-indexed color
				
				vertex.cx = colors[i * 4 + 0];
				vertex.cy = colors[i * 4 + 1];
				vertex.cz = colors[i * 4 + 2];
				vertex.cw = colors[i * 4 + 3];
			}
			
			// add unique vertex if it doesn't exist yet, or add an index for the existing vertex
			
			std::map<Vertex, int>::iterator w = weldVertices.find(vertex);
			
			int weldIndex;
			
			if (w == weldVertices.end())
			{
				// unique vertex. allocate index and add it to the map
				
				weldIndex = int(weldVertices.size());
				
				weldVertices[vertex] = weldIndex;
				
				m_vertices.push_back(vertex);
			}
			else
			{
				// duplicate vertex. just take the index
				
				weldIndex = w->second;
			}
			
			m_indices.push_back(isEnd ? ~weldIndex : weldIndex);
		}
		
		log(logIndent, "output: %d vertices, %d indices\n",
			(int)m_vertices.size(),
			(int)m_indices.size());
		
		logIndent--;
	}
	
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
	
	bool dumpMeshes = false;
	bool verbose = false;
	const char * filename = "test.fbx";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-v"))
			verbose = true;
		else if (!strcmp(argv[i], "-m"))
			dumpMeshes = true;
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
				const FbxRecord vertexIndices = model.firstChild("PolygonVertexIndex");
				
				const FbxRecord normalsLayer = model.firstChild("LayerElementNormal");
				const FbxRecord normals = normalsLayer.firstChild("Normals");
				
				const FbxRecord uvsLayer = model.firstChild("LayerElementUV");
				const FbxRecord uvs = uvsLayer.firstChild("UV");
				const FbxRecord uvIndices = uvsLayer.firstChild("UVIndex");
				
				const FbxRecord colorsLayer = model.firstChild("LayerElementColor");
				const FbxRecord colors = colorsLayer.firstChild("Colors");
				const FbxRecord colorIndices = colorsLayer.firstChild("ColorIndex");
				
				log(logIndent, "found a mesh! name: %s, hasVertices: %d, hasNormals: %d, hasUVs: %d\n",
					modelProps[0].c_str(),
					vertices.isValid(),
					normals.isValid(),
					uvs.isValid());
				
				logIndent++;
				
				mesh.construct(
					logIndent,
					vertices.captureProperties<float>(),		
					vertexIndices.captureProperties<int>(),
					normals.captureProperties<float>(),
					uvs.captureProperties<float>(),
					uvIndices.captureProperties<int>(),
					colors.captureProperties<float>(),
					colorIndices.captureProperties<int>());
				
				// LayerElementNormal.Normals
				// LayerElementUV.UV
				// LayerElementMaterial.Materials
				// LayerElementTexture.TextureId
				
				logIndent--;
			}
		}
	}
	
	// dump mesh data
	
	if (dumpMeshes)
	{
		log(logIndent, "-- dumping mesh data --\n");
		
		for (std::list<Mesh>::iterator i = meshes.begin(); i != meshes.end(); ++i)
		{
			const Mesh & mesh = *i;
			
			log(logIndent, "mesh: numVertices=%d, numIndices=%d\n", int(mesh.m_vertices.size()), int(mesh.m_indices.size()));
			
			logIndent++;
			
			int polygonIndex = ~0;
			
			for (size_t i = 0; i < mesh.m_indices.size(); ++i)
			{
				int index = mesh.m_indices[i];
				
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
				
				log(logIndent, "[%05d] position = (%+7.2f %+7.2f %+7.2f), normal = (%+5.2f %+5.2f %+5.2f), color = (%4.2f %4.2f %4.2f %4.2f), uv = (%+5.2f %+5.2f)\n",
					index,
					mesh.m_vertices[index].px,
					mesh.m_vertices[index].py,
					mesh.m_vertices[index].pz,
					mesh.m_vertices[index].nx,
					mesh.m_vertices[index].ny,
					mesh.m_vertices[index].nz,
					mesh.m_vertices[index].cx,
					mesh.m_vertices[index].cy,
					mesh.m_vertices[index].cz,
					mesh.m_vertices[index].cw,
					mesh.m_vertices[index].tx,
					mesh.m_vertices[index].ty);
							
				logIndent--;
				
				if (end)
				{
					log(logIndent, "\n");
					
					polygonIndex = ~(polygonIndex + 1);
				}
			}
			
			logIndent--;
		}
	}
	
	return 0;
}
