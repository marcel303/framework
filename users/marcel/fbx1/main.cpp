#include <assert.h>
#include <list>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <vector>
#include "fbx.h"

#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "Mat4x4.h"

static bool logEnabled = false;

static int getTimeMS()
{
	return clock() * 1000 / CLOCKS_PER_SEC;
}

static void log(int logIndent, const char * fmt, ...)
{
	if (logEnabled)
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

class FbxObject
{
public:
	FbxObject(const std::string & type, const std::string & name, bool persistent = false)
	{
		this->type = type;
		this->name = name;
		this->persistent = persistent;
		parent = 0;
	}
	
	std::string type;
	std::string name;
	bool persistent;
	FbxObject * parent;
	std::vector<FbxObject*> children;
};

class FbxMesh : public FbxObject
{
public:
	Mat4x4 transform;
	
	std::vector<float> vertices;
	std::vector<int> vertexIndices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<int> uvIndices;
	std::vector<float> colors;
	std::vector<int> colorIndices;
	
	// deformers
	
	struct Deformer
	{
		static const int kMaxEntries = 4;
		
		struct Entry
		{
			int index;
			float weight;
		};
		
		Entry entries[kMaxEntries];
		int numEntries;
		
		Deformer()
		{
			numEntries = 0;
		}
		
		int size() const
		{
			return numEntries;
		}
		
		void add(int index, float weight)
		{
			if (numEntries < 4)
			{
				entries[numEntries].index = index;
				entries[numEntries].weight = weight;
				numEntries++;
			}
			else
			{
				// todo: replace lowest weight < weight (if any)
			}
		}
	};
	
	std::vector<Deformer> deformers;
	
	FbxMesh(const std::string & type, const std::string & name)
		: FbxObject("Mesh", name)
	{
	}
};

class FbxPose : public FbxObject
{
public:
	std::map<std::string, Mat4x4> matrices;
	
	FbxPose(const std::string & name)
		: FbxObject("Pose", name, true)
	{
	}
};

class FbxDeformer : public FbxObject
{
public:
	std::vector<int> indices;
	std::vector<float> weights;
	
	FbxDeformer(const std::string & name)
		: FbxObject("Deformer", name)
	{
	}
};

class FbxTransformChannel
{
public:
	class RegularChannel
	{
	public:
		struct Key
		{
			int64_t timeStamp;
			float value;
		};
		
		struct KeyList
		{
			std::vector<Key> keys;
			
			void read(int & logIndent, const FbxRecord & record)
			{
				const int keyCount = record.firstChild("KeyCount").captureProperty<int>(0);
				
				if (keyCount != 0)
				{
					const FbxRecord key = record.firstChild("Key");
					const std::vector<FbxValue> values = key.captureProperties<FbxValue>();
					const int stride = int(values.size()) / keyCount;
					
					//log(logIndent, "keyCount=%d\n", keyCount);
					
					for (size_t i = 0; i + stride <= values.size(); i += stride)
					{
						float value = get<float>(values[i + 1]);
						
						// round with a fixed precision so small floating point drift is eliminated from the exported values
						value = int(value * 1000.f + .5f) / 1000.f;
						
						// don't write duplicate values, unless it's the first/last key in the list. exporters sometimes write a fixed number of keys (sampling based), with lots of duplicates
						const bool isDuplicate = keys.size() >= 1 && keys.back().value == value && i + stride != values.size();
						
						if (!isDuplicate)
						{
							Key temp;
							temp.timeStamp = get<int64_t>(values[i + 0]);
							temp.value = value;
							keys.push_back(temp);
						
							//log(logIndent, "%014lld -> %f\n", temp.timeStamp, temp.value);
						}
					}
				}
			}
		};
		
		KeyList X;
		KeyList Y;
		KeyList Z;
		
		void read(int & logIndent, const FbxRecord & record)
		{
			for (FbxRecord channel = record.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
			{
				const std::string name = channel.captureProperty<std::string>(0);
				
				//log(logIndent, "stream: name=%s\n", name.c_str());
				
				logIndent++;
				{
					if (name == "X")
						X.read(logIndent, channel);
					if (name == "Y")
						Y.read(logIndent, channel);
					if (name == "Z")
						Z.read(logIndent, channel);
				}
				logIndent--;
			}
		}
	};
	
	RegularChannel T;
	RegularChannel R;
	RegularChannel S;
	
	void read(int & logIndent, const FbxRecord & record)
	{
		for (FbxRecord channel = record.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
		{
			const std::string name = channel.captureProperty<std::string>(0);
			
			if (name == "T")
			{
				//log(logIndent, "translation\n");
				
				logIndent++;
				{
					T.read(logIndent, channel);
				}
				logIndent--;
				
			}
			else if (name == "R")
			{
				//log(logIndent, "rotation\n");
				
				logIndent++;
				{
					R.read(logIndent, channel);
				}
				logIndent--;
			}
			else if (name == "S")
			{
				//log(logIndent, "scale\n");
				
				logIndent++;
				{
					S.read(logIndent, channel);
				}
				logIndent--;
			}
		}
	}
};

//

template <typename Key, typename Value, unsigned int PoolSize>
class HashMap
{
	class Node
	{
	public:
		Node * next;
		unsigned int hash;
		Key key;
		Value value;
	};
	
	class Allocator
	{
		class Block
		{
		public:
			Block * next;
			Node * nodes;
			unsigned int numNodes;
			
			Block()
			{
				next = 0;
				nodes = (Node*)malloc(sizeof(Node) * PoolSize);
				numNodes = 0;
			}
			
			~Block()
			{
				free(nodes);
				nodes = 0;
			}
			
			Node * alloc()
			{
				return &nodes[numNodes++];
			}
		};
		
		void newBlock()
		{
			Block * block = new Block();
			block->next = currentBlock;
			currentBlock = block;
		}
		
		Block * currentBlock;
		
	public:
		Allocator()
		{
			currentBlock = 0;
		}
		
		~Allocator()
		{
			Block * block = currentBlock;
			
			while (block != 0)
			{
				Block * next = block->next;
				delete block;
				block = next;
			}
		}
		
		Node * alloc()
		{
			if (!currentBlock)
			{
				newBlock();
			}
			
			Node * node = currentBlock->alloc();
			
			if (currentBlock->numNodes == PoolSize)
			{
				currentBlock = 0;
			}
			
			return node;
		}
	};
	
	Node ** m_buckets;
	unsigned int m_bucketCount;
	
	Allocator m_nodeAlloc;
	Node * m_currentAlloc;
	
	template <typename T> static unsigned int calcHash(const T & value)
	{
		if (sizeof(T) < 4)
		{
			const int numBytes = sizeof(T);
			const uint8_t * bytes = (uint8_t*)&value;
			
			unsigned int result = 0;
			
			for (int i = 0; i < numBytes; ++i)
				result = result * 17 + bytes[i];
			
			return result;
		}
		else
		{
			const int numWords = sizeof(T) >> 2;
			const uint32_t * words = (uint32_t*)&value;
			
			unsigned int result = 0;
			
			for (int i = 0; i < numWords; ++i)
				result = result * 982451653 + words[i];
			
			return result;
		}
	}
	
public:
	HashMap(unsigned int bucketCount)
	{
		m_buckets = new Node*[bucketCount];
		m_bucketCount = bucketCount;
		for (unsigned int i = 0; i < bucketCount; ++i)
			m_buckets[i] = 0;
		
		m_currentAlloc = 0;
	}
	
	~HashMap()
	{
		delete [] m_buckets;
		m_buckets = 0;
	}
	
	Key & preAllocKey()
	{
		if (!m_currentAlloc)
			m_currentAlloc = m_nodeAlloc.alloc();
		
		return m_currentAlloc->key;
	}
	
	bool allocOrFetchValue(Value & value)
	{
		const unsigned int hash = calcHash(m_currentAlloc->key);
		const unsigned int bucketIndex = hash % m_bucketCount;
		
		for (const Node * node = m_buckets[bucketIndex]; node != 0; node = node->next)
		{
			if (node->hash == hash && node->key == m_currentAlloc->key)
			{
				value = node->value;
				return false;
			}
		}
		
		Node * node = m_currentAlloc;
		m_currentAlloc = 0;
		
		node->next = m_buckets[bucketIndex];
		node->hash = hash;
		node->value = value;
		m_buckets[bucketIndex] = node;
		
		return true;
	}
};

//

class Mesh
{
public:
	class Vertex
	{
	public:
		inline bool operator==(const Vertex & w) const
		{
			return !memcmp(this, &w, sizeof(Vertex));
		}
		
		float px, py, pz;     // position
		float nx, ny, nz;     // normal
		float tx, ty;         // texture
		float cx, cy, cz, cw; // color
		
		uint8_t bi[4]; // blend indices
		uint8_t bw[4]; // blend weights
	};
	
	Mat4x4 m_transform;
	
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
		const std::vector<int> & colorIndices,
		const std::vector<FbxMesh::Deformer> & deformers)
	{
		#if 0
		m_vertices.resize(vertices.size() / 3);
		for (size_t i = 0; i < vertices.size() / 3; ++i)
		{
			Vertex & v = m_vertices[i];
			v.px = vertices[i * 3 + 0];
			v.py = vertices[i * 3 + 1];
			v.pz = vertices[i * 3 + 2];
			v.nx = v.ny = v.nz = 0.f;
			v.tx = v.ty = 0.f;
			v.cx = v.cy = v.cz = v.cw = 1.f;
			for (int j = 0; j < 4; ++j)
			{
				v.bi[j] = 0;
				v.bw[j] = 0;
			}
		}
		m_indices = vertexIndices;
		return;
		#endif
		
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
		
		const int time1 = getTimeMS();
		
		m_vertices.reserve(vertices.size());
		m_indices.reserve(vertexIndices.size());
		
		typedef HashMap<Vertex, int, 1000> WeldVertices;
		WeldVertices weldVertices(vertexIndices.size() / 3);
		
		for (size_t i = 0; i < vertexIndices.size(); ++i)
		{
			const bool isEnd = vertexIndices[i] < 0;
			const size_t vertexIndex = isEnd ? ~vertexIndices[i] : vertexIndices[i];
			
			// fill in vertex members with the available data from the various input arrays
			
			Vertex & vertex = weldVertices.preAllocKey();
			
			// position
			
			if (vertexIndex < vertices.size() / 3)
			{
				vertex.px = vertices[vertexIndex * 3 + 0];
				vertex.py = vertices[vertexIndex * 3 + 1];
				vertex.pz = vertices[vertexIndex * 3 + 2];
			}
			else
			{
				vertex.px = vertex.py = vertex.pz = 0.f;
			}
			
			// normal
			
			if (i < normals.size() / 3)
			{
				vertex.nx = normals[i * 3 + 0];
				vertex.ny = normals[i * 3 + 1];
				vertex.nz = normals[i * 3 + 2];
			}
			else
			{
				vertex.nx = vertex.ny = vertex.nz = 0.f;
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
			else if (uvIndices.size() == 0 && i < uvs.size() / 2)
			{
				// non-indexed UV
				
				vertex.tx = uvs[i * 2 + 0];
				vertex.ty = uvs[i * 2 + 1];
			}
			else
			{
				vertex.tx = vertex.ty = 0.f;
			}
			
			// color
			
			if (colorIndices.size() >= vertexIndices.size())
			{	
				// indexed color
				
				const size_t colorIndex = colorIndices[i];
				
				if (colorIndex < colors.size() / 4)
				{
					vertex.cx = colors[colorIndex * 4 + 0];
					vertex.cy = colors[colorIndex * 4 + 1];
					vertex.cz = colors[colorIndex * 4 + 2];
					vertex.cw = colors[colorIndex * 4 + 3];
				}
			}
			else if (colorIndices.size() == 0 && i < colors.size() / 4)
			{
				// non-indexed color
				
				vertex.cx = colors[i * 4 + 0];
				vertex.cy = colors[i * 4 + 1];
				vertex.cz = colors[i * 4 + 2];
				vertex.cw = colors[i * 4 + 3];
			}
			else
			{
				vertex.cx = vertex.cy = vertex.cz = vertex.cw = 1.f;
			}
			
			// deformers
			
			const int numDeformers = vertexIndex >= deformers.size() ? 0 : deformers[vertexIndex].numEntries < 4 ? deformers[vertexIndex].numEntries : 4;
			
			for (int d = 0; d < numDeformers; ++d)
			{
				vertex.bi[d] = deformers[vertexIndex].entries[d].index;
				vertex.bw[d] = deformers[vertexIndex].entries[d].weight * 255.f;
				
				//printf("added %d, %d\n", vertex.bi[d], vertex.bw[d]);
			}
			for (int d = numDeformers; d < 4; ++d)
			{
				vertex.bi[d] = 0;
				vertex.bw[d] = 0;
			}
			
			// add unique vertex if it doesn't exist yet, or add an index for the existing vertex
			
			int weldIndex = int(m_vertices.size());
			
			if (weldVertices.allocOrFetchValue(weldIndex))
			{
				// unique vertex. add it
				
				m_vertices.push_back(vertex);
			}
			else
			{
				// duplicate vertex
			}
			
			m_indices.push_back(isEnd ? ~weldIndex : weldIndex);
		}
		
		const int time2 = getTimeMS();
		log(logIndent, "time: %d ms\n", time2-time1);
		
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

static Mat4x4 MatrixTranslation(const Vec3 & v)
{
	Mat4x4 t;
	t.MakeTranslation(v);
	return t;
}

static Mat4x4 MatrixRotation(const Vec3 & v)
{
	Mat4x4 x, y, z;
	x.MakeRotationX(v[0] * M_PI / 180.f);
	y.MakeRotationY(v[1] * M_PI / 180.f);
	z.MakeRotationZ(v[2] * M_PI / 180.f);
	return x * y * z;
}

static Mat4x4 MatrixScaling(const Vec3 & v)
{
	Mat4x4 s;
	s.MakeScaling(v);
	return s;
}

int main(int argc, char * argv[])
{
	int logIndent = 0;
	
	// parse command line
	
	bool dumpMeshes = false;
	bool drawMeshes = false;
	bool verbose = false;
	const char * filename = "test.fbx";
	
	for (int i = 1; i < argc; ++i)
	{
		if (!strcmp(argv[i], "-v"))
			verbose = true;
		else if (!strcmp(argv[i], "-m"))
			dumpMeshes = true;
		else if (!strcmp(argv[i], "-d"))
			drawMeshes = true;
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
	
	// build FBX object list
	
	logEnabled = true;
	
	typedef std::vector<FbxObject*> ObjectList;
	ObjectList objectList;
	typedef std::map<std::string, FbxObject*> ObjectsByName;
	ObjectsByName objectsByName;
	
	FbxObject * scene = new FbxObject("Scene", "Scene", true);
	objectsByName[scene->name] = scene;
	
	// extract meshes
	
	log(logIndent, "-- extracting mesh data --\n");
	
	const int time1 = getTimeMS();
	
	std::vector<FbxMesh*> fbxMeshes;
	
	for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
	{
		for (FbxRecord object = objects.firstChild(); object.isValid(); object = object.nextSibling())
		{
			FbxObject * fbxObject = 0;
		
			std::vector<std::string> objectProps = object.captureProperties<std::string>();
			const std::string objectName = objectProps.size() >= 1 ? objectProps[0] : "";
			const std::string objectType = objectProps.size() >= 2 ? objectProps[1] : "";
				
			if (object.name == "Model")
			{
				const FbxRecord & model = object;
				
				if (objectType == "Mesh")
				{
					FbxMesh * mesh = new FbxMesh("Mesh", objectName);
					fbxObject = mesh;
					fbxMeshes.push_back(mesh); // fixme, remove?
					
					FbxRecord properties = model.firstChild("Properties60");
					
					class Float3 : public Vec3
					{
					public:
						Float3(float x, float y, float z)
							: Vec3(x, y, z)
						{
						}
						
						void load(const std::vector<FbxValue> & values)
						{
							if (values.size() >= 4) (*this)[0] = values[3].getDouble();
							if (values.size() >= 5) (*this)[1] = values[4].getDouble();
							if (values.size() >= 6) (*this)[2] = values[5].getDouble();
						}
					};
					
					Float3 translationLocal(0.f, 0.f, 0.f);
					Float3 rotationOffset(0.f, 0.f, 0.f); // translation offset for rotation pivot
					Float3 rotationPivot(0.f, 0.f, 0.f); // center of rotation relative to node origin
					Float3 rotationPre(0.f, 0.f, 0.f); // rotation applied before animation rotation
					Float3 rotationLocal(0.f, 0.f, 0.f);
					Float3 rotationPost(0.f, 0.f, 0.f); // rotation applied after animation rotation
					Float3 scalingLocal(1.f, 1.f, 1.f);
					Float3 scalingOffset(0.f, 0.f, 0.f); // translation offset for the scaling pivot
					Float3 scalingPivot(0.f, 0.f, 0.f); // center of scaling relative to node origin
					
					bool rotationIsActive = false;
					bool scalingIsActive = false;
					int rotationOrder = 0;
					
					for (FbxRecord property = properties.firstChild("Property"); property.isValid(); property = property.nextSibling("Property"))
					{
						std::vector<FbxValue> values = property.captureProperties<FbxValue>();
						
						if (values.size() >= 1)
						{
							const std::string & name = values[0].getString();
							
							if (name == "Lcl Translation")
								translationLocal.load(values);
							else if (name == "Lcl Rotation")
								rotationLocal.load(values);
							else if (name == "Lcl Scaling")
								scalingLocal.load(values);
							else if (name == "RotationOffset")
								rotationOffset.load(values);
							else if (name == "RotationPivot")
								rotationPivot.load(values);
							else if (name == "ScalingOffset")
								scalingOffset.load(values);
							else if (name == "ScalingPivot")
								scalingPivot.load(values);
							else if (name == "PreRotation")
								rotationPre.load(values);
							else if (name == "PostRotation")
								rotationPost.load(values);
							else if (name == "RotationActive")
								rotationIsActive = values.size() >= 4 ? values[3].getBool() : false;
							else if (name == "ScalingActive")
								scalingIsActive = values.size() >= 4 ? values[3].getBool() : false;
							
							// non supported properties ..
							else if (name == "GeometricTranslation")
							{
								Float3 temp(0.f, 0.f, 0.f);
								temp.load(values);
								if (temp[0] != 0.f || temp[1] != 0.f || temp[2] != 0.f)
									log(logIndent, "warning: geometric translation is non zero");
							}
							else if (name == "GeometricRotation")
							{
								Float3 temp(0.f, 0.f, 0.f);
								temp.load(values);
								if (temp[0] != 0.f || temp[1] != 0.f || temp[2] != 0.f)
									log(logIndent, "warning: geometric rotation is non zero");
							}
							else if (name == "GeometricScaling")
							{
								Float3 temp(1.f, 1.f, 1.f);
								temp.load(values);
								if (temp[0] != 1.f || temp[1] != 1.f || temp[2] != 1.f)
									log(logIndent, "warning: geometric scaling is not one");
							}
							else if (name == "RotationOrder")
							{
								if (values.size() >= 4)
								{
									rotationOrder = values[3].getInt();
									if (rotationOrder != 0)
										log(logIndent, "warning: rotation order is not XYZ");
								}
							}
						}
					}
					
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
					
					if (!rotationIsActive)
					{
						// if RotationActive is false, ignore RotationOrder, PreRotation and PostRotation
						
						rotationPre.SetZero();
						rotationPost.SetZero();
					}
					
					if (!scalingIsActive)
					{
						// ???
					}
					
					mesh->transform =
						MatrixTranslation(translationLocal) *
						MatrixTranslation(rotationOffset) *
						MatrixTranslation(rotationPivot) *
						MatrixRotation(rotationPre) *
						MatrixRotation(rotationLocal) *
						MatrixRotation(rotationPost) *
						MatrixTranslation(-rotationPivot) *
						MatrixTranslation(scalingOffset) *
						MatrixTranslation(scalingPivot) *
						MatrixScaling(scalingLocal) *
						MatrixTranslation(-scalingPivot);
					
					vertices.capturePropertiesAsFloat(mesh->vertices);
					vertexIndices.capturePropertiesAsInt(mesh->vertexIndices);
					normals.capturePropertiesAsFloat(mesh->normals);
					uvs.capturePropertiesAsFloat(mesh->uvs);
					uvIndices.capturePropertiesAsInt(mesh->uvIndices);
					colors.capturePropertiesAsFloat(mesh->colors);
					colorIndices.capturePropertiesAsInt(mesh->colorIndices);
					
					log(logIndent, "found a mesh! name: %s, hasVertices: %d (%d), hasNormals: %d, hasUVs: %d\n",
						objectName.c_str(),
						vertices.isValid(),
						int(mesh->vertices.size()),
						normals.isValid(),
						uvs.isValid());
					
					logIndent++;
					{
						log(logIndent+1, "transform:\n");
						for (int i = 0; i < 4; ++i)
						{
							log(logIndent + 2, "%8.2f %8.2f %8.2f %8.2f\n",
								mesh->transform(i, 0),
								mesh->transform(i, 1),
								mesh->transform(i, 2),
								mesh->transform(i, 3));
						}
					}
					logIndent--;
					
					// todo:
					// LayerElementMaterial.Materials
					// LayerElementTexture.TextureId
				}
				else
				{
					log(logIndent, "unknown object type: %s\n", objectType.c_str());
				}
			}
			else if (object.name == "Pose")
			{
				const FbxRecord & pose = object;
				
				FbxPose * fbxPose = new FbxPose(objectName);
				fbxObject = fbxPose;
				
				std::vector<std::string> poseProperties = pose.captureProperties<std::string>();
				
				std::string poseName;
				std::string poseType;
				
				if (poseProperties.size() >= 1)
					poseName = poseProperties[0];
				if (poseProperties.size() >= 2)
					poseType = poseProperties[1];
				
				// type = BindPose
				
				log(logIndent, "pose! name=%s, type=%s\n", poseName.c_str(), poseType.c_str());
				
				for (FbxRecord poseNode = pose.firstChild("PoseNode"); poseNode.isValid(); poseNode = poseNode.nextSibling("PoseNode"))
				{
					std::vector<std::string> nodeProperties = poseNode.firstChild("Node").captureProperties<std::string>();
					
					std::string nodeName;
					
					if (nodeProperties.size() >= 1)
						nodeName = nodeProperties[0];
					
					const std::vector<float> matrix = poseNode.firstChild("Matrix").captureProperties<float>();
					
					if (matrix.size() == 16)
					{
						Mat4x4 & fbxMatrix = fbxPose->matrices[nodeName];
						
						memcpy(fbxMatrix.m_v, &matrix[0], sizeof(float) * 16);
					}
					
					//log(logIndent, "pose node! matrixSize=%d, node=%s\n", int(matrix.size()), nodeName.c_str());
				}
			}
			else if (object.name == "Deformer")
			{
				const FbxRecord & deformer = object;
				
				FbxDeformer * fbxDeformer = new FbxDeformer(objectName);
				fbxObject = fbxDeformer;
				
				// deformer = bone with vertex indices + weights
				
				// todo: reference the connections table to determine which mesh to deform
				
				// string: <name>
				// string: <type>
				
				std::string name = deformer.captureProperty<std::string>(0);
				std::string type = deformer.captureProperty<std::string>(1);
				
				// type = Cluster, Skin
				
				deformer.firstChild("Indexes").capturePropertiesAsInt(fbxDeformer->indices);
				deformer.firstChild("Weights").capturePropertiesAsFloat(fbxDeformer->weights);
				
				//log(logIndent, "deformer! name=%s, type=%s, numIndices=%d, numWeights=%d\n", name.c_str(), type.c_str(), int(fbxDeformer->indices.size()), int(fbxDeformer->weights.size()));
			}
			else if (object.name == "Material")
			{
				fbxObject = new FbxObject("Material", objectName);
			}
			else if (object.name == "Texture")
			{
				fbxObject = new FbxObject("Texture", objectName);
			}
			
			if (fbxObject != 0)
			{
				if (objectsByName.count(fbxObject->name) == 0)
				{
					objectsByName[fbxObject->name] = fbxObject;
					objectList.push_back(fbxObject);
				}
				else
				{
					printf("duplicate object name !!!\n");
					delete fbxObject;
				}
			}
		}
	}
	
	// relations
	
	FbxRecord relations = reader.firstRecord("Relations");
	
	for (FbxRecord relation = relations.firstChild(); relation.isValid(); relation = relation.nextSibling())
	{
		std::vector<std::string> properties = relation.captureProperties<std::string>();
		
		if (properties.size() >= 2)
		{
			const std::string & object = properties[0];
			const std::string & type = properties[1];
			
			(void)object;
			(void)type;
			
			/*
			log(logIndent, "relation record=%s, object=%s, type=%s\n",
				relation.name.c_str(),
				object.c_str(),
				type.c_str());
			*/
		}
	}
	
	// connections = scene hierarchy, including bones
	
	FbxRecord connections = reader.firstRecord("Connections");
	
	for (FbxRecord connection = connections.firstChild("Connect"); connection.isValid(); connection = connection.nextSibling("Connect"))
	{
		std::vector<std::string> properties = connection.captureProperties<std::string>();
		
		if (properties.size() >= 3)
		{
			const std::string & fromName = properties[1];
			const std::string & toName = properties[2];
			
			if (fromName == toName)
			{
				log(logIndent, "warning: connect to self? %s\n", fromName.c_str());
			}
			else
			{
				ObjectsByName::iterator from = objectsByName.find(fromName);
				ObjectsByName::iterator to = objectsByName.find(toName);
	
				if (from != objectsByName.end() && to != objectsByName.end())
				{
					//log(logIndent, "connect %s -> %s\n", fromName.c_str(), toName.c_str());
					
					FbxObject * fromObject = from->second;
					FbxObject * toObject = to->second;
									
					toObject->children.push_back(fromObject);
					fromObject->parent = toObject;
				}
				else
				{
					log(logIndent, "error: unable to connect %s -> %s\n", fromName.c_str(), toName.c_str());
				}
			}
		}
	}
	
	FbxRecord takes = reader.firstRecord("Takes");
	
	logIndent++;
	
	for (FbxRecord take = takes.firstChild("Take"); take.isValid(); take = take.nextSibling("Take"))
	{
		const std::string name = take.captureProperty<std::string>(0);
		
		log(logIndent, "take: %s\n", name.c_str());
		
		std::string fileName = take.firstChild("FileName").captureProperty<std::string>(0);
		
		// ReferenceTime (int, timestamp?)
		
		logIndent++;
		
		for (FbxRecord model = take.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
		{
			std::string modelName = model.captureProperty<std::string>(0);
			
			//log(logIndent, "model: %s\n", modelName.c_str());
			
			logIndent++;
			
			for (FbxRecord channel = model.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
			{
				std::string channelName = channel.captureProperty<std::string>(0);
				
				//log(logIndent, "channel: %s\n", channelName.c_str());
				
				if (channelName == "Transform")
				{
					FbxTransformChannel transformChannel;
					
					logIndent++;
					{
						transformChannel.read(logIndent, channel);
					}
					logIndent--;
				}
			}
			
			logIndent--;
		}
		
		logIndent--;
	}
	
	logIndent--;
	
	// purge objects that aren't connected to the scene
	
	ObjectList garbage;
	
	for (ObjectList::iterator i = objectList.begin(); i != objectList.end(); ++i)
	{
		FbxObject * object = *i;
		
		bool isDead = true;
		
		for (FbxObject * parent = object; parent != 0; parent = parent->parent)
		{
			if (parent->persistent)
				isDead = false;
		}
		
		if (isDead)
		{
			log(logIndent, "garbage collecting object! %s\n", object->name.c_str());
			
			garbage.push_back(object);
		}
	}
	
	for (ObjectList::iterator i = garbage.begin(); i != garbage.end(); ++i)
	{
		FbxObject * object = *i;
		
		objectsByName.erase(object->name);
		
		delete object;
	}
	
	garbage.clear();
	objectList.clear();
	
	// apply deformers to meshes
	
	std::vector<FbxDeformer*> deformers;
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
		if (object->type == "Deformer")
		{
			FbxDeformer * deformer = static_cast<FbxDeformer*>(object);
			
			if (deformer->indices.size() != deformer->weights.size())
			{
				log(logIndent, "error: deformer indices / weights mismatch");
				continue;
			}
			
			FbxMesh * mesh = 0;
			
		#if 0
			for (size_t j = 0; j < deformer->children.size(); ++j)
			{
				FbxObject * child = deformer->children[j];
				
				if (child->type == "Mesh")
				{
					mesh = static_cast<FbxMesh*>(child);
					break;
				}
			}
		#else
			for (FbxObject * parent = deformer->parent; parent != 0; parent = parent->parent)
			{
				if (parent->type == "Mesh")
				{
					mesh = static_cast<FbxMesh*>(parent);
					break;
				}
			}
		#endif
			
			if (mesh != 0)
			{
				//log(logIndent, "found mesh for deformer! %s\n", mesh->name.c_str());
				
				const int deformerIndex = int(deformers.size());
				
				deformers.push_back(deformer);
				
				if (mesh->deformers.empty())
				{
					mesh->deformers.resize(mesh->vertices.size());
				}
				
				for (size_t j = 0; j < deformer->indices.size(); ++j)
				{
					const int vertexIndex = deformer->indices[j];
					
					if (vertexIndex < int(mesh->deformers.size()))
					{
						mesh->deformers[vertexIndex].add(deformerIndex, deformer->weights[j]);
					}
					else
					{
						log(logIndent, "error: deformer index %d is out of range\n", vertexIndex);
					}
				}
			}
		}
	}
	
	//logEnabled = false;
	
	// finalize meshes by invoking the powers of the awesome vertex welding machine
	
	std::list<Mesh> meshes;
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
		if (object->type == "Mesh")
		{
			FbxMesh * fbxMesh = static_cast<FbxMesh*>(object);
			
			meshes.push_back(Mesh());
			Mesh & mesh = meshes.back();
			
			mesh.construct(
				logIndent,
				fbxMesh->vertices,
				fbxMesh->vertexIndices,
				fbxMesh->normals,
				fbxMesh->uvs,
				fbxMesh->uvIndices,
				fbxMesh->colors,
				fbxMesh->colorIndices,
				fbxMesh->deformers);
			
			mesh.m_transform = fbxMesh->transform;
		}
	}
	
	// find the pose object
	
	FbxPose * pose = 0;
	
	for (ObjectsByName::iterator j = objectsByName.begin(); j != objectsByName.end(); ++j)
	{
		FbxObject * object = j->second;
		
		if (object->type == "Pose")
		{
			pose = static_cast<FbxPose*>(object);
		}
	}
	
	std::vector<Mat4x4> deformerMatrices;
	deformerMatrices.resize(deformers.size());
	
	if (pose)
	{
		for (size_t i = 0; i < deformers.size(); ++i)
		{
			const FbxDeformer * deformer = deformers[i];
			const std::string & deformerName = deformer->children[0]->name;
			if (pose->matrices.count(deformerName) != 0)
				deformerMatrices[i] = pose->matrices[deformerName].CalcInv();
			else
			{
				printf("warning: no pose matrix for deformer %s\n", deformerName.c_str());
				deformerMatrices[i].MakeIdentity();
			}
		}
	}
	else
	{
		for (size_t i = 0; i < deformers.size(); ++i)
			deformerMatrices[i].MakeIdentity();
	}
	
	const int time2 = getTimeMS();
	
	printf("time: %d ms\n", time2-time1);
	
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
	
	if (drawMeshes)
	{
		// initialize SDL
		
		SDL_Init(SDL_INIT_EVERYTHING);
		if (SDL_SetVideoMode(640, 480, 32, SDL_OPENGL) < 0)
		{
			log(0, "failed to intialize SDL");
			exit(-1);
		}
		
		bool wireframe = false;
		
		bool stop = false;
		
		while (!stop)
		{
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE)
						stop = true;
					else if (e.key.keysym.sym == SDLK_w)
						wireframe = !wireframe;
				}
			}
			
			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClearDepth(1.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			glColor3ub(255, 255, 255);
			
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			
			static float r = 0.f;
			r += 1.f;
			
			for (std::list<Mesh>::iterator i = meshes.begin(); i != meshes.end(); ++i)
			{
				const Mesh & mesh = *i;
				
				glPushMatrix();
				
				glTranslatef(0.f, -.5f, 0.f);
				
				glRotatef(r, 0.f, 1.f, 0.f);
				//glRotatef(r/10.f, 1.f, 0.f, 0.f);
				
				glMultMatrixf(mesh.m_transform.m_v);
				glRotatef(-90.f, 1.f, 0.f, 0.f);
				const float scale = 0.005f;
				glScalef(scale, scale, scale);
			
				int vertexCount = 0;
				
				std::vector<Mat4x4> boneToObjectMatrices;
				boneToObjectMatrices.resize(deformerMatrices.size());
				for (size_t boneIndex = 0; boneIndex < deformerMatrices.size(); ++boneIndex)
				{
					boneToObjectMatrices[boneIndex] = deformerMatrices[boneIndex].CalcInv();
					
					Mat4x4 rot;
					rot.MakeRotationX(r * boneIndex / 200.f);
					boneToObjectMatrices[boneIndex] = boneToObjectMatrices[boneIndex] * rot;
					
					boneToObjectMatrices[boneIndex](3,0) += sin(M_PI * r/180.f * boneIndex / 1.23f) * 10.f;
					boneToObjectMatrices[boneIndex](3,1) += sin(M_PI * r/180.f * boneIndex / 2.34f) * 10.f;
					boneToObjectMatrices[boneIndex](3,2) += sin(M_PI * r/180.f * boneIndex / 3.45f) * 10.f;
				}
				
				for (size_t i = 0; i < mesh.m_indices.size(); ++i)
				{
					bool begin = vertexCount == 0;
					
					if (begin)
					{
						glBegin(GL_POLYGON);
					}
					
					int index = mesh.m_indices[i];
					
					bool end = false;
					
					if (index < 0)
					{
						// negative indices mark the end of a polygon
						
						index = ~index;
						
						end = true;
					}
					
					const Mesh::Vertex & v = mesh.m_vertices[index];
					
					Vec3 p(0.f, 0.f, 0.f);
					
					for (int j = 0; j < 4; ++j)
					{
						const int boneIndex = v.bi[j];
						const float boneWeight = v.bw[j] / 255.f;
						
						if (boneWeight == 0.f)
							continue;
						
						// todo: pose + bone matrix
						
						const Mat4x4 & objectToBone = deformerMatrices[boneIndex];
						const Mat4x4 & boneToObject = boneToObjectMatrices[boneIndex];
						
						p += ((boneToObject * objectToBone) * Vec3(v.px, v.py, v.pz)) * boneWeight;
					}
					
					float r = 1.f;
					float g = 1.f;
					float b = 1.f;
					
				#if 1
					r *= (v.nx + 1.f) / 2.f;
					g *= (v.ny + 1.f) / 2.f;
					b *= (v.nz + 1.f) / 2.f;
				#endif
				#if 0
					r *= v.bi[0]/25.f;
					g *= v.bi[1]/25.f;
					b *= v.bi[2]/25.f;
				#endif
				
					glColor3f(r, g, b);
					glVertex3f(p[0], p[1], p[2]);
					
					vertexCount++;
					
					if (end)
					{
						glEnd();
						
						vertexCount = 0;
					}
				}
				
				if (vertexCount != 0)
				{
					assert(false);
					
					glEnd();
				}
				
			#if 0
				glDisable(GL_DEPTH_TEST);
				for (ObjectsByName::iterator j = objectsByName.begin(); j != objectsByName.end(); ++j)
				{
					FbxObject * object = j->second;
					
					if (object->type == "Pose")
					{
						FbxPose * pose = static_cast<FbxPose*>(object);
						
						glBegin(GL_POINTS);
						{
							for (std::map<std::string, Mat4x4>::iterator i = pose->matrices.begin(); i != pose->matrices.end(); ++i)
							{
								const Mat4x4 & m = i->second;
								glColor3ub(255, 255, 255);
								glVertex3f(m(3, 0), m(3, 1), m(3, 2));
							}
						}
						glEnd();
					}
				}
				glEnable(GL_DEPTH_TEST);
			#endif
				
				glPopMatrix();
			}
			
			SDL_GL_SwapBuffers();
		}

		
		
		SDL_Quit();
	}
	
	return 0;
}
