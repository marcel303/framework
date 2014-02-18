#pragma once

#include <map>
#include <string>
#include <vector>
#include "Quat.h"
#include "Vec3.h"

namespace Model
{
	// forward declarations
	
	class Anim;
	class AnimKey;
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
	
	class Mesh
	{
	public:
		Vertex * m_vertices;
		int m_numVertices;
		
		int * m_indices;
		int m_numIndices;
		
		Mesh();
		~Mesh();
		
		void allocateVB(int numVertices);
		void allocateIB(int numIndices);
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
	};
	
	class BoneSet
	{
	public:
		Bone * m_bones;
		int m_numBones;
		
		BoneSet();
		~BoneSet();
		
		void allocate(int numBones);
		void calculatePoseMatrices(); // calculate pose matrices given the current set of bone transforms
		void calculateBoneMatrices(); // calculate bone transforms given the current set of pose matrices
	};
	
	struct AnimKey
	{
		float time;
		
		Vec3 translation;
		float rotation[4];
		Vec3 scale;
		
		static void interpolate(BoneTransform & result, const AnimKey & key1, const AnimKey & key2, float t, RotationType rotationType);
	};
	
	class Anim
	{
	public:
		int m_numBones;
		int * m_numKeys;
		AnimKey * m_keys;
		RotationType m_rotationType;
		
		Anim();
		~Anim();
		
		void allocate(int numBones, int numKeys, RotationType rotationType);
		bool evaluate(float time, BoneTransform * transforms);
	};
	
	class AnimSet
	{
	public:
		std::map<std::string, Anim*> m_animations;
		
		AnimSet();
		~AnimSet();
	};
	
	//
	
	class Loader
	{
	public:
		virtual MeshSet * loadMeshSet(const char * filename) = 0;
		virtual BoneSet * loadBoneSet(const char * filename) = 0;
		virtual AnimSet * loadAnimSet(const char * filename, const BoneSet * boneSet) = 0;
	};
	
	//
	
	class Cache
	{
		std::map<std::string, MeshSet*> m_meshes;
		std::map<std::string, BoneSet*> m_bones;
		std::map<std::string, AnimSet*> m_animSets;
	
	public:
		MeshSet * findOrCreateMeshSet(const char * filename);
		BoneSet * findOrCreateBoneSet(const char * filename);
		AnimSet * findOrCreateAnimSet(const std::vector<std::string> & filenames);
	};
}

//

enum ModelDrawFlags
{
	DrawMesh         = 0x1,
	DrawBones        = 0x2,
	DrawNormals      = 0x4,
	DrawPoseMatrices = 0x8
};

class AnimModel
{
public:
	Model::MeshSet * m_meshes;
	Model::BoneSet * m_bones;
	Model::AnimSet * m_animations;
	
	Model::Anim * currentAnim;
	
public:
	float x;
	float y;
	float z;
	Vec3 axis;
	float angle;
	float scale;
	
	bool animIsDone;
	float animTime;
	int animLoop;
	float animSpeed;
	
	AnimModel(const char * filename);
	AnimModel(Model::MeshSet * meshes, Model::BoneSet * bones, Model::AnimSet * animations);
	~AnimModel();
	
	void startAnim(const char * name, int loop = 1);
	
	void process(float timeStep);
	
	void draw(int drawFlags = DrawMesh);
	void drawEx(Vec3 position, Vec3 axis, float angle = 0.f, float scale = 1.f, int drawFlags = DrawMesh);
	void drawEx(const Mat4x4 & matrix, int drawFlags = DrawMesh);
};
