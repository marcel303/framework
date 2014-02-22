#include <list>
#include "framework.h"
#include "model_fbx.h"

#include "../fbx1/fbx.h" // todo: move to framework

using namespace Model;

// helper functions

static void fbxLog(int logIndent, const char * fmt, ...);
static bool readFile(const char * filename, std::vector<uint8_t> & bytes);

static Mat4x4 matrixTranslation(const Vec3 & v);
static Mat4x4 matrixRotation(const Vec3 & v, RotationType rotationType);
static Mat4x4 matrixScaling(const Vec3 & v);

static bool logEnabled = true;

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
		vsprintf(temp, fmt, va);
		va_end(va);
		
		printf("%s%s", tabs, temp);
	}
}

static bool readFile(const char * filename, std::vector<uint8_t> & bytes)
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

// FBX types

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
					
					//fbxLog(logIndent, "keyCount=%d\n", keyCount);
					
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
							
							//fbxLog(logIndent, "%014lld -> %f\n", temp.time, temp.value);
						}
						
						if (time > endTime)
						{
							endTime = time;
						}
					}
					
					//fbxLog(logIndent, "got %d unique keys\n", keys.size());
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
						
						//printf("got default value %f!\n", key.value);
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
				
				//fbxLog(logIndent, "stream: name=%s\n", name.c_str());
				
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
	
	/*
	bool evaluateRaw(const Channel & t, const Channel & r, const Channel & s, float time, Mat4x4 & result) const
	{
		Vec3 translation;
		Vec3 rotation;
		Vec3 scale(1.f, 1.f, 1.f);
		
		evaluateRaw(t, r, s, time, translation, rotation, scale);
		
		result = matrixTranslation(translation) * matrixRotation(rotation, RotationType_EulerXYZ) * matrixScaling(scale);
		
		return time >= m_endTime;
	}
	*/
	
	/*
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
				
				fassert(time >= key1.time && time <= key2.time);
				
				const float t = (time - key1.time) / (key2.time - key1.time);
				
				fassert(t >= 0.f && t <= 1.f);
				
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
				
				fassert(key == firstKey || key == lastKey);
				
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
	*/
};

class FbxAnim
{
public:
	std::string name;
	std::map<std::string, FbxAnimTransform> transforms;
};

namespace Model
{
	MeshSet * LoaderFbxBinary::loadMeshSet(const char * filename)
	{
		return 0;
	}
	
	BoneSet * LoaderFbxBinary::loadBoneSet(const char * filename)
	{
		return 0;
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
			fbxLog(logIndent, "failed to open %s\n", filename);
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
			
			fbxLog(logIndent, "take: %s\n", name.c_str());
			
			anims.push_back(FbxAnim());
			FbxAnim & anim = anims.back();
			
			anim.name = name;
			
			std::string fileName = take.firstChild("FileName").captureProperty<std::string>(0);
			
			// ReferenceTime (int, timestamp?)
			
			logIndent++;
			
			for (FbxRecord model = take.firstChild("Model"); model.isValid(); model = model.nextSibling("Model"))
			{
				std::string modelName = model.captureProperty<std::string>(0);
				
				fbxLog(logIndent, "model: %s\n", modelName.c_str());
				
				logIndent++;
				
				for (FbxRecord channel = model.firstChild("Channel"); channel.isValid(); channel = channel.nextSibling("Channel"))
				{
					std::string channelName = channel.captureProperty<std::string>(0);
					
					//fbxLog(logIndent, "channel: %s\n", channelName.c_str());
					
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
			
			animation->allocate(boneNameToBoneIndex.size(), numAnimKeys, RotationType_Quat);
			
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
			
			fbxLog(logIndent, "added animation: %s\n", anim.name.c_str());
			
			animations[anim.name] = animation;
		}
		
		AnimSet * animSet = new AnimSet();
		animSet->m_animations = animations;
		
		return animSet;
	}
}
