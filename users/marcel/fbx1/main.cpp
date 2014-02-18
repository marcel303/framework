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

#include "../framework/model.h"

using namespace Model;

static bool logEnabled = true;

static bool treatAsMesh(const std::string & name)
{
	// todo: create separate object for non meshes
	//return name == "Mesh" || name == "LimbNode" || name == "Null" || name == "Root";
	return true;
}

static RotationType convertRotationOrder(int order)
{
	switch (order)
	{
	case 0:
		return RotationType_EulerXYZ;
	default:
		assert(false);
		return RotationType_EulerXYZ;
	}
}

static Mat4x4 matrixTranslation(const Vec3 & v);
static Mat4x4 matrixRotation(const Vec3 & v, RotationType rotationType);
static Mat4x4 matrixScaling(const Vec3 & v);

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
				log(m_logIndent, "string: %s\n", value.getString());
				break;
			
			case FbxValue::TYPE_INVALID:
				log(m_logIndent, "(invalid)\n");
				break;
		}
	}
};

//

class FbxDeformer;
class FbxMesh;
class FbxObject;
class FbxPose;

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

	virtual void helpMeDebugWithVTables() { }
	
	std::string type;
	std::string name;
	bool persistent;
	FbxObject * parent;
};

class FbxMesh : public FbxObject
{
public:
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
				// replace a lower weight entry (if one exists)
				
				int minWeightIndex = 0;
				
				for (int i = 1; i < numEntries; ++i)
				{
					if (entries[i].weight < entries[minWeightIndex].weight)
						minWeightIndex = i;
				}
				
				if (weight > entries[minWeightIndex].weight)
				{
					entries[minWeightIndex].index = index;
					entries[minWeightIndex].weight = weight;
				}
			}
		}
	};
	
	Mat4x4 transform;
	
	std::vector<float> vertices;
	std::vector<int> vertexIndices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<int> uvIndices;
	std::vector<float> colors;
	std::vector<int> colorIndices;
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
	Mat4x4 transform;
	Mat4x4 transformLink;

	FbxMesh * mesh;
	
	FbxDeformer(const std::string & name)
		: FbxObject("Deformer", name)
		, mesh(0)
	{
	}
};

class FbxAnimTransform
{
public:
	class Channel
	{
	public:
		struct Key
		{
			float time;
			float value;
		};
		
		struct KeyList
		{
			std::vector<Key> keys;
			
			void evaluate(float time, float & value) const
			{
				if (!keys.empty())
				{
					const Key * firstKey = &keys[0];
					const Key * key = firstKey;
					const Key * lastKey = firstKey + keys.size() - 1;
					
					while (key != lastKey && time >= key[1].time)
					{
						key++;
					}
					
					if (key != lastKey && time >= key->time)
					{
						const Key & key1 = key[0];
						const Key & key2 = key[1];
						
						assert(time >= key1.time && time <= key2.time);
						
						const float t = (time - key1.time) / (key2.time - key1.time);
						
						assert(t >= 0.f && t <= 1.f);
						
						value = key1.value * (1.f - t) + key2.value * t;
					}
					else
					{
						// either the first or last key in the animation. copy value
						
						assert(key == firstKey || key == lastKey);
						
						value = key->value;
					}
				}
			}
			
			void read(int & logIndent, const FbxRecord & record, float & endTime)
			{
				const int keyCount = record.firstChild("KeyCount").captureProperty<int>(0);
				
				if (keyCount != 0)
				{
					const FbxRecord key = record.firstChild("Key");
					std::vector<FbxValue> values;
					key.captureProperties<FbxValue>(values);
					const int stride = int(values.size()) / keyCount;
					
					//log(logIndent, "keyCount=%d\n", keyCount);
					
					for (size_t i = 0; i + stride <= values.size(); i += stride)
					{
						float value = get<float>(values[i + 1]);
						
					#if 1 // todo: enable duplicate removal
						// round with a fixed precision so small floating point drift is eliminated from the exported values
						value = int(value * 1000.f + .5f) / 1000.f;
						
						// don't write duplicate values, unless it's the first/last key in the list. exporters sometimes write a fixed number of keys (sampling based), with lots of duplicates
						const bool isDuplicate = keys.size() >= 1 && keys.back().value == value;// && i + stride != values.size();
					#else
						const bool isDuplicate = false;
					#endif
						
						const float time = float((get<int64_t>(values[i + 0]) / 1000000) / 1000.0 / 60.0);
						
						if (!isDuplicate)
						{
							Key key;
							key.time = time;
							key.value = value;
							keys.push_back(key);
							
							//log(logIndent, "%014lld -> %f\n", temp.time, temp.value);
						}
						
						if (time > endTime)
						{
							endTime = time;
						}
					}
					
					//log(logIndent, "got %d unique keys\n", keys.size());
				}
			}
		};
		
		KeyList X;
		KeyList Y;
		KeyList Z;
		
		void read(int & logIndent, const FbxRecord & record, float & endTime)
		{
			for (FbxRecord channel = record.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
			{
				const std::string name = channel.captureProperty<std::string>(0);
				
				//log(logIndent, "stream: name=%s\n", name.c_str());
				
				logIndent++;
				{
					if (name == "X")
						X.read(logIndent, channel, endTime);
					if (name == "Y")
						Y.read(logIndent, channel, endTime);
					if (name == "Z")
						Z.read(logIndent, channel, endTime);
				}
				logIndent--;
			}
		}
	};
	
	Channel T;
	Channel R;
	Channel S;
	
	std::vector<AnimKey> animKeys;
	
	float m_endTime;
	
	FbxAnimTransform()
	{
		m_endTime = 0.f;
	}
	
	void read(int & logIndent, const FbxRecord & record)
	{
		m_endTime = 0.f;
		
		for (FbxRecord channel = record.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
		{
			const std::string name = channel.captureProperty<std::string>(0);
			
			logIndent++;
			{
				if (name == "T") T.read(logIndent, channel, m_endTime);
				if (name == "R") R.read(logIndent, channel, m_endTime);
				if (name == "S") S.read(logIndent, channel, m_endTime);
			}
			logIndent--;
		}
		
		buildAnimKeyFrames(T, R, S);
	}
	
	void buildAnimKeyFrames(const Channel & t, const Channel & r, const Channel & s)
	{
		// extract key frames times from all channels
		
		std::vector<float> times;
		
	#define ADD_TIMES(keyList) \
		for (size_t i = 0; i < keyList.keys.size(); ++i) \
			times.push_back(keyList.keys[i].time)
		
		ADD_TIMES(T.X);
		ADD_TIMES(T.Y);
		ADD_TIMES(T.Z);
		ADD_TIMES(R.X);
		ADD_TIMES(R.Y);
		ADD_TIMES(R.Z);
		ADD_TIMES(S.X);
		ADD_TIMES(S.Y);
		ADD_TIMES(S.Z);
		
	#undef ADD_TIMES
		
		// remove duplicate key frame times
		
		std::sort(times.begin(), times.end());
		
		std::vector<float> uniqueTimes;
		uniqueTimes.reserve(times.size());
		
		if (times.size() >= 1)
		{
			uniqueTimes.push_back(times[0]);
			for (size_t i = 1; i < times.size(); ++i)
			{
				if (times[i] != uniqueTimes.back())
					uniqueTimes.push_back(times[i]);
			}
		}
		
		// sample key frames and produce a merged set of key frames containing data for all channels
		
		animKeys.resize(uniqueTimes.size());
		
		for (size_t i = 0; i < uniqueTimes.size(); ++i)
		{
			const float time = uniqueTimes[i];
			
			// sample the separate key frame channels
			
			AnimKey & animKey = animKeys[i];
			
			Quat quat;
			evaluateRaw(t, r, s, time, animKey.translation, quat, animKey.scale);
			
			animKey.time = time;
			animKey.rotation[0] = quat[0];
			animKey.rotation[1] = quat[1];
			animKey.rotation[2] = quat[2];
			animKey.rotation[3] = quat[3];
			animKey.scale = Vec3(0.f, 0.f, 0.f);
		}
	}
	
	void evaluateRaw(const Channel & t, const Channel & r, const Channel & s, float time, Vec3 & translation, Vec3 & rotation, Vec3 & scale) const
	{
		t.X.evaluate(time, translation[0]);
		t.Y.evaluate(time, translation[1]);
		t.Z.evaluate(time, translation[2]);
		r.X.evaluate(time, rotation[0]);
		r.Y.evaluate(time, rotation[1]);
		r.Z.evaluate(time, rotation[2]);
		s.X.evaluate(time, scale[0]);
		s.Y.evaluate(time, scale[1]);
		s.Z.evaluate(time, scale[2]);
	}
	
	void evaluateRaw(const Channel & t, const Channel & r, const Channel & s, float time, Vec3 & translation, Quat & rotation, Vec3 & scale) const
	{
		Vec3 rotationVec;
		
		evaluateRaw(t, r, s, time, translation, rotationVec, scale);
		
		rotationVec *= M_PI / 180.f;
		
		Quat quatX;
		Quat quatY;
		Quat quatZ;
		
		quatX.fromAxisAngle(Vec3(1.f, 0.f, 0.f), rotationVec[0]);
		quatY.fromAxisAngle(Vec3(0.f, 1.f, 0.f), rotationVec[1]);
		quatZ.fromAxisAngle(Vec3(0.f, 0.f, 1.f), rotationVec[2]);
		
		rotation = quatZ * quatY * quatX;
	}
	
	bool evaluateRaw(const Channel & t, const Channel & r, const Channel & s, float time, Mat4x4 & result) const
	{
		Vec3 translation;
		Vec3 rotation;
		Vec3 scale(1.f, 1.f, 1.f);
		
		evaluateRaw(t, r, s, time, translation, rotation, scale);
		
		result = matrixTranslation(translation) * matrixRotation(rotation, RotationType_EulerXYZ) * matrixScaling(scale);
		
		return time >= m_endTime;
	}
	
	bool evaluate(float time, Mat4x4 & result) const
	{
		// 21 ms to 11 ms
		
		//if (rand() % 4)
		//	return evaluateRaw(T, R, S, time, result);
		
		Quat quat;
		Vec3 translation;
		
		if (animKeys.empty())
		{
			quat.makeIdentity();
		}
		else
		{
			const AnimKey * firstKey = &animKeys[0];
			const AnimKey * key = firstKey;
			const AnimKey * lastKey = firstKey + animKeys.size() - 1;
			
			while (key != lastKey && time >= key[1].time)
			{
				key++;
			}
			
			if (key != lastKey && time >= key->time)
			{
				const AnimKey & key1 = key[0];
				const AnimKey & key2 = key[1];
				
				assert(time >= key1.time && time <= key2.time);
				
				const float t = (time - key1.time) / (key2.time - key1.time);
				
				assert(t >= 0.f && t <= 1.f);
				
				const float t1 = 1.f - t;
				const float t2 = t;
				
				Quat quat1(key1.rotation[0], key1.rotation[1], key1.rotation[2], key1.rotation[3]);
				Quat quat2(key2.rotation[0], key2.rotation[1], key2.rotation[2], key2.rotation[3]);
				
				quat = quat1.slerp(quat2, t);
				translation = key1.translation * t1 + key2.translation * t2;
			}
			else
			{
				// either the first or last key in the animation. copy value
				
				assert(key == firstKey || key == lastKey);
				
				quat = Quat(key->rotation[0], key->rotation[1], key->rotation[2], key->rotation[3]);
				translation = key->translation;
			}
		}
		
		quat.toMatrix3x3(result);
		
		result(0, 3) = 0.f;
		result(1, 3) = 0.f;
		result(2, 3) = 0.f;
		
		result(3, 0) = translation[0];
		result(3, 1) = translation[1];
		result(3, 2) = translation[2];
		result(3, 3) = 1.f;
		
		return time >= m_endTime;
	}
};

class FbxAnim
{
public:
	std::string name;
	std::map<std::string, FbxAnimTransform> transforms;
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

class MeshBuilder
{
public:
	class Vertex : public Model::Vertex
	{
	public:
		inline bool operator==(const Vertex & w) const
		{
			return !memcmp(this, &w, sizeof(Vertex));
		}
		
		/*
		float px, py, pz;     // position
		float nx, ny, nz;     // normal
		float tx, ty;         // texture
		float cx, cy, cz, cw; // color
		
		uint8_t boneIndices[4]; // blend indices
		uint8_t boneWeights[4]; // blend weights
		*/
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
		const std::vector<int> & colorIndices,
		const std::vector<FbxMesh::Deformer> & deformers)
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
				vertex.boneIndices[d] = deformers[vertexIndex].entries[d].index;
				vertex.boneWeights[d] = int8_t(deformers[vertexIndex].entries[d].weight * 255.f);
				
				//log(logIndent, "added %d, %d\n", vertex.blendIndices[d], vertex.boneWeights[d]);
			}
			for (int d = numDeformers; d < 4; ++d)
			{
				vertex.boneIndices[d] = 0;
				vertex.boneWeights[d] = 0;
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
		
		// triangulate
		
		std::vector<int> temp;
		temp.reserve(m_indices.size());
		
		if (m_indices.size() >= 3)
		{
			for (size_t i = 0; i < m_indices.size(); )
			{
				int index1 = m_indices[i + 0];
				int index2 = m_indices[i + 1];
				int index3 = m_indices[i + 2];
				
				if (index1 < 0)
				{
					// single point?!
					i += 1;
					continue;
				}
				
				if (index2 < 0)
				{
					// line segment?!
					i += 2;
					continue;
				}
				
				// at the very least it's a triangle
				temp.push_back(index1);
				temp.push_back(index2);
				temp.push_back(index3);
				i += 3;
				
				if (index3 < 0)
				{
					// we're done
					continue;
				}
				
				// generate additional triangles
				
				while (i < m_indices.size())
				{
					index2 = index3;
					index3 = m_indices[i];
					i += 1;
					
					temp.push_back(index1);
					temp.push_back(index2);
					temp.push_back(index3);
					
					if (index3 < 0)
						break;
				}
			}
			
			log(logIndent, "triangulation result: %d -> %d indices\n", int(m_indices.size()), int(temp.size()));
			
			m_indices = temp;
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

static Mat4x4 matrixTranslation(const Vec3 & v)
{
	Mat4x4 t;
	t.MakeTranslation(v);
	return t;
}

static Mat4x4 matrixRotation(const Vec3 & v, RotationType rotationType)
{
	const Vec3 temp = v * M_PI / 180.f;
	
	Quat quatX;
	Quat quatY;
	Quat quatZ;
	
	quatX.fromAxisAngle(Vec3(1.f, 0.f, 0.f), temp[0]);
	quatY.fromAxisAngle(Vec3(0.f, 1.f, 0.f), temp[1]);
	quatZ.fromAxisAngle(Vec3(0.f, 0.f, 1.f), temp[2]);
	
	Quat quat;
	
	switch (rotationType)
	{
	case RotationType_EulerXYZ:
		quat = quatZ * quatY * quatX;
		break;
		
	default:
		assert(false);
		quat.makeIdentity();
		break;
	}
	
	return quat.toMatrix();
}

static Mat4x4 matrixScaling(const Vec3 & v)
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
		log(logIndent, "failed to open %s\n", filename);
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
	
	typedef std::vector<FbxObject*> ObjectList;
	typedef std::map<std::string, FbxObject*> ObjectsByName;
	ObjectsByName objectsByName;
	
	FbxObject * scene = new FbxObject("Scene", "Scene", true);
	objectsByName[scene->name] = scene;
	
	// extract meshes
	
	log(logIndent, "-- extracting mesh data --\n");
	
	const int time1 = getTimeMS();
	
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
				
				// todo: allocate generic node object for non mesh
				
				if (treatAsMesh(objectType))
				{
					FbxMesh * mesh = new FbxMesh("Mesh", objectName);
					fbxObject = mesh;
					
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
					
					RotationType rotationType = convertRotationOrder(rotationOrder);
					
					mesh->transform =
						matrixTranslation(translationLocal) *
						matrixTranslation(rotationOffset) *
						matrixTranslation(rotationPivot) *
						matrixRotation(rotationPre, rotationType) *
						matrixRotation(rotationLocal, rotationType) *
						matrixRotation(rotationPost, rotationType) *
						matrixTranslation(-rotationPivot) *
						matrixTranslation(scalingOffset) *
						matrixTranslation(scalingPivot) *
						matrixScaling(scalingLocal) *
						matrixTranslation(-scalingPivot);
					
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
					log(logIndent, "unknown object type: %s (name=%s)\n", objectType.c_str(), objectName.c_str());
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
					else
					{
						log(logIndent, "warning: pose matrix doesn't contain 16 elements\n");
					}
					
					//log(logIndent, "pose node! matrixSize=%d, node=%s\n", int(matrix.size()), nodeName.c_str());
				}
			}
			else if (object.name == "Deformer")
			{
				const FbxRecord & deformer = object;
				
				FbxDeformer * fbxDeformer = new FbxDeformer(objectName);
				fbxObject = fbxDeformer;
				
				//std::string name = deformer.captureProperty<std::string>(0);
				//std::string type = deformer.captureProperty<std::string>(1);
				
				// type = Cluster, Skin
				
				deformer.firstChild("Indexes").capturePropertiesAsInt(fbxDeformer->indices);
				deformer.firstChild("Weights").capturePropertiesAsFloat(fbxDeformer->weights);
				
				std::vector<float> transform;
				std::vector<float> transformLink;
				
				deformer.firstChild("Transform").capturePropertiesAsFloat(transform);
				deformer.firstChild("TransformLink").capturePropertiesAsFloat(transformLink);
				
				if (transform.size() == 16)
					memcpy(fbxDeformer->transform.m_v, &transform[0], sizeof(float) * 16);
				else
					fbxDeformer->transform.MakeIdentity();
				if (transformLink.size() == 16)
					memcpy(fbxDeformer->transformLink.m_v, &transformLink[0], sizeof(float) * 16);
				else
					fbxDeformer->transformLink.MakeIdentity();
				
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
				}
				else
				{
					log(logIndent, "duplicate object name !!!\n");
					delete fbxObject;
				}
			}
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
					
					// todo: figure out a better way to deal with object to object connections
					//       the current implementation breaks if another type of node object
					//       is present in the scene
					if (fromObject->type == "Mesh" && toObject->type == "Deformer")
					{
						FbxMesh * mesh = static_cast<FbxMesh*>(fromObject);
						FbxDeformer * deformer = static_cast<FbxDeformer*>(toObject);
						
						deformer->mesh = mesh;
					}
					else if (toObject->type == "Mesh" && fromObject->type == "Mesh")
					{
						fromObject->parent = toObject;
					}
					else
					{
						fromObject->parent = toObject;
					}
				}
				else
				{
					log(logIndent, "error: unable to connect %s -> %s\n", fromName.c_str(), toName.c_str());
				}
			}
		}
	}
	
	FbxRecord takes = reader.firstRecord("Takes");
	
	typedef std::list<FbxAnim> AnimList;
	AnimList anims;
	
	logIndent++;
	
	for (FbxRecord take = takes.firstChild("Take"); take.isValid(); take = take.nextSibling("Take"))
	{
		const std::string name = take.captureProperty<std::string>(0);
		
		anims.push_back(FbxAnim());
		FbxAnim & anim = anims.back();
		
		anim.name = name;
		
		log(logIndent, "take: %s\n", name.c_str());
		
		std::string fileName = take.firstChild("FileName").captureProperty<std::string>(0);
		
		// ReferenceTime (int, timestamp?)
		
		logIndent++;
		
		for (FbxRecord model = take.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
		{
			std::string modelName = model.captureProperty<std::string>(0);
			
			log(logIndent, "model: %s\n", modelName.c_str());
			
			logIndent++;
			
			for (FbxRecord channel = model.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
			{
				std::string channelName = channel.captureProperty<std::string>(0);
				
				//log(logIndent, "channel: %s\n", channelName.c_str());
				
				if (channelName == "Transform")
				{
					logIndent++;
					{
						FbxAnimTransform & animTransform = anim.transforms[modelName];
						
						animTransform.read(logIndent, channel);
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
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
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
	
	// generate indices for meshes
	
	typedef std::map<std::string, int> ModelNameToBoneIndex;
	ModelNameToBoneIndex modelNameToBoneIndex;
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
		if (object->type == "Mesh")
		{
			if (modelNameToBoneIndex.count(object->name) == 0)
			{
				const int index = int(modelNameToBoneIndex.size());
				modelNameToBoneIndex[object->name] = index;
			}
			else
			{
				log(logIndent, "warning: duplicate object name: %s\n", object->name.c_str());
			}
		}
	}
	
	// allocate bone matrix for each mesh
	
	log(logIndent, "allocating %d bones\n", int(modelNameToBoneIndex.size()));
	
	std::vector<Mat4x4> objectToBoneMatrices;
	std::vector<Mat4x4> boneToObjectMatrices;
	std::vector<int> boneParentIndices;
	
	objectToBoneMatrices.resize(modelNameToBoneIndex.size());
	boneToObjectMatrices.resize(modelNameToBoneIndex.size());
	boneParentIndices.resize(modelNameToBoneIndex.size(), -1);
	
	// setup mesh hierarchy
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
		if (object->type == "Mesh")
		{
			assert(modelNameToBoneIndex.count(object->name) != 0);
			const int index = modelNameToBoneIndex[object->name];
			
			FbxObject * parent = object->parent;
			
			while (parent)
			{
				if (parent->type == "Mesh")
					break;
				parent = parent->parent;
			}
			
			if (parent)
			{
				assert(modelNameToBoneIndex.count(parent->name) != 0 && boneParentIndices[index] == -1);
				boneParentIndices[index] = modelNameToBoneIndex[parent->name];
			}
			else
			{
				boneParentIndices[index] = -1;
			}
		}
	}
	
	// apply deformers to meshes
	
	std::vector<FbxDeformer*> deformers; // debug only?
	
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
			
			// todo: track deformer -> deformer connections and find skin deformer object (which has a reference to the mesh)
			
			FbxMesh * mesh = 0;
			
			for (FbxObject * parent = deformer->parent; parent != 0; parent = parent->parent)
			{
				if (parent->type == "Mesh")
				{
					mesh = static_cast<FbxMesh*>(parent);
					break;
				}
			}
			
			FbxMesh * boneMesh = 0;
			
			for (FbxObject * parent = deformer->mesh; parent != 0; parent = parent->parent)
			{
				if (parent->type == "Mesh")
				{
					boneMesh = static_cast<FbxMesh*>(parent);
					break;
				}
			}
			
			if (mesh != 0 && boneMesh != 0)
			{
				//log(logIndent, "found mesh for deformer! %s\n", mesh->name.c_str());
				
				const std::string & modelName = boneMesh->name;
				const int boneIndex = modelNameToBoneIndex[modelName];
				
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
						mesh->deformers[vertexIndex].add(boneIndex, deformer->weights[j]);
					}
					else
					{
						log(logIndent, "error: deformer index %d is out of range [max=%d]\n", vertexIndex, int(mesh->deformers.size()));
					}
				}
			}
			else
			{
				log(logIndent, "error: deformer %s: mesh=%p, boneMesh=%p\n", deformer->name.c_str(), mesh, boneMesh);
			}
		}
	}
	
	// finalize meshes by invoking the powers of the awesome vertex welding machine
	
	std::list<MeshBuilder> meshes; // todo: deprecate and use framework classes
	
	for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
	{
		FbxObject * object = i->second;
		
		if (object->type == "Mesh")
		{
			FbxMesh * fbxMesh = static_cast<FbxMesh*>(object);
			
			meshes.push_back(MeshBuilder());
			MeshBuilder & meshBuilder = meshes.back();
			
			// todo: triangulate mesh
			
			meshBuilder.construct(
				logIndent,
				fbxMesh->vertices,
				fbxMesh->vertexIndices,
				fbxMesh->normals,
				fbxMesh->uvs,
				fbxMesh->uvIndices,
				fbxMesh->colors,
				fbxMesh->colorIndices,
				fbxMesh->deformers);
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
	
	if (pose)
	{
		for (ModelNameToBoneIndex::iterator i = modelNameToBoneIndex.begin(); i != modelNameToBoneIndex.end(); ++i)
		{
			const std::string & modelName = i->first;
			const int boneIndex = i->second;
			
			if (pose->matrices.count(modelName) != 0)
			{
				objectToBoneMatrices[boneIndex] = pose->matrices[modelName].CalcInv();
			}
			else
			{
				log(logIndent, "warning: no pose matrix for deformer %s\n", modelName.c_str());
				objectToBoneMatrices[boneIndex].MakeIdentity();
			}
		}
	}
	else
	{
		for (ModelNameToBoneIndex::iterator i = modelNameToBoneIndex.begin(); i != modelNameToBoneIndex.end(); ++i)
		{
			const int boneIndex = i->second;
			objectToBoneMatrices[boneIndex].MakeIdentity();
		}
	}
	
	const int time2 = getTimeMS();
	
	printf("time: %d ms\n", time2-time1);
	
	// dump mesh data
	
	if (dumpMeshes)
	{
		log(logIndent, "-- dumping mesh data --\n");
		
		for (std::list<MeshBuilder>::iterator i = meshes.begin(); i != meshes.end(); ++i)
		{
			const MeshBuilder & mesh = *i;

			log(logIndent, "mesh: numVertices=%d, numIndices=%d\n", int(mesh.m_vertices.size()), int(mesh.m_indices.size()));
			
			logIndent++;
			
			int polygonIndex = ~0;
			int vertexIndex = 0;
			
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
				
				vertexIndex++;
				
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
					if (vertexIndex != 3)
						exit(-1); // fixme
					
					log(logIndent + 1, "[%d vertices]\n", vertexIndex);
					
					polygonIndex = ~(polygonIndex + 1);
					vertexIndex = 0;
				}
			}
			
			logIndent--;
		}
	}
	
	// >>> framework code begin
	
	// create meshes
	
	std::vector<Mesh*> meshes2;
	
	for (std::list<MeshBuilder>::iterator i = meshes.begin(); i != meshes.end(); ++i)
	{
		const MeshBuilder & meshBuilder = *i;
		
		Mesh * mesh = new Mesh();
		
		mesh->allocateVB(meshBuilder.m_vertices.size());
		
		memcpy(mesh->m_vertices, &meshBuilder.m_vertices[0], sizeof(mesh->m_vertices[0]) * mesh->m_numVertices);
		
		mesh->allocateIB(meshBuilder.m_indices.size());
		
		for (size_t j = 0; j < meshBuilder.m_indices.size(); ++j)
		{
			int index = meshBuilder.m_indices[j];
			
			if (index < 0)
				index = ~index;
			
			mesh->m_indices[j] = index;
		}
		
		meshes2.push_back(mesh);
	}
	
	// create mesh set
	
	MeshSet * meshSet = new MeshSet();
	meshSet->allocate(meshes2.size());
	for (size_t i = 0; i < meshes2.size(); ++i)
		meshSet->m_meshes[i] = meshes2[i];
	
	// create bone set
	
	BoneSet * boneSet = new BoneSet();
	
	boneSet->allocate(modelNameToBoneIndex.size());
	
	for (size_t i = 0; i < modelNameToBoneIndex.size(); ++i)
	{
		boneSet->m_bones[i].poseMatrix = objectToBoneMatrices[i];
		boneSet->m_bones[i].parent = boneParentIndices[i];
	}
	
	boneSet->calculateBoneMatrices();
	
	// create animations
	
	std::map<std::string, Anim*> animations;
	
	for (AnimList::iterator i = anims.begin(); i != anims.end(); ++i)
	{
		const FbxAnim & anim = *i;
		
		int numAnimKeys = 0;
		
		for (std::map<std::string, FbxAnimTransform>::const_iterator i = anim.transforms.begin(); i != anim.transforms.end(); ++i)
		{
			const FbxAnimTransform & animTransform = i->second;
			
			numAnimKeys += animTransform.animKeys.size();
		}
		
		std::map<int, const FbxAnimTransform*> boneIndexToAnimTransform;
		
		for (std::map<std::string, FbxAnimTransform>::const_iterator i = anim.transforms.begin(); i != anim.transforms.end(); ++i)
		{
			const std::string & modelName = i->first;
			const FbxAnimTransform & animTransform = i->second;
			const int boneIndex = modelNameToBoneIndex[modelName];
			boneIndexToAnimTransform[boneIndex] = &animTransform;
		}
		
		Anim * animation = new Anim();
		
		animation->allocate(modelNameToBoneIndex.size(), numAnimKeys, RotationType_Quat);
		
		AnimKey * finalAnimKey = animation->m_keys;
		
		for (size_t boneIndex = 0; boneIndex < modelNameToBoneIndex.size(); ++boneIndex)
		{
			std::map<int, const FbxAnimTransform*>::iterator i = boneIndexToAnimTransform.find(boneIndex);
			
			if (i != boneIndexToAnimTransform.end())
			{
				const FbxAnimTransform & animTransform = *i->second;
				const std::vector<AnimKey> & animKeys = animTransform.animKeys;
				
				for (size_t j = 0; j < animKeys.size(); ++j)
				{
					*finalAnimKey++ = animKeys[j];
				}
				
				animation->m_numKeys[boneIndex] = animKeys.size();
			}
			else
			{
				animation->m_numKeys[boneIndex] = 0;
			}
		}
		
		printf("added animation: %s\n", anim.name.c_str());
		
		animations[anim.name] = animation;
	}
	
	// create anim set
	
	AnimSet * animSet = new AnimSet();
	animSet->m_animations = animations;
	
	// create model
	
	AnimModel * model = new AnimModel(meshSet, boneSet, animSet);
	
	model->startAnim("Take 001", -1);
	
	//delete model;
	//model = 0;
	
	// <<< framework code end
	
	if (drawMeshes)
	{
		// initialize SDL
		
		SDL_Init(SDL_INIT_EVERYTHING);
		//if (SDL_SetVideoMode(640, 480, 32, SDL_OPENGL) < 0)
		if (SDL_SetVideoMode(1200, 900, 32, SDL_OPENGL) < 0)
		{
			log(0, "failed to intialize SDL");
			exit(-1);
		}
		
		bool wireframe = false;
		bool lightEnabled = false;
		bool drawBlendWeights = false;
		bool drawBlendIndices = false;
		bool drawTexcoords = false;
		bool drawNormalColors = true;
		
		int drawFlags = DrawMesh;
		
		bool stop = false;
		
		while (!stop)
		{
			// process input
			
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE)
						stop = true;
					else if (e.key.keysym.sym == SDLK_w)
						wireframe = !wireframe;
					else if (e.key.keysym.sym == SDLK_b)
						drawFlags ^= DrawBones;
					else if (e.key.keysym.sym == SDLK_p)
						drawFlags = drawFlags ^ DrawPoseMatrices;
					else if (e.key.keysym.sym == SDLK_n)
						drawFlags = drawFlags ^ DrawNormals;
					else if (e.key.keysym.sym == SDLK_l)
						lightEnabled = !lightEnabled;						
					else if (e.key.keysym.sym == SDLK_F1)
						drawBlendWeights = !drawBlendWeights;
					else if (e.key.keysym.sym == SDLK_F2)
						drawBlendIndices = !drawBlendIndices;
					else if (e.key.keysym.sym == SDLK_F3)
						drawTexcoords = !drawTexcoords;
					else if (e.key.keysym.sym == SDLK_F4)
						drawNormalColors = !drawNormalColors;
				}
			}
			
			// process animation
			
			static float time = 0.f;
			time += 1.f / 60.f;
			
			bool isDone = true;
			
			std::vector<Mat4x4> boneMatrices;
			boneMatrices.resize(modelNameToBoneIndex.size());
			
			for (ModelNameToBoneIndex::iterator i = modelNameToBoneIndex.begin(); i != modelNameToBoneIndex.end(); ++i)
			{
				const std::string & modelName = i->first;
				const int boneIndex = i->second;
				
				FbxMesh * mesh = static_cast<FbxMesh*>(objectsByName[modelName]);
				assert(mesh && mesh->type == "Mesh" && mesh->name == modelName);
				
				boneMatrices[boneIndex] = mesh->transform;
			}
			
		#if 1
			const int evalTime1 = getTimeMS();
			
			//for (int l = 0; l < 100; ++l) // fixme, remove
			if (anims.size() >= 1)
			{
				const FbxAnim & anim = anims.front();
				
				for (std::map<std::string, FbxAnimTransform>::const_iterator i = anim.transforms.begin(); i != anim.transforms.end(); ++i)
				{
					const std::string & modelName = i->first;
					const FbxAnimTransform & animTransform = i->second;
					
					std::map<std::string, int>::iterator j = modelNameToBoneIndex.find(modelName);
					
					if (j != modelNameToBoneIndex.end())
					{
						const int boneIndex = j->second;
						
						isDone &= animTransform.evaluate(time, boneMatrices[boneIndex]);
					}
					else
					{
						log(logIndent, "warning: no model found for anim channel: %s\n", modelName.c_str());
					}
				}
			}
			
			if (isDone)
			{
				log(logIndent, "animation is done. restart\n");
				
				time = 0.f;
			}
			
			const int evalTime2 = getTimeMS();
			
			//printf("anim eval time: %d ms\n", evalTime2 - evalTime1);
		#endif
			
			for (size_t i = 0; i < boneToObjectMatrices.size(); ++i)
			{
				Mat4x4 globalTransform;
				globalTransform.MakeIdentity();
				
				int boneIndex = i;
				
				while (boneIndex != -1)
				{
					globalTransform =
						boneMatrices[boneIndex] *
						globalTransform;
					
					boneIndex = boneParentIndices[boneIndex];
				}
				
				boneToObjectMatrices[i] = globalTransform;
			}

			// draw
			
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
			
			static float r = 130.f;
			r += 1.f / 10.f;
			
			Vec4 light(1.f, 1.f, 1.f, 0.f);
			light.Normalize();
			glLightfv(GL_LIGHT0, GL_POSITION, &light[0]);
			glEnable(GL_LIGHT0);
			if (lightEnabled)
				glEnable(GL_LIGHTING);
			else
				glDisable(GL_LIGHTING);
			glEnable(GL_NORMALIZE);
			
			glPushMatrix();
			
			glTranslatef(0.f, -.5f, 0.f);
			
			glRotatef(r, 0.f, 1.f, 0.f);
			
		#if 1
			glRotatef(-90.f, 1.f, 0.f, 0.f);
		#else
			glTranslatef(0.f, .5f, 0.f);
			glRotatef(-90.f, 0.f, 0.f, 1.f);
		#endif
			const float scale = 0.005f;
			glScalef(scale, scale, scale);
			
			model->process(1.f / 60.f);
			model->draw(drawFlags);
			
			glTranslatef(150.f, 0.f, 0.f);
			
			/*
		#if 1
			for (std::list<MeshBuilder>::iterator i = meshes.begin(); i != meshes.end(); ++i)
			{
				const MeshBuilder & mesh = *i;
				
				int vertexCount = 0;
				
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
					
					const MeshBuilder::Vertex & v = mesh.m_vertices[index];
					
					// -- software vertex blend (soft skinned) --
					Vec3 p(0.f, 0.f, 0.f);
					Vec3 n(0.f, 0.f, 0.f);
					for (int j = 0; j < 4; ++j)
					{
						if (v.boneWeights[j] == 0)
							continue;
						const int boneIndex = v.boneIndices[j];
						const float boneWeight = v.boneWeights[j] / 255.f;
						const Mat4x4 & objectToBone = objectToBoneMatrices[boneIndex];
						const Mat4x4 & boneToObject = boneToObjectMatrices[boneIndex];
						p += (boneToObject * objectToBone).Mul4(Vec3(v.px, v.py, v.pz)) * boneWeight;
						n += (boneToObject * objectToBone).Mul (Vec3(v.nx, v.ny, v.nz)) * boneWeight;
					}
					// -- software vertex blend (soft skinned) --
					
					float r = 1.f;
					float g = 1.f;
					float b = 1.f;
					
					if (drawNormalColors)
					{
						r *= (n[0] + 1.f) / 2.f;
						g *= (n[1] + 1.f) / 2.f;
						b *= (n[2] + 1.f) / 2.f;
					}
					if (drawBlendIndices)
					{
						r *= v.boneIndices[0] / float(modelNameToBoneIndex.size());
						g *= v.boneIndices[1] / float(modelNameToBoneIndex.size());
						b *= v.boneIndices[2] / float(modelNameToBoneIndex.size());
					}
					if (drawBlendWeights)
					{
						r *= (1.f + v.boneWeights[0] / 255.f) / 2.f;
						g *= (1.f + v.boneWeights[1] / 255.f) / 2.f;
						b *= (1.f + v.boneWeights[2] / 255.f) / 2.f;
					}
					if (drawTexcoords)
					{
						r *= v.tx;
						g *= v.ty;
					}
					
					glColor3f(r, g, b);
					glNormal3f(n[0], n[1], n[2]);
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
			}
		#endif
			*/
			
			glPopMatrix();
			
			SDL_GL_SwapBuffers();
		}

		
		
		SDL_Quit();
	}
	
	return 0;
}
