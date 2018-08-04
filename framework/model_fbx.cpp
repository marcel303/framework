/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include <algorithm>
#include <list>
#include <time.h>
#include "framework.h"
#include "internal.h"
#include "model_fbx.h"

#include "../libfbx/fbx.h" // todo: move to framework

/*

TODO:

- Garbage collect objects when loading bone set. Ensure we don't allocate bone indices for objects that will never draw.
- Apply geometric transform the vertex data.
- Transform normals as well as vertices to get them into global object space.

*/

static bool logEnabled = false;

//

using namespace AnimModel;

// forward declarations

class FbxDeformer;
class FbxMesh;
class FbxObject;
class FbxPose;

// helper functions

static void fbxLog(int logIndent, const char * fmt, ...);
static int getTimeMS();

static RotationType convertRotationOrder(int order);
static Mat4x4 matrixTranslation(const Vec3 & v);
static Mat4x4 matrixRotation(const Vec3 & v, RotationType rotationType);
static Mat4x4 matrixScaling(const Vec3 & v);

static FbxObject * readFbxGeometry(int & logIndent, const FbxRecord & model, const std::string & objectType, const std::string & objectName);
static FbxObject * readFbxModel(int & logIndent, const FbxRecord & model, const std::string & objectType, const std::string & objectName);
static FbxPose * readFbxPose(int & logIndent, const FbxRecord & pose, const std::string & objectType, const std::string & objectName);
static FbxObject * readFbxDeformer(int & logIndent, const FbxRecord & deformer, const std::string & objectType, const std::string & objectName);

//

static void fbxLog(int logIndent, const char * fmt, ...)
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
		vsprintf_s(temp, sizeof(temp), fmt, va);
		va_end(va);
		
		logInfo("%s%s", tabs, temp);
	}
}

static int getTimeMS()
{
	return clock() * 1000 / CLOCKS_PER_SEC;
}

static RotationType convertRotationOrder(int order)
{
	switch (order)
	{
	case 0:
		return RotationType_EulerXYZ;
	default:
		fassert(false);
		return RotationType_EulerXYZ;
	}
}

static Mat4x4 matrixTranslation(const Vec3 & v)
{
	Mat4x4 t;
	t.MakeTranslation(v);
	return t;
}

static Mat4x4 matrixRotation(const Vec3 & v, RotationType rotationType)
{
	const Vec3 temp = v * float(M_PI / 180.f);
	
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
		fassert(false);
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

// FBX types

// todo : according to the FBX documentation, FbxMesh should derive from FbxGeometry. this explains why there's so much duplicate code between the two. todo : remove redundant code in FbxMesh and make FbxMesh derive from FbxGeometry

enum MappingType
{
	kMappingType_ByPolygonVertex,
	kMappingType_ByVertex
};

static MappingType toMappingType(const std::string & name)
{
	return
		name == "ByVertice" || name == "ByVertex"
		? MappingType::kMappingType_ByVertex
		: MappingType::kMappingType_ByPolygonVertex;
}

class FbxObject
{
public:
	FbxObject()
	{
	}
	
	FbxObject(const std::string & type, const std::string & name, bool persistent = false)
	{
		this->type = type;
		this->name = name;
		this->persistent = persistent;
		parent = 0;
	}
	
	virtual ~FbxObject() { }
	virtual void helpMeDebugWithVTables() { }
	
	std::string type;
	std::string name;
	bool persistent;
	FbxObject * parent;
};

class FbxGeometry : public FbxObject
{
public:
	std::vector<float> vertices;
	std::vector<int> vertexIndices;
	std::vector<float> normals;
	std::vector<int> normalIndices;
	std::vector<float> uvs;
	std::vector<int> uvIndices;
	std::vector<float> colors;
	std::vector<int> colorIndices;
	
	MappingType normalMappingType;
	MappingType uvMappingType;
	
	FbxGeometry(const std::string & name)
		: FbxObject("Geometry", name)
		, normalMappingType(kMappingType_ByPolygonVertex)
		, uvMappingType(kMappingType_ByPolygonVertex)
	{
	}
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
	std::vector<int> normalIndices;
	std::vector<float> uvs;
	std::vector<int> uvIndices;
	std::vector<float> colors;
	std::vector<int> colorIndices;
	std::vector<Deformer> deformers;
	
	MappingType normalMappingType;
	MappingType uvMappingType;
	
	FbxMesh(const std::string & name)
		: FbxObject("Mesh", name, true)
		, normalMappingType(kMappingType_ByPolygonVertex)
		, uvMappingType(kMappingType_ByPolygonVertex)
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
						
						fassert(time >= key1.time && time <= key2.time);
						
						const float t = (time - key1.time) / (key2.time - key1.time);
						
						fassert(t >= 0.f && t <= 1.f);
						
						value = key1.value * (1.f - t) + key2.value * t;
					}
					else
					{
						// either the first or last key in the animation. copy value
						
						fassert(key == firstKey || key == lastKey);
						
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
					
					//fbxLog(logIndent, "keyCount=%d", keyCount);
					
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
							
							//fbxLog(logIndent, "%014lld -> %f", temp.time, temp.value);
						}
						
						if (time > endTime)
						{
							endTime = time;
						}
					}
					
					//fbxLog(logIndent, "got %d unique keys", keys.size());
				}
				else
				{
					const FbxRecord _default = record.firstChild("Default");
					
					if (_default.isValid())
					{
						Key key;
						key.time = 0.f;
						key.value = _default.captureProperty<float>(0);
						keys.push_back(key);
					}
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
				
				//fbxLog(logIndent, "stream: name=%s", name.c_str());
				
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
		
		rotationVec *= float(M_PI / 180.f);
		
		Quat quatX;
		Quat quatY;
		Quat quatZ;
		
		quatX.fromAxisAngle(Vec3(1.f, 0.f, 0.f), rotationVec[0]);
		quatY.fromAxisAngle(Vec3(0.f, 1.f, 0.f), rotationVec[1]);
		quatZ.fromAxisAngle(Vec3(0.f, 0.f, 1.f), rotationVec[2]);
		
		rotation = quatZ * quatY * quatX;
	}
};

class FbxAnim
{
public:
	std::string name;
	std::map<std::string, FbxAnimTransform> transforms;
};

static FbxObject * readFbxGeometry(int & logIndent, const FbxRecord & model, const std::string & objectType, const std::string & objectName)
{
	FbxObject * result = 0;
	
	FbxGeometry * geometry = new FbxGeometry(objectName);
	result = geometry;
	
	const FbxRecord vertices = model.firstChild("Vertices");
	const FbxRecord vertexIndices = model.firstChild("PolygonVertexIndex");
	
	const FbxRecord normalsLayer = model.firstChild("LayerElementNormal");
	const FbxRecord normals = normalsLayer.firstChild("Normals");
	const FbxRecord normalIndices = normalsLayer.firstChild("NormalsIndex");
	const std::string normalMappingType = normalsLayer.firstChild("MappingInformationType").captureProperty<std::string>(0);
	geometry->normalMappingType = toMappingType(normalMappingType);
	
	const FbxRecord uvsLayer = model.firstChild("LayerElementUV");
	const FbxRecord uvs = uvsLayer.firstChild("UV");
	const FbxRecord uvIndices = uvsLayer.firstChild("UVIndex");
	const std::string uvMappingType = uvsLayer.firstChild("MappingInformationType").captureProperty<std::string>(0);
	geometry->uvMappingType = toMappingType(uvMappingType);
	
	const FbxRecord colorsLayer = model.firstChild("LayerElementColor");
	const FbxRecord colors = colorsLayer.firstChild("Colors");
	const FbxRecord colorIndices = colorsLayer.firstChild("ColorIndex");
	
	vertices.capturePropertyArray(0, geometry->vertices);
	vertexIndices.capturePropertyArray(0, geometry->vertexIndices);
	normals.capturePropertyArray(0, geometry->normals);
	normalIndices.capturePropertyArray(0, geometry->normalIndices);
	uvs.capturePropertyArray(0, geometry->uvs);
	uvIndices.capturePropertyArray(0, geometry->uvIndices);
	colors.capturePropertyArray(0, geometry->colors);
	colorIndices.capturePropertyArray(0, geometry->colorIndices);
	
	fbxLog(logIndent, "found a geometry! name: %s, hasVertices: %d (%d), hasNormals: %d, hasUVs: %d",
		objectName.c_str(),
		vertices.isValid(),
		int(geometry->vertices.size()),
		normals.isValid(),
		uvs.isValid());
	
	return result;
}

static FbxObject * readFbxModel(int & logIndent, const FbxRecord & model, const std::string & objectType, const std::string & objectName)
{
	// todo: allocate generic node object for non mesh
	
	FbxObject * result = 0;
	
	//if (treatAsMesh(objectType))
	{
		FbxMesh * mesh = new FbxMesh(objectName);
		result = mesh;
		
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
						fbxLog(logIndent, "warning: geometric translation is non zero");
				}
				else if (name == "GeometricRotation")
				{
					Float3 temp(0.f, 0.f, 0.f);
					temp.load(values);
					if (temp[0] != 0.f || temp[1] != 0.f || temp[2] != 0.f)
						fbxLog(logIndent, "warning: geometric rotation is non zero");
				}
				else if (name == "GeometricScaling")
				{
					Float3 temp(1.f, 1.f, 1.f);
					temp.load(values);
					if (temp[0] != 1.f || temp[1] != 1.f || temp[2] != 1.f)
						fbxLog(logIndent, "warning: geometric scaling is not one");
				}
				else if (name == "RotationOrder")
				{
					if (values.size() >= 4)
					{
						rotationOrder = values[3].getInt();
						if (rotationOrder != 0)
							fbxLog(logIndent, "warning: rotation order is not XYZ");
					}
				}
			}
		}
		
		// FBX version 6000
		
		const FbxRecord vertices = model.firstChild("Vertices");
		const FbxRecord vertexIndices = model.firstChild("PolygonVertexIndex");
		
		const FbxRecord normalsLayer = model.firstChild("LayerElementNormal");
		const FbxRecord normals = normalsLayer.firstChild("Normals");
		const FbxRecord normalIndices = normalsLayer.firstChild("NormalsIndex");
		const std::string normalMappingType = normalsLayer.firstChild("MappingInformationType").captureProperty<std::string>(0);
		mesh->normalMappingType = toMappingType(normalMappingType);
		
		const FbxRecord uvsLayer = model.firstChild("LayerElementUV");
		const FbxRecord uvs = uvsLayer.firstChild("UV");
		const FbxRecord uvIndices = uvsLayer.firstChild("UVIndex");
		const std::string uvMappingType = uvsLayer.firstChild("MappingInformationType").captureProperty<std::string>(0);
		mesh->uvMappingType = toMappingType(uvMappingType);
		
		const FbxRecord colorsLayer = model.firstChild("LayerElementColor");
		const FbxRecord colors = colorsLayer.firstChild("Colors");
		const FbxRecord colorIndices = colorsLayer.firstChild("ColorIndex");
		
		vertices.capturePropertiesAsFloat(mesh->vertices);
		vertexIndices.capturePropertiesAsInt(mesh->vertexIndices);
		normals.capturePropertiesAsFloat(mesh->normals);
		normalIndices.capturePropertiesAsInt(mesh->normalIndices);
		uvs.capturePropertiesAsFloat(mesh->uvs);
		uvIndices.capturePropertiesAsInt(mesh->uvIndices);
		colors.capturePropertiesAsFloat(mesh->colors);
		colorIndices.capturePropertiesAsInt(mesh->colorIndices);
		
		// FBX version 5800 ( < 6000 )
		
		const FbxRecord geometryUV = model.firstChild("GeometryUVInfo");
		
		if (geometryUV.isValid())
		{
			// todo: check if "MappingInformationType".prop[0] == "ByPolygon"
			
			const FbxRecord uvs = geometryUV.firstChild("TextureUV");
			const FbxRecord uvIndices = geometryUV.firstChild("TextureUVVerticeIndex");
			
			if (uvs.isValid())
				uvs.capturePropertiesAsFloat(mesh->uvs);
			if (uvIndices.isValid())
				uvIndices.capturePropertiesAsInt(mesh->uvIndices);
		}
		
		// transform
		
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
		
		fbxLog(logIndent, "found a mesh! name: %s, hasVertices: %d (%d), hasNormals: %d, hasUVs: %d",
			objectName.c_str(),
			vertices.isValid(),
			int(mesh->vertices.size()),
			normals.isValid(),
			uvs.isValid());
		
		logIndent++;
		{
			fbxLog(logIndent+1, "transform:");
			for (int i = 0; i < 4; ++i)
			{
				fbxLog(logIndent + 2, "%8.2f %8.2f %8.2f %8.2f",
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
	
	return result;
}

static FbxPose * readFbxPose(int & logIndent, const FbxRecord & pose, const std::string & objectType, const std::string & objectName)
{
	FbxPose * fbxPose = new FbxPose(objectName);
	
	std::vector<std::string> poseProperties = pose.captureProperties<std::string>();
	
	std::string poseName;
	std::string poseType;
	
	if (poseProperties.size() >= 1)
		poseName = poseProperties[0];
	if (poseProperties.size() >= 2)
		poseType = poseProperties[1];
	
	// type = BindPose
	
	fbxLog(logIndent, "pose! name=%s, type=%s", poseName.c_str(), poseType.c_str());
	
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
			fbxLog(logIndent, "warning: pose matrix doesn't contain 16 elements");
		}
		
		//fbxLog(logIndent, "pose node! matrixSize=%d, node=%s", int(matrix.size()), nodeName.c_str());
	}
	
	return fbxPose;
}

static FbxPose * readFbxPose(int logIndent, const FbxReader & reader)
{
	for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
	{
		for (FbxRecord object = objects.firstChild(); object.isValid(); object = object.nextSibling())
		{
			if (object.name == "Pose")
			{
				std::vector<std::string> objectProps = object.captureProperties<std::string>();
				const std::string objectName = objectProps.size() >= 1 ? objectProps[0] : "";
				const std::string objectType = objectProps.size() >= 2 ? objectProps[1] : "";
				
				return readFbxPose(logIndent, object, objectType, objectName);
			}
		}
	}
	
	return 0;
}

static FbxObject * readFbxDeformer(int & logIndent, const FbxRecord & deformer, const std::string & objectType, const std::string & objectName)
{
	FbxDeformer * fbxDeformer = new FbxDeformer(objectName);
	
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
	
	//fbxLog(logIndent, "deformer! name=%s, type=%s, numIndices=%d, numWeights=%d", name.c_str(), type.c_str(), int(fbxDeformer->indices.size()), int(fbxDeformer->weights.size()));
	
	return fbxDeformer;
}

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
			Node nodes[PoolSize];
			unsigned int numNodes;
			
			Block()
			{
				next = 0;
				numNodes = 0;
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
	class Vertex : public AnimModel::Vertex
	{
	public:
		inline bool operator==(const Vertex & w) const
		{
			return !memcmp(this, &w, sizeof(Vertex));
		}
	};
	
	std::vector<Vertex> m_vertices;
	std::vector<int> m_indices;
	
	void construct(
		int logIndent,
		const std::vector<float> & vertices,
		const std::vector<int> & vertexIndices,
		const std::vector<float> & normals,
		const std::vector<int> & normalIndices,
		const MappingType normalMappingType,
		const std::vector<float> & uvs,
		const std::vector<int> & uvIndices,
		const MappingType uvMappingType,
		const std::vector<float> & colors,
		const std::vector<int> & colorIndices,
		const std::vector<FbxMesh::Deformer> & deformers)
	{
		fbxLog(logIndent, "-- running da welding machine! --");
		
		logIndent++;
		
		fbxLog(logIndent, "input: %d vertices, %d indices, %d normals (%d indices), %d UVs (%d indices), %d colors (%d indices)",
			(int)vertices.size()/3,
			(int)vertexIndices.size(),
			(int)normals.size()/3,
			(int)normalIndices.size(),
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
			
			if (!normalIndices.empty())
			{
				const int lookupIndex =
					normalMappingType == kMappingType_ByVertex
						? vertexIndex
						: i;
				
				if (lookupIndex < normalIndices.size())
				{
					// indexed UV
					
					const size_t normalIndex = normalIndices[lookupIndex];
					
					if (normalIndex < normals.size() / 3)
					{
						vertex.nx = normals[normalIndex * 3 + 0];
						vertex.ny = normals[normalIndex * 3 + 1];
						vertex.nz = normals[normalIndex * 3 + 2];
					}
					else
					{
						vertex.nx = vertex.ny = vertex.nz = 0.f;
					}
				}
				else
				{
					vertex.nx = vertex.ny = vertex.nz = 0.f;
				}
			}
			else
			{
				const int normalIndex =
					normalMappingType == kMappingType_ByVertex
						? vertexIndex
						: i;
				
				if (normalIndex < normals.size() / 3)
				{
					vertex.nx = normals[normalIndex * 3 + 0];
					vertex.ny = normals[normalIndex * 3 + 1];
					vertex.nz = normals[normalIndex * 3 + 2];
				}
				else
				{
					vertex.nx = vertex.ny = vertex.nz = 0.f;
				}
			}
			
			// uv
			
			// todo : should look at ReferenceInformationType in the FBX file for LayerElementNormal, LayerElementUV. it can be Direct or IndexToDirect. in the IndexToDirect case, indices are used. otherwise elements are mapped directly. see https://banexdevblog.wordpress.com/2014/06/23/a-quick-tutorial-about-the-fbx-ascii-format/ for more information
			
			if (!uvIndices.empty())
			{
				const int lookupIndex =
					uvMappingType == kMappingType_ByVertex
						? vertexIndex
						: i;
				
				if (lookupIndex < uvIndices.size())
				{
					// indexed UV
					
					const size_t uvIndex = uvIndices[lookupIndex];
					
					if (uvIndex * 2 < uvs.size())
					{
						vertex.tx = uvs[uvIndex * 2 + 0];
						vertex.ty = uvs[uvIndex * 2 + 1];
					}
				}
				else
				{
					vertex.tx = vertex.ty = 0.f;
				}
			}
			else
			{
				const int uvIndex =
					uvMappingType == kMappingType_ByVertex
						? vertexIndex
						: i;
				
				if (uvIndex < uvs.size() / 2)
				{
					// non-indexed UV
					
					vertex.tx = uvs[uvIndex * 2 + 0];
					vertex.ty = uvs[uvIndex * 2 + 1];
				}
				else
				{
					vertex.tx = vertex.ty = 0.f;
				}
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
				vertex.boneWeights[d] = uint8_t(deformers[vertexIndex].entries[d].weight * 255.f);
				
				//fbxLog(logIndent, "added %d, %d", vertex.blendIndices[d], vertex.boneWeights[d]);
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
			for (size_t i = 0; i + 3 <= m_indices.size(); )
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
				
				while (i + 1 <= m_indices.size())
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
			
			fbxLog(logIndent, "triangulation result: %d -> %d indices", int(m_indices.size()), int(temp.size()));
			
			m_indices = temp;
		}
		
		const int time2 = getTimeMS();
		fbxLog(logIndent, "time: %d ms", time2 - time1);
		
		fbxLog(logIndent, "output: %d vertices, %d indices",
			(int)m_vertices.size(),
			(int)m_indices.size());
		
		logIndent--;
	}
};

namespace AnimModel
{
	bool LoaderFbxBinary::readFile(const char * filename, std::vector<unsigned char> & bytes)
	{
		bool result = true;
		
		FILE * file = fopen(filename, "rb");
		
		if (!file)
		{
			result = false;
		}
		else
		{
			const size_t p1 = ftell(file);
			fseek(file, 0, SEEK_END);
			const size_t p2 = ftell(file);
			fseek(file, 0, SEEK_SET);
			
			const size_t numBytes = p2 - p1;
			bytes.resize(numBytes);
			
			if (fread(&bytes[0], numBytes, 1, file) != 1)
			{
				result = false;
			}
			
			fclose(file);
		}
		
		return result;
	}
	
	MeshSet * LoaderFbxBinary::loadMeshSet(const char * filename, const BoneSet * boneSet)
	{
		// set up bone name -> bone index map
		
		typedef std::map<std::string, int> BoneNameToBoneIndexMap;
		BoneNameToBoneIndexMap boneNameToBoneIndex;
		
		for (int i = 0; i < boneSet->m_numBones; ++i)
			boneNameToBoneIndex[boneSet->m_bones[i].name] = i;
		
		//
		
		int logIndent = 0;
		
		// read file contents
		
		std::vector<unsigned char> bytes;
		
		if (!readFile(filename, bytes))
		{
			fbxLog(logIndent, "failed to open %s", filename);
			return 0;
		}
		
		// open FBX file
		
		FbxReader reader;
		
		reader.openFromMemory(&bytes[0], bytes.size());
		
		// gather a list of all objects
		
		typedef std::map<std::string, FbxObject*> ObjectsByName;
		ObjectsByName objectsByName;
		
		FbxObject * scene = new FbxObject("Scene", "Scene", true);
		objectsByName[scene->name] = scene;
		
		const int time1 = getTimeMS();
		
		for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
		{
			for (FbxRecord object = objects.firstChild(); object.isValid(); object = object.nextSibling())
			{
				FbxObject * fbxObject = 0;
				
				std::string objectName;
				std::string objectType;
				
				if (reader.getVersion() >= 7000)
				{
					const int64_t id = object.captureProperty<int64_t>(0);
					std::vector<std::string> objectProps = object.captureProperties<std::string>();
					
					char name[64];
					sprintf_s(name, sizeof(name), "%lld", id);
					
					objectName = name;
					objectType = objectProps.size() >= 3 ? objectProps[objectProps.size() - 1] : "";
				}
				else
				{
					std::vector<std::string> objectProps = object.captureProperties<std::string>();
					
					objectName = objectProps.size() >= 1 ? objectProps[0] : "";
					objectType = objectProps.size() >= 2 ? objectProps[1] : "";
				}
				
				if (object.name == "Geometry")
				{
					fbxObject = readFbxGeometry(logIndent, object, objectType, objectName);
				}
				else if (object.name == "Model")
				{
					fbxObject = readFbxModel(logIndent, object, objectType, objectName);
				}
				else if (object.name == "Pose")
				{
					fbxObject = readFbxPose(logIndent, object, objectType, objectName);
				}
				else if (object.name == "Deformer")
				{
					fbxObject = readFbxDeformer(logIndent, object, objectType, objectName);
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
						fbxLog(logIndent, "duplicate object name !!!");
						delete fbxObject;
					}
				}
				else
				{
					fbxLog(logIndent, "unknown object type: %s (name=%s)", objectType.c_str(), objectName.c_str());
				}
			}
		}
		
		// FBX version 5800 (pre 6000?) seems to store models directly in the root node, instead of in "Objects"
		
		for (FbxRecord model = reader.firstRecord("Model"); model.isValid(); model = model.nextSibling("Model"))
		{
			std::vector<std::string> objectProps = model.captureProperties<std::string>();
			const std::string objectName = objectProps.size() >= 1 ? objectProps[0] : "";
			const std::string objectType = "Mesh";
			
			FbxObject * fbxObject = readFbxModel(logIndent, model, objectType, objectName);
			
			// todo: remove code deduplication..
			if (fbxObject != 0)
			{
				fbxObject->parent = scene;
				
				if (objectsByName.count(fbxObject->name) == 0)
				{
					objectsByName[fbxObject->name] = fbxObject;
				}
				else
				{
					fbxLog(logIndent, "duplicate object name !!!");
					delete fbxObject;
				}
			}
			else
			{
				fbxLog(logIndent, "unknown object type: %s (name=%s)", objectType.c_str(), objectName.c_str());
			}
		}
		
		// connections = scene hierarchy, including bones
		
		FbxRecord connections = reader.firstRecord("Connections");
		
		for (FbxRecord connection = connections.firstChild("Connect"); connection.isValid(); connection = connection.nextSibling("Connect"))
		{
			std::vector<std::string> properties = connection.captureProperties<std::string>();
			
			if (properties.size() < 3)
			{
				fbxLog(logIndent, "warning: connection doesn't specify from-to");
			}
			else
			{
				const std::string & fromName = properties[1];
				const std::string & toName = properties[2];
				
				if (fromName == toName)
				{
					fbxLog(logIndent, "warning: connect to self? %s", fromName.c_str());
				}
				else
				{
					ObjectsByName::iterator from = objectsByName.find(fromName);
					ObjectsByName::iterator to = objectsByName.find(toName);
					
					if (from != objectsByName.end() && to != objectsByName.end())
					{
						//fbxLog(logIndent, "connect %s -> %s", fromName.c_str(), toName.c_str());
						
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
						fbxLog(logIndent, "error: unable to connect %s -> %s", fromName.c_str(), toName.c_str());
					}
				}
			}
		}
		
		// FBX >= 7000 (?)
		
		for (FbxRecord connection = connections.firstChild("C"); connection.isValid(); connection = connection.nextSibling("C"))
		{
			std::vector<int64_t> properties = connection.captureProperties<int64_t>();
			
			if (properties.size() < 3)
			{
				fbxLog(logIndent, "warning: connection doesn't specify from-to");
			}
			else
			{
				char fromNameBuffer[64];
				char toNameBuffer[64];
				
				sprintf_s(fromNameBuffer, sizeof(fromNameBuffer), "%lld", properties[1]);
				sprintf_s(toNameBuffer, sizeof(toNameBuffer), "%lld", properties[2]);
				
				const std::string fromName = fromNameBuffer;
				const std::string toName = toNameBuffer;
				
				if (fromName == toName)
				{
					fbxLog(logIndent, "warning: connect to self? %s", fromName.c_str());
				}
				else
				{
					ObjectsByName::iterator from = objectsByName.find(fromName);
					ObjectsByName::iterator to = objectsByName.find(toName);
					
					if (from != objectsByName.end() && to != objectsByName.end())
					{
						//fbxLog(logIndent, "connect %s -> %s", fromName.c_str(), toName.c_str());
						
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
						else if (fromObject->type == "Geometry" && toObject->type == "Mesh")
						{
							FbxGeometry * geometry = static_cast<FbxGeometry*>(fromObject);
							FbxMesh * mesh = static_cast<FbxMesh*>(toObject);
							
							// todo : would be nice to refactor this and avoid the copying here
							mesh->vertices = geometry->vertices;
							mesh->vertexIndices = geometry->vertexIndices;
							mesh->normals = geometry->normals;
							mesh->normalIndices = geometry->normalIndices;
							mesh->normalMappingType = geometry->normalMappingType;
							mesh->uvs = geometry->uvs;
							mesh->uvIndices = geometry->uvIndices;
							mesh->uvMappingType = geometry->uvMappingType;
							mesh->colors = geometry->colors;
							mesh->colorIndices = geometry->colorIndices;
						}
						else if (fromObject->type == "Mesh" && toObject->type == "Mesh")
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
						fbxLog(logIndent, "error: unable to connect %s -> %s", fromName.c_str(), toName.c_str());
					}
				}
			}
		}
		
		// purge objects that aren't connected to the scene
		
		typedef std::vector<FbxObject*> ObjectList;
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
				fbxLog(logIndent, "garbage collecting object! name=%s, type=%s", object->name.c_str(), object->type.c_str());
				
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
					fbxLog(logIndent, "error: deformer indices / weights mismatch");
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
					//fbxLog(logIndent, "found mesh for deformer! %s", mesh->name.c_str());
					
					const std::string & boneName = boneMesh->name;
					fassert(boneNameToBoneIndex.count(boneName) != 0);
					const int boneIndex = boneNameToBoneIndex[boneName];
					
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
							fbxLog(logIndent, "error: deformer index %d is out of range [max=%d]", vertexIndex, int(mesh->deformers.size()));
						}
					}
				}
				else
				{
					fbxLog(logIndent, "error: deformer %s: mesh=%p, boneMesh=%p", deformer->name.c_str(), mesh, boneMesh);
				}
			}
		}
		
		// transform vertices into global object space
		
		FbxPose * pose = readFbxPose(logIndent, reader);
		
		if (pose)
		{
			for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
			{
				FbxObject * object = i->second;
				
				if (object->type == "Mesh")
				{
					FbxMesh * mesh = static_cast<FbxMesh*>(object);
					
					fbxLog(logIndent, "transforming vertices for %s into global object space", mesh->name.c_str());
					
					// todo: include geometric transform as well
					
					const Mat4x4 & globalTransform = pose->matrices[mesh->name];
					
					for (size_t k = 0; k + 3 <= mesh->vertices.size(); k += 3)
					{
						// todo: transform normal as well
						const float x = mesh->vertices[k + 0];
						const float y = mesh->vertices[k + 1];
						const float z = mesh->vertices[k + 2];
						Vec3 p = globalTransform.Mul4(Vec3(x, y, z));
						mesh->vertices[k + 0] = p[0];
						mesh->vertices[k + 1] = p[1];
						mesh->vertices[k + 2] = p[2];
					}
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
				
				// fix up bone indices in the absence of deformers
				
				if (fbxMesh->deformers.size() == 0)
				{
					const std::string & boneName = fbxMesh->name;
					fassert(boneNameToBoneIndex.count(boneName) != 0);
					const int boneIndex = boneNameToBoneIndex[boneName];
					
					fbxMesh->deformers.resize(fbxMesh->vertices.size());
					
					for (size_t j = 0; j < fbxMesh->deformers.size(); ++j)
						fbxMesh->deformers[j].add(boneIndex, 1.f);
				}
				
				meshes.push_back(MeshBuilder());
				MeshBuilder & meshBuilder = meshes.back();
				
				// todo: triangulate mesh
				
				meshBuilder.construct(
					logIndent,
					fbxMesh->vertices,
					fbxMesh->vertexIndices,
					fbxMesh->normals,
					fbxMesh->normalIndices,
					fbxMesh->normalMappingType,
					fbxMesh->uvs,
					fbxMesh->uvIndices,
					fbxMesh->uvMappingType,
					fbxMesh->colors,
					fbxMesh->colorIndices,
					fbxMesh->deformers);
			}
		}
		
		const int time2 = getTimeMS();
		
		fbxLog(logIndent, "time: %d ms", time2-time1);
		
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
		{
			meshes2[i]->finalize();
			
			meshSet->m_meshes[i] = meshes2[i];
		}
		
		return meshSet;
	}
	
	BoneSet * LoaderFbxBinary::loadBoneSet(const char * filename)
	{
		int logIndent = 0;
		
		// read file contents
		
		std::vector<uint8_t> bytes;
		
		if (!readFile(filename, bytes))
		{
			fbxLog(logIndent, "failed to open %s", filename);
			return 0;
		}
		
		// open FBX file
		
		FbxReader reader;
		
		reader.openFromMemory(&bytes[0], bytes.size());
		
		// gather a list of all meshes
		
		typedef std::map<std::string, FbxObject> ObjectsByName;
		ObjectsByName objectsByName;
		
		{
			// FBX version 6000+ seems to store models in the "Objects" node
			
			for (FbxRecord objects = reader.firstRecord("Objects"); objects.isValid(); objects = objects.nextSibling("Objects"))
			{
				for (FbxRecord object = objects.firstChild(); object.isValid(); object = object.nextSibling())
				{
					if (object.name == "Model")
					{
						std::string objectName;
						std::string objectType;
						
						if (reader.getVersion() >= 7000)
						{
							const int64_t id = object.captureProperty<int64_t>(0);
							std::vector<std::string> objectProps = object.captureProperties<std::string>();
							
							char name[64];
							sprintf_s(name, sizeof(name), "%lld", id);
							
							objectName = name;
							objectType = objectProps.size() >= 3 ? objectProps[objectProps.size() - 1] : "";
						}
						else
						{
							std::vector<std::string> objectProps = object.captureProperties<std::string>();
							
							objectName = objectProps.size() >= 1 ? objectProps[0] : "";
							objectType = objectProps.size() >= 2 ? objectProps[1] : "";
						}
						
						// pretend null nodes or mesh so they will be added to the hierarchy
						if (objectType == "Null")
							objectType = "Mesh";
						
						if (objectType == "Mesh" && !objectName.empty())
						{
							//fbxLog(logIndent, "found mesh! %s", objectName.c_str());
							
							objectsByName[objectName] = FbxObject(objectType, objectName);
						}
					}
				}
			}
		}
		
		{
			// FBX version 5800 (pre 6000?) seems to store models directly in the root node, instead of in "Objects"
			
			for (FbxRecord model = reader.firstRecord("Model"); model.isValid(); model = model.nextSibling("Model"))
			{
				std::vector<std::string> objectProps = model.captureProperties<std::string>();
				const std::string objectName = objectProps.size() >= 1 ? objectProps[0] : "";
				
				if (!objectName.empty())
				{
					//fbxLog(logIndent, "found mesh! %s", objectName.c_str());
					
					objectsByName[objectName] = FbxObject("Mesh", objectName);
				}
			}
		}
		
		// set up connections
		
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
					fbxLog(logIndent, "warning: connect to self? %s", fromName.c_str());
				}
				else
				{
					ObjectsByName::iterator from = objectsByName.find(fromName);
					ObjectsByName::iterator to = objectsByName.find(toName);
					
					if (from != objectsByName.end() && to != objectsByName.end())
					{
						//fbxLog(logIndent, "connect %s -> %s", fromName.c_str(), toName.c_str());
						
						FbxObject & fromObject = from->second;
						FbxObject & toObject = to->second;
						
						if (toObject.type == "Mesh" && fromObject.type == "Mesh")
						{
							fromObject.parent = &toObject;
						}
					}
				}
			}
		}
		
		// FBX >= 7000 ?
		
		for (FbxRecord connection = connections.firstChild("C"); connection.isValid(); connection = connection.nextSibling("C"))
		{
			std::vector<int64_t> properties = connection.captureProperties<int64_t>();
			
			if (properties.size() < 3)
			{
				fbxLog(logIndent, "warning: connection doesn't specify from-to");
			}
			else
			{
				char fromNameBuffer[64];
				char toNameBuffer[64];
				
				sprintf_s(fromNameBuffer, sizeof(fromNameBuffer), "%lld", properties[1]);
				sprintf_s(toNameBuffer, sizeof(toNameBuffer), "%lld", properties[2]);
				
				const std::string fromName = fromNameBuffer;
				const std::string toName = toNameBuffer;
				
				if (fromName == toName)
				{
					fbxLog(logIndent, "warning: connect to self? %s", fromName.c_str());
				}
				else
				{
					ObjectsByName::iterator from = objectsByName.find(fromName);
					ObjectsByName::iterator to = objectsByName.find(toName);
					
					if (from != objectsByName.end() && to != objectsByName.end())
					{
						//fbxLog(logIndent, "connect %s -> %s", fromName.c_str(), toName.c_str());
						
						FbxObject & fromObject = from->second;
						FbxObject & toObject = to->second;
						
						if (toObject.type == "Mesh" && fromObject.type == "Mesh")
						{
							fromObject.parent = &toObject;
						}
					}
				}
			}
		}
		
		// generate indices for meshes
		
		typedef std::map<std::string, int> ModelNameToBoneIndex;
		ModelNameToBoneIndex modelNameToBoneIndex;
		
		//fbxLog(logIndent, "generating %d bone indices", int(objectsByName.size()));
		
		for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
		{
			FbxObject & object = i->second;
			
			if (object.type == "Mesh")
			{
				if (modelNameToBoneIndex.count(object.name) == 0)
				{
					const int index = int(modelNameToBoneIndex.size());
					modelNameToBoneIndex[object.name] = index;
					
					//fbxLog(logIndent, "bone %s = index %d", object.name.c_str(), index);
				}
				else
				{
					fbxLog(logIndent, "warning: duplicate object name: %s", object.name.c_str());
				}
			}
		}
		
		// look for pose
		
		FbxPose * pose = readFbxPose(logIndent, reader);
		
		// allocate bone matrix for each mesh
		
		//fbxLog(logIndent, "allocating %d bones", int(modelNameToBoneIndex.size()));
		
		std::vector<Mat4x4> objectToBoneMatrices;
		std::vector<Mat4x4> boneToObjectMatrices;
		std::vector<int> boneParentIndices;
		
		objectToBoneMatrices.resize(modelNameToBoneIndex.size());
		boneToObjectMatrices.resize(modelNameToBoneIndex.size());
		boneParentIndices.resize(modelNameToBoneIndex.size(), -1);
		
		// setup mesh hierarchy
		
		for (ObjectsByName::iterator i = objectsByName.begin(); i != objectsByName.end(); ++i)
		{
			FbxObject & object = i->second;
			
			if (object.type == "Mesh")
			{
				fassert(modelNameToBoneIndex.count(object.name) != 0);
				const int index = modelNameToBoneIndex[object.name];
				
				FbxObject * parent = object.parent;
				
				while (parent)
				{
					if (parent->type == "Mesh")
						break;
					parent = parent->parent;
				}
				
				if (parent)
				{
					fassert(modelNameToBoneIndex.count(parent->name) != 0 && boneParentIndices[index] == -1);
					boneParentIndices[index] = modelNameToBoneIndex[parent->name];
				}
				else
				{
					boneParentIndices[index] = -1;
				}
			}
		}
		
		// fill in pose matrices
		
		for (ModelNameToBoneIndex::iterator i = modelNameToBoneIndex.begin(); i != modelNameToBoneIndex.end(); ++i)
		{
			const std::string & modelName = i->first;
			const int boneIndex = i->second;
				
			if (pose && pose->matrices.count(modelName) != 0)
			{
				objectToBoneMatrices[boneIndex] = pose->matrices[modelName].CalcInv();
			}
			else
			{
				objectToBoneMatrices[boneIndex].MakeIdentity();
			}
		}
		
		// create bone set
		
		BoneSet * boneSet = new BoneSet();
		
		boneSet->allocate(modelNameToBoneIndex.size());
		
		for (ModelNameToBoneIndex::iterator i = modelNameToBoneIndex.begin(); i != modelNameToBoneIndex.end(); ++i)
		{
			const std::string & boneName = i->first;
			const int boneIndex = i->second;
			boneSet->m_bones[boneIndex].name = boneName;
			boneSet->m_bones[boneIndex].poseMatrix = objectToBoneMatrices[boneIndex];
			boneSet->m_bones[boneIndex].parent = boneParentIndices[boneIndex];
		}
		
		boneSet->calculateBoneMatrices();
		boneSet->sortBoneIndices();
		
		return boneSet;
	}
	
	AnimSet * LoaderFbxBinary::loadAnimSet(const char * filename, const BoneSet * boneSet)
	{
		// set up bone name -> bone index map
		
		typedef std::map<std::string, int> BoneNameToBoneIndexMap;
		BoneNameToBoneIndexMap boneNameToBoneIndex;
		
		for (int i = 0; i < boneSet->m_numBones; ++i)
			boneNameToBoneIndex[boneSet->m_bones[i].name] = i;
		
		//
		
		int logIndent = 0;
		
		// read file contents
		
		std::vector<uint8_t> bytes;
		
		if (!readFile(filename, bytes))
		{
			fbxLog(logIndent, "failed to open %s", filename);
			return 0;
		}
		
		// open FBX file
		
		FbxReader reader;
		
		reader.openFromMemory(&bytes[0], bytes.size());
		
		// read take data
		
		FbxRecord takes = reader.firstRecord("Takes");
		
		typedef std::list<FbxAnim> AnimList;
		AnimList anims;
		
		logIndent++;
		
		for (FbxRecord take = takes.firstChild("Take"); take.isValid(); take = take.nextSibling("Take"))
		{
			const std::string name = take.captureProperty<std::string>(0);
			
			fbxLog(logIndent, "take: %s", name.c_str());
			
			anims.push_back(FbxAnim());
			FbxAnim & anim = anims.back();
			
			anim.name = name;
			
			std::string fileName = take.firstChild("FileName").captureProperty<std::string>(0);
			
			// ReferenceTime (int, timestamp?)
			
			logIndent++;
			
			for (FbxRecord model = take.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
			{
				std::string modelName = model.captureProperty<std::string>(0);
				
				fbxLog(logIndent, "model: %s", modelName.c_str());
				
				logIndent++;
				
				for (FbxRecord channel = model.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
				{
					std::string channelName = channel.captureProperty<std::string>(0);
					
					//fbxLog(logIndent, "channel: %s", channelName.c_str());
					
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
		
		// create anim set
		
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
				const int boneIndex = boneNameToBoneIndex[modelName];
				boneIndexToAnimTransform[boneIndex] = &animTransform;
			}
			
			Anim * animation = new Anim();
			
			animation->allocate(boneNameToBoneIndex.size(), numAnimKeys, RotationType_Quat, false);
			
			AnimKey * finalAnimKey = animation->m_keys;
			
			for (size_t boneIndex = 0; boneIndex < boneNameToBoneIndex.size(); ++boneIndex)
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
			
			fbxLog(logIndent, "added animation: %s", anim.name.c_str());
			
			animations[anim.name] = animation;
		}
		
		AnimSet * animSet = new AnimSet();
		animSet->m_animations = animations;
		
		return animSet;
	}
}
