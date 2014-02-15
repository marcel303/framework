#include <assert.h>
#include "Mat4x4.h"
#include "model.h"

namespace Model
{
	Mesh::Mesh()
	{
		m_vertices = 0;
		m_numVertices = 0;
		
		m_indices = 0;
		m_numIndices = 0;
	}
	
	Mesh::~Mesh()
	{
		allocateVB(0);
		allocateIB(0);
	}
	
	void Mesh::allocateVB(int numVertices)
	{
		if (m_numVertices)
		{
			delete [] m_vertices;
			m_vertices = 0;
			m_numVertices = 0;
		}
		
		if (numVertices)
		{
			m_vertices = new Vertex[numVertices];
			m_numVertices = numVertices;
			
			memset(m_vertices, 0, sizeof(m_vertices[0]) * numVertices);
		}
	}
	
	void Mesh::allocateIB(int numIndices)
	{
		if (m_numIndices)
		{
			delete [] m_indices;
			m_indices = 0;
			m_numIndices = 0;
		}
		
		if (numIndices)
		{
			m_indices = new int[numIndices];
			m_numIndices = numIndices;
			
			memset(m_indices, 0, sizeof(m_indices[0]) * numIndices);
		}
	}
	
	//
	
	MeshSet::MeshSet()
	{
		m_meshes = 0;
		m_numMeshes = 0;
	}
	
	MeshSet::~MeshSet()
	{
		allocate(0);
	}
	
	void MeshSet::allocate(int numMeshes)
	{
		if (m_numMeshes)
		{
			for (int i = 0; i < m_numMeshes; ++i)
			{
				delete m_meshes[i];
				m_meshes[i] = 0;
			}
			delete [] m_meshes;
			m_meshes = 0;
			m_numMeshes = 0;
		}
		
		if (numMeshes)
		{
			m_meshes = new Mesh*[numMeshes];
			m_numMeshes = numMeshes;
			
			for (int i = 0; i < numMeshes; ++i)
				m_meshes[i] = 0;
		}
	}
	
	//
	
	void BoneTransform::clear()
	{
		translation.SetZero();
		rotation.makeIdentity();
		scale = Vec3(1.f, 1.f, 1.f);
	}
	
	void BoneTransform::add(BoneTransform & result, const BoneTransform & transform)
	{
		result.translation += transform.translation;
		result.rotation *= transform.rotation;
		result.scale += transform.scale;
	}
	
	//
	
	Skeleton::Skeleton()
	{
		m_bones = 0;
		m_numBones = 0;
	}
	
	Skeleton::~Skeleton()
	{
		allocate(0);
	}
	
	void Skeleton::allocate(int numBones)
	{
		if (m_numBones)
		{
			delete [] m_bones;
			m_bones = 0;
			m_numBones = 0;
		}
		
		if (numBones)
		{
			m_bones = new Bone[numBones];
			m_numBones = numBones;
		}
	}
	
	void Skeleton::calculatePoseMatrices()
	{
		for (int i = 0; i < m_numBones; ++i)
		{
			Mat4x4 & poseMatrix = m_bones[i].poseMatrix;
			
			poseMatrix.MakeIdentity();
			
			int boneIndex = i;
			
			while (boneIndex != -1)
			{
				Mat4x4 matrix;
				
				matrix = m_bones[boneIndex].transform.rotation.toMatrix();
				matrix.SetTranslation(m_bones[boneIndex].transform.translation);
				
				poseMatrix = matrix * poseMatrix;
				
				boneIndex = m_bones[boneIndex].parent;
			}
			
			poseMatrix = poseMatrix.CalcInv();
		}
	}
	
	//
	
	void AnimKey::interpolate(BoneTransform & result, const AnimKey & key1, const AnimKey & key2, float t, RotationType rotationType)
	{
		const float t1 = 1.f - t;
		const float t2 = t;
		
		Quat quat;
		
		if (rotationType == RotationType_Quat)
		{
			Quat quat1(key1.rotation[0], key1.rotation[1], key1.rotation[2], key1.rotation[3]);
			Quat quat2(key2.rotation[0], key2.rotation[1], key2.rotation[2], key2.rotation[3]);
			
			quat = quat1.slerp(quat2, t);
		}
		else
		{
			const float rotation[3] =
			{
				key1.rotation[0] * t1 + key2.rotation[0],
				key1.rotation[1] * t1 + key2.rotation[1],
				key1.rotation[2] * t1 + key2.rotation[2]
			};
			
			Mat4x4 rx, ry, rz;
			
			rx.MakeRotationX(rotation[0], false);
			ry.MakeRotationY(rotation[1], false);
			rz.MakeRotationZ(rotation[2], false);
			
			Mat4x4 r;
			
			if (rotationType == RotationType_EulerXYZ)
			{
				r = rz * ry * rx;
			}
			else
			{
				assert(false);
				r.MakeIdentity();
			}
			
			quat.fromMatrix(r);
		}
		
		result.translation += (key1.translation * t1) + (key2.translation * t2);
		result.rotation *= quat;
		result.scale += (key1.scale * t1) + (key2.scale * t2);
	}
	
	//
	
	Anim::Anim()
	{
		m_numBones = 0;
		m_numKeys = 0;
		m_keys = 0;
		m_rotationType = RotationType_Quat;
	}
	
	Anim::~Anim()
	{
		allocate(0, 0, m_rotationType);
	}
	
	void Anim::allocate(int numBones, int numKeys, RotationType rotationType)
	{
		if (m_numBones)
		{
			m_numBones = 0;
			delete [] m_numKeys;
			m_numKeys = 0;
			delete [] m_keys;
			m_keys = 0;
		}
		
		if (numBones)
		{
			m_numBones = numBones;
			m_numKeys = new int[numBones];
			m_keys = new AnimKey[numKeys];
		}
		
		m_rotationType = rotationType;
	}
	
	bool Anim::evaluate(float time, BoneTransform * transforms)
	{
		bool isDone = true;
		
		const AnimKey * firstKey = m_keys;
		
		for (int i = 0; i < m_numBones; ++i)
		{
			const int numKeys = m_numKeys[i];
			
			if (numKeys)
			{
				const AnimKey * key = firstKey;
				const AnimKey * lastKey = firstKey + numKeys - 1;
				
				while (key != lastKey && time >= key[1].time)
				{
					key++;
				}
				
				BoneTransform transform;
				
				if (key != lastKey && time >= key->time)
				{
					const AnimKey & key1 = key[0];
					const AnimKey & key2 = key[1];
					
					assert(time >= key1.time && time <= key2.time);
					
					const float t = (time - key1.time) / (key2.time - key1.time);
					
					assert(t >= 0.f && t <= 1.f);
					
					AnimKey::interpolate(transform, key1, key2, t, m_rotationType);
				}
				else
				{
					// either the first or last key in the animation. copy value
					
					assert(key == firstKey || key == lastKey);
					
					AnimKey::interpolate(transform, *key, *key, 0.f, m_rotationType);
				}
				
				BoneTransform::add(transforms[i], transform);
				
				if (key != lastKey)
				{
					isDone = false;
				}
				
				firstKey += numKeys;
			}
		}
		
		return isDone;
	}
	
	//
	
	AnimSet::AnimSet()
	{
	}
	
	AnimSet::~AnimSet()
	{
		for (std::map<std::string, Anim*>::iterator i = m_animations.begin(); i != m_animations.end(); ++i)
		{
			Anim * anim = i->second;
			
			delete anim;
		}
		
		m_animations.clear();
	}
	
	//
	
	Mesh * Cache::findOrCreateMesh(const char * filename)
	{
		return 0;
	}
	
	Skeleton * Cache::findOrCreateBody(const char * filename)
	{
		return 0;
	}
	
	AnimSet * Cache::findOrCreateAnimSet(const std::vector<std::string> & filenames)
	{
		return 0;
	}
}
