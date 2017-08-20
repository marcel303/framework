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

#pragma once

#include <map>
#include <string>
#include <vector>
#include "framework.h"
#include "Mat4x4.h"
#include "Quat.h"
#include "Vec3.h"

namespace AnimModel
{
	// forward declarations
	
	class Anim;
	struct AnimKey;
	class Bone;
	class BoneSet;
	class BoneTransform;
	class Mesh;
	class MeshSet;
	class Vertex;
	
	//
	
	enum RotationType
	{
		RotationType_Quat,
		RotationType_EulerXYZ
	};
	
	//
	
	class Vertex
	{
	public:
		float px, py, pz;
		float nx, ny, nz;
		float tx, ty;
		float cx, cy, cz, cw;
		
		unsigned char boneIndices[4];
		unsigned char boneWeights[4];
	};
	
	class Material
	{
	public:
		Shader shader;
	};
	
	class Mesh
	{
	public:
		Vertex * m_vertices;
		int m_numVertices;
		
		int * m_indices;
		int m_numIndices;
		
		GLuint m_vertexArray;
		GLuint m_indexArray;
		GLuint m_vertexArrayObject;
		
		Material m_material;
		
		Mesh();
		~Mesh();
		
		void allocateVB(int numVertices);
		void allocateIB(int numIndices);
		void finalize();
	};
	
	class MeshSet
	{
	public:
		Mesh ** m_meshes;
		int m_numMeshes;
		
		MeshSet();
		~MeshSet();
		
		void allocate(int numMeshes);
	};
	
	//
	
	class BoneTransform
	{
	public:
		Vec3 translation;
		Quat rotation;
		Vec3 scale;
		
		void clear();
		static void add(BoneTransform & result, const BoneTransform & transform);
	};

	class Bone
	{
	public:
		Bone()
		{
			parent = -1;
		}
		
		std::string name;
		BoneTransform transform;
		Mat4x4 poseMatrix;
		int parent;
		int originalIndex;
	};
	
	class BoneSet
	{
	public:
		Bone * m_bones;
		int m_numBones;
		bool m_bonesAreSorted;
		
		BoneSet();
		~BoneSet();
		
		void allocate(int numBones);
		void calculatePoseMatrices(); // calculate pose matrices given the current set of bone transforms
		void calculateBoneMatrices(); // calculate bone transforms given the current set of pose matrices
		void sortBoneIndices();
		
		int findBone(const std::string & name);
	};
	
	struct AnimKey
	{
		float time;
		
		Vec3 translation;
		float rotation[4];
		Vec3 scale;
		
		static void interpolate(BoneTransform & result, const AnimKey & key1, const AnimKey & key2, float t, RotationType rotationType);
	};
	
	struct AnimTrigger
	{
		float time;
		int loop;
		std::string actions;
		Dictionary args;
	};
	
	class Anim
	{
	public:
		int m_numBones;
		int * m_numKeys;
		AnimKey * m_keys;
		RotationType m_rotationType;
		bool m_isAdditive;
		
		bool m_rootMotion;
		int m_loop;
		std::vector<AnimTrigger> m_triggers;
		
		Anim();
		~Anim();
		
		void allocate(int numBones, int numKeys, RotationType rotationType, bool isAdditive);
		bool evaluate(float time, BoneTransform * transforms, int boneIndex = -1);
		void triggerActions(float oldTime, float newTime, int loop);
	};
	
	class AnimSet
	{
	public:
		std::map<std::string, Anim*> m_animations;
		
		AnimSet();
		~AnimSet();
		
		void rename(const std::string & name);
		void mergeFromAndFree(AnimSet * animSet);
	};
	
	//
	
	class Loader
	{
	public:
		virtual ~Loader() { }
		
		virtual MeshSet * loadMeshSet(const char * filename, const BoneSet * boneSet) = 0;
		virtual BoneSet * loadBoneSet(const char * filename) = 0;
		virtual AnimSet * loadAnimSet(const char * filename, const BoneSet * boneSet) = 0;
	};
}

class ModelCacheElem
{
public:
	AnimModel::MeshSet * meshSet;
	AnimModel::BoneSet * boneSet;
	AnimModel::AnimSet * animSet;
	
	Mat4x4 meshToObject;
	
	std::string rootNode;
	
	ModelCacheElem();
	void free();
	void load(const char * filename);
};

class ModelCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, ModelCacheElem> Map;
	
	Map m_map;
	
	void clear();
	void reload();
	ModelCacheElem & findOrCreate(const char * name);
};

extern ModelCache g_modelCache;
