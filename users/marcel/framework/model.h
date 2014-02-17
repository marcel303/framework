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
	class BoneTransform;
	class Mesh;
	class MeshSet;
	class Skeleton;
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
		
		BoneTransform transform;
		Mat4x4 poseMatrix;
		int parent;
	};
	
	class Skeleton
	{
	public:
		Bone * m_bones;
		int m_numBones;
		
		Skeleton();
		~Skeleton();
		
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
	
	class Cache
	{
		std::map<std::string, Mesh*> m_meshes;
		std::map<std::string, Skeleton*> m_skeletons;
		std::map<std::string, AnimSet*> m_animSets;
	
	public:
		Mesh * findOrCreateMesh(const char * filename);
		Skeleton * findOrCreateBody(const char * filename);
		AnimSet * findOrCreateAnimSet(const std::vector<std::string> & filenames);
	};
}

//

enum ModelDrawFlags
{
	DrawMesh    = 0x1,
	DrawBones   = 0x2,
	DrawNormals = 0x4
};

class AnimModel
{
	Model::MeshSet * m_meshes;
	Model::Skeleton * m_skeleton;
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
	AnimModel(Model::MeshSet * meshes, Model::Skeleton * skeleton, Model::AnimSet * animations);
	~AnimModel();
	
	void startAnim(const char * name, int loop = 1);
	
	void process(float timeStep);
	
	void draw();
	void drawEx(Vec3 position, Vec3 axis, float angle = 0.f, float scale = 1.f, int drawFlags = DrawMesh);
	void drawEx(const Mat4x4 & matrix, int drawFlags = DrawMesh);
};
